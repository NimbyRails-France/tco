package fr.nimby.tco.map

data class Point(val x: Double, val y: Double) {
    operator fun plus(other: Point) = Point(x + other.x, y + other.y)
    operator fun minus(other: Point) = Point(x - other.x, y - other.y)
    operator fun times(scale: Double) = Point(x * scale, y * scale)
}
data class Node(val id: Long, val a: Long?, val b: Long?, val point: Point)

/** Native fractions already run A -> B, including trains travelling in reverse. */
fun position(nodes: Map<Long, Node>, track: Long, fraction: Double): Point? {
    if (!fraction.isFinite() || fraction !in 0.0..1.0) return null
    val node = nodes[track] ?: return null
    fun boundary(id: Long?): Point {
        val other = nodes[id] ?: return node.point
        return if (other.a == track || other.b == track) (node.point + other.point) * .5 else node.point
    }
    val start = boundary(node.a)
    val end = boundary(node.b)
    return if (fraction <= .5) start + (node.point - start) * (fraction * 2)
    else node.point + (end - node.point) * ((fraction - .5) * 2)
}

data class Viewport(val center: Point = Point(0.0, 0.0), val scale: Double = .1) {
    fun project(point: Point, width: Double, height: Double) = Point((point.x - center.x) * scale + width / 2, (center.y - point.y) * scale + height / 2)
    fun pan(dx: Double, dy: Double) = copy(center = center + Point(-dx / scale, dy / scale))
    fun zoom(factor: Double, cursor: Point, width: Double, height: Double): Viewport {
        val offset = Point(cursor.x - width / 2, height / 2 - cursor.y)
        val world = center + offset * (1 / scale)
        val next = (scale * factor).coerceIn(1e-7, 50.0)
        return Viewport(world - offset * (1 / next), next)
    }
    companion object {
        fun fit(nodes: Collection<Node>, width: Double, height: Double): Viewport {
            val points = nodes.map(Node::point).filter { it.x.isFinite() && it.y.isFinite() }
            if (points.isEmpty()) return Viewport()
            val x0 = points.minOf(Point::x); val x1 = points.maxOf(Point::x)
            val y0 = points.minOf(Point::y); val y1 = points.maxOf(Point::y)
            return Viewport(Point((x0 + x1) / 2, (y0 + y1) / 2),
                minOf((width - 50).coerceAtLeast(1.0) / (x1 - x0).coerceAtLeast(1.0), (height - 90).coerceAtLeast(1.0) / (y1 - y0).coerceAtLeast(1.0)).coerceAtLeast(1e-7))
        }
    }
}
