package fr.nimby.tco

/** The published viewport map owns images independently of the loader's LRU.
 * Bound that map too, including failed attempts, so many distinct/missing
 * paths cannot turn one viewport refresh into an unbounded decoding job. */
internal fun <T> visibleImages(paths: Iterable<String>, maximum: Int = 256, byteBudget: Long = 32L * 1024 * 1024,
    checkpoint: () -> Unit = {}, bytes: (T) -> Long, load: (String) -> T?): Map<String, T> {
    val result = LinkedHashMap<String, T>()
    val visited = HashSet<String>(); var retained = 0L
    for(path in paths) {
        checkpoint()
        if(path in visited) continue
        if(visited.size >= maximum) break
        visited += path
        val image = load(path) ?: continue
        val size = bytes(image)
        if(size < 0 || size > byteBudget - retained) continue
        result[path] = image; retained += size
    }
    return result
}
