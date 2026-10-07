package fr.nimby.tco

import java.awt.Color
import java.awt.image.BufferedImage
import java.io.InputStream
import java.nio.file.Files
import java.nio.file.attribute.FileTime
import java.util.concurrent.CancellationException
import javax.imageio.ImageIO
import kotlin.test.*

class SignalImagesTest {
    @Test fun growingStreamStopsAfterOneByteBeyondTheLimit() {
        var read = 0
        val endless = object : InputStream() {
            override fun read(): Int { read++; return 7 }
            override fun read(bytes: ByteArray, offset: Int, length: Int): Int {
                bytes.fill(7, offset, offset + length); read += length; return length
            }
        }
        assertFailsWith<IllegalArgumentException> { readSignalImageBytes(endless, maximum = 8 * 1024 * 1024) }
        assertEquals(8 * 1024 * 1024 + 1, read)
        assertContentEquals(byteArrayOf(1, 2, 3), readSignalImageBytes(byteArrayOf(1, 2, 3).inputStream(), maximum = 3))
    }

    @Test fun cancelledReadIsPropagatedAndDoesNotBecomeAFailedCachedImage() {
        val file = Files.createTempFile("tco-cancel-", ".png")
        try {
            writePng(file)
            var failures = 0; var checkpoints = 0
            val loader = SignalImages(reportFailure = { _, _ -> failures++ })
            assertFailsWith<CancellationException> {
                loader.loadVisible(listOf(file.toString())) {
                    if (++checkpoints == 3) throw CancellationException("viewport changed")
                }
            }
            assertEquals(0, failures)
            assertNotNull(loader.load(file.toString()))
        } finally { Files.deleteIfExists(file) }
    }

    @Test fun corruptContentIsCachedAndAChangedFileRecoversImmediately() {
        val file = Files.createTempFile("tco-corrupt-", ".png")
        try {
            Files.write(file, byteArrayOf(1, 2, 3))
            var failures = 0
            val loader = SignalImages(reportFailure = { _, _ -> failures++ })
            repeat(10) { assertNull(loader.load(file.toString())) }
            assertEquals(1, failures)
            writePng(file)
            Files.setLastModifiedTime(file, FileTime.fromMillis(1234567))
            assertNotNull(loader.load(file.toString()))
            assertEquals(1, failures)
        } finally { Files.deleteIfExists(file) }
    }

    @Test fun missingFilesRetryAfterOneSecondAndFailuresShareTheBoundedCache() {
        val directory = Files.createTempDirectory("tco-missing-")
        val first = directory.resolve("first.png")
        try {
            var now = 0L; var failures = 0
            val loader = SignalImages(nanoTime = { now }, reportFailure = { _, _ -> failures++ })
            repeat(10) { assertNull(loader.load(first.toString())) }
            assertEquals(1, failures)
            writePng(first)
            now = 999_999_999
            assertNull(loader.load(first.toString()))
            now++
            assertNotNull(loader.load(first.toString()))
            val missing = directory.resolve("evicted.png").toString()
            assertNull(loader.load(missing))
            repeat(256) { assertNull(loader.load(directory.resolve("$it.png").toString())) }
            val before = failures
            assertNull(loader.load(missing))
            assertEquals(before + 1, failures)
        } finally { Files.deleteIfExists(first); Files.deleteIfExists(directory) }
    }

    private fun writePng(file: java.nio.file.Path) {
        val source = BufferedImage(1, 1, BufferedImage.TYPE_INT_ARGB)
        source.setRGB(0, 0, Color.RED.rgb)
        ImageIO.write(source, "png", file.toFile())
    }

    @Test fun largePngRetainsOnlyAMarkerThumbnailAndKeepsItsTransparentBounds() {
        val file = Files.createTempFile("tco-marker-", ".png")
        try {
            val source = BufferedImage(1024, 512, BufferedImage.TYPE_INT_ARGB)
            source.createGraphics().let { graphics ->
                try { graphics.color = Color.RED; graphics.fillRect(256, 128, 512, 256) }
                finally { graphics.dispose() }
            }
            ImageIO.write(source, "png", file.toFile())
            val image = assertNotNull(SignalImages().load(file.toString()))
            assertEquals(128, image.bitmap.width); assertEquals(64, image.bitmap.height)
            assertEquals(32, image.offset.x); assertEquals(16, image.offset.y)
            assertEquals(64, image.size.width); assertEquals(32, image.size.height)
        } finally { Files.deleteIfExists(file) }
    }
}
