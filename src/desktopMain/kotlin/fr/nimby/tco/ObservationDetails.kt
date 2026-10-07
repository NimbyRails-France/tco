package fr.nimby.tco

import fr.nimby.tco.i18n.tr
import fr.nimby.tco.i18n.I18n

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.VerticalScrollbar
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.rememberScrollbarAdapter
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.key
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import fr.nimby.sdk.*
import java.time.Instant
import java.time.ZoneOffset
import java.time.format.DateTimeFormatter

private fun Long.hex() = toULong().toString(16)
private fun calendar(value: Instant?, formatter: DateTimeFormatter): String =
    value?.let { runCatching { formatter.format(it) }.getOrNull() } ?: tr("Indisponible")
private fun offset(seconds: Int?): String = seconds?.let {
    val magnitude = kotlin.math.abs(it.toLong())
    "${if (it < 0) "−" else ""}${magnitude / 3600}:%02d:%02d".format(magnitude / 60 % 60, magnitude % 60)
} ?: "?"

@Composable internal fun ObservationDetails(observation: Observation?, trainId: Long?, trackId: Long?, modifier: Modifier = Modifier, focus: () -> Unit) {
    if (trainId == null && trackId == null) return
    val formatter = remember(I18n.language) {
        DateTimeFormatter.ofPattern(if (I18n.language == "fr") "dd/MM/yyyy HH:mm:ss" else "yyyy-MM-dd HH:mm:ss",
            java.util.Locale.forLanguageTag(I18n.language)).withZone(ZoneOffset.UTC)
    }
    val selected = trainId?.let { observation?.train(it) }
    val lineStops = remember(trainId, ObservationSource(observation?.lineStops)) {
        if (trainId == null) emptyList() else observation?.lineStops.orEmpty().sortedBy(LineStop::index)
    }
    Column(modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(6.dp)) {
        HorizontalDivider()
        // Keep the primary action outside the scrolling details, including
        // when a small window shows only a few lines of the selected train.
        if (trainId != null) {
            OutlinedButton(onClick = focus, enabled = selected?.train?.position != null, modifier = Modifier.fillMaxWidth()) {
                Text(tr("Centrer sur le train"))
            }
        }
        key(trainId, trackId) {
            val scroll = rememberScrollState()
            Box(Modifier.weight(1f).fillMaxWidth()) {
                Column(Modifier.fillMaxSize().padding(end = 12.dp).verticalScroll(scroll), verticalArrangement = Arrangement.spacedBy(6.dp)) details@{
                    if (observation == null) { Text(tr("Détails indisponibles — attente d’une capture récente")); return@details }
                    // The SDK owns indexes beside its copied tables; unchanged tables also
                    // reuse their indexes across captures, including large station maps.
                    fun station(id: Long?) = id?.let { observation.station(it)?.name?.takeIf(String::isNotBlank) ?: tr("Gare {0}", it.hex()) } ?: tr("Indisponible")
                    if (trainId != null) {
                        val observed = selected
                        val train = observed?.train
                        val service = observed?.service
                        val details = observed?.details
                        Text(train?.name ?: tr("Train absent de cette capture"), style = MaterialTheme.typography.titleMedium)
                        Text("ID ${trainId.hex()}", style = MaterialTheme.typography.bodySmall)
                        Text(tr("Voyageurs : {0}", details?.passengers ?: tr("indisponible")))
                        Text(tr("Ligne : {0}", service?.lineName ?: tr("indisponible")))
                        Text(tr("Service : {0}", when (service?.state) {
                            TrainState.Driving -> tr("En marche"); TrainState.StationStop -> tr("Arrêt en gare"); TrainState.TimedStop -> tr("Arrêt temporisé"); TrainState.Depot -> tr("Dépôt")
                            TrainState.DispatchWait -> tr("Attente de départ"); TrainState.SignalWait -> tr("Attente au signal"); TrainState.Mothballed -> tr("Remisé"); TrainState.NotPresent -> tr("Hors réseau")
                            TrainState.Other -> tr("Autre état natif"); else -> tr("Indisponible")
                        }))
                        Text(tr("Gare actuelle : {0}", service?.stationId?.let(::station) ?: tr("Hors gare ou non renseignée")))
                        train?.position?.let { Text(tr("Voie {0} · %.1f %%", it.trackId.hex()).format(java.util.Locale.forLanguageTag(I18n.language), it.fraction * 100), style = MaterialTheme.typography.bodySmall) }
                        Text(tr("Prochain arrêt : {0}", station(service?.stopStationId)))
                        Text(tr("Horaire de la partie"), style = MaterialTheme.typography.titleSmall)
                        Text(tr("Arrivée : {0}", calendar(service?.arrival, formatter)))
                        Text(tr("Départ : {0}", calendar(service?.departure, formatter)))
                        Text(tr("Dispatch : {0}", calendar(service?.dispatchRetry, formatter)))
                        Text(tr("Roulement : {0} · service {1}", details?.scheduleId?.hex() ?: tr("indisponible"), details?.shiftId?.hex() ?: tr("indisponible")), style = MaterialTheme.typography.bodySmall)
                        Text(tr("Arrêts de la ligne · temps relatifs"), style = MaterialTheme.typography.titleSmall)
                        if (observation.lineStops == null) Text(tr("Liste indisponible"))
                        else if (observation.lineStops?.isEmpty() == true) Text(tr("Aucun arrêt observé"))
                        lineStops.forEach { stop ->
                            val platform = observation.platform(stop.trackId)?.name
                            Text("${stop.index + 1}. ${station(stop.stationId)}${platform?.let { " · $it" }.orEmpty()}\n${offset(stop.arrivalOffsetSeconds)} / ${offset(stop.departureOffsetSeconds)}",
                                style = MaterialTheme.typography.bodySmall)
                        }
                    } else if (trackId != null) {
                        val track = observation.track(trackId)
                        Text(tr("Voie {0}", trackId.hex()), style = MaterialTheme.typography.titleMedium)
                        Text(station(track?.stationId))
                        observation.platform(trackId)?.name?.let { Text(tr("Quai : {0}", it)) }
                        Text(tr("Limite : {0}", track?.speedLimitKmh?.takeIf { it.isFinite() }?.let { "%.0f km/h".format(java.util.Locale.forLanguageTag(I18n.language), it) } ?: tr("indisponible")))
                        fun occupants(values: List<TrackUsage>?) = values?.filter { it.trackId == trackId }?.map { it.trainId }?.distinct()
                            ?.joinToString { id -> observation.train(id)?.train?.name ?: id.hex() }?.ifEmpty { tr("Aucun") } ?: tr("Indisponible")
                        Text(tr("Occupation : {0}", occupants(observation.occupations)))
                        Text(tr("Réservations : {0}", occupants(observation.reservations)))
                        Text(tr("Signaux : {0}", observation.signals.count { it.position.trackId == trackId }))
                        track?.stationId?.let { id -> Text(tr("{0} sections de quai dans cette gare", observation.platforms.count { it.stationId == id })) }
                    }
                }
                VerticalScrollbar(rememberScrollbarAdapter(scroll), Modifier.align(Alignment.CenterEnd).fillMaxHeight())
            }
        }
    }
}
