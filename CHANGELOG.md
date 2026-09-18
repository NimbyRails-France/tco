# Changelog

## [Unreleased]

## [0.5.2] - 2026-09-16

### Améliorations
- Les signaux restent lisibles lorsque vous dézoomez, avec une taille d’affichage réglable.
- Jusqu’à six signaux superposés peuvent être distingués : chacun est relié à sa voie et dispose de ses propres informations.
- Les balises et les signaux situés au même endroit restent visibles ensemble.
- Les groupes plus importants sont représentés par un compteur pour préserver la lisibilité.

Le TCO affiche l’état du jeu sans le modifier.

## [0.5.1] - 2026-09-16

TCO 0.5.1 corrige la taille et le placement des textures des signaux.

- Textures plus petites en vue d’ensemble, avec taille maximale réglable de 8 à 48 pixels.
- Placement local et fixe par signal : suppression des longs traits et des déplacements destinés à trouver une place libre.
- Les signaux qui se recouvrent affichent un compteur gris, sans choisir la couleur d’un seul signal pour représenter le groupe.
- Suppression des symboles bleus de remplacement. Un « ? » indique une image ou un état indisponible.
- Au survol : identifiant et état natif disponible du signal ou du groupe. Les textures d’origine du jeu et des mods restent celles fournies par le SDK.

Le zoom ne change pas la texture associée à un identifiant. Les changements réels de la simulation et le clignotement continuent d’être actualisés.

SDK 0.7.0 inclus. Installateur Windows x64, ZIP portable et manifestes pour les mises à jour autonomes et NRF Hub.

## [0.5.0] - 2026-09-16

Le TCO 0.5.0 utilise le nouveau SDK 0.7.0 et facilite la sélection des trains.

- Migration vers l’API C++20 de NimbyRailsFranceSDK 0.7.0, inclus dans les paquets.
- Recherche par nom de train, identifiant ou ligne, sans distinction de casse.
- Filtres combinables par localisation et vitesse : en mouvement, à l’arrêt mesuré ou vitesse non mesurée.
- Compteur de résultats, réinitialisation des filtres et sélection du train conservée pendant la recherche.
- Version du TCO affichée dans l’en-tête et le titre de la fenêtre.
- Installateur Windows x64, ZIP portable et manifestes de mise à jour autonome / NRF Hub.

Validation : compilation Release, trois tests automatiques, démarrage de l’interface et contrôle du SDK. Les SDK absents ou anciens sont rejetés. Cette version n’a pas été vérifiée en direct dans une partie.

Les positions restent des repères de voie expérimentaux. Une vitesse inconnue ou fournie par défaut ne signifie pas un arrêt mesuré. Le TCO reste en lecture seule.

## [0.4.0] - 2026-09-15

TCO 0.4.0 : textures natives des signaux, rouge clignotant visible, repères, réglage de taille et vérification du SDK au démarrage.

- Installateur Windows par utilisateur et ZIP portable.
- SDK 0.6.x / ABI 1 inclus ; ancienne DLL refusée avec un message explicite.
- Mise à jour autonome via tco-latest.json ; gestion déléguée à NRF Hub pour ses installations.
- Tests de rendu, péremption et manifeste réussis ; vérification en jeu sur 27 signaux.

Les positions restent des repères de voie expérimentaux. Aucun nom d'aspect ferroviaire n'est déduit des couleurs. Installateur non signé.

## [0.2.0] - 2026-09-14

TÃ©lÃ©charger **NimbyTco-0.2.0-windows-x64.zip** dans Assets, extraire tout le dossier et lancer NimbyTco.exe aprÃ¨s avoir chargÃ© une partie dans NIMBY Rails. Qt et le SDK 0.5.0 sont inclus. Garder tous les fichiers et sous-dossiers ensemble ; le TCO s'utilise Ã  cÃ´tÃ© du jeu. Pour mettre Ã  jour, fermer l'ancien TCO et extraire cette version dans un nouveau dossier.

- Cliquer sur un train pour afficher ses portions rÃ©servÃ©es en vert.
- CarrÃ©s rouges : occupation native de tous les trains.
- Path calculÃ© disponible en option pour comparaison.
- Retraits pris en compte Ã  chaque capture ; sÃ©lection et expiration aprÃ¨s 1,5 seconde effacent les donnÃ©es anciennes. Indisponible ne signifie pas voie libre.

Validation : tests synthÃ©tiques du rafraÃ®chissement, sÃ©lection, disponibilitÃ© indÃ©pendante et rendu rÃ©ussis ; paquet autonome lancÃ© avec seulement Windows dans PATH, 15 captures avec rÃ©servations et occupation disponibles en huit secondes.

ExpÃ©rimental et en lecture seule. Les repÃ¨res sont placÃ©s Ã  la voie : les bornes gÃ©omÃ©triques prÃ©cises ne sont pas tracÃ©es. Ã‰tat des aiguilles et aspects des signaux non validÃ©s. Les portions rÃ©servÃ©es ne forment pas un itinÃ©raire ordonnÃ© et les rÃ©servations virtuelles de scripts ne sont pas interrogÃ©es.

[Installation et utilisation](https://github.com/NimbyRails-France/tco/blob/v0.2.0/README.md) Â· [SDK 0.5.0](https://github.com/NimbyRails-France/sdk/releases/tag/v0.5.0)

## [0.1.0] - 2026-09-14

## Installer et lancer

Télécharger **NimbyTco-0.1.0-windows-x64.zip** dans Assets, extraire **tout** le ZIP,
puis ouvrir **NimbyTco.exe**. Lancer NIMBY Rails et charger une partie ; laisser le
PID vide pour la détection automatique. Qt et le runtime SDK sont déjà inclus.
Aucune installation de Qt ou du SDK séparément, aucun fichier à copier dans le jeu.
**Instructions complètes : [README](https://github.com/NimbyRails-France/tco#installer-depuis-une-release).**

- Carte du réseau navigable : zoom au curseur, déplacement libre et cadrage global.
- Trains et vitesses actualisés depuis le SDK 0.4.0.
- Clic sur un train pour afficher son Path en doré ; choix si plusieurs se superposent.
- Sélection sans déplacement automatique de caméra.
- Vue liste pour examiner les segments et leurs observations.

Windows x64. Version expérimentale, en lecture seule. Certaines positions restent
indisponibles ; aiguilles complètes, réservations et aspects des signaux non validés.
Le SDK refuse un exécutable de jeu inconnu ; empreinte compatible dans le README.

Validation : compilation Release et démarrage/observations réelles depuis le paquet
avec les dépendances fournies. Empreinte du ZIP dans **SHA256SUMS.txt**.
Pour développer : [SDK 0.4.0](https://github.com/NimbyRails-France/sdk/releases/tag/v0.4.0).
