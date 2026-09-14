// Synthetic SDK snapshots only. No game process is opened or modified.
#include "../backend.h"
#include "../mapitem.h"
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QPainter>
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
uint32_t NimbySdk_OpenProcess(uint32_t,uint32_t,NimbySession* out) noexcept{*out=1;return NIMBY_OK;}
uint32_t NimbySdk_CloseSession(NimbySession) noexcept{return NIMBY_OK;}
uint32_t NimbySdk_CaptureSnapshot(NimbySession,NimbySnapshot* out) noexcept{
 const int value=mode.load();if(delay.load()){waiting=true;while(delay.load())std::this_thread::sleep_for(std::chrono::milliseconds(10));waiting=false;}
 if(value==5){*out=0;return NIMBY_DATA_UNAVAILABLE;}*out=value+1;return NIMBY_OK;
}
uint32_t NimbySdk_ReleaseSnapshot(NimbySnapshot) noexcept{return NIMBY_OK;}
uint32_t NimbySdk_CopyTrains(NimbySnapshot s,NimbyTrain* out,uint32_t cap,uint32_t* n) noexcept{
 std::vector<NimbyTrain> rows;if(s!=5){NimbyTrain t{};t.id=train;t.track_id=track;t.flags=NIMBY_TRAIN_PRESENT|NIMBY_TRAIN_POSITION_VALID;std::strcpy(t.name_utf8,"Fixture A");rows.push_back(t);t.id=train+0x10000;std::strcpy(t.name_utf8,"Fixture B");rows.push_back(t);}return output(rows,out,cap,n);
}
uint32_t NimbySdk_CopyTracks(NimbySnapshot,NimbyTrack* out,uint32_t cap,uint32_t* n) noexcept{return output(std::vector<NimbyTrack>{{track,0,30},{track+0x10000,0,30}},out,cap,n);}
uint32_t NimbySdk_CopyStations(NimbySnapshot,NimbyStation*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_OK;}
uint32_t NimbySdk_CopySignals(NimbySnapshot,NimbySignal*,uint32_t,uint32_t* n) noexcept{*n=0;return NIMBY_OK;}
uint32_t NimbySdk_CopyTrackNodes(NimbySnapshot,NimbyTrackNode* out,uint32_t cap,uint32_t* n) noexcept{return output(std::vector<NimbyTrackNode>{{track,0,0,0,0}},out,cap,n);}
uint32_t NimbySdk_CopyTrainPathTracks(NimbySnapshot s,uint64_t,uint64_t* out,uint32_t cap,uint32_t* n) noexcept{
 *n=0;if(s==3||s==5)return NIMBY_DATA_UNAVAILABLE;std::vector<uint64_t> path;if(s!=4){path.push_back(track);if(s==1)path.push_back(track+0x10000);}return output(path,out,cap,n);
}
const char* NimbySdk_StatusString(uint32_t) noexcept{return "Synthetic capture failure";}
uint32_t NimbySdk_CopyTrackReservations(NimbySnapshot s,NimbyTrackUsage* out,uint32_t cap,uint32_t* n) noexcept{
 *n=0;if(s==3)return NIMBY_DATA_UNAVAILABLE;
 std::vector<NimbyTrackUsage> rows;if(s!=4&&s!=5){rows.push_back({train,track,.2,.9});rows.push_back({train+0x10000,track,.1,.3});if(s==1)rows.push_back({train,track+0x10000,0,1});}
 return output(rows,out,cap,n);
}
uint32_t NimbySdk_CopyTrackOccupations(NimbySnapshot s,NimbyTrackUsage* out,uint32_t cap,uint32_t* n) noexcept{
 if(s==4){*n=0;return NIMBY_DATA_UNAVAILABLE;}
 return output(std::vector<NimbyTrackUsage>{{train,track,.2,.3}},out,cap,n);
}
}
int main(int argc,char** argv){
 QGuiApplication app(argc,argv);Backend backend;
 auto size=[&]{return backend.data().value("mapPath").toList().size();};
 auto require=[&](bool ok,const char* what){if(!ok){delay=false;std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}std::printf("PASS %s\n",what);};
 backend.connectGame("42");require(waitFor([&]{return size()==2;}),"first Path");
 auto reserved=[&]{return backend.data().value("mapReservations").toList().size();};
 require(reserved()==2,"reservations filtered to selected train");
 mode=1;require(waitFor([&]{return size()==1;}),"shorter Path replaces all previous entries");
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
 require(reserved()==0&&backend.data()["mapOccupations"].toList().isEmpty()&&backend.data()["usageStale"].toBool(),"missing updates expire reservations and occupation");
 delay=false;require(waitFor([&]{return size()==2&&!backend.data()["pathStale"].toBool();}),"fresh capture restores Path");
 mode=4;require(waitFor([&]{return size()==0&&backend.data()["selectedTrainId"].toString().isEmpty();}),"missing train clears Path");
 mode=0;require(waitFor([&]{return size()==2;}),"train returns");
 mode=5;require(waitFor([&]{return !backend.data()["live"].toBool()&&size()==0;}),"capture error clears Path");
 backend.disconnectGame();require(size()==0,"disconnect clears Path");
 MapItem map;map.setWidth(400);map.setHeight(300);NimbyTrackNode node{track,0,0,0,0};
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
 return 0;
}
