#include "backend.h"
#include "mapitem.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QDebug>
#include <QSet>
#include <cstdio>
int main(int argc,char** argv) {
    QGuiApplication app(argc,argv);app.setApplicationName("Nimby TCO");
    QQuickStyle::setStyle("Basic");
    qmlRegisterType<MapItem>("Nimby.Map",1,0,"RailMap");
    Backend backend;QQmlApplicationEngine engine;
    engine.setInitialProperties({{"backend",QVariant::fromValue(&backend)}});
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
    engine.loadFromModule("NimbyTco","Main");
    int liveFrames=0,usageFrames=0;QSet<QString> observed;
    if(app.arguments().contains("--verify-live")){
        if(!engine.rootObjects().isEmpty())engine.rootObjects().first()->setProperty("visible",false);
        QObject::connect(&backend,&Backend::changed,&app,[&]{const auto d=backend.data();if(!d.value("live").toBool()){qInfo()<<"WAIT"<<d.value("status");return;}
            ++liveFrames;const auto rows=d.value("trains").toList();if(!rows.isEmpty()){auto t=rows.first().toMap();observed.insert(t["track"].toString()+QString::number(t["fraction"].toDouble(),'g',16));}
            if(d["reservationsAvailable"].toBool()&&d["occupationsAvailable"].toBool())++usageFrames;
            std::printf("LIVE %d capture_ms=%lld reserved=%lld available=%d occupied=%lld available=%d\n",liveFrames,
                d["captureMs"].toLongLong(),static_cast<long long>(d["mapReservations"].toList().size()),d["reservationsAvailable"].toBool(),
                static_cast<long long>(d["mapOccupations"].toList().size()),d["occupationsAvailable"].toBool());std::fflush(stdout);
            qInfo()<<"LIVE"<<liveFrames<<d["updated"]<<"captureMs"<<d["captureMs"]<<"positioned"<<d["positionedCount"]
                <<"reservations"<<d["reservationsAvailable"]<<d["mapReservations"].toList().size()
                <<"occupation"<<d["occupationsAvailable"]<<d["mapOccupations"].toList().size();
        });
        QTimer::singleShot(8000,&app,[&]{std::printf("VERIFY frames=%d usage_frames=%d distinct_positions=%lld\n",liveFrames,usageFrames,static_cast<long long>(observed.size()));std::fflush(stdout);app.exit(liveFrames>=2&&usageFrames>=2?0:2);});
    }
    if(app.arguments().contains("--smoke"))QTimer::singleShot(3500,&app,&QCoreApplication::quit);
    backend.connectGame(QString());return app.exec();
}
