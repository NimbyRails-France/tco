# NimbyTco dans CLion

Ouvrir **ce dossier** comme un projet indépendant. Profils **Debug** et **Release**, configurations d’exécution partagées dans `.run/`.

Les profils sont définis dans `CMakePresets.json`. Compiler avec `cmake --preset Debug`, puis `cmake --build --preset Debug`. Tester avec `ctest --preset Debug`. Remplacer Debug par Release pour la distribution.

Qt 6.11.2 MinGW est attendu dans `C:/Qt/6.11.2/mingw_64` et son compilateur dans `C:/Qt/Tools/mingw1310_64`. Le runtime Qt est déployé automatiquement après compilation pour permettre Run/Debug dans CLion.

Le SDK est une dépendance externe : compiler et installer d’abord le projet voisin `../sdk` en Release. Le profil cherche son package CMake dans `../sdk/install/Release`. Pour utiliser un SDK distribué ailleurs, modifier `NimbyRailsSDK_DIR` dans un preset utilisateur. Aucun source du SDK n’est compilé dans ce projet.

Les emplacements de compilation, de distribution et les réglages personnels CLion sont exclus de Git. Adapter les chemins des outils avec `CMakeUserPresets.json` sur une autre machine. Les sources et configurations partagées restent dans le dépôt tco.
