import "TrainFilter.js" as TrainFilter
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var backend
    required property var updater
    width: 1440; height: 900; minimumWidth: 1060; minimumHeight: 650
    visible: true
    title: "Nimby TCO v" + Qt.application.version + " — observation du réseau"
    color: "#0b1117"
    font.family: "Segoe UI"
    palette.windowText: "#d6e1eb"
    palette.text: "#d6e1eb"
    palette.buttonText: "#d6e1eb"
    palette.button: "#243443"
    palette.base: "#14202b"
    palette.highlight: "#58c4c7"
    property var live: backend.data
    property string selectedTrack: ""
    property string selectedSignal: ""
    readonly property string selectedSignalText: {
        if (!selectedSignal) return ""
        for (let track of tracks) for (let signal of track.signals || [])
            if (signal.id === selectedSignal) return signal.kind+" · "+signal.id+" · "+signal.aspect+" · "+signal.specificState
        return "Signal hors de la vue courante"
    }
    property bool panelMode: true
    property real signalSize: 64
    Connections {
        target: root.backend
        function onChanged() {
            if (!root.backend.data.live) { root.selectedSignal=""; root.selectedTrack="" }
        }
    }
    property var tracks: live.tracks || []
    property var trains: live.trains || []
    property var filteredTrains: trains.filter(function(train) {
        return TrainFilter.matches(train, trainSearch.text, locationFilter.currentIndex, motionFilter.currentIndex)
    })
    property var stations: [{id: "", name: "Toutes les gares"}].concat(live.stations || [])
    property string stationFilter: ""
    property var visibleTracks: tracks
    function updateView(page) { backend.setView(scope.currentIndex, stationFilter, selectedTrack, page) }
    onStationFilterChanged: updateView(0)
    onSelectedTrackChanged: updateView(0)

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 18
        RowLayout {
            Layout.fillWidth: true; spacing: 14
            Rectangle {
                width: 42; height: 42; radius: 7; color: "#143d41"
                Label { anchors.centerIn: parent; text: "NR"; color: "#75e3df"; font.bold: true; font.pixelSize: 20 }
            }
            ColumnLayout {
                spacing: 0
                Label { text: "NIMBY / TCO v" + Qt.application.version; font.pixelSize: 24; font.weight: Font.DemiBold; color: "#edf5fa" }
                Label { text: "TABLEAU DE CONTRÔLE OPTIQUE · LABORATOIRE SDK"; font.pixelSize: 10; font.letterSpacing: 1.5; color: "#8a9ba9" }
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                Layout.preferredWidth: 116; Layout.preferredHeight: 30; radius: 15; color: live.live ? "#173c33" : "#3e3121"
                Label { anchors.centerIn: parent; text: live.live ? "●  EN DIRECT" : "●  EN ATTENTE"; color: live.live ? "#7bdbae" : "#efbe72"; font.pixelSize: 11; font.bold: true }
            }
            TextField { id: pid; Layout.preferredWidth: 142; placeholderText: "PID · auto si vide"; selectByMouse: true; onAccepted: backend.connectGame(text) }
            Button { text: "Connecter"; onClicked: backend.connectGame(pid.text) }
            Button { text: "Déconnecter"; onClicked: backend.disconnectGame() }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 14
            Repeater {
                model: [ {label:"CIRCULATIONS",value:live.trainCount || 0,accent:"#f1bf70"},
                    {label:"SEGMENTS DE VOIE",value:live.trackCount || 0,accent:"#68d2d0"},
                    {label:"GARES",value:live.stationCount || 0,accent:"#a6b3d6"},
                    {label:"SIGNAUX / BALISES",value:(live.signalCount || 0)+" / "+(live.baliseCount || 0),accent:"#68d2d0"} ]
                Rectangle {
                    required property var modelData
                    Layout.fillWidth: true; Layout.preferredHeight: 80; radius: 8; color: "#141e28"; border.color: "#26333f"
                    Column { anchors.left: parent.left; anchors.leftMargin: 18; anchors.verticalCenter: parent.verticalCenter; spacing: 5
                        Label { text: modelData.label; color: "#8fa2b3"; font.pixelSize: 10; font.letterSpacing: 1 }
                        Label { text: String(modelData.value); color: modelData.accent; font.pixelSize: 26; font.weight: Font.DemiBold }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
            Rectangle {
                Layout.preferredWidth: 290; Layout.fillHeight: true; color: "#111b24"; radius: 8; border.color: "#26333f"
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 16
                    Label { text: "CIRCULATIONS"; font.bold: true; font.pixelSize: 12; font.letterSpacing: 1; color: "#a6b7c7" }
                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: "Choisir un train pour afficher ses réservations."; color: "#8095a8"; font.pixelSize: 11 }
                    TextField {
                        id: trainSearch; objectName: "trainSearch"
                        Layout.fillWidth: true; placeholderText: "Nom, identifiant ou ligne…"; selectByMouse: true
                        onTextChanged: trainList.positionViewAtBeginning()
                    }
                    ComboBox {
                        id: locationFilter; objectName: "locationFilter"; Layout.fillWidth: true
                        model: ["Toutes les localisations", "Trains localisés", "Trains non localisés"]
                        onActivated: trainList.positionViewAtBeginning()
                    }
                    ComboBox {
                        id: motionFilter; objectName: "motionFilter"; Layout.fillWidth: true
                        model: ["Toutes les vitesses", "En mouvement", "À l’arrêt (mesuré)", "Vitesse non mesurée"]
                        onActivated: trainList.positionViewAtBeginning()
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { Layout.fillWidth: true; text: root.filteredTrains.length+" / "+root.trains.length+" trains"; color: "#8fa2b3" }
                        Button { text: "Effacer"; enabled: trainSearch.text!=="" || locationFilter.currentIndex!==0 || motionFilter.currentIndex!==0
                            onClicked: { trainSearch.clear(); locationFilter.currentIndex=0; motionFilter.currentIndex=0; trainList.positionViewAtBeginning() } }
                    }
                    ListView {
                        id: trainList; objectName: "trainList"
                        ScrollBar.vertical: ScrollBar {}
                        Label { anchors.centerIn: parent; width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                            visible: trainList.count===0; text: root.live.live ? "Aucun train pour ces filtres" : "En attente des trains"; color: "#8fa2b3" }
                        Layout.fillHeight: true; Layout.fillWidth: true; model: root.filteredTrains; spacing: 8; clip: true
                        delegate: Rectangle {
                            id: trainCard
                            required property var modelData
                            width: ListView.view.width; height: 92; radius: 6
                            color: root.live.selectedTrainId === modelData.id ? "#233a43" : "#1a2733"
                            border.color: root.live.selectedTrainId === modelData.id ? "#68d2d0" : "#263c4c"
                            Column {
                                anchors.fill: parent; anchors.margins: 12; spacing: 6
                                Label { width: parent.width; elide: Text.ElideRight; text: trainCard.modelData.name || trainCard.modelData.id; color: "#e7edf3"; font.bold: true; font.pixelSize: 14 }
                                Label { text: trainCard.modelData.speedAvailable && !trainCard.modelData.speedDefaulted ? trainCard.modelData.speed.toFixed(1)+" km/h" : "Vitesse non mesurée"; color: "#efbe72"; font.pixelSize: 16 }
                                Label { text: trainCard.modelData.positioned ? "Voie …"+trainCard.modelData.track.slice(-8) : "Non localisé dans les états validés"; font.pixelSize: 10; color: "#8c9fac" }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { backend.selectTrain(trainCard.modelData.id); root.selectedTrack = trainCard.modelData.track; root.stationFilter=""; station.currentIndex=0; if(trainCard.modelData.positioned) scope.currentIndex=3 } }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 104; color: "#182a32"; radius: 6
                        Label { anchors.fill: parent; anchors.margins: 12; wrapMode: Text.WordWrap; color: "#9fb9c7"; font.pixelSize: 11
                            text: "LECTURE SEULE\n\nLe panneau associe les symboles à leur voie. L’occupation des cantons reste inconnue." }
                    }
                }
            }
            Rectangle {
                Layout.fillHeight: true; Layout.fillWidth: true; color: "#0f1922"; radius: 8; border.color: "#26333f"
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 18; spacing: 12
                    RowLayout {
                        Label { text: "SYNOPTIQUE"; color: "#dce5ec"; font.pixelSize: 15; font.bold: true; font.letterSpacing: 1 }
                        Button { text: root.panelMode ? "Liste" : "Panneau TCO"; onClicked: root.panelMode=!root.panelMode }
                        Item { Layout.fillWidth: true }
                        ComboBox { id: scope; visible: !root.panelMode; Layout.preferredWidth: 158; model: ["Circulations", "Avec signaux", "Toutes les voies", "Sélection"]; onCurrentIndexChanged: root.updateView(0) }
                        ComboBox { id: station; visible: !root.panelMode; Layout.preferredWidth: 200; model: root.stations; textRole: "name"; onActivated: root.stationFilter=root.stations[currentIndex].id }
                        ComboBox { id: symbols; visible: !root.panelMode; Layout.preferredWidth: 142; model: ["Symboles TCO", "Contraste élevé"] }
                    }
                    RowLayout {
                        Label { Layout.fillWidth: true; text: root.panelMode ? "Carte entière · caméra libre · capture "+(root.live.captureMs || 0)+" ms" : "Segments indépendants · positions en fraction de voie"; color: "#91a6b9"; font.pixelSize: 12 }
                        Button { visible: !root.panelMode; text: "‹"; enabled: (live.page || 0)>0; onClicked: root.updateView(live.page-1) }
                        Label { visible: !root.panelMode; text: ((live.page || 0)+1)+" / "+(live.pages || 1)+" · "+(live.filteredCount || 0)+" voies"; color: "#91a6b9" }
                        Button { visible: !root.panelMode; text: "›"; enabled: (live.page || 0)+1<(live.pages || 1); onClicked: root.updateView(live.page+1) }
                    }
                    RowLayout {
                        spacing: 12
                        Label { text: "━  Voie observée"; color: "#b8c7d2"; font.pixelSize: 11 }
                        Label { text: "▰  Train"; color: "#efbe72"; font.pixelSize: 11 }
                        Label { text: "Signaux : textures natives"; color: "#74c4e8"; font.pixelSize: 11 }
                        Label { text: "◇  Balise"; color: "#c5b8ef"; font.pixelSize: 11 }
                        Label { text: "M  Repère"; color: "#e6c789"; font.pixelSize: 11 }
                        Item { Layout.fillWidth: true }
                        Label { text: "Taille : "+Math.round(root.signalSize); color: "#b8c7d2"; font.pixelSize: 11 }
                        Slider { Layout.preferredWidth: 110; from:24; to:96; stepSize:4; value:root.signalSize; onMoved:root.signalSize=value }
                    }
                    Panel {
                        Layout.fillWidth: true; Layout.fillHeight: true; visible: root.panelMode
                        liveData: root.live
                        signalSize: root.signalSize
                        onTrainSelected: trainId => backend.selectTrain(trainId)
                        
                    }
                    ListView {
                        id: board
                        visible: !root.panelMode
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 10
                        model: root.visibleTracks
                        ScrollBar.vertical: ScrollBar { }
                        delegate: Rectangle {
                            id: segment
                            required property var modelData
                            width: ListView.view.width - 12; height: 188; radius: 5
                            color: "#14212c"; border.color: root.selectedTrack===modelData.id ? "#4c9296" : "#263746"
                            Row {
                                anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 12; spacing: 16
                                Label { text: "VOIE "+segment.modelData.id; font.family: "Consolas"; font.pixelSize: 11; color: "#93abbc" }
                                Label { text: segment.modelData.station; color: "#c3d2dd"; font.pixelSize: 11 }
                                Label { text: segment.modelData.limit.toFixed(0)+" km/h"; color: "#8095a8"; font.pixelSize: 11 }
                            }
                            Item {
                                id: rail
                                anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 52
                                y: 132; height: 8
                                Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: symbols.currentIndex===1 ? 6 : 3; color: "#b9c8d3" }
                                Repeater {
                                    model: 2
                                    Rectangle { required property int index; x: index===0 ? 0 : rail.width-2; y:-5; width:2; height:18; color:"#dce7ec" }
                                }
                                Repeater {
                                    model: segment.modelData.signals
                                    Item {
                                        id: sig
                                        required property var modelData
                                        x: modelData.fraction*rail.width; y:-28; width:24; height:52
                                        Image { id: signalImage; x:-width/2; y:24-height; width:root.signalSize; height:root.signalSize
                                            source:sig.modelData.textureUrl || ""; sourceSize.width:192; sourceSize.height:192
                                            fillMode:Image.PreserveAspectFit; asynchronous:false }
                                        Rectangle { visible:signalImage.status!==Image.Ready; x:0; y:15; height:17; width:2; color:"#758ea1" }
                                        Rectangle { x:-7; y:0; width:16; height:16; radius:(sig.modelData.balise||sig.modelData.marker)?0:8; rotation:sig.modelData.balise?45:0
                                            visible:signalImage.status!==Image.Ready
                                            color:"#14212c"; border.width:symbols.currentIndex===1?3:2; border.color:sig.modelData.balise?"#c5b8ef":"#74c4e8" }
                                        Label { visible:sig.modelData.marker&&signalImage.status!==Image.Ready; x:-4; y:0; text:"M"; color:"#e6c789"; font.pixelSize:12 }
                                        Label { visible:signalImage.status!==Image.Ready; x:-18; y:-18; text:sig.modelData.aspect; font.pixelSize:9; color:"#b9c8d3" }
                                        MouseArea { x:-root.signalSize/2; y:24-root.signalSize; width:root.signalSize; height:root.signalSize+12; hoverEnabled:true; cursorShape:Qt.PointingHandCursor
                                            onClicked: root.selectedSignal=sig.modelData.id
                                            ToolTip.visible:containsMouse
                                            ToolTip.text:sig.modelData.kind+"\n"+sig.modelData.id+"\n"+sig.modelData.aspect+" · sens "+sig.modelData.direction+"\n"+sig.modelData.specificState
                                        }
                                    }
                                }
                                Repeater {
                                    model: segment.modelData.trains
                                    Item {
                                        id: marker
                                        required property var modelData
                                        x: modelData.fraction*rail.width; width:1; height:10
                                        Rectangle { x:-13; y:-5; width:26; height:13; radius:2; color:"#efbe72"; border.color:"#ffe4ae" }
                                        Label { x:-48; y:17; width:96; horizontalAlignment:Text.AlignHCenter; text:marker.modelData.name; color:"#f4cc8b"; font.pixelSize:11; font.bold:true }
                                        Label { x:-45; y:-57; width:90; horizontalAlignment:Text.AlignHCenter; text:!marker.modelData.speedAvailable || marker.modelData.speedDefaulted ? "Vitesse inconnue" : (marker.modelData.direction>0?"→ ":"← ")+marker.modelData.speed.toFixed(1)+" km/h"; color:"#efbe72"; font.pixelSize:11 }
                                    }
                                }
                            }
                        }
                        Label { anchors.centerIn:parent; visible:board.count===0; text:root.live.live ? "Aucune voie pour ce filtre" : "En attente de données du jeu"; color:"#8fa4b7"; font.pixelSize:17 }
                    }
                    Label { Layout.fillWidth:true; elide:Text.ElideRight; text:root.selectedSignalText || "Voies et positions observées · états natifs des signaux dans la liste"; color:"#869dad"; font.pixelSize:11 }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: root.live.status || "Déconnecté"; color:root.live.live?"#7bdbae":"#efbe72"; font.pixelSize:12; Layout.fillWidth:true; elide:Text.ElideRight }
            Label { text:updater.status; color:"#91a6b9"; font.pixelSize:11 }
            Button { text:updater.ready?"Redémarrer et mettre à jour":"Vérifier les mises à jour"; onClicked:updater.ready?updater.restart():updater.check() }
                    Label { text: "NimbyRailsFranceSDK 0.7 · observation expérimentale · "+(root.live.updated || "--:--:--"); color:"#758b9c"; font.pixelSize:11 }
        }
    }
}
