package fr.nimby.tco

import fr.nimby.sdk.*
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.*
import java.nio.file.Path
import kotlin.test.*

@OptIn(ExperimentalCoroutinesApi::class)
class ObservationControllerTest {
    private fun observation() = Observation(1, 42, "test", emptyList(), emptyList(), emptyList(), emptyList(),
        emptyList(), emptyList(), emptyList(), emptyList(), emptyList(), null, null, null, null, null)
    private class Client(val read: () -> Observation) : ObservationClient {
        var closed = 0
        override fun capture(selectedTrainId: Long?) = read()
        override fun close() { closed++ }
    }
    @Test fun cancellationDuringOpenClosesAcquiredSession() = runTest {
        val client = Client { error("A cancelled session must not capture") }
        lateinit var controller: ObservationController
        controller = ObservationController(backgroundScope, StandardTestDispatcher(testScheduler)) { _, _ ->
            controller.disconnect()
            client
        }
        controller.connect(Path.of("test"), 42)
        runCurrent()
        assertEquals(1, client.closed)
        assertNull(controller.observation)
    }
    @Test fun readFailureClearsOldDataAndClosesSession() = runTest {
        var reads = 0
        val client = Client { if (++reads == 1) observation() else error("Jeu fermé") }
        val controller = ObservationController(backgroundScope, StandardTestDispatcher(testScheduler)) { _, _ -> client }
        controller.connect(Path.of("test"), 42)
        runCurrent()
        assertNotNull(controller.observation)
        advanceTimeBy(250); runCurrent()
        assertNull(controller.observation)
        assertEquals("Jeu fermé", controller.status)
        assertEquals(1, client.closed)
    }
    @Test fun disconnectClearsDataAndReleasesSession() = runTest {
        val client = Client { observation() }
        val controller = ObservationController(backgroundScope, StandardTestDispatcher(testScheduler)) { _, _ -> client }
        controller.connect(Path.of("test"), 42)
        runCurrent()
        assertNotNull(controller.observation)
        controller.disconnect(); runCurrent()
        assertNull(controller.observation)
        assertEquals(1, client.closed)
    }
    @Test fun waitsForLoadedGameAndResumesWithoutReopening() = runTest {
        var reads = 0
        var opens = 0
        val client = Client { if (++reads == 1) throw SdkException(8, "CaptureSnapshot") else observation() }
        val controller = ObservationController(backgroundScope, StandardTestDispatcher(testScheduler)) { _, _ -> opens++; client }
        controller.connect(Path.of("test"), 42)
        runCurrent()
        assertNull(controller.observation)
        assertTrue(controller.status.contains("attente"))
        assertEquals(0, client.closed)
        advanceTimeBy(500); runCurrent()
        assertNotNull(controller.observation)
        assertEquals(1, opens)
        assertTrue(controller.status.contains("Connecté"))
        controller.disconnect(); runCurrent()
        assertEquals(1, client.closed)
    }
}
