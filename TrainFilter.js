.pragma library

function matches(train, query, location, motion) {
    const needle = query.trim().toLowerCase()
    if (needle && ![train.name, train.id, train.line].some(function(value) {
        return String(value || "").toLowerCase().indexOf(needle) !== -1
    })) return false
    if (location === 1 && !train.positioned) return false
    if (location === 2 && train.positioned) return false
    const measured = train.speedAvailable && !train.speedDefaulted
    if (motion === 1 && (!measured || train.speed <= 0)) return false
    if (motion === 2 && (!measured || train.speed !== 0)) return false
    if (motion === 3 && measured) return false
    return true
}
