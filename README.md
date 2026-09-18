# Nimby TCO 0.5.2

Tableau de contrôle optique expérimental pour NIMBY Rails, avec textures natives
des signaux, clignotement observé, repères et taille réglable.

## Installation

Utiliser [NRF Hub](https://github.com/NimbyRails-France/hub/releases/latest),
ou télécharger `NimbyTco-0.5.2-Setup.exe` dans les
[releases](https://github.com/NimbyRails-France/tco/releases/latest).
Le ZIP portable est également disponible. Qt et le SDK sont inclus.
Lancer le jeu et charger une partie, puis ouvrir le TCO. Laisser le PID vide
pour la détection automatique. Le TCO fonctionne à côté du jeu ; ne pas placer
son paquet dans le dossier du jeu.

Le TCO requiert **SDK 0.7.1 ou ultérieur en 0.7.x / API C++20**. Il vérifie la DLL au démarrage et affiche
une erreur si elle est absente, ancienne ou incomplète. `--check-sdk` permet
un contrôle sans interface (code 0 compatible, code 3 incompatible).
Le jeu reconnu est identifié par SHA-256 :
`FFF49AC21720ABFC824C2B4F68B862727630EB0DB71CFE1F9EA8F685D0DB10AE`.

## Mises à jour

Le paquet autonome vérifie `tco-latest.json` au démarrage puis toutes les six
heures. Il télécharge les nouvelles versions en HTTPS, vérifie leur SHA-256 et
les installe à sa fermeture. Le bouton de redémarrage permet de les appliquer
immédiatement. Les installations du Hub lui délèguent leurs mises à jour.
Les installateurs sont actuellement non signés avec un certificat de publication.

## Affichage

Molette pour zoomer, glisser pour déplacer la carte. Cliquer sur un train pour
afficher ses réservations en vert ; l'occupation native est rouge. Le Path
calculé reste un calque facultatif. La liste permet de consulter les détails.
Les textures des signaux suivent le sélecteur natif ; une balise ne recouvre
plus le signal situé sur la même voie. Les données périmées sont effacées.

La taille réglable des symboles reste constante à l’écran, même au dézoom.
Les marges transparentes des images sont retirées pour rendre les mods lisibles.
Jusqu’à six signaux superposés sont espacés à l’écran, chacun relié à sa voie
par un trait et avec sa propre infobulle. Les groupes plus nombreux restent
sous un compteur gris, sans attribuer au groupe la couleur d’un signal.
Un « ? » indique un état ou une
image indisponible. Survoler un signal ou un groupe affiche les identifiants
et les états natifs disponibles. Le zoom ne choisit jamais une autre texture
pour un même signal ; les changements réels du jeu continuent à être actualisés.

Les positions sont repérées à la voie, sans interpolation exacte des courbes.
Les réservations sont des portions natives et ne représentent pas nécessairement
les réservations virtuelles des scripts. Les noms d'aspects ferroviaires ne sont
pas déduits des couleurs. Une donnée indisponible ne signifie pas voie libre.
Aucune écriture dans le jeu n'est effectuée par le TCO.

La liste des trains propose une recherche instantanée par nom, identifiant
hexadécimal ou nom de ligne, sans distinction de casse. Les filtres de
localisation et de vitesse se combinent avec la recherche. Une vitesse absente
ou fournie par défaut par le SDK reste « non mesurée », jamais un arrêt
mesuré. Le compteur indique les résultats visibles ; « Effacer » réinitialise
les filtres. Ils concernent la liste des trains ; la carte conserve le réseau
complet et ses occupations. La sélection du train reste active pendant la recherche.

## Construire

Qt 6.11.2 MinGW x64, CMake et Ninja, puis :

```powershell
powershell -File build.ps1 -SdkRoot C:/SDK/NimbyRailsFranceSDK-0.7.1
powershell -File package-installer.ps1 -SdkRoot C:/SDK/NimbyRailsFranceSDK-0.7.1 -Iscc 'C:/Program Files (x86)/Inno Setup 6/ISCC.exe' -FeedUrl 'https://github.com/NimbyRails-France/tco/releases/latest/download/tco-latest.json' -ReleaseBaseUrl 'https://github.com/NimbyRails-France/tco/releases/download/v0.5.2'
```

La publication doit joindre `tco-latest.json`, `project.json`, l'installateur,
le ZIP et leurs empreintes. Publier le manifeste après les exécutables.
Voir `THIRD_PARTY.md` et `licenses/` pour les composants redistribués.

## Projet CLion indépendant

Profils Debug/Release et configurations Run/Debug : [guide CLion](docs/clion.md).

## Versions, changelog et notifications

- La version de référence est dans `VERSION`. Elle doit correspondre à `CMakeLists.txt` ou à `package.json` et son lockfile, selon le projet.
- Documenter les changements dans `CHANGELOG.md`, sous `[Unreleased]` pendant le développement, puis dans une section `## [X.Y.Z] - AAAA-MM-JJ` au moment de publier.
- Après une CI réussie, créer le tag `vX.Y.Z` sur le commit vérifié et publier sa release GitHub avec les notes de cette section (`python .woodpecker/check-release.py --notes`). Joindre les artefacts construits avec l'outillage habituel lorsqu'ils sont nécessaires.
- Les builds Woodpecker sont annoncés dans le salon Discord `1550478726557470791`. Seules les releases GitHub publiées, versionnées et avec des notes sont annoncées dans `1549088597594873907`. Un push ou un tag seul ne publie aucune annonce de mise à jour.
- La CI refuse les incohérences de versions et les tags sans changelog daté. Les releases en brouillon ne sont pas annoncées. Une correction des notes modifie l'annonce existante.

Woodpecker compile Windows x64 avec MinGW et exécute les tests CTest autonomes sous Wine. Cela ne remplace pas les essais dans le jeu ni la validation native Windows des installateurs et scripts PowerShell.

La compilation du TCO utilise les en-têtes SDK 0.7.1, épinglés au commit du tag publié, comme son packaging. Passer aux en-têtes 0.7.2 nécessite aussi d'adapter les symboles du pont dynamique `sdkclient.cpp` et les fixtures de tests ; la CI ne change pas cette dépendance implicitement.

## Canaux de publication

**Stable** : `vX.Y.Z` (release normale). **Bêta** : `vX.Y.Z-beta.N`. **Alpha** : `vX.Y.Z-alpha.N` (ces deux dernières sont des prereleases GitHub). `N` commence à 1. Le Hub mémorise un canal par projet, stable par défaut, sans basculer vers un autre canal si aucune release n’existe. Un retour vers une version plus ancienne nécessite une installation manuelle.

`VERSION` et le manifeste portent la version complète ; la version CMake garde seulement `X.Y.Z`. Publier le ZIP et son `project.json` dans la **même release**, avec son changelog. Pour le Hub lui-même, publier l’installateur et `hub-latest.json`. Le manifeste donne la taille, le SHA-256, le dossier racine et les règles de compatibilité. Aucun catalogue central ne doit être modifié.

La politique est dans `release-channels.json`. Le contrôle `.woodpecker/check-release.py` refuse les autres canaux. Une release de test n’est jamais marquée comme dernière version stable.

## Publier une mise à jour

- **main** : canal stable.
- **alpha** : canal alpha.
- **beta** : canal beta.

Un commit ordinaire lance les vérifications sans publier. Pour publier, préparez la même version dans `VERSION` et les fichiers de version du projet, puis ajoutez une entrée datée dans `CHANGELOG.md`. Décrivez les nouveautés, améliorations et corrections du point de vue des utilisateurs.

Le titre exact du commit de publication est `release X.Y.Z` (exemple : alpha : `release 0.4.0-alpha.1` ; beta : `release 0.4.0-beta.1`). Poussez ce commit sur la branche du canal choisi. La compilation, les tests et la préparation des téléchargements doivent réussir avant la publication GitHub et son annonce Discord. Une version déjà publiée ne peut pas être remplacée : choisissez un nouveau numéro.

Ne créez pas le tag à la main. Les préversions restent dans leur canal et ne remplacent pas la version stable.
