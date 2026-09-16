#pragma once
#include <QImage>
#include <QImageReader>
#include <algorithm>

// Size the visible symbol, not the transparent canvas supplied by a mod.
// Preserve transparent frames and opaque images; never infer an aspect/color.
inline QImage trimSignalImage(const QImage& source) {
 if(source.isNull()||!source.hasAlphaChannel())return source;
 const auto image=source.convertToFormat(QImage::Format_ARGB32);
 int left=image.width(),top=image.height(),right=-1,bottom=-1;
 for(int y=0;y<image.height();++y){
  const auto* row=reinterpret_cast<const QRgb*>(image.constScanLine(y));
  for(int x=0;x<image.width();++x)if(qAlpha(row[x])!=0){
   left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
  }
 }
 if(right<left)return source;
 const QRect bounds=QRect(QPoint(left,top),QPoint(right,bottom)).adjusted(-1,-1,1,1).intersected(source.rect());
 return bounds==source.rect()?source:source.copy(bounds);
}

inline QImage loadSignalImage(const QString& path) {
 QImageReader reader(path);const auto size=reader.size();
 if(size.isValid())reader.setScaledSize(size.scaled(192,192,Qt::KeepAspectRatio));
 return trimSignalImage(reader.read());
}
