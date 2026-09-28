package fr.nimby.tco

import fr.nimby.tco.i18n.*
import kotlin.test.*

class LocalizationTest {
    @AfterTest fun reset() { I18n.configure("auto", "fr") }
    @Test fun systemLanguageAndExplicitChoice() {
        I18n.configure("auto", "fr-CA"); assertEquals("Connecter", tr("Connecter"))
        I18n.configure("auto", "de-DE"); assertEquals("Connect", tr("Connecter"))
        I18n.choose("fr"); assertEquals("Connecter", tr("Connecter"))
        I18n.choose("unknown"); assertEquals("auto", I18n.preference); assertEquals("en", I18n.language)
    }
    @Test fun everyEnglishMessagePreservesItsArguments() {
        val placeholder = Regex("\\{[0-9]+}")
        assertTrue(english.size >= 70)
        english.forEach { (source, translated) ->
            assertTrue(translated.isNotBlank())
            assertEquals(placeholder.findAll(source).map { it.value }.toSet(), placeholder.findAll(translated).map { it.value }.toSet(), source)
        }
    }
    @Test fun deferredMessagesAndLiteralUserNames() {
        val text = message("Ligne : {0}", "North {1} / Étoile")
        I18n.choose("en"); assertEquals("Line: North {1} / Étoile", text.text)
        I18n.choose("fr"); assertEquals("Ligne : North {1} / Étoile", text.text)
        assertEquals("raw {0}", UiText("raw {0}", literal = true).text)
    }
}
