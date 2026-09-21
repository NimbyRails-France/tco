package fr.nimby.tco

import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.toComposeImageBitmap
import androidx.compose.ui.graphics.toPixelMap
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import org.jetbrains.skia.Data
import org.jetbrains.skia.Image
import org.jetbrains.skia.Surface
import org.jetbrains.skia.svg.SVGDOM
import java.nio.file.Path
import kotlin.io.path.*
import kotlin.math.*

data class SignalImage(val bitmap: ImageBitmap, val offset: IntOffset, val size: IntSize)
class SignalImages {
    private data class Key(val path: String, val modified: Long, val size: Long)
    private val cache = LinkedHashMap<Key, SignalImage?>(32, .75f, true)
    private var cachedBytes = 0L
    private fun bytes(image: SignalImage?) = image?.bitmap?.let { it.width.toLong() * it.height * 4 } ?: 0
    @Synchronized fun load(filename: String): SignalImage? = runCatching {
        val path = Path(filename)
        val key = Key(filename, path.getLastModifiedTime().toMillis(), path.fileSize())
        if (cache.containsKey(key)) return cache[key]
        require(key.size in 1..8 * 1024 * 1024)
        val bytes = path.readBytes()
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
            Image.makeFromEncoded(bytes).toComposeImageBitmap()
        }
        val pixels = image.toPixelMap()
        var left = image.width; var top = image.height; var right = -1; var bottom = -1
        for (y in 0 until image.height) for (x in 0 until image.width) if (pixels[x, y].alpha > .01f) {
            left = min(left, x); top = min(top, y); right = max(right, x); bottom = max(bottom, y)
        }
        val result = if (right < left) null else SignalImage(image, IntOffset(left, top), IntSize(right - left + 1, bottom - top + 1))
        cache[key] = result; cachedBytes += bytes(result)
        while (cache.size > 256 || cachedBytes > 32 * 1024 * 1024) {
            val oldest = cache.entries.iterator()
            cachedBytes -= bytes(oldest.next().value); oldest.remove()
        }
        result
    }.getOrNull()
}
