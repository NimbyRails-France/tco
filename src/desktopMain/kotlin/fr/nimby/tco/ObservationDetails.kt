package fr.nimby.tco

import fr.nimby.tco.i18n.tr
import fr.nimby.tco.i18n.I18n

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
    if (epoch == null || timeUs == null) return tr("Indisponible")
    val pattern = if (I18n.language == "fr") "dd/MM/yyyy HH:mm:ss" else "yyyy-MM-dd HH:mm:ss"
    return runCatching { DateTimeFormatter.ofPattern(pattern, java.util.Locale.forLanguageTag(I18n.language)).withZone(ZoneOffset.UTC)
        .format(Instant.ofEpochSecond(Math.addExact(epoch, Math.floorDiv(timeUs, 1_000_000)))) }.getOrDefault(tr("Indisponible"))
}
private fun offset(seconds: Int?): String = seconds?.let {
    val magnitude = kotlin.math.abs(it.toLong())
    "${if (it < 0) "−" else ""}${magnitude / 3600}:%02d:%02d".format(magnitude / 60 % 60, magnitude % 60)
} ?: "?"

@Composable internal fun ObservationDetails(observation: Observation?, trainId: Long?, trackId: Long?, focus: () -> Unit) {
    if (trainId == null && trackId == null) return
    Column(Modifier.fillMaxWidth().heightIn(max = 320.dp).verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(6.dp)) {
        HorizontalDivider()
        if (observation == null) { Text(tr("Détails indisponibles — attente d’une capture récente")); return@Column }
        val stations = observation.stations.associate { it.id to it.name }
        fun station(id: Long?) = id?.let { stations[it]?.takeIf(String::isNotBlank) ?: tr("Gare {0}", it.hex()) } ?: tr("Indisponible")
        if (trainId != null) {
            val train = observation.trains.find { it.id == trainId }
            val service = observation.services.find { it.trainId == trainId }
            val details = observation.details.find { it.trainId == trainId }
            Text(train?.name ?: tr("Train absent de cette capture"), style = MaterialTheme.typography.titleMedium)
            Text("ID ${trainId.hex()}", style = MaterialTheme.typography.bodySmall)
            Text(tr("Voyageurs : {0}", details?.passengers ?: tr("indisponible")))
            Text(tr("Ligne : {0}", service?.lineName ?: tr("indisponible")))
            Text(tr("Service : {0}", when (service?.status) {
                1 -> tr("En marche"); 2 -> tr("Arrêt en gare"); 3 -> tr("Arrêt temporisé"); 4 -> tr("Dépôt")
                5 -> tr("Attente de départ"); 6 -> tr("Attente au signal"); 7 -> tr("Remisé"); 8 -> tr("Hors réseau")
                9 -> tr("Autre état natif"); else -> tr("Indisponible")
            }))
            Text(tr("Position : {0}", station(service?.stationId)))
            train?.position?.let { Text(tr("Voie {0} · %.1f %%", it.trackId.hex()).format(java.util.Locale.forLanguageTag(I18n.language), it.fraction * 100), style = MaterialTheme.typography.bodySmall) }
            Text(tr("Prochain arrêt : {0}", station(service?.stopStationId)))
            Text(tr("Horaire de la partie"), style = MaterialTheme.typography.titleSmall)
            Text(tr("Arrivée : {0}", calendar(service?.gameEpochSeconds, service?.arrivalTimeUs)))
            Text(tr("Départ : {0}", calendar(service?.gameEpochSeconds, service?.departureTimeUs)))
            Text(tr("Dispatch : {0}", calendar(service?.gameEpochSeconds, service?.dispatchTimeUs)))
            Text(tr("Roulement : {0} · service {1}", details?.scheduleId?.hex() ?: tr("indisponible"), details?.shiftId?.hex() ?: tr("indisponible")), style = MaterialTheme.typography.bodySmall)
            OutlinedButton(onClick = focus, enabled = train?.position != null) { Text(tr("Centrer sur le train")) }
            Text(tr("Arrêts de la ligne · temps relatifs"), style = MaterialTheme.typography.titleSmall)
            if (observation.lineStops == null) Text(tr("Liste indisponible"))
            else if (observation.lineStops?.isEmpty() == true) Text(tr("Aucun arrêt observé"))
            observation.lineStops.orEmpty().sortedBy(LineStop::index).forEach { stop ->
                val platform = observation.platforms.find { it.trackId == stop.trackId }?.name
                Text("${stop.index + 1}. ${station(stop.stationId)}${platform?.let { " · $it" }.orEmpty()}\n${offset(stop.arrivalOffsetSeconds)} / ${offset(stop.departureOffsetSeconds)}",
                    style = MaterialTheme.typography.bodySmall)
            }
        } else if (trackId != null) {
            val track = observation.tracks.find { it.id == trackId }
            Text(tr("Voie {0}", trackId.hex()), style = MaterialTheme.typography.titleMedium)
            Text(station(track?.stationId))
            observation.platforms.find { it.trackId == trackId }?.name?.let { Text(tr("Quai : {0}", it)) }
            Text(tr("Limite : {0}", track?.speedLimitKmh?.takeIf { it.isFinite() }?.let { "%.0f km/h".format(java.util.Locale.forLanguageTag(I18n.language), it) } ?: tr("indisponible")))
            fun occupants(values: List<TrackUsage>?) = values?.filter { it.trackId == trackId }?.map { it.trainId }?.distinct()
                ?.joinToString { id -> observation.trains.find { it.id == id }?.name ?: id.hex() }?.ifEmpty { tr("Aucun") } ?: tr("Indisponible")
            Text(tr("Occupation : {0}", occupants(observation.occupations)))
            Text(tr("Réservations : {0}", occupants(observation.reservations)))
            Text(tr("Signaux : {0}", observation.signals.count { it.position.trackId == trackId }))
            track?.stationId?.let { id -> Text(tr("{0} sections de quai dans cette gare", observation.platforms.count { it.stationId == id })) }
        }
    }
}
