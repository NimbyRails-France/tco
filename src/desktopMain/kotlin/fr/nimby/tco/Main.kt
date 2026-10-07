package fr.nimby.tco

import fr.nimby.tco.i18n.tr
import fr.nimby.tco.i18n.I18n

import androidx.compose.foundation.*
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.*
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.window.*
import fr.nimby.sdk.*
import fr.nimby.tco.map.*
import fr.nimby.tco.trains.*
import java.nio.file.Path
import javax.swing.JFileChooser
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlin.math.*

fun main(args: Array<String>) {
    I18n.configure(java.util.prefs.Preferences.userRoot().node("fr/nimbyrails/tco").get("language", "auto"), java.util.Locale.getDefault().toLanguageTag())
    if (args.firstOrNull() == "--package-smoke-test") {
        check(System.getProperty("os.name").startsWith("Windows"))
        // Load the actual Windows Skiko DLL without starting the UI or the game.
        org.jetbrains.skia.Surface.makeRasterN32Premul(2, 2).use { surface ->
            check(surface.width == 2)
        }
        // JNA is an implementation dependency of the SDK client. Resolve it
        // from the packaged runtime without exposing it as TCO's public API.
        check(Class.forName("com.sun.jna.Native").getField("POINTER_SIZE").getInt(null) == 8)
        println("PASS: packaged TCO ${TcoBuildInfo.VERSION}, Windows JVM, application classes and native dependencies")
        return
    }
    DiagnosticLog.forComponent("tco").startApplication(TcoBuildInfo.VERSION)
    if (args.firstOrNull() == "--check-sdk") {
        val path = Path.of(args.getOrNull(1) ?: error(tr("Chemin SDK requis")))
        val pid = args.getOrNull(2)?.toIntOrNull() ?: error(tr("PID requis"))
        NimbyClient.open(path, pid).use { client ->
            val snapshot = client.capture()
            println(tr("PID={0} trains={1} voies={2} signaux={3}", snapshot.processId, snapshot.trains.size, snapshot.tracks.size, snapshot.signals.size))
        }
        return
    }
    application {
        Window(onCloseRequest = ::exitApplication, title = "Nimby TCO", state = rememberWindowState(width = 1280.dp, height = 840.dp)) {
            MaterialTheme(colorScheme = darkColorScheme(primary = Color(0xff79b8ff), surface = Color(0xff17202c), background = Color(0xff101721))) {
                Surface(Modifier.fillMaxSize()) { TcoScreen() }
            }
        }
    }
}

@Composable private fun TcoScreen() {
    val scope = rememberCoroutineScope { kotlinx.coroutines.CoroutineExceptionHandler { _, error -> DiagnosticLog.forComponent("tco").write("Background task failed", error) } }
    val controller = remember { ObservationController(scope) }
    DisposableEffect(controller) { onDispose { controller.disconnect() } }
    val preferences = remember { java.util.prefs.Preferences.userRoot().node("fr/nimbyrails/tco") }
    var sdk by remember { mutableStateOf(System.getenv("NRF_SDK_LIBRARY") ?: preferences.get("sdk", "")) }
    var pid by remember { mutableStateOf("") }
    var filter by remember { mutableStateOf(TrainFilter()) }
    var showPath by remember { mutableStateOf(false) }
    var signalSize by remember { mutableStateOf(16f) }
    var fit by remember { mutableStateOf(0) }
    var focus by remember { mutableStateOf(0) }
    var selectedTrack by remember { mutableStateOf<Long?>(null) }
    val observation = controller.observation
    val visible = remember(observation, filter) { observation?.trains.orEmpty().filter { filter.matches(it.id, it.name, observation?.service(it.id)?.lineName, it.speedKmh, it.position != null) } }
    Column(Modifier.fillMaxSize().padding(14.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
        Row(Modifier.fillMaxWidth(), verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
            Text(tr("Tableau de contrôle optique"), style = MaterialTheme.typography.headlineSmall, modifier = Modifier.weight(1f))
            var languageMenu by remember { mutableStateOf(false) }
            Box {
                TextButton({ languageMenu = true }) { Text(tr("Langue") + " · " + I18n.languageName(I18n.preference)) }
                DropdownMenu(languageMenu, { languageMenu = false }) {
                    listOf("auto", "fr", "en").forEach { code ->
                        DropdownMenuItem(text = { Text(I18n.languageName(code)) }, onClick = {
                            I18n.choose(code); preferences.put("language", code); languageMenu = false
                        })
                    }
                }
            }
        }
        if (System.getenv("NRF_MANAGED_BY_HUB") == "1") Text(tr("Installation et mises à jour gérées par NRF Hub"), style = MaterialTheme.typography.bodySmall)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            OutlinedTextField(sdk, { sdk = it }, label = { Text(tr("Bibliothèque SDK")) }, singleLine = true, modifier = Modifier.weight(1f))
            Button(onClick = {
                val chooser = JFileChooser().apply {
                    locale = java.util.Locale.forLanguageTag(I18n.language)
                    updateUI()
                    dialogTitle = tr("Choisir la bibliothèque SDK")
                    approveButtonText = tr("Ouvrir")
                }
                if (chooser.showOpenDialog(null) == JFileChooser.APPROVE_OPTION) { sdk = chooser.selectedFile.absolutePath; preferences.put("sdk", sdk) }
            }) { Text(tr("Parcourir")) }
            OutlinedTextField(pid, { pid = it }, label = { Text(tr("PID facultatif")) }, singleLine = true, modifier = Modifier.width(140.dp))
            Button(onClick = { preferences.put("sdk", sdk); controller.connect(Path.of(sdk), pid.toIntOrNull()) }, enabled = sdk.isNotBlank() && (pid.isBlank() || pid.toIntOrNull()?.let { it > 0 } == true)) { Text(tr("Connecter")) }
            OutlinedButton(onClick = controller::disconnect) { Text(tr("Déconnecter")) }
        }
        Row {
            Text(controller.status, Modifier.weight(1f))
            TextButton(onClick = { runCatching {
                val log = DiagnosticLog.forComponent("tco")
                java.nio.file.Files.createDirectories(log.directory)
                java.awt.Desktop.getDesktop().open(log.directory.toFile())
            }.onFailure { DiagnosticLog.forComponent("tco").write("Cannot open log folder", it) } }) { Text(tr("Journaux")) }
        }
        Row(Modifier.weight(1f), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Column(Modifier.width(300.dp).fillMaxHeight(), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedTextField(filter.query, { filter = filter.copy(query = it) }, label = { Text(tr("Train, ligne ou identifiant")) }, singleLine = true)
                Row {
                    Checkbox(filter.locatedOnly, { filter = filter.copy(locatedOnly = it) }); Text(tr("Position connue"), Modifier.padding(top = 12.dp))
                }
                Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                    SpeedFilter.entries.forEach { speed ->
                        FilterChip(filter.speed == speed, { filter = filter.copy(speed = speed) }, label = { Text(when (speed) { SpeedFilter.ALL -> tr("Tous"); SpeedFilter.MOVING -> tr("Roule"); SpeedFilter.STOPPED -> tr("Arrêt"); SpeedFilter.UNKNOWN -> "?" }) })
                    }
                }
                Text("${visible.size} / ${observation?.trains?.size ?: 0} trains")
                TextButton(onClick = { filter = TrainFilter() }) { Text(tr("Effacer les filtres")) }
                LazyColumn(Modifier.weight(1f)) {
                    items(visible, key = Train::id) { train ->
                        Column(Modifier.fillMaxWidth().background(if (controller.selectedTrain == train.id) Color(0xff294661) else Color.Transparent)
                            .clickable { controller.selectedTrain = train.id; selectedTrack = null }.padding(10.dp)) {
                            Text(train.name.ifEmpty { train.id.toULong().toString(16) })
                            Text(observation?.service(train.id)?.lineName.orEmpty(), style = MaterialTheme.typography.bodySmall)
                            Text(train.speedKmh?.let { "%.1f km/h".format(java.util.Locale.forLanguageTag(I18n.language), it) } ?: tr("Vitesse non mesurée"), style = MaterialTheme.typography.bodySmall)
                        }
                    }
                }
                // Both regions share only the height left by the filters. A
                // long detail sheet must not consume the train list's height.
                ObservationDetails(observation, controller.selectedTrain, selectedTrack, Modifier.weight(1f)) { focus++ }
            }
            Column(Modifier.weight(1f)) {
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedButton(onClick = { fit++ }) { Text(tr("Tout le réseau")) }
                    Checkbox(showPath, { showPath = it }); Text(tr("Itinéraire"), Modifier.padding(top = 12.dp))
                    Text(tr("Signaux"), Modifier.padding(top = 12.dp))
                    Slider(signalSize, { signalSize = it }, valueRange = 8f..48f, modifier = Modifier.width(160.dp))
                }
                RailMap(observation, controller.selectedTrain, { controller.selectedTrain = it; selectedTrack = null }, showPath, signalSize, fit, focus, Modifier.weight(1f).fillMaxWidth(),
                    selectTrack = { selectedTrack = it; controller.selectedTrain = null })
                Text(tr("{0} voies • {1} signaux • ", observation?.tracks?.size ?: 0, observation?.signals?.size ?: 0) +
                    if (observation?.occupations == null) tr("Occupation indisponible") else tr("Occupations observées"), style = MaterialTheme.typography.bodySmall)
            }
        }
    }
}
