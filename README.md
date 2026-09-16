# Nimby TCO 0.5.1

Tableau de contrôle optique expérimental pour NIMBY Rails, avec textures natives
des signaux, clignotement observé, repères et taille réglable.

## Installation

Utiliser [NRF Hub](https://github.com/NimbyRails-France/hub/releases/latest),
ou télécharger `NimbyTco-0.5.1-Setup.exe` dans les
[releases](https://github.com/NimbyRails-France/tco/releases/latest).
Le ZIP portable est également disponible. Qt et le SDK sont inclus.
Lancer le jeu et charger une partie, puis ouvrir le TCO. Laisser le PID vide
pour la détection automatique. Le TCO fonctionne à côté du jeu ; ne pas placer
son paquet dans le dossier du jeu.

Le TCO requiert **SDK 0.7.x / API C++20**. Il vérifie la DLL au démarrage et affiche
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

Les textures sont petites en vue d’ensemble et grandissent avec le zoom.
Les signaux qui se recouvrent sont regroupés sous un compteur gris, sans
attribuer au groupe la couleur d’un signal. Un « ? » indique un état ou une
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
powershell -File build.ps1 -SdkRoot C:/SDK/NimbyRailsFranceSDK-0.7.0
powershell -File package-installer.ps1 -SdkRoot C:/SDK/NimbyRailsFranceSDK-0.7.0 -Iscc 'C:/Program Files (x86)/Inno Setup 6/ISCC.exe' -FeedUrl 'https://github.com/NimbyRails-France/tco/releases/latest/download/tco-latest.json' -ReleaseBaseUrl 'https://github.com/NimbyRails-France/tco/releases/download/v0.5.1'
```

La publication doit joindre `tco-latest.json`, `project.json`, l'installateur,
le ZIP et leurs empreintes. Publier le manifeste après les exécutables.
Voir `THIRD_PARTY.md` et `licenses/` pour les composants redistribués.

## Projet CLion indépendant

Profils Debug/Release et configurations Run/Debug : [guide CLion](docs/clion.md).
