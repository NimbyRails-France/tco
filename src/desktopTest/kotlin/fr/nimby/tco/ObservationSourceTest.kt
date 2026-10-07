package fr.nimby.tco

import kotlin.test.*

class ObservationSourceTest {
    @Test fun geometryKeysNeverScanTheNetworkAndOnlyMatchTheSameCopiedSource() {
        val rows: List<Int> = object : AbstractList<Int>() {
            override val size get() = 248_480
            override fun get(index: Int): Int = error("Comparing a UI key must not visit network rows")
        }
        assertEquals(ObservationSource(rows), ObservationSource(rows))
        assertEquals(ObservationSource(rows).hashCode(), ObservationSource(rows).hashCode())
        assertNotEquals(ObservationSource(rows), ObservationSource(listOf(1, 2)))
        assertNotEquals(ObservationSource(listOf(1, 2)), ObservationSource(listOf(1, 2)))
        assertEquals(ObservationSource<List<Int>?>(null), ObservationSource<List<Int>?>(null))
    }
}
