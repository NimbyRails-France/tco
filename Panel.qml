import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nimby.Map 1.0
Rectangle {
 id: panel
 required property var liveData
 property real signalSize: 20
 signal trainSelected(string trainId)
 color: "#070b0a"; border.color: "#38493f"; clip: true
 RailMap { id: map; anchors.fill: parent; anchors.topMargin:48; anchors.bottomMargin:42; snapshotData: panel.liveData; showCalculatedPath: pathToggle.checked; signalSize: panel.signalSize }
 MouseArea {
  id: mapMouse
  hoverEnabled: true
  property string signalText: ""
  ToolTip.visible: containsMouse && !pressed && signalText.length>0
  ToolTip.text: signalText
  ToolTip.delay: 350
  Timer { interval: 250; running: mapMouse.containsMouse && !mapMouse.pressed; repeat: true
   onTriggered: mapMouse.signalText=map.signalTextAt(mapMouse.mouseX,mapMouse.mouseY) }
  anchors.fill: map; property real lastX; property real lastY
  property real pressX; property real pressY; property bool moved: false
  cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
  onPressed: mouse=>{lastX=pressX=mouse.x;lastY=pressY=mouse.y;moved=false;trainPicker.close()}
  onPositionChanged: mouse=>{
   if(!pressed){signalText=map.signalTextAt(mouse.x,mouse.y);return}
   if(!moved && Math.hypot(mouse.x-pressX,mouse.y-pressY)<6)return
   moved=true;map.moveView(mouse.x-lastX,mouse.y-lastY);lastX=mouse.x;lastY=mouse.y
  }
  onClicked: mouse=>{
   if(moved)return
   const hits=map.trainsAt(mouse.x,mouse.y)
   if(hits.length===1)panel.trainSelected(hits[0].id)
   else if(hits.length>1){
    trainPicker.candidates=hits
    trainPicker.x=Math.max(0,Math.min(mouse.x,panel.width-trainPicker.width))
    trainPicker.y=Math.max(48,Math.min(mouse.y,panel.height-trainPicker.height-42))
    trainPicker.open()
   }
  }
  onWheel: wheel=>map.zoomAt(wheel.angleDelta.y>0?1.3:1/1.3,wheel.x,wheel.y)
  onDoubleClicked: mouse=>map.zoomAt(2,mouse.x,mouse.y)
 }
 Popup {
  id: trainPicker
  property var candidates: []
  width: Math.min(340,panel.width)
  height: Math.min(300,Math.max(80,panel.height-90),48+candidates.length*40)
  padding: 8
  contentItem: ColumnLayout {
   Label { text: "Choisir un train ("+trainPicker.candidates.length+")" }
   ListView {
    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
    model: trainPicker.candidates
    ScrollBar.vertical: ScrollBar {}
    delegate: ItemDelegate {
     required property var modelData
     width: ListView.view.width; height: 40
     text: modelData.name+" · "+modelData.id
     onClicked: { panel.trainSelected(modelData.id); trainPicker.close() }
    }
   }
  }
 }
 Rectangle {
  anchors.top: parent.top; width: parent.width; height: 48; color: "#dd101b16"
  RowLayout {
   anchors.fill: parent; anchors.margins: 8
   Label { text: "RÉSEAU COMPLET"; color: "#edf1df"; font.bold: true; font.letterSpacing: 1 }
   Label { Layout.fillWidth: true; elide: Text.ElideRight; text: (panel.liveData.selectedTrain || "Aucun train")+" · Réservations : "+(panel.liveData.usageStale?"périmées":panel.liveData.reservationsAvailable?panel.liveData.mapReservations.length:"indisponibles"); color: "#65db87" }
   CheckBox { id: pathToggle; text: "Path calculé"; checked: false; ToolTip.visible: hovered; ToolTip.text: panel.liveData.pathAvailable ? panel.liveData.pathSize+" voies calculées — ne prouve pas une réservation" : "Path indisponible" }
   Item { Layout.fillWidth: true }
   Button { text: "Tout voir"; onClicked: map.fit() }
   Button { text: "Centrer train"; enabled: !!panel.liveData.selectedTrackId; onClicked: map.focusTrain() }
  }
 }
 Rectangle {
  anchors.bottom: parent.bottom; width: parent.width; height: 42; color: "#df101b16"
  Label { anchors.fill: parent; anchors.margins: 8; color: "#a8b9a8"; font.pixelSize: 11; wrapMode: Text.WordWrap
   text: "Vert : réservations du train sélectionné · rouge : occupation de tous les trains ("+(panel.liveData.usageStale?"périmée":panel.liveData.occupationsAvailable?panel.liveData.mapOccupations.length+" portions":"indisponible")+") · doré : Path optionnel\nNombre gris : signaux regroupés · ? : état/image indisponible · survol : identifiant et état natif · traits : position sur la voie" }
 }
}
