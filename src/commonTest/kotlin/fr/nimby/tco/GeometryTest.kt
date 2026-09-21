package fr.nimby.tco
import fr.nimby.tco.map.*
import fr.nimby.tco.trains.*
import kotlin.test.*

class GeometryTest {
    private val nodes = listOf(Node(1, null, 2, Point(0.0, 0.0)), Node(2, 1, 3, Point(10.0, 0.0)), Node(3, 2, null, Point(20.0, 0.0))).associateBy(Node::id)
    @Test fun adjacentTracksMeet() { assertEquals(position(nodes, 1, 1.0), position(nodes, 2, 0.0)); assertEquals(Point(10.0, 0.0), position(nodes, 2, .5)) }
    @Test fun missingLinkNeverInventsGeometry() { assertEquals(Point(0.0, 0.0), position(nodes, 1, 0.0)); assertNull(position(nodes, 1, Double.NaN)) }
    @Test fun zoomKeepsCursorAnchor() {
        val view = Viewport(Point(3.0, 8.0), 2.0); val point = Point(10.0, 6.0)
        val cursor = view.project(point, 900.0, 600.0)
        assertEquals(cursor, view.zoom(2.0, cursor, 900.0, 600.0).project(point, 900.0, 600.0))
    }
    @Test fun unknownSpeedIsNotStopped() {
        assertFalse(TrainFilter(speed = SpeedFilter.STOPPED).matches(1, "Train", null, null, true))
        assertTrue(TrainFilter(speed = SpeedFilter.UNKNOWN).matches(1, "Train", null, null, true))
        assertTrue(TrainFilter(query = "rer").matches(1, "Train", "RER A", 0.0, true))
    }
}
