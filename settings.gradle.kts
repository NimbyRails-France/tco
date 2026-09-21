pluginManagement { repositories { google(); gradlePluginPortal(); mavenCentral() } }
dependencyResolutionManagement { repositories { google(); mavenCentral() } }
rootProject.name = "nimby-tco"
val sdkClient = providers.gradleProperty("nrfSdkClientDir").orNull ?: "../sdk/kotlin-client"
includeBuild(sdkClient)
