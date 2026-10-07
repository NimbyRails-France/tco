package fr.nimby.tco

import fr.nimby.tco.i18n.tr
import fr.nimby.tco.i18n.UiText
import fr.nimby.tco.i18n.message

import androidx.compose.runtime.*
import fr.nimby.sdk.*
import kotlinx.coroutines.*
import java.nio.file.Path

class ObservationController(
    private val scope: CoroutineScope,
    private val io: CoroutineDispatcher = Dispatchers.IO,
    private val nanoTime: () -> Long = System::nanoTime,
    private val openClient: (Path, Int) -> ObservationClient = NimbyClient::open,
) {
    private val log = DiagnosticLog.forComponent("tco")
    var observation by mutableStateOf<Observation?>(null); private set
    private var statusText by mutableStateOf(message("Choisir le SDK puis connecter le jeu"))
    val status get() = statusText.text
    var selectedTrain by mutableStateOf<Long?>(null)
    private var generation = 0L
    private var reader: Job? = null
    fun connect(library: Path, processId: Int?) {
        log.write("Connect SDK=$library gamePid=${processId ?: "auto"}")
        disconnect()
        val ticket = generation
        statusText = message("Recherche du jeu…")
        reader = scope.launch {
            var client: ObservationClient? = null
            var expiry: Job? = null
            try {
                val pid = processId ?: withContext(io) { GameProcesses.discover().singleOrNull()?.pid }
                    ?: error(tr("Indiquer le PID : aucun jeu unique détecté"))
                statusText = message("Connexion au jeu • PID {0}", pid)
                // Assign inside the worker so cancellation during open cannot
                // discard a live native session before finally can close it.
                withContext(io) { client = openClient(library, pid) }
                val connection = checkNotNull(client)
                log.write("SDK connected gamePid=$pid")
                var available = false
                var successes = 0L
                var unavailableCount = 0L
                var nextSummary = nanoTime()
                    while (isActive && ticket == generation) {
                        val train = selectedTrain
                        val started = nanoTime()
                        val next = try {
                            withContext(io) { connection.capture(train) }
                        } catch (unavailable: SdkException) {
                            if (unavailable.status != 8) throw unavailable
                            unavailableCount++
                            val now = nanoTime()
                            if (available || unavailableCount == 1L || now >= nextSummary) {
                                log.write("Waiting for a loaded game/stable observation; unavailable=$unavailableCount", level = "WARN")
                                nextSummary = now + 30_000_000_000L
                            }
                            available = false
                            // Menus, loading and changing saves have no stable
                            // observation. Keep the session so loading a game
                            // does not require reconnecting the TCO manually.
                            if (ticket == generation) {
                                expiry?.cancel()
                                observation = null
                                statusText = message("Jeu détecté • attente d’une partie chargée ou d’une observation stable")
                            }
                            delay(500)
                            continue
                        }
                        successes++
                        val now = nanoTime()
                        if (!available || now >= nextSummary) {
                            log.write("Observation ${if (available) "heartbeat" else "available/recovered"}: gamePid=$pid gameSHA256=${next.gameHash} trains=${next.trains.size} tracks=${next.tracks.size} signals=${next.signals.size} selectedTrain=$train clock=${next.clock} successes=$successes unavailable=$unavailableCount captureMs=${(now-started)/1_000_000} ageMs=${System.currentTimeMillis()-next.capturedAtMillis}")
                            nextSummary = now + 30_000_000_000L
                        }
                        available = true
                        if (ticket == generation && train == selectedTrain) {
                            observation = next
                            statusText = message("Connecté au jeu • PID {0}", pid)
                            expiry?.cancel()
                            expiry = launch {
                                delay(2_000)
                                if (ticket == generation) {
                                    observation = null
                                    statusText = message("Observation périmée — attente du jeu")
                                }
                            }
                        }
                        delay(nextCaptureDelayMs(started, nanoTime()))
                    }
            } catch (cancelled: CancellationException) { throw cancelled }
            catch (failure: Exception) { log.write("Observation interrupted", failure); if (ticket == generation) { observation = null; statusText = failure.message?.let { UiText(it, literal = true) } ?: message("Observation interrompue") } }
            finally {
                expiry?.cancel()
                withContext(NonCancellable + io) { try { client?.close() } catch (failure: Exception) { log.write("SDK session close failed", failure) } }
            }
        }
    }
    fun disconnect() { log.write("Disconnect"); generation++; reader?.cancel(); reader = null; observation = null; statusText = message("Déconnecté") }
}
