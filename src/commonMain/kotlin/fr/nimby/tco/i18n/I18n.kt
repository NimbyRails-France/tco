package fr.nimby.tco.i18n

import androidx.compose.runtime.*

/** The application language is independent from the observed game's data.
 * Reading the observable choice makes Compose update labels in place: changing
 * language never reconnects the SDK or resets filters and selection. */
object I18n {
    var preference by mutableStateOf("auto"); private set
    private var system by mutableStateOf("fr")
    val language get() = if (preference == "auto") system else preference
    fun configure(choice: String, systemLanguage: String) {
        system = if (systemLanguage.lowercase().replace('_', '-').substringBefore('-') == "fr") "fr" else "en"
        choose(choice)
    }
    fun choose(choice: String) { preference = choice.takeIf { it in listOf("auto", "fr", "en") } ?: "auto" }
    fun languageName(code: String) = when (code) { "fr" -> "Français"; "en" -> "English"; else -> tr("Automatique (système)") }
}

private val argument = Regex("\\{([0-9]+)}")

/** Lookup first, substitute once. User names and paths containing braces are
 * literal arguments, never another translation template or executable code. */
fun tr(source: String, vararg values: Any?): String {
    val pattern = if (I18n.language == "fr") source else english[source] ?: source
    return argument.replace(pattern) { match ->
        val index = match.groupValues[1].toInt()
        require(index < values.size) { "Missing translation argument $index: $source" }
        values[index].toString()
    }
}

/** Store the message rather than its rendered text so connection status also
 * follows a language change without restarting an observation session. */
data class UiText(val source: String, val values: List<Any?> = emptyList(), val literal: Boolean = false) {
    val text get() = if (literal) source else tr(source, *values.toTypedArray())
}
fun message(source: String, vararg values: Any?) = UiText(source, values.toList())
