package fr.nimby.tco

/** Keep the start-to-start cadence, skipping missed slots without catch-up. */
internal fun nextCaptureDelayMs(startedNanos: Long, finishedNanos: Long, periodMs: Long = 250): Long {
    require(periodMs in 1..60_000)
    val period = periodMs * 1_000_000
    val elapsed = (finishedNanos - startedNanos).coerceAtLeast(0)
    return (period - elapsed % period + 999_999) / 1_000_000
}
