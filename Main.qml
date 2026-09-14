import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var backend
    width: 1440; height: 900; minimumWidth: 1060; minimumHeight: 650
    visible: true
    title: "Nimby TCO — observation du réseau"
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
    property bool panelMode: true
    Connections {
        target: root.backend
        function onChanged() {
            if (!root.backend.data.live) { root.selectedSignal=""; root.selectedTrack="" }
        }
    }
    property var tracks: live.tracks || []
    property var trains: live.trains || []
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
                Label { text: "NIMBY / TCO"; font.pixelSize: 24; font.weight: Font.DemiBold; color: "#edf5fa" }
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
                Layout.preferredWidth: 252; Layout.fillHeight: true; color: "#111b24"; radius: 8; border.color: "#26333f"
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 16
                    Label { text: "CIRCULATIONS"; font.bold: true; font.pixelSize: 12; font.letterSpacing: 1; color: "#a6b7c7" }
                    Label { text: "Choisir un train pour afficher ses réservations."; color: "#8095a8"; font.pixelSize: 11 }
                    ListView {
                        Layout.fillHeight: true; Layout.fillWidth: true; model: root.trains; spacing: 8; clip: true
                        delegate: Rectangle {
                            id: trainCard
                            required property var modelData
                            width: ListView.view.width; height: 92; radius: 6
                            color: root.selectedTrack === modelData.track && modelData.positioned ? "#233a43" : "#1a2733"
                            border.color: root.selectedTrack === modelData.track && modelData.positioned ? "#68d2d0" : "#263c4c"
                            Column {
                                anchors.fill: parent; anchors.margins: 12; spacing: 6
                                Label { text: trainCard.modelData.name; color: "#e7edf3"; font.bold: true; font.pixelSize: 14 }
                                Label { text: trainCard.modelData.present ? trainCard.modelData.speed.toFixed(1)+" km/h" : "Vitesse indisponible"; color: "#efbe72"; font.pixelSize: 16 }
                                Label { text: trainCard.modelData.positioned ? "Voie …"+trainCard.modelData.track.slice(-8) : "Non localisé dans les états validés"; font.pixelSize: 10; color: "#8c9fac" }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { backend.selectTrain(trainCard.modelData.id); root.selectedTrack = trainCard.modelData.track; root.stationFilter=""; station.currentIndex=0; scope.currentIndex=3 } }
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
                        spacing: 22
                        Label { text: "━  Voie observée"; color: "#b8c7d2"; font.pixelSize: 11 }
                        Label { text: "▰  Train"; color: "#efbe72"; font.pixelSize: 11 }
                        Label { text: "○  Signal : état inconnu"; color: "#74c4e8"; font.pixelSize: 11 }
                        Label { text: "◇  Balise"; color: "#c5b8ef"; font.pixelSize: 11 }
                    }
                    Panel {
                        Layout.fillWidth: true; Layout.fillHeight: true; visible: root.panelMode
                        liveData: root.live
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
                            width: ListView.view.width - 12; height: 142; radius: 5
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
                                y: 86; height: 8
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
                                        Rectangle { x:0; y:15; height:17; width:2; color:"#758ea1" }
                                        Rectangle { x:-7; y:0; width:16; height:16; radius:sig.modelData.balise?0:8; rotation:sig.modelData.balise?45:0
                                            color:"#14212c"; border.width:symbols.currentIndex===1?3:2; border.color:sig.modelData.balise?"#c5b8ef":"#74c4e8" }
                                        MouseArea { anchors.fill:parent; hoverEnabled:true; cursorShape:Qt.PointingHandCursor
                                            onClicked: root.selectedSignal=sig.modelData.kind+" · "+sig.modelData.id+" · état inconnu"
                                            ToolTip.visible:containsMouse
                                            ToolTip.text:sig.modelData.kind+"\n"+sig.modelData.id+"\nÉtat inconnu · sens "+sig.modelData.direction
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
                                        Label { x:-45; y:-57; width:90; horizontalAlignment:Text.AlignHCenter; text:(marker.modelData.direction>0?"→ ":"← ")+marker.modelData.speed.toFixed(1)+" km/h"; color:"#efbe72"; font.pixelSize:11 }
                                    }
                                }
                            }
                        }
                        Label { anchors.centerIn:parent; visible:board.count===0; text:root.live.live ? "Aucune voie pour ce filtre" : "En attente de données du jeu"; color:"#8fa4b7"; font.pixelSize:17 }
                    }
                    Label { Layout.fillWidth:true; elide:Text.ElideRight; text:root.selectedSignal || "Voies et positions observées · réservations et états des aiguilles non validés"; color:"#869dad"; font.pixelSize:11 }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: root.live.status || "Déconnecté"; color:root.live.live?"#7bdbae":"#efbe72"; font.pixelSize:12; Layout.fillWidth:true; elide:Text.ElideRight }
                    Label { text: "NimbyRailsSDK 0.5 · observation expérimentale · "+(root.live.updated || "--:--:--"); color:"#758b9c"; font.pixelSize:11 }
        }
    }
}
