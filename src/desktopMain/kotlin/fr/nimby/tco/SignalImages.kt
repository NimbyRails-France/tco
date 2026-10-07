package fr.nimby.tco

import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.toComposeImageBitmap
import androidx.compose.ui.graphics.toPixelMap
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import org.jetbrains.skia.Data
import org.jetbrains.skia.Image
import org.jetbrains.skia.Surface
import org.jetbrains.skia.Rect
import org.jetbrains.skia.svg.SVGDOM
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.io.InputStream
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.attribute.BasicFileAttributes
import java.nio.file.attribute.FileTime
import java.util.concurrent.CancellationException
import kotlin.io.path.*
import kotlin.math.*

data class SignalImage(val bitmap: ImageBitmap, val offset: IntOffset, val size: IntSize)
private const val maximumImageFileBytes = 8 * 1024 * 1024
private const val imageFailureRetryNanos = 1_000_000_000L

/** Bound the stream itself: the file can grow after its attributes were read. */
internal fun readSignalImageBytes(input: InputStream, checkpoint: () -> Unit = {}, maximum: Int = maximumImageFileBytes): ByteArray {
    val output = ByteArrayOutputStream(minOf(8192, maximum))
    val buffer = ByteArray(minOf(8192, maximum + 1))
    var total = 0
    while (true) {
        checkpoint()
        val count = input.read(buffer, 0, minOf(buffer.size, maximum + 1 - total))
        if (count < 0) return output.toByteArray()
        total += count
        require(total <= maximum) { "Signal image exceeds $maximum bytes" }
        output.write(buffer, 0, count)
    }
}

class SignalImages internal constructor(
    private val nanoTime: () -> Long = System::nanoTime,
    private val reportFailure: (String, Exception) -> Unit = { filename, failure ->
        fr.nimby.sdk.DiagnosticLog.forComponent("tco").write("Cannot load signal image: $filename", failure)
    },
) {
    private data class Key(val modified: FileTime, val size: Long)
    private data class Entry(val key: Key?, val image: SignalImage?, val retryAt: Long? = null)
    // One slot per path, including failed loads. Old versions cannot accumulate.
    private val cache = LinkedHashMap<String, Entry>(32, .75f, true)
    private var cachedBytes = 0L
    private fun bytes(image: SignalImage?) = image?.bitmap?.let { it.width.toLong() * it.height * 4 } ?: 0
    fun loadVisible(paths: Iterable<String>, checkpoint: () -> Unit = {}): Map<String, SignalImage> =
        visibleImages(paths, checkpoint = checkpoint, bytes = ::bytes, load = { load(it, checkpoint) })

    @Synchronized fun load(filename: String, checkpoint: () -> Unit = {}): SignalImage? {
        checkpoint()
        val previous = cache[filename]
        val now = nanoTime()
        // Only missing/unreadable assets skip stat, for at most one second.
        // This visual placeholder retry never controls observation freshness.
        if (previous?.key == null && previous?.retryAt?.let { now - it < 0 } == true) return null
        val path: Path
        val key: Key
        try {
            path = Path(filename)
            val attributes = Files.readAttributes(path, BasicFileAttributes::class.java)
            key = Key(attributes.lastModifiedTime(), attributes.size())
        } catch (cancelled: CancellationException) { throw cancelled
        } catch (failure: Exception) {
            remember(filename, Entry(null, null, now + imageFailureRetryNanos))
            reportFailure(filename, failure)
            return null
        }
        if (previous?.key == key && (previous.retryAt == null || now - previous.retryAt < 0)) return previous.image
        return try {
            require(key.size in 1..maximumImageFileBytes)
            val result = path.inputStream().use { decode(path, readSignalImageBytes(it, checkpoint)) }
            checkpoint()
            remember(filename, Entry(key, result))
            result
        } catch (cancelled: CancellationException) { throw cancelled
        } catch (failure: Exception) {
            // Invalid content stays cached until its attributes change. An I/O
            // error can recover without a content change, so allow a later retry.
            remember(filename, Entry(key, null, if (failure is IOException) now + imageFailureRetryNanos else null))
            reportFailure(filename, failure)
            null
        }
    }

    private fun remember(filename: String, entry: Entry) {
        cachedBytes -= bytes(cache.put(filename, entry)?.image)
        cachedBytes += bytes(entry.image)
        while (cache.size > 256 || cachedBytes > 32 * 1024 * 1024) {
            val oldest = cache.entries.iterator()
            cachedBytes -= bytes(oldest.next().value.image); oldest.remove()
        }
    }

    private fun decode(path: Path, bytes: ByteArray): SignalImage? {
        val image = if (path.extension.equals("svg", true)) {
            Data.makeFromBytes(bytes).use { data -> SVGDOM(data).use { svg ->
                val box = svg.root?.viewBox
                val aspect = if (box != null && box.height > 0 && box.width > 0) box.width / box.height else 1f
                val width = if (aspect >= 1) 128 else (128 * aspect).roundToInt().coerceAtLeast(1)
                val height = if (aspect <= 1) 128 else (128 / aspect).roundToInt().coerceAtLeast(1)
                Surface.makeRasterN32Premul(width, height).use { surface ->
                    surface.canvas.clear(0); svg.setContainerSize(width.toFloat(), height.toFloat()); svg.render(surface.canvas)
                    surface.makeImageSnapshot().toComposeImageBitmap()
                }
            } }
        } else {
            // Inspect the PNG header before asking the decoder to allocate pixels.
            require(bytes.size >= 24 && bytes.take(8) == listOf(137,80,78,71,13,10,26,10).map(Int::toByte))
            val header = java.nio.ByteBuffer.wrap(bytes).order(java.nio.ByteOrder.BIG_ENDIAN)
            val width = header.getInt(16); val height = header.getInt(20)
            require(width in 1..4096 && height in 1..4096 && width.toLong() * height <= 4_194_304)
            // Markers are tiny. Keep a bounded thumbnail after decoding instead
            // of retaining/scanning a full 4096-pixel asset for every marker.
            Image.makeFromEncoded(bytes).use { decoded ->
                val scale = minOf(1.0, 128.0 / max(width, height))
                val targetWidth = (width * scale).roundToInt().coerceAtLeast(1)
                val targetHeight = (height * scale).roundToInt().coerceAtLeast(1)
                Surface.makeRasterN32Premul(targetWidth, targetHeight).use { surface ->
                    surface.canvas.clear(0)
                    surface.canvas.drawImageRect(decoded, Rect.makeWH(targetWidth.toFloat(), targetHeight.toFloat()))
                    surface.makeImageSnapshot().toComposeImageBitmap()
                }
            }
        }
        val pixels = image.toPixelMap()
        var left = image.width; var top = image.height; var right = -1; var bottom = -1
        for (y in 0 until image.height) for (x in 0 until image.width) if (pixels[x, y].alpha > .01f) {
            left = min(left, x); top = min(top, y); right = max(right, x); bottom = max(bottom, y)
        }
        return if (right < left) null else SignalImage(image, IntOffset(left, top), IntSize(right - left + 1, bottom - top + 1))
    }
}
