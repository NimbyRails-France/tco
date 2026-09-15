#include "../updater.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL line %d\n",__LINE__);return 1;}}while(false)
int main(){
 QJsonObject good{{"schema",1},{"product","NimbyTco"},{"platform","windows-x64"},{"version","0.4.0"},{"url","https://example.com/setup.exe"},{"sha256",QString(64,'a')},{"size",1024}};
 UpdateRelease r;CHECK(parseRelease(good,"0.3.0",r));CHECK(r.size==1024);
 CHECK(!parseRelease(good,"0.4.0",r));CHECK(!parseRelease(good,"1.0.0",r));
 for(auto url:{"http://example.com/setup.exe","file:///C:/setup.exe","https://user:pass@example.com/setup.exe"}){auto o=good;o["url"]=url;CHECK(!parseRelease(o,"0.3.0",r));}
 for(auto version:{"0.4.0-evil","999999999.0.0","0.4","junk"}){auto o=good;o["version"]=version;CHECK(!parseRelease(o,"0.3.0",r));}
 for(double size:{-1.,0.,1.5,536870913.,1e30}){auto o=good;o["size"]=size;CHECK(!parseRelease(o,"0.3.0",r));}
 auto o=good;o["sha256"]="bad";CHECK(!parseRelease(o,"0.3.0",r));o=good;o["platform"]="linux";CHECK(!parseRelease(o,"0.3.0",r));
 std::puts("Update manifest validation passed");
}
