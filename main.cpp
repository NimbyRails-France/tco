#include "backend.h"
#include "mapitem.h"
#include "updater.h"
#include "sdkclient.h"
#include <windows.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QDebug>
#include <QSet>
#include <QImageReader>
#include <QQuickWindow>
#include <cstdio>
int main(int argc,char** argv) {
    QGuiApplication app(argc,argv);app.setApplicationName("Nimby TCO");
    app.setOrganizationName("NimbyTools");app.setApplicationVersion(TCO_VERSION);
    QString sdkError;
    if(!initializeSdkClient(sdkError)){
        qCritical().noquote()<<sdkError;
        if(!app.arguments().contains("--check-sdk"))MessageBoxW(nullptr,reinterpret_cast<LPCWSTR>(sdkError.utf16()),L"Nimby TCO — SDK incompatible",MB_OK|MB_ICONERROR);
        return 3;
    }
    if(app.arguments().contains("--check-sdk"))return 0;
    Updater updater;
    QQuickStyle::setStyle("Basic");
    qmlRegisterType<MapItem>("Nimby.Map",1,0,"RailMap");
    Backend backend;QQmlApplicationEngine engine;
    engine.setInitialProperties({{"backend",QVariant::fromValue(&backend)},{"updater",QVariant::fromValue(&updater)}});
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
    engine.loadFromModule("NimbyTco","Main");
    int liveFrames=0,usageFrames=0,textureFrames=0;QSet<QString> observed,decodedTextures;bool textureError=false;
    const bool verifyTextures=app.arguments().contains("--verify-textures");
    const bool verifyBlink=app.arguments().contains("--verify-blink");QSet<int> blinkPhases;int previousBlinkPhase=-1;
    QTimer blinkCheck;
    if(verifyBlink){
        QObject::connect(&blinkCheck,&QTimer::timeout,&app,[&]{
            const auto data=backend.data();if(!data["live"].toBool()||engine.rootObjects().isEmpty())return;
            for(const auto& entry:data["mapSignals"].toList()){
                const auto signal=entry.toMap();const int phase=signal["textureState"].toInt();
                if(!signal["stateAvailable"].toBool()||(phase!=9&&phase!=10)||blinkPhases.contains(phase))continue;
                // Let the scene graph present the newly received phase first.
                if(previousBlinkPhase!=phase){previousBlinkPhase=phase;continue;}
                if(auto* window=qobject_cast<QQuickWindow*>(engine.rootObjects().first())){
                    const auto args=app.arguments();const int at=args.indexOf("--screenshot");
                    if(at>=0&&at+1<args.size()&&window->grabWindow().save(args[at+1]+QString(".phase%1.png").arg(phase)))blinkPhases.insert(phase);
                }
            }
        });blinkCheck.start(100);
    }
    if(app.arguments().contains("--verify-live")||verifyTextures){
        if(!engine.rootObjects().isEmpty()){
            engine.rootObjects().first()->setProperty("visible",verifyTextures);
            if(verifyTextures){engine.rootObjects().first()->setProperty("panelMode",app.arguments().contains("--verify-map"));backend.setView(1,QString(),QString(),0);}
        }
        QObject::connect(&backend,&Backend::changed,&app,[&]{const auto d=backend.data();if(!d.value("live").toBool()){qInfo()<<"WAIT"<<d.value("status");return;}
            ++liveFrames;const auto rows=d.value("trains").toList();if(!rows.isEmpty()){auto t=rows.first().toMap();observed.insert(t["track"].toString()+QString::number(t["fraction"].toDouble(),'g',16));}
            if(d["reservationsAvailable"].toBool()&&d["occupationsAvailable"].toBool())++usageFrames;
            if(verifyTextures){
                if(d["signalTextureCount"].toInt()>0&&d["signalTextureCount"]==d["signalCount"])++textureFrames;
                for(const auto& entry:d["mapSignals"].toList()){
                    const auto path=entry.toMap()["texturePath"].toString();if(path.isEmpty()||decodedTextures.contains(path))continue;
                    QImageReader reader(path);reader.setScaledSize(QSize(64,64));const auto image=reader.read();
                    if(image.isNull()){qWarning()<<"Texture decode failed"<<path<<reader.errorString();textureError=true;}
                    else decodedTextures.insert(path);
                }
            }
            std::printf("LIVE %d signal_states=%d capture_ms=%lld reserved=%lld available=%d occupied=%lld available=%d\n",liveFrames,d["signalStateCount"].toInt(),
                d["captureMs"].toLongLong(),static_cast<long long>(d["mapReservations"].toList().size()),d["reservationsAvailable"].toBool(),
                static_cast<long long>(d["mapOccupations"].toList().size()),d["occupationsAvailable"].toBool());std::fflush(stdout);
            qInfo()<<"LIVE"<<liveFrames<<d["updated"]<<"captureMs"<<d["captureMs"]<<"positioned"<<d["positionedCount"]
                <<"reservations"<<d["reservationsAvailable"]<<d["mapReservations"].toList().size()
                <<"occupation"<<d["occupationsAvailable"]<<d["mapOccupations"].toList().size();
        });
        QTimer::singleShot(8000,&app,[&]{
            bool screenshotOk=true;const auto args=app.arguments();const auto at=args.indexOf("--screenshot");
            if(at>=0){screenshotOk=false;if(at+1<args.size()&&!engine.rootObjects().isEmpty()){
                if(auto* window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()))screenshotOk=window->grabWindow().save(args[at+1]);}}
            std::printf("VERIFY frames=%d usage_frames=%d distinct_positions=%lld texture_frames=%d decoded_textures=%lld\n",liveFrames,usageFrames,static_cast<long long>(observed.size()),textureFrames,static_cast<long long>(decodedTextures.size()));std::fflush(stdout);
            std::printf("BLINK phases=%lld\n",static_cast<long long>(blinkPhases.size()));std::fflush(stdout);
            app.exit(liveFrames>=2&&usageFrames>=2&&screenshotOk&&(!verifyBlink||blinkPhases.size()==2)&&(!verifyTextures||(textureFrames>=2&&!textureError&&!decodedTextures.isEmpty()))?0:2);
        });
    }
    if(app.arguments().contains("--smoke"))QTimer::singleShot(3500,&app,&QCoreApplication::quit);
    backend.connectGame(QString());return app.exec();
}
