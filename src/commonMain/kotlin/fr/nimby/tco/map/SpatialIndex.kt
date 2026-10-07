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

/** One owned array, partitioned in place; no list copies or full sort per level. */
private class SpatialTree<T>(source: List<T>, private val boundsOf: (T) -> Bounds) {
    private class Branch(val bounds: Bounds, val from: Int, val until: Int, val a: Branch? = null, val b: Branch? = null)
    private val values = source.toMutableList()
    private fun coordinate(value: T, horizontal: Boolean): Double = boundsOf(value).let {
        if (horizontal) it.left * .5 + it.right * .5 else it.bottom * .5 + it.top * .5
    }
    private fun partition(from: Int, until: Int, middle: Int, horizontal: Boolean) {
        var low = from; var high = until
        var attempts = 2 * (32 - (until - from).countLeadingZeroBits())
        fun swap(a: Int, b: Int) { val value = values[a]; values[a] = values[b]; values[b] = value }
        while (high - low > 1) {
            // Bound adversarial partitions; equal coordinates use a single pass.
            if (--attempts == 0) {
                values.subList(low, high).sortBy { coordinate(it, horizontal) }; return
            }
            val pivot = coordinate(values[(low + high) / 2], horizontal)
            var smaller = low; var current = low; var larger = high
            while (current < larger) {
                val value = coordinate(values[current], horizontal)
                when { value < pivot -> swap(current++, smaller++); value > pivot -> swap(current, --larger); else -> current++ }
            }
            when { middle < smaller -> high = smaller; middle >= larger -> low = larger; else -> return }
        }
    }
    private fun build(from: Int, until: Int): Branch? {
        if (from == until) return null
        val first = boundsOf(values[from])
        var left = first.left; var bottom = first.bottom; var right = first.right; var top = first.top
        for (i in from + 1 until until) boundsOf(values[i]).let {
            left = minOf(left, it.left); bottom = minOf(bottom, it.bottom)
            right = maxOf(right, it.right); top = maxOf(top, it.top)
        }
        val bounds = Bounds(left, bottom, right, top)
        if (until - from <= 32) return Branch(bounds, from, until)
        val middle = (from + until) / 2
        partition(from, until, middle, right - left >= top - bottom)
        return Branch(bounds, from, until, build(from, middle), build(middle, until))
    }
    private val root = build(0, values.size)
    fun query(bounds: Bounds, accept: (T) -> Unit) {
        fun visit(branch: Branch?) {
            if (branch == null || !branch.bounds.intersects(bounds)) return
            if (branch.a == null) for (i in branch.from until branch.until) {
                val value = values[i]; if (boundsOf(value).intersects(bounds)) accept(value)
            } else { visit(branch.a); visit(branch.b) }
        }
        visit(root)
    }
}

/** Immutable trees for both rail strokes and the full geometry of each track.
 * Crossing tracks and isolated nodes survive culling; marker lookup can use
 * visible track IDs without scanning every signal or train while panning. */
class RailIndex(nodes: Map<Long, Node>) {
    private data class TrackBounds(val id: Long, val bounds: Bounds)
    private val rails = SpatialTree(buildList {
        for (node in nodes.values) {
            fun addLink(id: Long?) {
                val other = nodes[id] ?: return
                if (node.id < other.id || (other.a != node.id && other.b != node.id)) add(RailSegment(node.id, node.point, other.point))
            }
            addLink(node.a); if (node.b != node.a) addLink(node.b)
        }
    }, RailSegment::bounds)
    private val tracks = SpatialTree(nodes.values.map { node ->
        val first = position(nodes, node.id, 0.0) ?: node.point
        val last = position(nodes, node.id, 1.0) ?: node.point
        TrackBounds(node.id, Bounds.between(first, last).union(Bounds.between(node.point, node.point)))
    }, TrackBounds::bounds)
    fun visible(bounds: Bounds): List<RailSegment> = buildList { rails.query(bounds, ::add) }
    fun visibleTracks(bounds: Bounds): List<Long> = buildList { tracks.query(bounds) { add(it.id) } }
}
