package fr.nimby.tco

import kotlin.test.*

class PollCadenceTest {
    @Test fun captureWorkConsumesThePeriodInsteadOfAddingAnotherPause() {
        assertEquals(250, nextCaptureDelayMs(0, 0))
        assertEquals(70, nextCaptureDelayMs(0, 180_000_000))
        assertEquals(1, nextCaptureDelayMs(0, 249_100_000))
    }
    @Test fun overrunsSkipMissedSlotsWithoutBusyRetry() {
        assertEquals(250, nextCaptureDelayMs(0, 250_000_000))
        assertEquals(230, nextCaptureDelayMs(0, 520_000_000))
        assertEquals(250, nextCaptureDelayMs(0, 10_000_000_000))
        assertFailsWith<IllegalArgumentException> { nextCaptureDelayMs(0, 0, 0) }
    }
}
