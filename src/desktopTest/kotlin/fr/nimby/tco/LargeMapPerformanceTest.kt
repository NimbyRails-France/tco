package fr.nimby.tco

import fr.nimby.tco.map.*
import kotlin.test.*

class LargeMapPerformanceTest {
    @Test fun visibleMarkerQueryMatchesTheFullScanOnOneHundredThousandTracks() {
        val count = 100_000
        val nodes = (0L until count.toLong()).associateWith { id ->
            Node(id, if (id % 500 > 0) id - 1 else null, if (id % 500 < 499) id + 1 else null,
                Point((id % 500) * 100.0, (id / 500) * 100.0))
        }
        val started = System.nanoTime(); val index = RailIndex(nodes)
        val buildMs = (System.nanoTime() - started) / 1_000_000.0
        val indexed = mutableListOf<Double>(); val scanned = mutableListOf<Double>(); var maxCandidates = 0
        repeat(40) { sample ->
            val bounds = Bounds(1000.0 + sample * 300, 2000.0 + sample * 100,
                1600.0 + sample * 300, 2600.0 + sample * 100)
            val referenceStart = System.nanoTime()
            val expected = nodes.keys.filter { val p = position(nodes, it, .75)!!; Bounds.between(p, p).intersects(bounds) }.toSet()
            val scanUs = (System.nanoTime() - referenceStart) / 1000.0
            val queryStart = System.nanoTime()
            val candidates = index.visibleTracks(bounds)
            val actual = candidates.filter { val p = position(nodes, it, .75)!!; Bounds.between(p, p).intersects(bounds) }.toSet()
            val queryUs = (System.nanoTime() - queryStart) / 1000.0
            assertEquals(expected, actual); assertTrue(candidates.size < count / 100)
            maxCandidates = maxOf(maxCandidates, candidates.size)
            if (sample >= 8) { scanned += scanUs; indexed += queryUs }
        }
        fun report(name: String, samples: List<Double>) {
            val sorted = samples.sorted(); fun p(percent: Int) = sorted[(sorted.size * percent + 99) / 100 - 1]
            println("{\"scenario\":\"$name\",\"tracks\":$count,\"samples\":${samples.size},\"p50_us\":${p(50)},\"p95_us\":${p(95)},\"p99_us\":${p(99)}}")
        }
        report("tco_previous_marker_scan", scanned); report("tco_indexed_marker_query", indexed)
        println("{\"tco_index_build_ms\":$buildMs,\"maximum_candidate_tracks\":$maxCandidates}")
    }
}
