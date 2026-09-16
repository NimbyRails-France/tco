#include "sdkclient.h"
#include <nimby/client.hpp>
#include <QCoreApplication>
#include <QLibrary>
#include <QVersionNumber>
namespace {
QLibrary library;
#define SDK_FUNCTION(name) decltype(&name) p##name=nullptr
SDK_FUNCTION(NimbyInternal_GetVersion);SDK_FUNCTION(NimbyInternal_GetSnapshotInfo);
SDK_FUNCTION(NimbyInternal_GetSimulationClock);
SDK_FUNCTION(NimbyInternal_CopyTrainServices);SDK_FUNCTION(NimbyInternal_CopyTrainDetails);SDK_FUNCTION(NimbyInternal_CopyPlatforms);SDK_FUNCTION(NimbyInternal_CopyTrainLineStops);
SDK_FUNCTION(NimbyInternal_OpenProcess);SDK_FUNCTION(NimbyInternal_CloseSession);
SDK_FUNCTION(NimbyInternal_CaptureSnapshot);SDK_FUNCTION(NimbyInternal_ReleaseSnapshot);
SDK_FUNCTION(NimbyInternal_CopyTrains);SDK_FUNCTION(NimbyInternal_CopyTracks);SDK_FUNCTION(NimbyInternal_CopyStations);
SDK_FUNCTION(NimbyInternal_CopySignals);SDK_FUNCTION(NimbyInternal_CopySignalStates);SDK_FUNCTION(NimbyInternal_CopySignalTextures);
SDK_FUNCTION(NimbyInternal_CopyTrackNodes);SDK_FUNCTION(NimbyInternal_CopyTrainPathTracks);
SDK_FUNCTION(NimbyInternal_CopyTrackReservations);SDK_FUNCTION(NimbyInternal_CopyTrackOccupations);SDK_FUNCTION(NimbyInternal_StatusString);
#undef SDK_FUNCTION
}
bool initializeSdkClient(QString& error){
 library.setFileName(QCoreApplication::applicationDirPath()+"/NimbyRailsFranceSDK.dll");
 library.setLoadHints(QLibrary::PreventUnloadHint);
 if(!library.load()){error="SDK absent ou impossible à charger : "+library.errorString();return false;}
 auto version=reinterpret_cast<decltype(&NimbyInternal_GetVersion)>(library.resolve("NimbyInternal_GetVersion"));
 NimbySdkVersion info{};info.struct_size=sizeof info;
 if(!version||version(&info)!=NIMBY_OK||info.abi_version!=NIMBY_OBSERVATION_ABI_VERSION||info.major!=0||info.minor!=7||info.patch<1){
  error="SDK incompatible : ce TCO nécessite NimbyRailsFranceSDK 0.7.1 ou ultérieur en 0.7.x (pont 2). Réinstallez le paquet depuis NRF Hub.";return false;
 }
#define LOAD(name) p##name=reinterpret_cast<decltype(&name)>(library.resolve(#name));if(!p##name){error="SDK incomplet : " #name;return false;}
 LOAD(NimbyInternal_GetVersion);LOAD(NimbyInternal_GetSnapshotInfo);LOAD(NimbyInternal_CopyTrainServices);LOAD(NimbyInternal_CopyTrainDetails);LOAD(NimbyInternal_CopyPlatforms);LOAD(NimbyInternal_CopyTrainLineStops);
 LOAD(NimbyInternal_GetSimulationClock);
 LOAD(NimbyInternal_OpenProcess);LOAD(NimbyInternal_CloseSession);LOAD(NimbyInternal_CaptureSnapshot);LOAD(NimbyInternal_ReleaseSnapshot);
 LOAD(NimbyInternal_CopyTrains);LOAD(NimbyInternal_CopyTracks);LOAD(NimbyInternal_CopyStations);LOAD(NimbyInternal_CopySignals);
 LOAD(NimbyInternal_CopySignalStates);LOAD(NimbyInternal_CopySignalTextures);LOAD(NimbyInternal_CopyTrackNodes);LOAD(NimbyInternal_CopyTrainPathTracks);
 LOAD(NimbyInternal_CopyTrackReservations);LOAD(NimbyInternal_CopyTrackOccupations);LOAD(NimbyInternal_StatusString);
#undef LOAD
 return true;
}
// Bind the private bridge required by the public inline API, only here.
// Keep startup diagnostics for missing or incompatible DLLs.
extern "C" {
uint32_t NimbyInternal_GetVersion(NimbySdkVersion* v) noexcept{return pNimbyInternal_GetVersion(v);}
uint32_t NimbyInternal_GetSimulationClock(NimbySnapshot s,NimbySimulationClock* out) noexcept{return pNimbyInternal_GetSimulationClock(s,out);}
uint32_t NimbyInternal_GetSnapshotInfo(NimbySnapshot s,NimbySnapshotInfo* o) noexcept{return pNimbyInternal_GetSnapshotInfo(s,o);}
uint32_t NimbyInternal_CopyTrainLineStops(NimbySnapshot s,uint64_t t,NimbyLineStop* o,uint32_t c,uint32_t* n) noexcept{return pNimbyInternal_CopyTrainLineStops(s,t,o,c,n);}
uint32_t NimbyInternal_OpenProcess(uint32_t a,uint32_t p,NimbySession* s) noexcept{return pNimbyInternal_OpenProcess(a,p,s);}
uint32_t NimbyInternal_CloseSession(NimbySession s) noexcept{return pNimbyInternal_CloseSession(s);}
uint32_t NimbyInternal_CaptureSnapshot(NimbySession s,NimbySnapshot* o) noexcept{return pNimbyInternal_CaptureSnapshot(s,o);}
uint32_t NimbyInternal_ReleaseSnapshot(NimbySnapshot s) noexcept{return pNimbyInternal_ReleaseSnapshot(s);}
#define COPY(name,type) uint32_t name(NimbySnapshot s,type* o,uint32_t c,uint32_t* n) noexcept{return p##name(s,o,c,n);}
COPY(NimbyInternal_CopyTrains,NimbyTrain) COPY(NimbyInternal_CopyTracks,NimbyTrack) COPY(NimbyInternal_CopyStations,NimbyStation)
COPY(NimbyInternal_CopySignals,NimbySignal) COPY(NimbyInternal_CopySignalStates,NimbySignalState) COPY(NimbyInternal_CopySignalTextures,NimbySignalTexture)
COPY(NimbyInternal_CopyTrackNodes,NimbyTrackNode) COPY(NimbyInternal_CopyTrackReservations,NimbyTrackUsage) COPY(NimbyInternal_CopyTrackOccupations,NimbyTrackUsage)
COPY(NimbyInternal_CopyTrainServices,NimbyTrainService) COPY(NimbyInternal_CopyTrainDetails,NimbyTrainDetails) COPY(NimbyInternal_CopyPlatforms,NimbyPlatform)
#undef COPY
uint32_t NimbyInternal_CopyTrainPathTracks(NimbySnapshot s,uint64_t t,uint64_t* o,uint32_t c,uint32_t* n) noexcept{return pNimbyInternal_CopyTrainPathTracks(s,t,o,c,n);}
const char* NimbyInternal_StatusString(uint32_t s) noexcept{return pNimbyInternal_StatusString(s);}
}
