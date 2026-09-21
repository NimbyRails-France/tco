package fr.nimby.tco

import androidx.compose.runtime.*
import fr.nimby.sdk.*
import kotlinx.coroutines.*
import java.nio.file.Path

class ObservationController(
    private val scope: CoroutineScope,
    private val io: CoroutineDispatcher = Dispatchers.IO,
    private val openClient: (Path, Int) -> ObservationClient = NimbyClient::open,
) {
    var observation by mutableStateOf<Observation?>(null); private set
    var status by mutableStateOf("Choisir le SDK puis connecter le jeu"); private set
    var selectedTrain by mutableStateOf<Long?>(null)
    private var generation = 0L
    private var reader: Job? = null
    fun connect(library: Path, processId: Int?) {
        disconnect()
        val ticket = generation
        status = "Recherche du jeu…"
        reader = scope.launch {
            var client: ObservationClient? = null
            var expiry: Job? = null
            try {
                val pid = processId ?: withContext(io) { GameProcesses.discover().singleOrNull()?.pid }
                    ?: error("Indiquer le PID : aucun jeu unique détecté")
                status = "Connexion au jeu • PID $pid"
                // Assign inside the worker so cancellation during open cannot
                // discard a live native session before finally can close it.
                withContext(io) { client = openClient(library, pid) }
                val connection = checkNotNull(client)
                    while (isActive && ticket == generation) {
                        val train = selectedTrain
                        val next = try {
                            withContext(io) { connection.capture(train) }
                        } catch (unavailable: SdkException) {
                            if (unavailable.status != 8) throw unavailable
                            // Menus, loading and changing saves have no stable
                            // observation. Keep the session so loading a game
                            // does not require reconnecting the TCO manually.
                            if (ticket == generation) {
                                expiry?.cancel()
                                observation = null
                                status = "Jeu détecté • attente d’une partie chargée ou d’une observation stable"
                            }
                            delay(500)
                            continue
                        }
                        if (ticket == generation && train == selectedTrain) {
                            observation = next
                            status = "Connecté au jeu • PID $pid"
                            expiry?.cancel()
                            expiry = launch {
                                delay(2_000)
                                if (ticket == generation) {
                                    observation = null
                                    status = "Observation périmée — attente du jeu"
                                }
                            }
                        }
                        delay(250)
                    }
            } catch (cancelled: CancellationException) { throw cancelled }
            catch (failure: Exception) { if (ticket == generation) { observation = null; status = failure.message ?: "Observation interrompue" } }
            finally {
                expiry?.cancel()
                withContext(NonCancellable + io) { client?.close() }
            }
        }
    }
    fun disconnect() { generation++; reader?.cancel(); reader = null; observation = null; status = "Déconnecté" }
}
