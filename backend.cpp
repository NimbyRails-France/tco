#include "backend.h"
#include <nimby/observation.h>
#include <windows.h>
#include <tlhelp32.h>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <map>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace {
QString id(uint64_t value) {return QString::number(value,16).rightJustified(16,'0').toUpper();}
QVariantMap empty(const QString& status) {
    return {{"status",status},{"live",false},{"tracks",QVariantList{}},{"trains",QVariantList{}},{"stations",QVariantList{}},
        {"trackCount",0},{"stationCount",0},{"signalCount",0},{"trainCount",0},{"baliseCount",0},{"updated",QString()}};
}
uint32_t discover() {
    HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(h==INVALID_HANDLE_VALUE)return 0;
    PROCESSENTRY32W p{};p.dwSize=sizeof p;uint32_t result=0;int count=0;
    if(Process32FirstW(h,&p))do{if(_wcsicmp(p.szExeFile,L"NimbyRails.exe")==0){result=p.th32ProcessID;++count;}}while(Process32NextW(h,&p));
    CloseHandle(h);return count==1?result:0;
}
void check(uint32_t code){if(code!=NIMBY_OK)throw std::runtime_error(NimbySdk_StatusString(code));}
template<class T,class F> std::vector<T> copy(NimbySnapshot h,F function) {
    uint32_t n=0;check(function(h,nullptr,0,&n));std::vector<T> result(n);
    check(function(h,result.data(),n,&n));return result;
}
QString kind(int value){switch(value){case 0:return "Sens unique";case 1:return "Arrêt quai";case 3:return "Balise";case 4:return "Chemin";case 5:return "Interdiction";case 6:return "Repère";default:return "Inconnu";}}
struct Snapshot {NimbySnapshot value{};~Snapshot(){if(value)NimbySdk_ReleaseSnapshot(value);}};
QVariantMap capture(NimbySession session,uint32_t pid,const ViewFilter& view) {
    QElapsedTimer elapsed;elapsed.start();
    Snapshot snap;check(NimbySdk_CaptureSnapshot(session,&snap.value));
    const auto trains=copy<NimbyTrain>(snap.value,NimbySdk_CopyTrains);
    const auto tracks=copy<NimbyTrack>(snap.value,NimbySdk_CopyTracks);
    const auto stations=copy<NimbyStation>(snap.value,NimbySdk_CopyStations);
    const auto signalRecords=copy<NimbySignal>(snap.value,NimbySdk_CopySignals);
    std::map<uint64_t,QString> names;QVariantList stationRows,trainRows,trackRows;
    for(const auto& s:stations){auto name=QString::fromUtf8(s.name_utf8);if(name.isEmpty())name="Gare "+id(s.id);names[s.id]=name;stationRows.push_back(QVariantMap{{"id",id(s.id)},{"name",name}});}
    std::map<uint64_t,QVariantList> byTrack,signalsByTrack;
    for(const auto& t:trains){
        const bool present=t.flags&NIMBY_TRAIN_PRESENT,position=t.flags&NIMBY_TRAIN_POSITION_VALID;
        QVariantMap value{{"id",id(t.id)},{"name",QString::fromUtf8(t.name_utf8)},{"speed",t.speed_mps*3.6},{"present",present},
            {"positioned",position},{"track",position?id(t.track_id):QString()},{"fraction",t.track_fraction},{"direction",t.direction}};
        trainRows.push_back(value);if(position)byTrack[t.track_id].push_back(value);
    }
    int balises=0;
    for(const auto& s:signalRecords){if(s.kind==NIMBY_SIGNAL_BALISE)++balises;
        signalsByTrack[s.track_id].push_back(QVariantMap{{"id",id(s.id)},{"kind",kind(s.kind)},{"balise",s.kind==NIMBY_SIGNAL_BALISE},
            {"fraction",s.track_fraction},{"direction",s.direction},{"aspect","Inconnu"}});
    }
    std::vector<const NimbyTrack*> filtered;
    for(const auto& t:tracks) {
        if(view.station && t.station_id!=view.station)continue;
        if(view.scope==0&&!byTrack.contains(t.id))continue;
        if(view.scope==1&&!signalsByTrack.contains(t.id))continue;
        if(view.scope==3&&t.id!=view.track)continue;
        filtered.push_back(&t);
    }
    constexpr size_t pageSize=500;
    const auto pages=std::max<size_t>(1,(filtered.size()+pageSize-1)/pageSize);
    const auto page=std::min<size_t>(view.page,pages-1);
    for(size_t i=page*pageSize;i<std::min(filtered.size(),(page+1)*pageSize);++i) {
        const auto& t=*filtered[i];
        trackRows.push_back(QVariantMap{{"id",id(t.id)},{"stationId",t.station_id?id(t.station_id):QString()},
        {"station",t.station_id?names[t.station_id]:QString("Hors gare")},{"limit",t.speed_limit_mps*3.6},
        {"trains",byTrack[t.id]},{"signals",signalsByTrack[t.id]}});
    }
    auto result=empty("Connecté · PID "+QString::number(pid));result["live"]=true;
    result["tracks"]=trackRows;result["trains"]=trainRows;result["stations"]=stationRows;
    result["trackCount"]=static_cast<int>(tracks.size());result["stationCount"]=static_cast<int>(stations.size());
    result["trainCount"]=static_cast<int>(trains.size());result["signalCount"]=static_cast<int>(signalRecords.size());result["baliseCount"]=balises;
    result["page"]=static_cast<int>(page);result["pages"]=static_cast<int>(pages);
    result["filteredCount"]=static_cast<int>(filtered.size());result["viewRevision"]=QVariant::fromValue<qulonglong>(view.revision);
    const auto nodes=copy<NimbyTrackNode>(snap.value,NimbySdk_CopyTrackNodes);
    const NimbyTrain* selected=nullptr;
    for(const auto& t:trains)if(t.id==view.train)selected=&t;
    if(!view.train)for(const auto& t:trains)if(t.flags&NIMBY_TRAIN_POSITION_VALID){selected=&t;break;}
    QVariantList mapPath,mapSignals;uint32_t pathSize=0;bool pathAvailable=false;
    if(selected){
        if(NimbySdk_CopyTrainPathTracks(snap.value,selected->id,nullptr,0,&pathSize)==NIMBY_OK){
            std::vector<uint64_t> entries(pathSize);
            check(NimbySdk_CopyTrainPathTracks(snap.value,selected->id,entries.data(),pathSize,&pathSize));
            for(auto entry:entries)mapPath.push_back(id(entry));pathAvailable=true;
        }
    }
    for(const auto& s:signalRecords)mapSignals.push_back(QVariantMap{{"track",id(s.track_id)},{"balise",s.kind==NIMBY_SIGNAL_BALISE}});
    result["mapGeometry"]=QByteArray(reinterpret_cast<const char*>(nodes.data()),static_cast<qsizetype>(nodes.size()*sizeof(NimbyTrackNode)));
    result["mapPath"]=mapPath;result["mapSignals"]=mapSignals;result["pathAvailable"]=pathAvailable;result["pathSize"]=pathSize;
    result["selectedTrainId"]=selected?id(selected->id):QString();
    result["selectedTrackId"]=selected&&(selected->flags&NIMBY_TRAIN_POSITION_VALID)?id(selected->track_id):QString();
    result["selectedTrain"]=selected?QString::fromUtf8(selected->name_utf8):QString();
    int positionedCount=0;for(const auto& t:trains)if(t.flags&NIMBY_TRAIN_POSITION_VALID)++positionedCount;
    result["positionedCount"]=positionedCount;
    result["captureMs"]=elapsed.elapsed();
    result["updated"]=QDateTime::currentDateTime().toString("HH:mm:ss.zzz");return result;
}
}
void ReaderThread::run() {
    NimbySession session=0;uint64_t previous=0;uint32_t pid=0;int attempts=0;
    while(!isInterruptionRequested()) {
        QElapsedTimer cycle;cycle.start();
        const auto wanted=request.load();
        if(wanted!=previous){if(session)NimbySdk_CloseSession(session);session=0;previous=wanted;pid=0;attempts=0;}
        if(wanted&&!pending.load()) {
            try {
                if(!session){
                    pid=static_cast<uint32_t>(wanted);if(!pid)pid=discover();
                    if(!pid)throw std::runtime_error("Aucun jeu unique détecté · démarrer le jeu ou saisir son PID");
                    check(NimbySdk_OpenProcess(NIMBY_OBSERVATION_ABI_VERSION,pid,&session));
                }
                ViewFilter filter;{std::lock_guard lock(viewMutex);filter=view;}
                auto data=capture(session,pid,filter);
                if(attempts++==0)qInfo()<<"SDK snapshot:"<<data["trainCount"]<<data["trackCount"]<<data["signalCount"];
                pending.store(true);emit received(wanted,data);
            }catch(const std::exception& e){
                // A new attempt reopens the process, so a replaced PID never reuses a handle.
                if(session)NimbySdk_CloseSession(session);session=0;
                pending.store(true);emit received(wanted,empty(QString::fromUtf8(e.what())));
            }
        }
        msleep(session?static_cast<unsigned long>(std::max<qint64>(20,250-cycle.elapsed())):1000);
    }
    if(session)NimbySdk_CloseSession(session);
}
Backend::Backend(QObject* parent):QObject(parent),data_(empty("Déconnecté")) {
    connect(&worker_,&ReaderThread::received,this,[this](uint64_t token,const QVariantMap& data){
        worker_.pending.store(false);
        {std::lock_guard lock(worker_.viewMutex);
            if(data.contains("viewRevision")&&data["viewRevision"].toULongLong()!=worker_.view.revision)return;}
        if(token!=worker_.request.load())return;data_=data;emit changed();
    });worker_.start();
}
Backend::~Backend(){worker_.requestInterruption();worker_.wait();}
void Backend::connectGame(const QString& text){
    bool ok=true;uint32_t pid=0;if(!text.trimmed().isEmpty())pid=text.trimmed().toUInt(&ok);
    if(!ok||(!text.trimmed().isEmpty()&&!pid)){disconnectGame();data_=empty("PID invalide");emit changed();return;}
    const auto token=(++generation_<<32)|pid;worker_.request.store(token);
    data_=empty("Connexion…");emit changed();
}
void Backend::disconnectGame(){worker_.request.store(0);data_=empty("Déconnecté");emit changed();}
void Backend::setView(int scope,const QString& station,const QString& track,int page){
    std::lock_guard lock(worker_.viewMutex);
    worker_.view={std::clamp(scope,0,3),std::max(0,page),station.toULongLong(nullptr,16),track.toULongLong(nullptr,16),worker_.view.revision+1,worker_.view.train};
}
void Backend::selectTrain(const QString& train){std::lock_guard lock(worker_.viewMutex);worker_.view.train=train.toULongLong(nullptr,16);++worker_.view.revision;}
