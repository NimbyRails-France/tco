#include "../signalimage.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
 QImage padded(128,128,QImage::Format_ARGB32);padded.fill(Qt::transparent);
 for(int y=48;y<80;++y)for(int x=32;x<96;++x)padded.setPixel(x,y,qRgba(255,0,0,255));
 const auto visible=trimSignalImage(padded);
 CHECK(visible.size()==QSize(66,34));
 CHECK(visible.pixel(1,1)==qRgba(255,0,0,255));
 CHECK(qAlpha(visible.pixel(0,0))==0);
 QImage transparent(128,128,QImage::Format_ARGB32);transparent.fill(Qt::transparent);
 CHECK(trimSignalImage(transparent)==transparent);
 QImage opaque(20,10,QImage::Format_RGB32);opaque.fill(Qt::black);
 CHECK(trimSignalImage(opaque)==opaque);
 CHECK(trimSignalImage(QImage()).isNull());
 padded.setPixel(0,0,qRgba(0,255,0,1));
 CHECK(qAlpha(trimSignalImage(padded).pixel(0,0))==1);
 std::puts("Signal image: transparent padding, retained pixels, empty frames and opaque assets passed.");
}
