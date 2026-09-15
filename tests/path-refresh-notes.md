# Validation du rafraîchissement des Paths

14 septembre 2026 (heure locale).

Le SDK et le rendu remplaçaient déjà la liste à chaque capture : l'observation
initiale en lecture seule de 25 snapshots a constaté 5 Paths modifiés, 5 devenus
indisponibles et 2 apparus. Le rendu isolé passe du doré #e4bf50 au fond #070b0a
lorsque le snapshot fournit une liste vide. Le cas précis signalé par l'utilisateur
n'est donc pas attribué à une accumulation systématique de la liste QML.

Deux conservations possibles sont supprimées : ancien Path pendant une nouvelle
sélection, et dernier Path affiché sans limite lorsque les captures ne reviennent
plus. La sélection efface immédiatement les champs Path et conserve la protection
par révision ; un timer GUI invalide le tracé après 1500 ms sans snapshot accepté.
Le peintre exige live, pathAvailable et absence de pathStale avant de dessiner.

`ctest --test-dir build --output-on-failure` exécute `path_refresh`.
Le test utilise une implémentation SDK synthétique (aucun processus jeu ouvert),
le vrai ReaderThread, le Backend et MapItem. Il couvre réduction, disparition,
liste vide valide, disparition du train, changement de sélection pendant une
capture, expiration, reprise, erreur de capture, déconnexion et pixels du rendu.
Les données synthétiques ne valident pas les offsets du jeu ni le chemin restant.

Le Path public reste l'appartenance au vecteur natif, sans interprétation des
voies déjà parcourues. Une conservation par le jeu n'est pas un cache QML ; il
faut identifier le train et observer cette transition avant de modifier le SDK.
