package fr.nimby.tco

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import fr.nimby.sdk.*
import java.time.Instant
import java.time.ZoneOffset
import java.time.format.DateTimeFormatter

private fun Long.hex() = toULong().toString(16)
private fun calendar(epoch: Long?, timeUs: Long?): String {
    if (epoch == null || timeUs == null) return "Indisponible"
    return runCatching { DateTimeFormatter.ofPattern("dd/MM/yyyy HH:mm:ss").withZone(ZoneOffset.UTC)
        .format(Instant.ofEpochSecond(Math.addExact(epoch, Math.floorDiv(timeUs, 1_000_000)))) }.getOrDefault("Indisponible")
}
private fun offset(seconds: Int?): String = seconds?.let {
    val magnitude = kotlin.math.abs(it.toLong())
    "${if (it < 0) "−" else ""}${magnitude / 3600}:%02d:%02d".format(magnitude / 60 % 60, magnitude % 60)
} ?: "?"

@Composable internal fun ObservationDetails(observation: Observation?, trainId: Long?, trackId: Long?, focus: () -> Unit) {
    if (trainId == null && trackId == null) return
    Column(Modifier.fillMaxWidth().heightIn(max = 320.dp).verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(6.dp)) {
        HorizontalDivider()
        if (observation == null) { Text("Détails indisponibles — attente d’une capture récente"); return@Column }
        val stations = observation.stations.associate { it.id to it.name }
        fun station(id: Long?) = id?.let { stations[it]?.takeIf(String::isNotBlank) ?: "Gare ${it.hex()}" } ?: "Indisponible"
        if (trainId != null) {
            val train = observation.trains.find { it.id == trainId }
            val service = observation.services.find { it.trainId == trainId }
            val details = observation.details.find { it.trainId == trainId }
            Text(train?.name ?: "Train absent de cette capture", style = MaterialTheme.typography.titleMedium)
            Text("ID ${trainId.hex()}", style = MaterialTheme.typography.bodySmall)
            Text("Voyageurs : ${details?.passengers ?: "indisponible"}")
            Text("Ligne : ${service?.lineName ?: "indisponible"}")
            Text("Service : ${when (service?.status) {
                1 -> "En marche"; 2 -> "Arrêt en gare"; 3 -> "Arrêt temporisé"; 4 -> "Dépôt"
                5 -> "Attente de départ"; 6 -> "Attente au signal"; 7 -> "Remisé"; 8 -> "Hors réseau"
                9 -> "Autre état natif"; else -> "Indisponible"
            }}")
            Text("Position : ${station(service?.stationId)}")
            train?.position?.let { Text("Voie ${it.trackId.hex()} · %.1f %%".format(it.fraction * 100), style = MaterialTheme.typography.bodySmall) }
            Text("Prochain arrêt : ${station(service?.stopStationId)}")
            Text("Horaire de la partie", style = MaterialTheme.typography.titleSmall)
            Text("Arrivée : ${calendar(service?.gameEpochSeconds, service?.arrivalTimeUs)}")
            Text("Départ : ${calendar(service?.gameEpochSeconds, service?.departureTimeUs)}")
            Text("Dispatch : ${calendar(service?.gameEpochSeconds, service?.dispatchTimeUs)}")
            Text("Roulement : ${details?.scheduleId?.hex() ?: "indisponible"} · service ${details?.shiftId?.hex() ?: "indisponible"}", style = MaterialTheme.typography.bodySmall)
            OutlinedButton(onClick = focus, enabled = train?.position != null) { Text("Centrer sur le train") }
            Text("Arrêts de la ligne · temps relatifs", style = MaterialTheme.typography.titleSmall)
            if (observation.lineStops == null) Text("Liste indisponible")
            else if (observation.lineStops?.isEmpty() == true) Text("Aucun arrêt observé")
            observation.lineStops.orEmpty().sortedBy(LineStop::index).forEach { stop ->
                val platform = observation.platforms.find { it.trackId == stop.trackId }?.name
                Text("${stop.index + 1}. ${station(stop.stationId)}${platform?.let { " · $it" }.orEmpty()}\n${offset(stop.arrivalOffsetSeconds)} / ${offset(stop.departureOffsetSeconds)}",
                    style = MaterialTheme.typography.bodySmall)
            }
        } else if (trackId != null) {
            val track = observation.tracks.find { it.id == trackId }
            Text("Voie ${trackId.hex()}", style = MaterialTheme.typography.titleMedium)
            Text(station(track?.stationId))
            observation.platforms.find { it.trackId == trackId }?.name?.let { Text("Quai : $it") }
            Text("Limite : ${track?.speedLimitKmh?.takeIf { it.isFinite() }?.let { "%.0f km/h".format(it) } ?: "indisponible"}")
            fun occupants(values: List<TrackUsage>?) = values?.filter { it.trackId == trackId }?.map { it.trainId }?.distinct()
                ?.joinToString { id -> observation.trains.find { it.id == id }?.name ?: id.hex() }?.ifEmpty { "Aucun" } ?: "Indisponible"
            Text("Occupation : ${occupants(observation.occupations)}")
            Text("Réservations : ${occupants(observation.reservations)}")
            Text("Signaux : ${observation.signals.count { it.position.trackId == trackId }}")
            track?.stationId?.let { id -> Text("${observation.platforms.count { it.stationId == id }} sections de quai dans cette gare") }
        }
    }
}
