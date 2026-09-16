// Synthetic SDK snapshots only. No game process is opened or modified.
#include "../backend.h"
#include <nimby/client.hpp>
#include "../mapitem.h"
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QPainter>
#include <QTemporaryDir>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <functional>
#include <thread>
#include <chrono>
namespace {
std::atomic<int> mode{0};std::atomic<bool> delay{false},waiting{false};
constexpr uint64_t train=0x5000000000001,track=0x1000000000001;
bool waitFor(const std::function<bool()>& predicate,int timeout=3500){QElapsedTimer t;t.start();while(t.elapsed()<timeout){QCoreApplication::processEvents();if(predicate())return true;QThread::msleep(10);}return false;}
template<class T> uint32_t output(const std::vector<T>& values,T* out,uint32_t cap,uint32_t* n){*n=static_cast<uint32_t>(values.size());if(!out)return NIMBY_OK;if(cap<*n)return NIMBY_BUFFER_TOO_SMALL;std::copy(values.begin(),values.end(),out);return NIMBY_OK;}
}
extern "C" {
uint32_t NimbyInternal_GetVersion(NimbySdkVersion* v) noexcept{v->abi_version=NIMBY_OBSERVATION_ABI_VERSION;v->major=0;v->minor=7;v->patch=1;return NIMBY_OK;}
uint32_t NimbyInternal_GetSimulationClock(NimbySnapshot,NimbySimulationClock*) noexcept{return NIMBY_DATA_UNAVAILABLE;}
uint32_t NimbyInternal_GetSnapshotInfo(NimbySnapshot,NimbySnapshotInfo* out) noexcept{out->process_id=42;return NIMBY_OK;}
uint32_t NimbyInternal_CopyTrainServices(NimbySnapshot,NimbyTrainService*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_OK;}
uint32_t NimbyInternal_CopyTrainDetails(NimbySnapshot,NimbyTrainDetails*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_OK;}
uint32_t NimbyInternal_CopyPlatforms(NimbySnapshot,NimbyPlatform*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_OK;}
uint32_t NimbyInternal_CopyTrainLineStops(NimbySnapshot,uint64_t,NimbyLineStop*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_DATA_UNAVAILABLE;}

uint32_t NimbyInternal_OpenProcess(uint32_t,uint32_t,NimbySession* out) noexcept{*out=1;return NIMBY_OK;}
uint32_t NimbyInternal_CloseSession(NimbySession) noexcept{return NIMBY_OK;}
uint32_t NimbyInternal_CaptureSnapshot(NimbySession,NimbySnapshot* out) noexcept{
 const int value=mode.load();if(delay.load()){waiting=true;while(delay.load())std::this_thread::sleep_for(std::chrono::milliseconds(10));waiting=false;}
 if(value==5){*out=0;return NIMBY_DATA_UNAVAILABLE;}*out=value+1;return NIMBY_OK;
}
uint32_t NimbyInternal_ReleaseSnapshot(NimbySnapshot) noexcept{return NIMBY_OK;}
uint32_t NimbyInternal_CopyTrains(NimbySnapshot s,NimbyTrain* out,uint32_t cap,uint32_t* n) noexcept{
 std::vector<NimbyTrain> rows;if(s!=5){NimbyTrain t{};t.id=train;t.track_id=track;t.flags=NIMBY_TRAIN_SPEED_VALID|NIMBY_TRAIN_POSITION_VALID;std::strcpy(t.name_utf8,"Fixture A");rows.push_back(t);t.id=train+0x10000;std::strcpy(t.name_utf8,"Fixture B");rows.push_back(t);}return output(rows,out,cap,n);
}
uint32_t NimbyInternal_CopyTracks(NimbySnapshot,NimbyTrack* out,uint32_t cap,uint32_t* n) noexcept{return output(std::vector<NimbyTrack>{{track,0,30},{track+0x10000,0,30}},out,cap,n);}
uint32_t NimbyInternal_CopyStations(NimbySnapshot,NimbyStation*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_OK;}
uint32_t NimbyInternal_CopySignals(NimbySnapshot,NimbySignal* out,uint32_t cap,uint32_t* n) noexcept{return output(std::vector<NimbySignal>{{0x8000000000001,track,0.5,1,4}},out,cap,n);}
uint32_t NimbyInternal_CopySignalStates(NimbySnapshot s,NimbySignalState* out,uint32_t cap,uint32_t* n) noexcept{
 NimbySignalState state{};state.signal_id=0x8000000000001;
 if(s==1){state.flags=NIMBY_SIGNAL_TEXTURE_STATE_VALID;state.texture_state=10;}
 return output(std::vector<NimbySignalState>{state},out,cap,n);
}
uint32_t NimbyInternal_CopySignalTextures(NimbySnapshot s,NimbySignalTexture* out,uint32_t cap,uint32_t* n) noexcept{
 NimbySignalTexture texture{};texture.signal_id=0x8000000000001;
 if(s==1){texture.flags=NIMBY_SIGNAL_TEXTURE_REFERENCE_VALID|NIMBY_SIGNAL_TEXTURE_FILE_VALID;std::strcpy(texture.file_path_utf8,"C:/fixture/signal.svg");}
 return output(std::vector<NimbySignalTexture>{texture},out,cap,n);
}
uint32_t NimbyInternal_CopyTrackNodes(NimbySnapshot,NimbyTrackNode* out,uint32_t cap,uint32_t* n) noexcept{return output(std::vector<NimbyTrackNode>{{track,0,0,0,0}},out,cap,n);}
uint32_t NimbyInternal_CopyTrainPathTracks(NimbySnapshot s,uint64_t,uint64_t* out,uint32_t cap,uint32_t* n) noexcept{
 *n=0;if(s==3||s==5)return NIMBY_DATA_UNAVAILABLE;std::vector<uint64_t> path;if(s!=4){path.push_back(track);if(s==1)path.push_back(track+0x10000);}return output(path,out,cap,n);
}
const char* NimbyInternal_StatusString(uint32_t) noexcept{return "Synthetic capture failure";}
uint32_t NimbyInternal_CopyTrackReservations(NimbySnapshot s,NimbyTrackUsage* out,uint32_t cap,uint32_t* n) noexcept{
 *n=0;if(s==3)return NIMBY_DATA_UNAVAILABLE;
 std::vector<NimbyTrackUsage> rows;if(s!=4&&s!=5){rows.push_back({train,track,.2,.9});rows.push_back({train+0x10000,track,.1,.3});if(s==1)rows.push_back({train,track+0x10000,0,1});}
 return output(rows,out,cap,n);
}
uint32_t NimbyInternal_CopyTrackOccupations(NimbySnapshot s,NimbyTrackUsage* out,uint32_t cap,uint32_t* n) noexcept{
 if(s==4){*n=0;return NIMBY_DATA_UNAVAILABLE;}
 return output(std::vector<NimbyTrackUsage>{{train,track,.2,.3}},out,cap,n);
}
}
int main(int argc,char** argv){
 QGuiApplication app(argc,argv);Backend backend;
 auto size=[&]{return backend.data().value("mapPath").toList().size();};
 auto require=[&](bool ok,const char* what){if(!ok){delay=false;std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}std::printf("PASS %s\n",what);};
 backend.connectGame("42");require(waitFor([&]{return size()==2;}),"first Path");
 auto signal=[&]{return backend.data()["tracks"].toList().first().toMap()["signals"].toList().first().toMap();};
 require(signal()["stateAvailable"].toBool()&&signal()["aspect"].toString()==QString::fromUtf8("État natif 10"),"native signal selector shown");
 require(!signal()["textureUrl"].toString().isEmpty()&&backend.data()["signalTextureCount"].toInt()==1,"native texture reaches the view");
 auto reserved=[&]{return backend.data().value("mapReservations").toList().size();};
 require(reserved()==2,"reservations filtered to selected train");
 mode=1;require(waitFor([&]{return size()==1;}),"shorter Path replaces all previous entries");
 require(!signal()["stateAvailable"].toBool()&&signal()["aspect"].toString()=="Inconnu","unavailable signal replaces last valid state");
 require(signal()["textureUrl"].toString().isEmpty()&&backend.data()["signalTextureCount"].toInt()==0,"unavailable texture clears old image");
 require(reserved()==1,"released reservation removed on next capture");
 mode=2;require(waitFor([&]{return size()==0&&!backend.data()["pathAvailable"].toBool();}),"absent Path clears overlay");
 require(reserved()==0&&!backend.data()["reservationsAvailable"].toBool()&&backend.data()["occupationsAvailable"].toBool(),"unavailable reservations do not hide independent occupation");
 mode=0;require(waitFor([&]{return size()==2;}),"Path returns");
 mode=3;require(waitFor([&]{return size()==0&&backend.data()["pathAvailable"].toBool();}),"valid empty Path clears overlay");
 require(reserved()==0&&backend.data()["reservationsAvailable"].toBool()&&!backend.data()["occupationsAvailable"].toBool(),"observed empty reservation differs from unavailable occupation");
 mode=0;require(waitFor([&]{return size()==2;}),"Path returns again");
 delay=true;require(waitFor([&]{return waiting.load();}),"capture in flight");
 backend.selectTrain("0005000000010001");require(size()==0,"selection clears old Path immediately");
 require(reserved()==0,"selection clears old reservations immediately");
 delay=false;require(waitFor([&]{return size()==2&&backend.data()["selectedTrainId"].toString()=="0005000000010001";}),"new selection received without old result");
 require(reserved()==1,"new train has its own reservations");
 delay=true;require(waitFor([&]{return waiting.load();}),"blocked capture fixture");
 require(waitFor([&]{return size()==0&&backend.data()["pathStale"].toBool();}),"missing updates expire Path");
 require(!signal()["stateAvailable"].toBool()&&signal()["aspect"].toString()==QString::fromUtf8("Périmé"),"signal state expires with stale capture");
 require(signal()["textureUrl"].toString().isEmpty()&&backend.data()["signalTextureCount"].toInt()==0,"expired texture clears old image and count");
 require(backend.data()["mapSignals"].toList().first().toMap()["texturePath"].toString().isEmpty(),"expired map texture cleared");
 require(reserved()==0&&backend.data()["mapOccupations"].toList().isEmpty()&&backend.data()["usageStale"].toBool(),"missing updates expire reservations and occupation");
 delay=false;require(waitFor([&]{return size()==2&&!backend.data()["pathStale"].toBool();}),"fresh capture restores Path");
 mode=4;require(waitFor([&]{return size()==0&&backend.data()["selectedTrainId"].toString().isEmpty();}),"missing train clears Path");
 mode=0;require(waitFor([&]{return size()==2;}),"train returns");
 mode=5;require(waitFor([&]{return !backend.data()["live"].toBool()&&size()==0;}),"capture error clears Path");
 backend.disconnectGame();require(size()==0,"disconnect clears Path");
 MapItem map;map.setWidth(400);map.setHeight(300);MapNode node{track,0,0,0,0};
 QVariantMap d{{"live",true},{"pathAvailable",true},{"mapGeometry",QByteArray(reinterpret_cast<char*>(&node),sizeof node)},{"mapPath",QVariantList{QString("1000000000001")}}};
 auto color=[&]{QImage image(400,300,QImage::Format_ARGB32_Premultiplied);QPainter p(&image);map.paint(&p);p.end();return image.pixelColor(200,150);};
 map.setData(d);require(color()==QColor("#070b0a"),"calculated Path hidden by default");
 map.setShowCalculatedPath(true);require(color()==QColor("#e4bf50"),"optional Path rendered");
 d["pathAvailable"]=false;map.setData(d);require(color()==QColor("#070b0a"),"unavailable Path cannot draw leftover IDs");
 d["pathAvailable"]=true;d["pathStale"]=true;map.setData(d);require(color()==QColor("#070b0a"),"expired Path cannot draw leftover IDs");
 map.setShowCalculatedPath(false);
 d["mapReservations"]=QVariantList{QVariantMap{{"track",QString("1000000000001")}}};
 d["mapOccupations"]=d["mapReservations"];d["reservationsAvailable"]=true;d["occupationsAvailable"]=true;
 auto pixels=[&](const QColor& target){QImage image(400,300,QImage::Format_ARGB32_Premultiplied);QPainter p(&image);map.paint(&p);p.end();int count=0;for(int y=140;y<161;++y)for(int x=190;x<211;++x)if(image.pixelColor(x,y)==target)++count;return count;};
 map.setData(d);require(pixels(QColor("#65db87"))>0&&pixels(QColor("#ff6b6b"))>0,"reservation and occupation have independent visible markers");
 d["usageStale"]=true;map.setData(d);require(pixels(QColor("#65db87"))==0&&pixels(QColor("#ff6b6b"))==0,"stale usage cannot render leftover records");
 d["usageStale"]=false;d["reservationsAvailable"]=false;map.setData(d);require(pixels(QColor("#65db87"))==0&&pixels(QColor("#ff6b6b"))>0,"unavailable reservation leaves available occupation visible");
 QTemporaryDir assets;require(assets.isValid(),"texture fixture directory");
 QImage texture(32,32,QImage::Format_ARGB32);texture.fill(QColor("#ff00ff"));
 const auto texturePath=assets.filePath("signal.png");require(texture.save(texturePath),"texture fixture written");
 auto signalPixels=[&](const QColor& target){QImage image(400,300,QImage::Format_ARGB32_Premultiplied);QPainter p(&image);map.paint(&p);p.end();int count=0;for(int y=0;y<300;++y)for(int x=0;x<400;++x)if(image.pixelColor(x,y)==target)++count;return count;};
 QVariantMap mapSignal{{"track",QString("1000000000001")},{"stateAvailable",true},{"texturePath",texturePath}};
 d["mapSignals"]=QVariantList{mapSignal};map.setData(d);map.zoomAt(0.000001,200,150);
 map.setSignalSize(16);const int small=signalPixels(QColor("#ff00ff"));require(small>0,"native texture visible at overview zoom");
 map.setSignalSize(32);require(signalPixels(QColor("#ff00ff"))>small*3,"texture resize changes rendered size");
 texture.fill(QColor("#00ffff"));const auto nextTexturePath=assets.filePath("next.png");require(texture.save(nextTexturePath),"second texture fixture written");
 mapSignal["texturePath"]=nextTexturePath;d["mapSignals"]=QVariantList{mapSignal};map.setData(d);
 require(signalPixels(QColor("#ff00ff"))==0&&signalPixels(QColor("#00ffff"))>0,"native state change replaces texture immediately");
 QVariantMap balise=mapSignal;balise["texturePath"]=texturePath;balise["balise"]=true;
 d["mapSignals"]=QVariantList{mapSignal,balise};map.setData(d);
 require(signalPixels(QColor("#ff00ff"))>small*3&&signalPixels(QColor("#00ffff"))>small*3,"balise on same track cannot cover signal texture");
 // Hundreds of signals at one anchor must never spread over the map.
 QVariantList crowded;for(int i=0;i<300;++i)crowded.append(mapSignal);
 d["mapSignals"]=crowded;map.setData(d);
 QImage denseImage(400,300,QImage::Format_ARGB32_Premultiplied);
 {QPainter painter(&denseImage);map.paint(&painter);}
 int distant=0;
 for(int y=0;y<300;++y)for(int x=0;x<400;++x)
  if((std::abs(x-200)>40||std::abs(y-150)>40)&&denseImage.pixelColor(x,y)!=QColor("#070b0a"))++distant;
 require(distant==0,"dense signals stay near their real anchor without long leaders");
 d["mapSignals"]=QVariantList{mapSignal};map.setData(d);
 const int overview=signalPixels(QColor("#00ffff"));
 map.zoomAt(1000000,200,150);
 require(std::abs(signalPixels(QColor("#00ffff"))-overview)<=4,"signal stays readable at the same screen size across zoom levels");
 mapSignal["stateAvailable"]=false;mapSignal["marker"]=true;d["mapSignals"]=QVariantList{mapSignal};map.setData(d);
 require(signalPixels(QColor("#00ffff"))==0,"unavailable state cannot render cached texture");
 require(signalPixels(QColor("#b9b9b9"))>0,"unavailable signal has neutral explicit fallback");
 // Opposite native textures must not substitute for each other across zoom.
 MapItem zoomMap;zoomMap.setWidth(400);zoomMap.setHeight(300);zoomMap.setSignalSize(20);
 MapNode pair[]={{track,0,0,0,0},{track+1,0,0,1000,0}};
 QVariantMap first{{"id","A"},{"track",QString::number(track,16)},{"stateAvailable",true},{"texturePath",texturePath},{"specificState","native.red"}};
 QVariantMap second=first;second["id"]="B";second["track"]=QString::number(track+1,16);second["texturePath"]=nextTexturePath;second["specificState"]="native.yellow";
 QVariantMap zoomData{{"mapGeometry",QByteArray(reinterpret_cast<char*>(pair),sizeof pair)},{"mapSignals",QVariantList{first,second}}};
 zoomMap.setData(zoomData);
 auto renderZoom=[&]{QImage img(400,300,QImage::Format_ARGB32_Premultiplied);QPainter painter(&img);zoomMap.paint(&painter);return img;};
 auto countColor=[&](const QImage& img,QColor target){int count=0;for(int y=0;y<img.height();++y)for(int x=0;x<img.width();++x)if(img.pixelColor(x,y)==target)++count;return count;};
 const auto detailed=renderZoom();
 require(countColor(detailed,QColor("#ff00ff"))>0&&countColor(detailed,QColor("#00ffff"))>0,"distinct native textures visible separately");
 zoomMap.zoomAt(.01,200,150);const auto grouped=renderZoom();
 require(countColor(grouped,QColor("#ff00ff"))>0&&countColor(grouped,QColor("#00ffff"))>0,"overlapping pair displays both native textures without extra zoom");
 bool redHit=false,yellowHit=false;
 for(int y=70;y<160;++y)for(int x=130;x<270;++x){
  const auto detail=zoomMap.signalTextAt(x,y);
  redHit|=detail.contains("native.red")&&!detail.contains("native.yellow");
  yellowHit|=detail.contains("native.yellow")&&!detail.contains("native.red");
 }
 require(redHit&&yellowHit,"separated symbols each retain their own tooltip");
 zoomData["mapSignals"]=QVariantList{second,first};zoomMap.setData(zoomData);
 require(renderZoom()==grouped,"snapshot order cannot swap separated symbols");
 zoomData["mapSignals"]=QVariantList{first,second};zoomMap.setData(zoomData);
 zoomMap.zoomAt(100,200,150);require(renderZoom()==detailed,"zoom round trip preserves exact signal textures and positions");
 zoomData["mapSignals"]=QVariantList{second,first};zoomMap.setData(zoomData);
 require(renderZoom()==detailed,"snapshot order cannot swap native aspects");
 // A balise and reverse-facing signal can share exactly the same anchor,
 // so zooming can never separate them geometrically.
 first["direction"]=-1;second["balise"]=true;second["track"]=first["track"];
 zoomData["mapSignals"]=QVariantList{first,second};zoomMap.setData(zoomData);
 const auto coincident=renderZoom();
 require(countColor(coincident,QColor("#ff00ff"))>0&&countColor(coincident,QColor("#00ffff"))>0,"coincident balise and reverse-facing signal both remain visible");
 return 0;
}
