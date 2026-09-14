# Nimby TCO 0.2.0

Tableau de contrôle optique expérimental pour NIMBY Rails, en C++20 et Qt Quick/QML.
Il utilise uniquement l'API publique de [NimbyRailsSDK](https://github.com/NimbyRails-France/sdk).

## Installer depuis une release

1. Aller dans les [releases](https://github.com/NimbyRails-France/tco/releases).
2. Dans **Assets**, télécharger **NimbyTco-0.2.0-windows-x64.zip** (pas « Source code »).
3. Extraire **tout** le ZIP dans un dossier, par exemple `C:/Games/NimbyTco/`.
4. Lancer NIMBY Rails et charger une partie, puis ouvrir **NimbyTco.exe** dans le dossier extrait.
5. Laisser le PID vide pour la détection automatique. Le TCO se connecte au jeu reconnu.

Windows x64 requis. Qt, MinGW et le runtime du SDK sont inclus : rien à installer
séparément. Garder tous les fichiers et sous-dossiers près de l'exécutable.
Le TCO s'exécute à côté du jeu : ne pas copier ces fichiers dans le dossier de NIMBY Rails.
Après redémarrage du jeu, la détection automatique recherche le nouveau processus.
Pour mettre à jour, fermer le TCO et extraire la nouvelle release dans un nouveau dossier.
Pour désinstaller, fermer le TCO et supprimer son dossier.

Vérifier le ZIP avec `Get-FileHash chemin/du/ZIP -Algorithm SHA256` et le fichier
`SHA256SUMS.txt` joint à la release.

## Utiliser la carte

- Molette : zoom autour du curseur. Glisser : déplacer librement la carte.
- **Tout voir** : cadrer le réseau entier. Double-clic : zoom.
- Cliquer sur un marqueur de train : afficher ses portions réservées en vert.
- Carrés rouges : occupation native de tous les trains. Cercles verts : réservations du train sélectionné.
- **Path calculé** : afficher aussi en doré les voies du Path, pour comparaison.
- Plusieurs trains proches : une liste permet de choisir par nom et identifiant.
- **Centrer train** : déplacer explicitement la caméra vers le train sélectionné.
- **Liste** : observer les voies et détails par segments.

Les positions sont repérées à la voie ; la fraction native est affichée mais
les marqueurs ne constituent pas une interpolation géométrique exacte.
La fréquence dépend de la taille de la sauvegarde ; le temps de capture est affiché.
Réservations, occupation et Path sont remplacés à chaque capture, y compris par une liste vide. Un changement
de train efface immédiatement l'ancien tracé ; les résultats en vol de l'ancienne
sélection sont rejetés. Après 1,5 seconde sans nouvelle capture reçue, chaque calque est
effacé et marqué « périmé » jusqu'au prochain snapshot. Cela ne transforme pas
le Path natif complet en chemin restant : les voies que le jeu conserve dans son
Path restent présentes tant que le SDK les observe.
Une position inconnue reste indisponible. Les Paths sont expérimentaux : ils ne
prouvent ni réservation ni occupation. Raccordements d'aiguilles incomplets ;
aspects des signaux et correspondance des textures non validés. Aucune écriture dans le jeu.

Le compteur de réservations indique des **portions natives**, pas des itinéraires
ordonnés. Les repères sont dessinés à la voie : les bornes d'intervalle sont lues
mais ne sont pas interpolées sur des courbes non validées. Une réservation virtuelle
de script peut ne pas figurer dans ces données. « Indisponible » ou « périmé »
ne signifie jamais voie libre. Les collections sont observées sans garantie d'un tick atomique.

## Jeu compatible

SDK 0.5.0, binaire reconnu par SHA-256 :
`FFF49AC21720ABFC824C2B4F68B862727630EB0DB71CFE1F9EA8F685D0DB10AE`.
Un autre binaire est refusé. Si le jeu est mis à jour, une adaptation du SDK peut
être nécessaire. Une absence de position n'est pas remplacée par une position inventée.

## Compiler depuis les sources

Installer Qt **6.11.2 MinGW x64**, ses outils MinGW 13.1, CMake et Ninja.
Télécharger et extraire le paquet de développement SDK 0.5.0 depuis les
[releases du SDK](https://github.com/NimbyRails-France/sdk/releases).
Depuis ce dépôt :

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ./build.ps1 -SdkRoot C:/SDK/NimbyRailsSDK-0.5.0
./build/NimbyTco.exe
```

Le script accepte `-QtRoot` (défaut `C:/Qt/6.11.2/mingw_64`) et `-QtTools`
(défaut `C:/Qt/Tools`). Dans CLion, ouvrir ce dépôt comme projet CMake,
choisir MinGW Qt et configurer `CMAKE_PREFIX_PATH` avec les dossiers Qt et SDK.
Le script effectue aussi le déploiement des DLL/plugins Qt. Il conserve le
`libwinpthread-1.dll` fourni par le SDK, requis par son compilateur GCC 15.

`--smoke` vérifie le lancement et l'arrêt. `--verify-live` reçoit les observations
pendant huit secondes et exige au moins deux captures avec réservations et occupation disponibles ; le nombre
de positions distinctes est aussi affiché, sans être une condition de succès.

## Organisation

- `main.cpp` : lancement Qt et modes de vérification.
- `backend.cpp/.h` : lecture des snapshots SDK sur un thread de travail.
- `mapitem.cpp/.h` : rendu et sélection des marqueurs sur la carte.
- `Main.qml`, `Panel.qml` : interface, navigation et sélection des trains.
- `build.ps1` : compilation et déploiement local.

Les IDs 64 bits restent des chaînes hexadécimales en QML. Les données sont copiées
et les résultats périmés sont rejetés après changement de connexion ou de sélection.
Les DLL Qt sont liées dynamiquement ; les notices des dépendances accompagnent le ZIP.
Voir [la recherche et ses limites](https://github.com/NimbyRails-France/sdk/tree/main/docs/research).
