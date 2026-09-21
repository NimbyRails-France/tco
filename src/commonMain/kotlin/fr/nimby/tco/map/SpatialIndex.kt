package fr.nimby.tco.map

data class Bounds(val left: Double, val bottom: Double, val right: Double, val top: Double) {
    fun intersects(other: Bounds) = left <= other.right && right >= other.left && bottom <= other.top && top >= other.bottom
    fun union(other: Bounds) = Bounds(minOf(left, other.left), minOf(bottom, other.bottom), maxOf(right, other.right), maxOf(top, other.top))
    companion object {
        fun between(a: Point, b: Point) = Bounds(minOf(a.x, b.x), minOf(a.y, b.y), maxOf(a.x, b.x), maxOf(a.y, b.y))
        fun visible(view: Viewport, width: Double, height: Double, margin: Double = 0.0): Bounds {
            val dx = (width / 2 + margin) / view.scale
            val dy = (height / 2 + margin) / view.scale
            return Bounds(view.center.x - dx, view.center.y - dy, view.center.x + dx, view.center.y + dy)
        }
    }
}

data class RailSegment(val track: Long, val from: Point, val to: Point) {
    val bounds = Bounds.between(from, to)
}

/** Immutable bounding-volume tree. Crossing segments survive viewport culling. */
class RailIndex(nodes: Map<Long, Node>) {
    private class Branch(val bounds: Bounds, val values: List<RailSegment>, val a: Branch? = null, val b: Branch? = null)
    private fun build(values: List<RailSegment>): Branch? {
        if (values.isEmpty()) return null
        val bounds = values.map(RailSegment::bounds).reduce(Bounds::union)
        if (values.size <= 32) return Branch(bounds, values)
        val horizontal = bounds.right - bounds.left >= bounds.top - bounds.bottom
        val sorted = values.sortedBy { if (horizontal) it.bounds.left + it.bounds.right else it.bounds.bottom + it.bounds.top }
        val middle = sorted.size / 2
        return Branch(bounds, emptyList(), build(sorted.subList(0, middle)), build(sorted.subList(middle, sorted.size)))
    }
    private val root = build(nodes.values.flatMap { node ->
        listOfNotNull(node.a, node.b).distinct().mapNotNull { id -> nodes[id]?.takeIf { other ->
            node.id < id || (other.a != node.id && other.b != node.id)
        }?.let { RailSegment(node.id, node.point, it.point) } }
    })
    fun visible(bounds: Bounds): List<RailSegment> = buildList {
        fun visit(branch: Branch?) {
            if (branch == null || !branch.bounds.intersects(bounds)) return
            branch.values.filterTo(this) { it.bounds.intersects(bounds) }
            visit(branch.a); visit(branch.b)
        }
        visit(root)
    }
}
