#include "mapitem.h"
#include <QPainter>
#include <algorithm>
#include <cstring>
#include <cmath>
MapItem::MapItem(QQuickItem* p):QQuickPaintedItem(p){setAntialiasing(true);}
QPointF MapItem::point(const NimbyTrackNode& n)const{return {(n.x-center_.x())*scale_+width()/2,(-n.y-center_.y())*scale_+height()/2};}
void MapItem::invalidate(){dirty_=true;update();}
void MapItem::setData(const QVariantMap& v){
 data_=v;auto bytes=v.value("mapGeometry").toByteArray();
 if(bytes!=geometry_){geometry_=bytes;nodes_.resize(bytes.size()/sizeof(NimbyTrackNode));if(!nodes_.empty())std::memcpy(nodes_.data(),bytes.data(),nodes_.size()*sizeof(NimbyTrackNode));
 index_.clear();for(size_t i=0;i<nodes_.size();++i)index_.emplace(nodes_[i].id,i);
 degree_.assign(nodes_.size(),0);
 for(size_t i=0;i<nodes_.size();++i){const auto& n=nodes_[i];for(auto link:{n.link_a,n.link_b}){auto t=index_.find(link);if(t==index_.end())continue;const auto& target=nodes_[t->second];if(n.id>link&&(target.link_a==n.id||target.link_b==n.id))continue;++degree_[i];++degree_[t->second];}}
 dirty_=true;
 if(!fitted_&&!nodes_.empty())fit();}
 update();emit dataChanged();
}
void MapItem::geometryChange(const QRectF& n,const QRectF& b){QQuickPaintedItem::geometryChange(n,b);if(!fitted_&&!nodes_.empty())fit();invalidate();}
void MapItem::fit(){if(nodes_.empty()||width()<1||height()<1)return;
 double x0=nodes_[0].x,x1=x0,y0=-nodes_[0].y,y1=y0;
 for(const auto& n:nodes_){x0=std::min(x0,n.x);x1=std::max(x1,n.x);y0=std::min(y0,-n.y);y1=std::max(y1,-n.y);}
 center_={(x0+x1)/2,(y0+y1)/2};scale_=std::max(1e-7,std::min((width()-50)/std::max(1.,x1-x0),(height()-90)/std::max(1.,y1-y0)));fitted_=true;invalidate();}
void MapItem::moveView(double x,double y){center_-=QPointF(x/scale_,y/scale_);invalidate();}
void MapItem::zoomAt(double factor,double x,double y){const QPointF cursor(x-width()/2,y-height()/2);const auto world=center_+cursor/scale_;scale_=std::clamp(scale_*factor,1e-7,50.);center_=world-cursor/scale_;invalidate();}
void MapItem::focusTrain(){const auto id=data_.value("selectedTrackId").toString().toULongLong(nullptr,16);auto i=index_.find(id);if(i==index_.end())return;auto n=nodes_[i->second];center_={n.x,-n.y};scale_=std::max(.05,std::min(width(),height())/3000.);invalidate();}
QVariantList MapItem::trainsAt(double x,double y) const{
 // Hit radius is in screen pixels, independent of the zoom level.
 std::vector<std::pair<double,QVariantMap>> hits;
 for(const auto& value:data_.value("trains").toList()){
  const auto train=value.toMap();if(!train.value("positioned").toBool())continue;
  auto node=index_.find(train.value("track").toString().toULongLong(nullptr,16));
  if(node==index_.end())continue;
  const auto delta=point(nodes_[node->second])-QPointF(x,y);
  const double distance=delta.x()*delta.x()+delta.y()*delta.y();
  if(distance<=100.)hits.emplace_back(distance,train);
 }
 std::stable_sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.first<b.first;});
 QVariantList result;for(const auto& hit:hits)result.append(hit.second);return result;
}
void MapItem::paint(QPainter* p){
 if(width()<1||height()<1)return;
 if(dirty_||background_.size()!=QSize(int(width()),int(height()))){
  background_=QImage(int(width()),int(height()),QImage::Format_ARGB32_Premultiplied);background_.fill(QColor("#070b0a"));QPainter b(&background_);b.setRenderHint(QPainter::Antialiasing);b.setPen(QPen(QColor("#a6b8ad"),1.3));
  const QRectF bounds(0,0,width(),height());
  for(const auto& n:nodes_){auto a=point(n);for(auto link:{n.link_a,n.link_b}){auto it=index_.find(link);if(it==index_.end())continue;const auto& target=nodes_[it->second];
    if(n.id>link&&(target.link_a==n.id||target.link_b==n.id))continue;
    auto z=point(target);if(!QRectF(a,z).normalized().adjusted(-2,-2,2,2).intersects(bounds))continue;b.drawLine(a,z);
  }}
  if(scale_>.05){b.setPen(QPen(QColor("#d3a873"),1.4));b.setBrush(QColor("#070b0a"));for(size_t i=0;i<nodes_.size();++i)if(degree_[i]>2){auto a=point(nodes_[i]);if(bounds.contains(a))b.drawEllipse(a,3,3);}}
  dirty_=false;
 }
 p->drawImage(0,0,background_);p->setRenderHint(QPainter::Antialiasing);
 auto visible=[&](QPointF a){return a.x()>-20&&a.x()<width()+20&&a.y()>-20&&a.y()<height()+20;};
 p->setPen(QPen(QColor("#e4bf50"),3));
 if(showPath_&&data_.value("live").toBool()&&data_.value("pathAvailable").toBool()&&!data_.value("pathStale").toBool())
 for(auto v:data_.value("mapPath").toList()){auto i=index_.find(v.toString().toULongLong(nullptr,16));if(i!=index_.end()){auto a=point(nodes_[i->second]);if(visible(a))p->drawPoint(a);}}
 // Track anchors only: curve interpolation and switch branches are not validated.
 auto usage=[&](const char* key,const char* available,const QColor& color,bool occupied){
  if(!data_.value("live").toBool()||data_.value("usageStale").toBool()||!data_.value(available).toBool())return;
  p->setPen(QPen(color,2));p->setBrush(Qt::NoBrush);
  for(const auto& value:data_.value(key).toList()){
   auto i=index_.find(value.toMap().value("track").toString().toULongLong(nullptr,16));if(i==index_.end())continue;
   const auto a=point(nodes_[i->second]);if(!visible(a))continue;
   if(occupied)p->drawRect(QRectF(a-QPointF(4,4),QSizeF(8,8)));else p->drawEllipse(a,7,7);
  }
 };
 usage("mapReservations","reservationsAvailable",QColor("#65db87"),false);
 usage("mapOccupations","occupationsAvailable",QColor("#ff6b6b"),true);
 if(scale_>.02){p->setPen(QPen(QColor("#80bdd8"),1));for(auto v:data_.value("mapSignals").toList()){auto m=v.toMap();auto i=index_.find(m["track"].toString().toULongLong(nullptr,16));if(i==index_.end())continue;auto a=point(nodes_[i->second]);if(!visible(a))continue;
  p->drawLine(a,a+QPointF(0,-8));if(m["balise"].toBool())p->drawRect(QRectF(a+QPointF(-2,-12),QSizeF(4,4)));else p->drawEllipse(a+QPointF(0,-11),3,3);
 }}
 p->setFont(QFont("Segoe UI",9));
 for(auto v:data_.value("trains").toList()){auto m=v.toMap();if(!m["positioned"].toBool())continue;auto i=index_.find(m["track"].toString().toULongLong(nullptr,16));if(i==index_.end())continue;
  auto a=point(nodes_[i->second]);if(!visible(a))continue;const bool selected=m["id"]==data_.value("selectedTrainId");
  p->setPen(Qt::NoPen);p->setBrush(QColor(selected?"#ffdf75":"#86d8c9"));p->drawEllipse(a,selected?5:2.5,selected?5:2.5);
  if(scale_>.025||selected){p->setPen(QColor("#f1eed6"));p->drawText(a+QPointF(7,14),m["name"].toString()+" · "+QString::number(m["fraction"].toDouble()*100,'f',1)+"%");}
 }
}
