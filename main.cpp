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
    int liveFrames=0;QSet<QString> observed;
    if(app.arguments().contains("--verify-live")){
        if(!engine.rootObjects().isEmpty())engine.rootObjects().first()->setProperty("visible",false);
        QObject::connect(&backend,&Backend::changed,&app,[&]{const auto d=backend.data();if(!d.value("live").toBool()){qInfo()<<"WAIT"<<d.value("status");return;}
            ++liveFrames;const auto rows=d.value("trains").toList();if(!rows.isEmpty()){auto t=rows.first().toMap();observed.insert(t["track"].toString()+QString::number(t["fraction"].toDouble(),'g',16));}
            qInfo()<<"LIVE"<<liveFrames<<d["updated"]<<"captureMs"<<d["captureMs"]<<"positioned"<<d["positionedCount"];
        });
        QTimer::singleShot(8000,&app,[&]{qInfo()<<"VERIFY frames"<<liveFrames<<"distinct first-train positions"<<observed.size();app.exit(liveFrames>=2?0:2);});
    }
    if(app.arguments().contains("--smoke"))QTimer::singleShot(3500,&app,&QCoreApplication::quit);
    backend.connectGame(QString());return app.exec();
}
