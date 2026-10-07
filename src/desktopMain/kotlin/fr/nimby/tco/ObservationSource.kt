package fr.nimby.tco

/** SDK collections keep their identity only after an exact comparison of the
 * fresh data. Compose can therefore compare this key in O(1), instead of
 * walking hundreds of thousands of nodes on its UI thread. */
internal class ObservationSource<T>(val value: T) {
    override fun equals(other: Any?): Boolean = other is ObservationSource<*> && value === other.value
    override fun hashCode(): Int = System.identityHashCode(value)
}
