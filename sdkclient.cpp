#include "sdkclient.h"
#include <nimby/observation.h>
#include <QCoreApplication>
#include <QLibrary>
#include <QVersionNumber>
namespace {
QLibrary library;
#define SDK_FUNCTION(name) decltype(&name) p##name=nullptr
SDK_FUNCTION(NimbySdk_OpenProcess);SDK_FUNCTION(NimbySdk_CloseSession);
SDK_FUNCTION(NimbySdk_CaptureSnapshot);SDK_FUNCTION(NimbySdk_ReleaseSnapshot);
SDK_FUNCTION(NimbySdk_CopyTrains);SDK_FUNCTION(NimbySdk_CopyTracks);SDK_FUNCTION(NimbySdk_CopyStations);
SDK_FUNCTION(NimbySdk_CopySignals);SDK_FUNCTION(NimbySdk_CopySignalStates);SDK_FUNCTION(NimbySdk_CopySignalTextures);
SDK_FUNCTION(NimbySdk_CopyTrackNodes);SDK_FUNCTION(NimbySdk_CopyTrainPathTracks);
SDK_FUNCTION(NimbySdk_CopyTrackReservations);SDK_FUNCTION(NimbySdk_CopyTrackOccupations);SDK_FUNCTION(NimbySdk_StatusString);
#undef SDK_FUNCTION
}
bool initializeSdkClient(QString& error){
 library.setFileName(QCoreApplication::applicationDirPath()+"/NimbyRailsSDK.dll");
 library.setLoadHints(QLibrary::PreventUnloadHint);
 if(!library.load()){error="SDK absent ou impossible à charger : "+library.errorString();return false;}
 auto version=reinterpret_cast<decltype(&NimbySdk_GetVersion)>(library.resolve("NimbySdk_GetVersion"));
 NimbySdkVersion info{};info.struct_size=sizeof info;
 if(!version||version(&info)!=NIMBY_OK||info.abi_version!=1||info.major!=0||info.minor!=6){
  error="SDK incompatible : ce TCO nécessite NimbyRailsSDK 0.6.x (ABI 1). Réinstallez le paquet depuis NRF Hub.";return false;
 }
#define LOAD(name) p##name=reinterpret_cast<decltype(&name)>(library.resolve(#name));if(!p##name){error="SDK incomplet : " #name;return false;}
 LOAD(NimbySdk_OpenProcess);LOAD(NimbySdk_CloseSession);LOAD(NimbySdk_CaptureSnapshot);LOAD(NimbySdk_ReleaseSnapshot);
 LOAD(NimbySdk_CopyTrains);LOAD(NimbySdk_CopyTracks);LOAD(NimbySdk_CopyStations);LOAD(NimbySdk_CopySignals);
 LOAD(NimbySdk_CopySignalStates);LOAD(NimbySdk_CopySignalTextures);LOAD(NimbySdk_CopyTrackNodes);LOAD(NimbySdk_CopyTrainPathTracks);
 LOAD(NimbySdk_CopyTrackReservations);LOAD(NimbySdk_CopyTrackOccupations);LOAD(NimbySdk_StatusString);
#undef LOAD
 return true;
}
extern "C" {
uint32_t NimbySdk_OpenProcess(uint32_t a,uint32_t p,NimbySession* s) noexcept{return pNimbySdk_OpenProcess(a,p,s);}
uint32_t NimbySdk_CloseSession(NimbySession s) noexcept{return pNimbySdk_CloseSession(s);}
uint32_t NimbySdk_CaptureSnapshot(NimbySession s,NimbySnapshot* o) noexcept{return pNimbySdk_CaptureSnapshot(s,o);}
uint32_t NimbySdk_ReleaseSnapshot(NimbySnapshot s) noexcept{return pNimbySdk_ReleaseSnapshot(s);}
#define COPY(name,type) uint32_t name(NimbySnapshot s,type* o,uint32_t c,uint32_t* n) noexcept{return p##name(s,o,c,n);}
COPY(NimbySdk_CopyTrains,NimbyTrain) COPY(NimbySdk_CopyTracks,NimbyTrack) COPY(NimbySdk_CopyStations,NimbyStation)
COPY(NimbySdk_CopySignals,NimbySignal) COPY(NimbySdk_CopySignalStates,NimbySignalState) COPY(NimbySdk_CopySignalTextures,NimbySignalTexture)
COPY(NimbySdk_CopyTrackNodes,NimbyTrackNode) COPY(NimbySdk_CopyTrackReservations,NimbyTrackUsage) COPY(NimbySdk_CopyTrackOccupations,NimbyTrackUsage)
#undef COPY
uint32_t NimbySdk_CopyTrainPathTracks(NimbySnapshot s,uint64_t t,uint64_t* o,uint32_t c,uint32_t* n) noexcept{return pNimbySdk_CopyTrainPathTracks(s,t,o,c,n);}
const char* NimbySdk_StatusString(uint32_t s) noexcept{return pNimbySdk_StatusString(s);}
}
