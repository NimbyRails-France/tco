package fr.nimby.tco

import androidx.compose.foundation.*
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.*
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.IntSize
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.window.*
import fr.nimby.sdk.*
import fr.nimby.tco.map.*
import fr.nimby.tco.trains.*
import java.nio.file.Path
import javax.swing.JFileChooser
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlin.math.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.DpOffset
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.text.drawText

private data class SignalMarker(val signals: List<Signal>, val anchor: Offset, val center: Offset)

@OptIn(androidx.compose.ui.ExperimentalComposeUiApi::class)
@Composable internal fun RailMap(observation: Observation?, selected: Long?, select: (Long) -> Unit, showPath: Boolean, signalSize: Float, fit: Int, focus: Int, modifier: Modifier, selectTrack: (Long) -> Unit = {}) {
    val nodes = remember(observation?.nodes) { observation?.nodes.orEmpty().filter { it.x.isFinite() && it.y.isFinite() }
        .associate { it.id to Node(it.id, it.linkA, it.linkB, Point(it.x, it.y)) } }
    val index by produceState<RailIndex?>(null, nodes) {
        value = null
        value = withContext(Dispatchers.Default) { RailIndex(nodes) }
    }
    val loader = remember { SignalImages() }
    val paths = remember(observation) { observation?.signals.orEmpty().mapNotNull(Signal::texturePath).distinct() }
    val images by produceState<Map<String, SignalImage>>(emptyMap(), paths) {
        value = withContext(Dispatchers.IO) { paths.mapNotNull { path -> loader.load(path)?.let { path to it } }.toMap() }
    }
    var viewport by remember { mutableStateOf(Viewport()) }
    var size by remember { mutableStateOf(IntSize.Zero) }
    var fitted by remember { mutableStateOf(false) }
    var candidates by remember { mutableStateOf(emptyList<Train>()) }
    var menuAt by remember { mutableStateOf(Offset.Zero) }
    var hovered by remember { mutableStateOf<Offset?>(null) }
    val measure = rememberTextMeasurer()
    LaunchedEffect(nodes.isEmpty(), fit, size) { if (nodes.isNotEmpty() && size.width > 0 && (!fitted || fit > 0)) { viewport = Viewport.fit(nodes.values, size.width.toDouble(), size.height.toDouble()); fitted = true } }
    LaunchedEffect(focus) { if (focus > 0) observation?.trains?.find { it.id == selected }?.position?.let { p -> position(nodes, p.trackId, p.fraction)?.let { viewport = Viewport(it, max(.05, min(size.width, size.height) / 3000.0)) } } }
    val latestNodes by rememberUpdatedState(nodes)
    val latestObservation by rememberUpdatedState(observation)
    val latestSelect by rememberUpdatedState(select)
    val latestSelectTrack by rememberUpdatedState(selectTrack)
    fun screen(point: Point): Offset = viewport.project(point, size.width.toDouble(), size.height.toDouble()).let { Offset(it.x.toFloat(), it.y.toFloat()) }
    val bounds = Bounds.visible(viewport, size.width.toDouble(), size.height.toDouble(), signalSize * 8.0)
    val rails = remember(index, bounds) { index?.visible(bounds).orEmpty() }
    val markers = remember(observation?.signals, nodes, viewport, size, signalSize) {
        observation?.signals.orEmpty().mapNotNull { signal -> position(nodes, signal.position.trackId, signal.position.fraction)?.let { point ->
            if (!Bounds.between(point, point).intersects(bounds)) null else signal to screen(point)
        } }.groupBy { (signal, point) -> Triple(signal.position.trackId, floor(point.x / (signalSize * 2)).toInt(), floor(point.y / (signalSize * 2)).toInt()) }
            .values.flatMap { group ->
                val sorted = group.sortedBy { it.first.position.fraction }
                val anchor = sorted.first().second
                if (sorted.size > 6) listOf(SignalMarker(sorted.map { it.first }, anchor, anchor))
                else {
                    val track = sorted.first().first.position.trackId
                    val a = position(nodes, track, 0.0)?.let(::screen) ?: anchor
                    val b = position(nodes, track, 1.0)?.let(::screen) ?: anchor
                    val length = hypot(b.x - a.x, b.y - a.y)
                    val axis = if (length > .01f) (b - a) / length else Offset(1f, 0f)
                    sorted.mapIndexed { i, (signal, original) ->
                        SignalMarker(listOf(signal), original, anchor + axis * ((i - (sorted.size - 1) / 2f) * (signalSize + 5)))
                    }
                }
            }
    }
    val tooltip = hovered?.let { cursor -> markers.minByOrNull { (it.center - cursor).getDistance() }
        ?.takeIf { (it.center - cursor).getDistance() <= signalSize }
        ?.signals?.joinToString("\n") { "Signal ${it.id.toULong().toString(16)} · état ${it.specificState ?: it.textureState?.toString() ?: "inconnu"}" } }
    Box(modifier.background(Color(0xff0c121b))) {
    Canvas(Modifier.fillMaxSize().onSizeChanged { size = it }
        .pointerInput(Unit) { detectDragGestures { change, drag -> change.consume(); viewport = viewport.pan(drag.x.toDouble(), drag.y.toDouble()) } }
        .pointerInput(Unit) { detectTapGestures { cursor ->
            val hits = latestObservation?.trains.orEmpty().mapNotNull { train -> train.position?.let { p -> position(latestNodes, p.trackId, p.fraction)?.let { point ->
                val q = viewport.project(point, size.width.toDouble(), size.height.toDouble())
                train to hypot(q.x - cursor.x, q.y - cursor.y)
            } } }.filter { it.second <= 10 }.sortedBy { it.second }.map { it.first }
            when (hits.size) {
                1 -> latestSelect(hits.single().id)
                0 -> latestNodes.values.minByOrNull { (screen(it.point) - cursor).getDistance() }
                    ?.takeIf { (screen(it.point) - cursor).getDistance() < 12 }?.let { latestSelectTrack(it.id) }
                else -> { candidates = hits; menuAt = cursor }
            }
        } }
        .onPointerEvent(PointerEventType.Move) { hovered = it.changes.firstOrNull()?.position }
        .onPointerEvent(PointerEventType.Exit) { hovered = null }
        .onPointerEvent(PointerEventType.Scroll) { event -> event.changes.firstOrNull()?.let { change ->
            viewport = viewport.zoom(1.15.pow(-change.scrollDelta.y.toDouble()), Point(change.position.x.toDouble(), change.position.y.toDouble()), size.width.toDouble(), size.height.toDouble())
        } }) {
        fun segment(track: Long, from: Double, to: Double, color: Color, width: Float) {
            val a = position(nodes, track, from) ?: return
            val b = position(nodes, track, to) ?: return
            if (from < .5 && to > .5) { val mid = nodes[track]?.point ?: return; drawLine(color, screen(a), screen(mid), width); drawLine(color, screen(mid), screen(b), width) }
            else drawLine(color, screen(a), screen(b), width)
        }
        rails.forEach { drawLine(Color(0xff657486), screen(it.from), screen(it.to), 1f) }
        if (showPath) observation?.selectedPath.orEmpty().forEach { segment(it, 0.0, 1.0, Color(0xffb58aff), 3f) }
        observation?.reservations.orEmpty().filter { it.trainId == selected }.forEach { segment(it.trackId, it.from, it.to, Color(0xff62cf99), 4f) }
        observation?.occupations.orEmpty().forEach { segment(it.trackId, it.from, it.to, Color(0xffed6262), 4f) }
        markers.forEach { marker ->
            val center = marker.center
            if (marker.signals.size > 1) {
                drawCircle(Color(0xff555e6d), signalSize / 2 + 3, center)
                val label = measure.measure(marker.signals.size.toString(), style = androidx.compose.ui.text.TextStyle(color = Color.White))
                drawText(label, topLeft = center - Offset(label.size.width / 2f, label.size.height / 2f))
                return@forEach
            }
            val signal = marker.signals.single()
            val image = signal.texturePath?.let(images::get)
            if (center != marker.anchor) drawLine(Color(0xff8791a1), marker.anchor, center, 1f)
            if (image != null && signal.textureState != null) {
                val ratio = signalSize / max(image.size.width, image.size.height)
                val target = IntSize((image.size.width * ratio).roundToInt().coerceAtLeast(1), (image.size.height * ratio).roundToInt().coerceAtLeast(1))
                drawImage(image.bitmap, srcOffset = image.offset, srcSize = image.size,
                    dstOffset = IntOffset((center.x - target.width / 2).roundToInt(), (center.y - target.height / 2).roundToInt()), dstSize = target)
            } else {
                val label = measure.measure("?", style = androidx.compose.ui.text.TextStyle(color = Color.LightGray))
                drawText(label, topLeft = center - Offset(label.size.width / 2f, label.size.height / 2f))
            }
        }
        observation?.trains.orEmpty().forEach { train -> train.position?.let { p -> position(nodes, p.trackId, p.fraction)?.let { point ->
            drawCircle(if (train.id == selected) Color(0xffffcc66) else Color(0xff73b9ff), if (train.id == selected) 6f else 4f, screen(point))
        } } }
    }
    DropdownMenu(candidates.isNotEmpty(), { candidates = emptyList() }, offset = with(LocalDensity.current) { DpOffset(menuAt.x.toDp(), menuAt.y.toDp()) }) {
        candidates.forEach { train -> DropdownMenuItem(text = { Text(train.name.ifEmpty { train.id.toULong().toString(16) }) }, onClick = { latestSelect(train.id); candidates = emptyList() }) }
    }
    if (tooltip != null) Surface(Modifier.align(Alignment.BottomStart).padding(8.dp), color = Color(0xee253347)) {
        Text(tooltip, Modifier.padding(10.dp), style = MaterialTheme.typography.bodySmall)
    }
    }
}
