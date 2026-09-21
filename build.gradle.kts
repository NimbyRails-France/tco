import org.jetbrains.compose.desktop.application.dsl.TargetFormat
import java.nio.file.Files
import java.security.MessageDigest

plugins {
    kotlin("multiplatform") version "2.2.20"
    id("org.jetbrains.kotlin.plugin.compose") version "2.2.20"
    id("org.jetbrains.compose") version "1.9.3"
}
group = "fr.nimbyrails"
version = file("VERSION").readText().trim()
require(Regex("(0|[1-9][0-9]{0,3})\\.(0|[1-9][0-9]{0,3})\\.(0|[1-9][0-9]{0,3})(?:-(alpha|beta)\\.[1-9][0-9]{0,8})?").matches(version.toString()))
val numericVersion = providers.gradleProperty("nativePackageVersion").orElse(version.toString().substringBefore('-')).get()
require(Regex("[0-9]+\\.[0-9]+\\.[0-9]+").matches(numericVersion))
val hostOs = when { System.getProperty("os.name").startsWith("Windows") -> "windows"; System.getProperty("os.name").startsWith("Mac") -> "macos"; else -> "linux" }
val hostArch = when (System.getProperty("os.arch")) { "amd64", "x86_64" -> "x64"; "aarch64", "arm64" -> "arm64"; else -> error("Architecture inconnue") }
val releasePlatform = "$hostOs-$hostArch"
val releaseRoot = "NimbyTco-${project.version}"
layout.buildDirectory = layout.projectDirectory.dir("build/kotlin-${System.getProperty("os.name").lowercase().replace(' ', '-')}")
kotlin {
    jvm("desktop")
    jvmToolchain(21)
    sourceSets {
        commonMain.dependencies {
            implementation(compose.runtime)
            implementation(compose.foundation)
            implementation(compose.material3)
            implementation("org.jetbrains.kotlinx:kotlinx-coroutines-core:1.10.2")
        }
        commonTest.dependencies { implementation(kotlin("test")) }
        val desktopMain by getting {
            dependencies {
                implementation(compose.desktop.currentOs)
                implementation("org.jetbrains.kotlinx:kotlinx-coroutines-swing:1.10.2")
                implementation("fr.nimbyrails:nimby-observation-client:0.7.3")
            }
        }
        val desktopTest by getting { dependencies {
            implementation(kotlin("test"))
            implementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.10.2")
        } }
    }
}
compose.desktop {
    application {
        mainClass = "fr.nimby.tco.MainKt"
        nativeDistributions {
            targetFormats(TargetFormat.Exe, TargetFormat.Msi, TargetFormat.Deb, TargetFormat.Rpm, TargetFormat.Dmg)
            packageName = "NimbyTco"
            packageVersion = numericVersion
            description = "Tableau de contrôle optique NIMBY Rails"
            vendor = "NimbyRails France"
            modules("java.desktop", "java.management", "jdk.unsupported")
            windows { menu = true; shortcut = true; upgradeUuid = "e92aab46-95b8-4fce-8120-8a1ac39fd8e3" }
            linux { packageName = "nimby-tco"; shortcut = true; menuGroup = "Game" }
            macOS { packageVersion = numericVersion.split('.').mapIndexed { index, part -> if (index == 0) part.toInt().coerceAtLeast(1).toString() else part }.joinToString("."); bundleID = "fr.nimbyrails.tco" }
        }
    }
}

// jpackage's desktop hook assumes this XDG directory exists. Include its
// creation in the DEB itself, so installation also works on minimal WSLg.
tasks.withType<org.jetbrains.compose.desktop.application.tasks.AbstractJPackageTask>().configureEach {
    doFirst {
        require('-' !in project.version.toString() || providers.gradleProperty("nativePackageVersion").isPresent) {
            "Une prérelease exige -PnativePackageVersion=X.Y.Z unique et croissante pour l’installateur natif."
        }
    }
    if (targetFormat == TargetFormat.Deb) {
        doLast {
            val deb = destinationDir.get().asFile.listFiles()!!.single { it.extension == "deb" }
            val staging = Files.createTempDirectory("nrf-deb-").toFile()
            fun run(vararg arguments: String) {
                val process = ProcessBuilder(*arguments).redirectErrorStream(true).start()
                val output = process.inputStream.bufferedReader().use { it.readText() }
                check(process.waitFor() == 0) { "DEB packaging failed: $output" }
            }
            try {
                run("dpkg-deb", "--raw-extract", deb.absolutePath, staging.absolutePath)
                val preinst = staging.resolve("DEBIAN/preinst")
                preinst.writeText(preinst.readText().replace("set -e", "set -e\nmkdir -p /usr/share/desktop-directories"))
                run("dpkg-deb", "--root-owner-group", "--build", staging.absolutePath, deb.absolutePath)
            } finally { staging.deleteRecursively() }
        }
    }
}

val hubArchive by tasks.registering(Zip::class) {
    group = "distribution"
    dependsOn("desktopTest", "createDistributable")
    archiveFileName.set("$releaseRoot-$releasePlatform.zip")
    destinationDirectory.set(layout.buildDirectory.dir("release/${project.version}/$releasePlatform"))
    isPreserveFileTimestamps = false
    isReproducibleFileOrder = true
    into(releaseRoot) { from(layout.buildDirectory.dir("compose/binaries/main/app/NimbyTco")) }
    eachFile { permissions { unix(if (file.canExecute()) "rwxr-xr-x" else "rw-r--r--") } }
    doFirst {
        require(releasePlatform in setOf("windows-x64", "linux-x64")) { "Le SDK du jeu n’est pas encore qualifié pour $releasePlatform" }
        val application = layout.buildDirectory.dir("compose/binaries/main/app/NimbyTco").get().asFile
        require(application.resolve(if (hostOs == "windows") "NimbyTco.exe" else "bin/NimbyTco").isFile) { "Application TCO absente" }
    }
}

tasks.register("prepareRelease") {
    group = "distribution"
    description = "Vérifie et prépare les installateurs et le paquet Hub, sans publication."
    dependsOn(hubArchive, "packageDistributionForCurrentOS")
    doLast {
        val archive = hubArchive.get().archiveFile.get().asFile
        val destination = archive.parentFile
        fun hash(file: File): String {
            val digest = MessageDigest.getInstance("SHA-256")
            file.inputStream().use { input -> val buffer = ByteArray(65536); while (true) { val count = input.read(buffer); if (count < 0) break; digest.update(buffer, 0, count) } }
            return digest.digest().joinToString("") { "%02x".format(it) }
        }
        val packages = layout.buildDirectory.dir("compose/binaries/main").get().asFile.walkTopDown()
            .filter { it.isFile && it.extension in setOf("exe", "msi", "deb", "rpm") && it.parentFile.name == it.extension }
            .map { it.copyTo(destination.resolve("$releaseRoot-$releasePlatform.${it.extension}"), overwrite = true) }.toList()
        require(packages.any { it.extension == if (hostOs == "windows") "exe" else "deb" })
        (packages + archive).forEach { destination.resolve("${it.name}.sha256").writeText("${hash(it)}  ${it.name}\n") }
        val metadata = mapOf("id" to "tco", "kind" to "tco", "name" to "Nimby TCO", "version" to project.version.toString(),
            "platform" to releasePlatform, "channel" to project.version.toString().substringAfter('-', "stable").substringBefore('.'),
            "rootFolder" to releaseRoot, "size" to archive.length(), "sha256" to hash(archive),
            "sdkMin" to "0.7.3", "sdkMaxExclusive" to "0.8.0",
            "gameSha256" to listOf(if (hostOs == "windows") "fff49ac21720abfc824c2b4f68b862727630eb0db71cfe1f9ea8f685d0db10ae" else "2581d0e8157f43acb137b2bd9d52e2a7c82bd8af8b62fab5d87f00cc27eefde6"),
            "url" to "https://github.com/NimbyRails-France/tco/releases/download/v${project.version}/${archive.name}")
        val json = groovy.json.JsonOutput.prettyPrint(groovy.json.JsonOutput.toJson(metadata)) + "\n"
        destination.resolve("project-$releasePlatform.json").writeText(json)
        destination.resolve("project.json").writeText(json)
        logger.lifecycle("Release TCO ${project.version} / $releasePlatform prête dans $destination")
    }
}
