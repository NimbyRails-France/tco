#include "backend.h"
#include <nimby/client.hpp>
#include "mapgeometry.h"
#include <windows.h>
#include <tlhelp32.h>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QUrl>
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
        {"trackCount",0},{"stationCount",0},{"signalCount",0},{"trainCount",0},{"baliseCount",0},{"updated",QString()},
        {"mapPath",QVariantList{}},{"pathAvailable",false},{"pathSize",0},{"pathStale",false},
        {"mapReservations",QVariantList{}},{"mapOccupations",QVariantList{}},
        {"reservationsAvailable",false},{"occupationsAvailable",false},{"usageStale",false},
        {"selectedTrainId",QString()},{"selectedTrackId",QString()},{"selectedTrain",QString()}};
}
void clearPath(QVariantMap& data,bool stale=false) {
    data["mapPath"]=QVariantList{};data["pathAvailable"]=false;data["pathSize"]=0;data["pathStale"]=stale;
}
void clearUsage(QVariantMap& data,bool stale=false) {
    data["mapReservations"]=QVariantList{};data["mapOccupations"]=QVariantList{};
    data["reservationsAvailable"]=false;data["occupationsAvailable"]=false;data["usageStale"]=stale;
}
void clearSignalStates(QVariantMap& data) {
    auto tracks=data.value("tracks").toList();
    for(auto& track:tracks){auto row=track.toMap();auto signalRows=row.value("signals").toList();
        for(auto& signal:signalRows){auto value=signal.toMap();value["aspect"]="Périmé";value["specificState"]=QString();value["stateAvailable"]=false;value["textureUrl"]=QString();value["texturePath"]=QString();signal=value;}
        row["signals"]=signalRows;track=row;}
    data["tracks"]=tracks;
    auto mapSignals=data.value("mapSignals").toList();
    for(auto& signal:mapSignals){auto value=signal.toMap();value["stateAvailable"]=false;value["specificState"]=QString();value["texturePath"]=QString();signal=value;}
    data["mapSignals"]=mapSignals;data["signalStateCount"]=0;data["signalTextureCount"]=0;
}
uint32_t discover() {
    HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(h==INVALID_HANDLE_VALUE)return 0;
    PROCESSENTRY32W p{};p.dwSize=sizeof p;uint32_t result=0;int count=0;
    if(Process32FirstW(h,&p))do{if(_wcsicmp(p.szExeFile,L"NimbyRails.exe")==0){result=p.th32ProcessID;++count;}}while(Process32NextW(h,&p));
    CloseHandle(h);return count==1?result:0;
}
QString kind(int value){switch(value){case 0:return "Sens unique";case 1:return "Arrêt quai";case 3:return "Balise";case 4:return "Chemin";case 5:return "Interdiction";case 6:return "Repère";default:return "Inconnu";}}
QVariantMap capture(nimby::Client& client,uint32_t pid,const ViewFilter& view) {
    QElapsedTimer elapsed;elapsed.start();
    const auto snap=client.capture();
    const auto trains=snap->getAllTrains();
    const auto tracks=snap->getAllTracks();
    const auto stations=snap->getAllStations();
    const auto signalRecords=snap->getAllSignals();
    auto texturePath=[&](uint64_t signal){
        const auto texture=snap->getSignalTextureById(signal);
        return texture&&texture->getFilePath()?QString::fromStdString(*texture->getFilePath()):QString();
    };
    std::map<uint64_t,QString> names;QVariantList stationRows,trainRows,trackRows;
    for(const auto& s:stations){auto name=QString::fromStdString(s.getName().value_or(""));if(name.isEmpty())name="Gare "+id(s.getId());names[s.getId()]=name;stationRows.push_back(QVariantMap{{"id",id(s.getId())},{"name",name}});}
    std::map<uint64_t,QVariantList> byTrack,signalsByTrack;
    for(const auto& t:trains){
        const auto position=t.getPosition();
        const auto speed=t.getSpeedKmh();
        const auto service=snap->getTrainServiceById(t.getId());
        const auto line=service?service->getLineName():std::nullopt;
        QVariantMap value{{"id",id(t.getId())},{"name",QString::fromStdString(t.getName())},
            {"speed",speed.value_or(0)},{"speedAvailable",speed.has_value()},{"speedDefaulted",t.isSpeedDefaulted()},
            {"line",line?QString::fromStdString(*line):QString()},
            {"positioned",position.has_value()},{"track",position?id(position->getTrackId()):QString()},
            {"fraction",position?position->getFraction():0},{"direction",position?position->getDirection():0}};
        trainRows.push_back(value);if(position)byTrack[position->getTrackId()].push_back(value);
    }
    int balises=0,signalStateCount=0,signalTextureCount=0;
    for(const auto& s:signalRecords){if(s.getKind()==NIMBY_SIGNAL_BALISE)++balises;
        const auto state=snap->getSignalStateById(s.getId());
        const auto selector=state?state->getTextureSelector():std::nullopt;
        const bool available=selector.has_value();
        if(available)++signalStateCount;
        const auto aspect=available?QString("État natif %1").arg(*selector):QString("Inconnu");
        const auto specificState=state?state->getSpecificState():std::nullopt;
        const auto specific=specificState?QString::fromStdString(specificState->system+":"+specificState->state):QString();
        const auto path=texturePath(s.getId());if(!path.isEmpty())++signalTextureCount;
        signalsByTrack[s.getTrackId()].push_back(QVariantMap{{"id",id(s.getId())},{"kind",kind(s.getKind())},{"balise",s.getKind()==NIMBY_SIGNAL_BALISE},{"marker",s.getKind()==NIMBY_SIGNAL_MARKER},
            {"fraction",s.getFraction()},{"direction",s.getDirection()},{"aspect",aspect},{"specificState",specific},{"stateAvailable",available},
            {"texturePath",path},{"textureUrl",path.isEmpty()?QString():QUrl::fromLocalFile(path).toString()}});
    }
    std::vector<const nimby::Track*> filtered;
    for(const auto& t:tracks) {
        if(view.station && t.getStationId().value_or(0)!=view.station)continue;
        if(view.scope==0&&!byTrack.contains(t.getId()))continue;
        if(view.scope==1&&!signalsByTrack.contains(t.getId()))continue;
        if(view.scope==3&&t.getId()!=view.track)continue;
        filtered.push_back(&t);
    }
    constexpr size_t pageSize=500;
    const auto pages=std::max<size_t>(1,(filtered.size()+pageSize-1)/pageSize);
    const auto page=std::min<size_t>(view.page,pages-1);
    for(size_t i=page*pageSize;i<std::min(filtered.size(),(page+1)*pageSize);++i) {
        const auto& t=*filtered[i];
        trackRows.push_back(QVariantMap{{"id",id(t.getId())},{"stationId",t.getStationId().value_or(0)?id(t.getStationId().value_or(0)):QString()},
        {"station",t.getStationId().value_or(0)?names[t.getStationId().value_or(0)]:QString("Hors gare")},{"limit",t.getSpeedLimitKmh()},
        {"trains",byTrack[t.getId()]},{"signals",signalsByTrack[t.getId()]}});
    }
    auto result=empty("Connecté · PID "+QString::number(pid));result["live"]=true;
    result["tracks"]=trackRows;result["trains"]=trainRows;result["stations"]=stationRows;
    result["trackCount"]=static_cast<int>(tracks.size());result["stationCount"]=static_cast<int>(stations.size());
    result["trainCount"]=static_cast<int>(trains.size());result["signalCount"]=static_cast<int>(signalRecords.size());result["baliseCount"]=balises;
    result["signalStateCount"]=signalStateCount;
    result["signalTextureCount"]=signalTextureCount;
    result["page"]=static_cast<int>(page);result["pages"]=static_cast<int>(pages);
    result["filteredCount"]=static_cast<int>(filtered.size());result["viewRevision"]=QVariant::fromValue<qulonglong>(view.revision);
    std::vector<MapNode> nodes;
    for(const auto& node:snap->getAllTrackNodes()){
        const auto xy=node.getCoordinates();
        nodes.push_back({node.getId(),node.getLinkAId().value_or(0),node.getLinkBId().value_or(0),xy.x,xy.y});
    }
    std::optional<nimby::Train> selected=snap->getTrainById(view.train);
    if(!view.train)for(const auto& t:trains)if(t.getPosition()){selected=t;break;}
    QVariantList mapPath,mapSignals;uint32_t pathSize=0;bool pathAvailable=false;
    if(selected){
        if(const auto path=snap->getPathTrackIdsForTrain(selected->getId())){
            for(auto entry:*path)mapPath.push_back(id(entry));
            pathSize=static_cast<uint32_t>(path->size());pathAvailable=true;
        }
    }
    for(const auto& s:signalRecords){
        const auto state=snap->getSignalStateById(s.getId());
        const auto selector=state?state->getTextureSelector():std::nullopt;
        const bool available=selector.has_value();
        const auto specific=state?state->getSpecificState():std::nullopt;
        mapSignals.push_back(QVariantMap{{"id",id(s.getId())},{"kind",kind(s.getKind())},
            {"direction",s.getDirection()},{"fraction",s.getFraction()},
            {"specificState",specific?QString::fromStdString(specific->system+":"+specific->state):QString()},
            {"track",id(s.getTrackId())},{"balise",s.getKind()==NIMBY_SIGNAL_BALISE},{"marker",s.getKind()==NIMBY_SIGNAL_MARKER},
            {"stateAvailable",available},{"textureState",selector.value_or(0)},{"texturePath",texturePath(s.getId())}});
    }
    result["mapGeometry"]=QByteArray(reinterpret_cast<const char*>(nodes.data()),static_cast<qsizetype>(nodes.size()*sizeof(MapNode)));
    result["mapPath"]=mapPath;result["mapSignals"]=mapSignals;result["pathAvailable"]=pathAvailable;result["pathSize"]=pathSize;
    result["selectedTrainId"]=selected?id(selected->getId()):QString();
    result["selectedTrackId"]=selected&&(selected->getPosition().has_value())?id(selected->getPosition()->getTrackId()):QString();
    result["selectedTrain"]=selected?QString::fromStdString(selected->getName()):QString();
    auto usage=[&](const auto& rows,const char* key,const char* availability,bool selectedOnly){
        QVariantList values;
        if(rows)for(const auto& row:*rows)if(!selectedOnly||(selected&&row.getTrainId()==selected->getId()))
            values.push_back(QVariantMap{{"train",id(row.getTrainId())},{"track",id(row.getTrackId())},
                {"begin",row.getBeginFraction()},{"end",row.getEndFraction()}});
        result[key]=values;result[availability]=rows.has_value()&&(!selectedOnly||selected.has_value());
    };
    usage(snap->getAllReservations(),"mapReservations","reservationsAvailable",true);
    usage(snap->getAllOccupations(),"mapOccupations","occupationsAvailable",false);
    result["usageStale"]=false;
    int positionedCount=0;for(const auto& t:trains)if(t.getPosition().has_value())++positionedCount;
    result["positionedCount"]=positionedCount;
    result["captureMs"]=elapsed.elapsed();
    result["updated"]=QDateTime::currentDateTime().toString("HH:mm:ss.zzz");return result;
}
}
void ReaderThread::run() {
    std::unique_ptr<nimby::Client> session;uint64_t previous=0;uint32_t pid=0;int attempts=0;
    while(!isInterruptionRequested()) {
        QElapsedTimer cycle;cycle.start();
        const auto wanted=request.load();
        if(wanted!=previous){session.reset();previous=wanted;pid=0;attempts=0;}
        if(wanted&&!pending.load()) {
            try {
                if(!session){
                    pid=static_cast<uint32_t>(wanted);if(!pid)pid=discover();
                    if(!pid)throw std::runtime_error("Aucun jeu unique détecté · démarrer le jeu ou saisir son PID");
                    session.reset(new nimby::Client(nimby::Client::connect(pid)));
                }
                ViewFilter filter;{std::lock_guard lock(viewMutex);filter=view;}
                auto data=capture(*session,pid,filter);
                if(attempts++==0)qInfo()<<"SDK snapshot:"<<data["trainCount"]<<data["trackCount"]<<data["signalCount"];
                pending.store(true);emit received(wanted,data);
            }catch(const std::exception& e){
                // A new attempt reopens the process, so a replaced PID never reuses a handle.
                session.reset();
                pending.store(true);emit received(wanted,empty(QString::fromUtf8(e.what())));
            }
        }
        msleep(session?static_cast<unsigned long>(std::max<qint64>(20,250-cycle.elapsed())):1000);
    }
    session.reset();
}
Backend::Backend(QObject* parent):QObject(parent),data_(empty("Déconnecté")) {
    pathExpiry_.setSingleShot(true);pathExpiry_.setInterval(1500);
    connect(&pathExpiry_,&QTimer::timeout,this,[this]{
        if(!data_.value("live").toBool())return;
        clearPath(data_,true);clearUsage(data_,true);clearSignalStates(data_);emit changed();
    });
    connect(&worker_,&ReaderThread::received,this,[this](uint64_t token,const QVariantMap& data){
        worker_.pending.store(false);
        {std::lock_guard lock(worker_.viewMutex);
            if(data.contains("viewRevision")&&data["viewRevision"].toULongLong()!=worker_.view.revision)return;}
        if(token!=worker_.request.load())return;
        data_=data;
        if(!data_.value("live").toBool()||!data_.value("pathAvailable").toBool()||data_.value("selectedTrainId").toString().isEmpty())clearPath(data_);
        pathExpiry_.stop();if(data_.value("live").toBool())pathExpiry_.start();
        emit changed();
    });worker_.start();
}
Backend::~Backend(){worker_.requestInterruption();worker_.wait();}
void Backend::connectGame(const QString& text){
    bool ok=true;uint32_t pid=0;if(!text.trimmed().isEmpty())pid=text.trimmed().toUInt(&ok);
    if(!ok||(!text.trimmed().isEmpty()&&!pid)){disconnectGame();data_=empty("PID invalide");emit changed();return;}
    const auto token=(++generation_<<32)|pid;worker_.request.store(token);
    pathExpiry_.stop();data_=empty("Connexion…");emit changed();
}
void Backend::disconnectGame(){worker_.request.store(0);pathExpiry_.stop();data_=empty("Déconnecté");emit changed();}
void Backend::setView(int scope,const QString& station,const QString& track,int page){
    std::lock_guard lock(worker_.viewMutex);
    worker_.view={std::clamp(scope,0,3),std::max(0,page),station.toULongLong(nullptr,16),track.toULongLong(nullptr,16),worker_.view.revision+1,worker_.view.train};
}
void Backend::selectTrain(const QString& train){
    const auto selected=train.toULongLong(nullptr,16);
    {std::lock_guard lock(worker_.viewMutex);worker_.view.train=selected;++worker_.view.revision;}
    // Invalidate the old overlay immediately; in-flight results are rejected by revision.
    pathExpiry_.stop();clearPath(data_);clearUsage(data_);clearSignalStates(data_);data_["selectedTrainId"]=selected?id(selected):QString();
    data_["selectedTrackId"]=QString();data_["selectedTrain"]=QString();
    for(const auto& value:data_.value("trains").toList()){
        const auto row=value.toMap();if(row.value("id")==data_["selectedTrainId"]){
            data_["selectedTrain"]=row.value("name");data_["selectedTrackId"]=row.value("track");break;
        }
    }
    emit changed();
}
