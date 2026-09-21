# Nimby TCO — Kotlin Compose

Le TCO est un projet Kotlin/Gradle pour IntelliJ IDEA, avec JDK 21.
Qt, QML, CMake et les compilateurs C++ ne sont plus nécessaires côté TCO.
Le SDK conserve son implémentation native et fournit un client Kotlin.

Cloner le dépôt SDK à côté de celui-ci (`../sdk/kotlin-client`), ou passer
`-PnrfSdkClientDir=/chemin/sdk/kotlin-client`.

```sh
./gradlew desktopTest run
./gradlew prepareRelease
```

Sous Windows, utiliser `gradlew.bat`. `build.ps1` compile et teste ;
`package.ps1` et `package-installer.ps1` préparent les paquets Gradle.
Les installateurs sont construits sur leur système cible : MSI/EXE Windows,
DEB/RPM Linux. `prepareRelease` produit aussi le ZIP portable et les manifestes
par plateforme, sans publier. Les sorties vont dans `build/kotlin-<système>/`.
Une préversion exige `-PnativePackageVersion=X.Y.Z` pour son installateur.
macOS n'est pas une plateforme SDK qualifiée pour cette migration.

Dans l'application, choisir la DLL Windows ou la bibliothèque `.so` Linux du
SDK. Le PID est facultatif pour la détection automatique. Le serveur de lecture
SDK doit être chargé dans le jeu sous Linux.

La carte, les trains, les filtres, les détails, les occupations, les réservations
et les images de signaux sont en Kotlin. La lecture d'une partie Linux réelle
avec 947 trains a été vérifiée sous WSL. Les 10 tests TCO passent sur Windows
et Linux ; cela ne valide pas toute l'intégration du mod dans le jeu.

La migration reste en cours : mise à jour autonome, recette des interfaces et
cycles d'installation/mise à niveau à terminer. Les anciennes sources Qt sont
retirées à la demande du projet ; leur historique demeure dans Git.
Les anciennes licences conservées dans `licenses/` concernent les distributions
historiques, pas de nouvelles dépendances Qt.

Aucune release n'est demandée par les commits de reprise. Les canaux et les
versions restent décrits dans `release-channels.json` et `CHANGELOG.md`.
