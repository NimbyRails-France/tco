# Nimby TCO — Kotlin Compose

Le TCO est un projet Kotlin/Gradle pour IntelliJ IDEA, avec JDK 21.
Qt, QML, CMake et les compilateurs C++ ne sont plus nécessaires côté TCO.
Le SDK conserve son implémentation native et fournit un client Kotlin.

Le sélecteur de l'en-tête propose **Automatique (système), Français, English**.
La préférence est mémorisée et change les textes sans reconnecter le SDK ni
réinitialiser les filtres. Automatique choisit le français pour un système `fr`,
l'anglais sinon. Les noms du réseau et les diagnostics bruts restent intacts.

The header language selector offers **Automatic (system), Français, English**.
Your preference is saved; changing language does not reconnect the SDK or reset
filters. Automatic mode uses French for a `fr` system language, English otherwise.
Network names and raw diagnostics remain unchanged.

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


## Logs de production

Le Hub propose **Téléchargements → Exporter les logs NRF** : un ZIP local
regroupe les journaux du Hub, du SDK/chargeur, des mods, du TCO et du banc,
ainsi qu'un résumé des versions. Aucun envoi automatique, aucune sauvegarde de
jeu ni fichier de réglages n'est inclus. Les logs peuvent contenir des chemins
personnels et des identifiants d'objets.

Les composants Windows écrivent sous `%LOCALAPPDATA%/NimbyRailsFrance/logs`,
chacun dans son dossier ; le Hub utilise `%LOCALAPPDATA%/NimbyRailsFrance/logs/hub`.
Rotation et regroupement des erreurs répétées limitent le volume. Le TCO et le
banc disposent aussi d'un bouton pour ouvrir leurs journaux.
Voir [le contrat de diagnostic](../sdk/docs/production-diagnostics.md) pour les
emplacements, la rétention, les tests et les limites en cas de crash natif.
