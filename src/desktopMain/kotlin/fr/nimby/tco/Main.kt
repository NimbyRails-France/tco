package fr.nimby.tco

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
    if (args.firstOrNull() == "--check-sdk") {
        val path = Path.of(args.getOrNull(1) ?: error("Chemin SDK requis"))
        val pid = args.getOrNull(2)?.toIntOrNull() ?: error("PID requis")
        NimbyClient.open(path, pid).use { client ->
            val snapshot = client.capture()
            println("PID=${snapshot.processId} trains=${snapshot.trains.size} voies=${snapshot.tracks.size} signaux=${snapshot.signals.size}")
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
    val scope = rememberCoroutineScope()
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
    val services = remember(observation) { observation?.services.orEmpty().associateBy(Service::trainId) }
    val visible = remember(observation, filter) { observation?.trains.orEmpty().filter { filter.matches(it.id, it.name, services[it.id]?.lineName, it.speedKmh, it.position != null) } }
    Column(Modifier.fillMaxSize().padding(14.dp), verticalArrangement = Arrangement.spacedBy(10.dp)) {
        Text("Tableau de contrôle optique", style = MaterialTheme.typography.headlineSmall)
        if (System.getenv("NRF_MANAGED_BY_HUB") == "1") Text("Installation et mises à jour gérées par NRF Hub", style = MaterialTheme.typography.bodySmall)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            OutlinedTextField(sdk, { sdk = it }, label = { Text("Bibliothèque SDK") }, singleLine = true, modifier = Modifier.weight(1f))
            Button(onClick = {
                val chooser = JFileChooser()
                if (chooser.showOpenDialog(null) == JFileChooser.APPROVE_OPTION) { sdk = chooser.selectedFile.absolutePath; preferences.put("sdk", sdk) }
            }) { Text("Parcourir") }
            OutlinedTextField(pid, { pid = it }, label = { Text("PID facultatif") }, singleLine = true, modifier = Modifier.width(140.dp))
            Button(onClick = { preferences.put("sdk", sdk); controller.connect(Path.of(sdk), pid.toIntOrNull()) }, enabled = sdk.isNotBlank() && (pid.isBlank() || pid.toIntOrNull()?.let { it > 0 } == true)) { Text("Connecter") }
            OutlinedButton(onClick = controller::disconnect) { Text("Déconnecter") }
        }
        Text(controller.status)
        Row(Modifier.weight(1f), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Column(Modifier.width(300.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedTextField(filter.query, { filter = filter.copy(query = it) }, label = { Text("Train, ligne ou identifiant") }, singleLine = true)
                Row {
                    Checkbox(filter.locatedOnly, { filter = filter.copy(locatedOnly = it) }); Text("Position connue", Modifier.padding(top = 12.dp))
                }
                Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                    SpeedFilter.entries.forEach { speed ->
                        FilterChip(filter.speed == speed, { filter = filter.copy(speed = speed) }, label = { Text(when (speed) { SpeedFilter.ALL -> "Tous"; SpeedFilter.MOVING -> "Roule"; SpeedFilter.STOPPED -> "Arrêt"; SpeedFilter.UNKNOWN -> "?" }) })
                    }
                }
                Text("${visible.size} / ${observation?.trains?.size ?: 0} trains")
                TextButton(onClick = { filter = TrainFilter() }) { Text("Effacer les filtres") }
                LazyColumn(Modifier.weight(1f)) {
                    items(visible, key = Train::id) { train ->
                        Column(Modifier.fillMaxWidth().background(if (controller.selectedTrain == train.id) Color(0xff294661) else Color.Transparent)
                            .clickable { controller.selectedTrain = train.id; selectedTrack = null }.padding(10.dp)) {
                            Text(train.name.ifEmpty { train.id.toULong().toString(16) })
                            Text(services[train.id]?.lineName.orEmpty(), style = MaterialTheme.typography.bodySmall)
                            Text(train.speedKmh?.let { "%.1f km/h".format(it) } ?: "Vitesse non mesurée", style = MaterialTheme.typography.bodySmall)
                        }
                    }
                }
                ObservationDetails(observation, controller.selectedTrain, selectedTrack) { focus++ }
            }
            Column(Modifier.weight(1f)) {
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedButton(onClick = { fit++ }) { Text("Tout le réseau") }
                    Checkbox(showPath, { showPath = it }); Text("Path", Modifier.padding(top = 12.dp))
                    Text("Signaux", Modifier.padding(top = 12.dp))
                    Slider(signalSize, { signalSize = it }, valueRange = 8f..48f, modifier = Modifier.width(160.dp))
                }
                RailMap(observation, controller.selectedTrain, { controller.selectedTrain = it; selectedTrack = null }, showPath, signalSize, fit, focus, Modifier.weight(1f).fillMaxWidth(),
                    selectTrack = { selectedTrack = it; controller.selectedTrain = null })
                Text("${observation?.tracks?.size ?: 0} voies • ${observation?.signals?.size ?: 0} signaux • " +
                    if (observation?.occupations == null) "Occupation indisponible" else "Occupations observées", style = MaterialTheme.typography.bodySmall)
            }
        }
    }
}
