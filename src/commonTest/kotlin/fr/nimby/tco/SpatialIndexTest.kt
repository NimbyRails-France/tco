package fr.nimby.tco

import fr.nimby.tco.map.*
import kotlin.test.*

class SpatialIndexTest {
    @Test fun keepsCrossingSegmentsAndDropsDistantNetwork() {
        val nodes = listOf(Node(1, null, 2, Point(-100.0, 0.0)), Node(2, 1, null, Point(100.0, 0.0)),
            Node(3, null, 4, Point(500.0, 500.0)), Node(4, 3, null, Point(600.0, 500.0))).associateBy(Node::id)
        val visible = RailIndex(nodes).visible(Bounds(-5.0, -5.0, 5.0, 5.0))
        assertEquals(listOf(1L), visible.map(RailSegment::track))
    }
    @Test fun treeMatchesExhaustiveBoundsAcrossZoomLevels() {
        val nodes = (0L..999L).associateWith { Node(it, if (it > 0) it - 1 else null,
            if (it < 999) it + 1 else null, Point(it.toDouble(), (it % 17).toDouble())) }
        val index = RailIndex(nodes)
        for (scale in listOf(.1, 1.0, 10.0)) {
            val bounds = Bounds.visible(Viewport(Point(500.0, 8.0), scale), 100.0, 20.0)
            val expected = (0L..998L).filter { Bounds.between(nodes.getValue(it).point, nodes.getValue(it + 1).point).intersects(bounds) }
            assertEquals(expected, index.visible(bounds).map(RailSegment::track).sorted())
        }
    }
    @Test fun visibleTracksKeepBothSidesOfSeamsBentTracksAndIsolatedNodes() {
        val nodes = listOf(Node(1, 2, 3, Point(0.0, 100.0)), Node(2, null, 1, Point(-100.0, 0.0)),
            Node(3, 1, null, Point(100.0, 0.0)), Node(4, null, null, Point(500.0, 500.0))).associateBy(Node::id)
        val index = RailIndex(nodes)
        assertTrue(1L in index.visibleTracks(Bounds(-1.0, 99.0, 1.0, 101.0)))
        assertEquals(setOf(1L, 2L), index.visibleTracks(Bounds(-51.0, 49.0, -49.0, 51.0)).toSet())
        assertEquals(listOf(4L), index.visibleTracks(Bounds(499.0, 499.0, 501.0, 501.0)))
    }
    @Test fun everyVisibleFractionRemainsCandidateWithCoincidentCoordinates() {
        val nodes = (0L..499L).associateWith { Node(it, if (it > 0) it - 1 else null,
            if (it < 499) it + 1 else null, Point((it % 7).toDouble(), (it % 3).toDouble())) }
        val index = RailIndex(nodes)
        for (x in 0..7) for (y in 0..3) {
            val bounds = Bounds(x - .25, y - .25, x + .25, y + .25)
            val candidates = index.visibleTracks(bounds).toSet()
            for (id in nodes.keys) for (fraction in listOf(0.0, .1, .5, .9, 1.0)) {
                val point = position(nodes, id, fraction)!!
                if (Bounds.between(point, point).intersects(bounds)) assertTrue(id in candidates)
            }
        }
    }
}
