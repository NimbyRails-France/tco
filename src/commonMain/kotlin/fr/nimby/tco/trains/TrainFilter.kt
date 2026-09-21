package fr.nimby.tco.trains

enum class SpeedFilter { ALL, MOVING, STOPPED, UNKNOWN }
data class TrainFilter(val query: String = "", val speed: SpeedFilter = SpeedFilter.ALL, val locatedOnly: Boolean = false) {
    fun matches(id: Long, name: String, line: String?, speedKmh: Double?, positioned: Boolean): Boolean {
        val search = query.trim()
        if (search.isNotEmpty() && !listOf(name, line.orEmpty(), id.toULong().toString(16)).any { it.contains(search, ignoreCase = true) }) return false
        if (locatedOnly && !positioned) return false
        return when (speed) {
            SpeedFilter.ALL -> true
            SpeedFilter.MOVING -> speedKmh != null && speedKmh > 0
            SpeedFilter.STOPPED -> speedKmh != null && speedKmh == 0.0
            SpeedFilter.UNKNOWN -> speedKmh == null
        }
    }
}
