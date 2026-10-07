package fr.nimby.tco

import kotlin.test.*

class VisibleImagesTest {
    private data class Image(val bytes: Long)
    @Test fun viewportMapHasItsOwnMemoryBoundEvenWhenTheLoaderEvictsImages() {
        val result = visibleImages((0..1000).map(Int::toString), maximum = 20, byteBudget = 30,
            bytes = Image::bytes) { Image(10) }
        assertEquals(listOf("0", "1", "2"), result.keys.toList())
        assertEquals(30, result.values.sumOf { it.bytes })
    }
    @Test fun duplicateAndMissingPathsCannotCauseUnboundedDecodeAttempts() {
        var attempts = 0
        val result = visibleImages(List(1000) { "missing${it / 2}" }, maximum = 4,
            bytes = Image::bytes) { attempts++; null }
        assertTrue(result.isEmpty()); assertEquals(4, attempts)
    }
    @Test fun viewportCancellationStopsBeforeAnotherDecode() {
        var attempts = 0
        assertFailsWith<IllegalStateException> {
            visibleImages(listOf("a", "b", "c"), checkpoint = { check(attempts == 0) }, bytes = Image::bytes) { attempts++; Image(1) }
        }
        assertEquals(1, attempts)
    }
}
