#include <QCoreApplication>
#include <QFile>
#include <QJSEngine>
#include <cstdio>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    QFile source(QStringLiteral(TCO_SOURCE_DIR "/TrainFilter.js"));
    if(!source.open(QIODevice::ReadOnly))return 1;
    QString code=QString::fromUtf8(source.readAll());
    code.remove(".pragma library");
    QJSEngine engine;
    if(engine.evaluate(code).isError())return 2;
    const auto checks=engine.evaluate(R"JS(
        var moving={name:"TER Paris",id:"000500ABCDEF0001",line:"Ligne Nord",positioned:true,speedAvailable:true,speedDefaulted:false,speed:80};
        var stopped=Object.assign({},moving,{speed:0});
        var unknown=Object.assign({},moving,{positioned:false,speedAvailable:false,speed:0});
        var defaulted=Object.assign({},stopped,{speedDefaulted:true});
        var checks=[
            matches(moving,"  ter PARIS ",1,1),
            matches(moving,"abcdef",0,0),
            matches(moving,"nord",0,0),
            !matches(moving,"absent",0,0),
            !matches(moving,"",2,0),
            !matches(unknown,"",1,0),
            matches(unknown,"",2,3),
            matches(stopped,"",1,2),
            !matches(moving,"",0,2),
            !matches(stopped,"",0,1),
            !matches(unknown,"",0,2),
            !matches(defaulted,"",0,2),
            matches(defaulted,"",0,3),
            !matches(moving,"",0,3),
            matches(unknown,"",0,0),
            matches({id:"123"},"123",0,0)
        ];
        checks.every(function(value){return value === true;});
    )JS");
    if(checks.isError()||!checks.toBool()){std::fprintf(stderr,"Train filters failed\n");return 3;}
    return 0;
}
