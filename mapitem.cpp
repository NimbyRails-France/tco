#include "mapitem.h"
#include "signalimage.h"
#include <QPainter>
#include <QImageReader>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <map>
MapItem::MapItem(QQuickItem* p):QQuickPaintedItem(p){setAntialiasing(true);}
void MapItem::setSignalSize(double value){if(!std::isfinite(value))return;value=std::clamp(value,8.,48.);if(signalSize_!=value){signalSize_=value;update();emit dataChanged();}}
QPointF MapItem::project(const QPointF& world)const{return {(world.x()-center_.x())*scale_+width()/2,(-world.y()-center_.y())*scale_+height()/2};}
QPointF MapItem::point(const MapNode& n)const{return project({n.x,n.y});}
std::optional<QPointF> MapItem::trainWorldPosition(const QVariantMap& train) const {
 if(!train.value("positioned").toBool())return std::nullopt;
 auto found=index_.find(train.value("track").toString().toULongLong(nullptr,16));
 bool valid=false;const double fraction=train.value("fraction").toDouble(&valid);
 if(found==index_.end()||!valid||!std::isfinite(fraction)||fraction<0||fraction>1)return std::nullopt;
 const auto& node=nodes_[found->second];const QPointF anchor(node.x,node.y);
 // The SDK exposes track anchors, not exact curve endpoints. Use shared
 // midpoints on reciprocal links so adjacent tracks meet without a jump.
 auto boundary=[&](uint64_t link){
  auto neighbour=index_.find(link);if(neighbour==index_.end())return anchor;
  const auto& other=nodes_[neighbour->second];
  if(other.link_a!=node.id&&other.link_b!=node.id)return anchor;
  return (anchor+QPointF(other.x,other.y))/2.;
 };
 // Fraction already uses the native A -> B orientation. Direction must not
 // invert it a second time for reverse-running trains.
 return fraction<=.5 ? boundary(node.link_a)+(anchor-boundary(node.link_a))*(fraction*2.)
                     : anchor+(boundary(node.link_b)-anchor)*((fraction-.5)*2.);
}
void MapItem::invalidate(){signalHits_.clear();dirty_=true;update();}
void MapItem::setData(const QVariantMap& v){
 signalHits_.clear();data_=v;auto bytes=v.value("mapGeometry").toByteArray();
 if(bytes!=geometry_){geometry_=bytes;nodes_.resize(bytes.size()/sizeof(MapNode));if(!nodes_.empty())std::memcpy(nodes_.data(),bytes.data(),nodes_.size()*sizeof(MapNode));
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
void MapItem::focusTrain(){
 for(const auto& value:data_.value("trains").toList()){
  const auto train=value.toMap();if(train.value("id")!=data_.value("selectedTrainId"))continue;
  if(const auto position=trainWorldPosition(train)){
   center_={position->x(),-position->y()};scale_=std::max(.05,std::min(width(),height())/3000.);invalidate();
  }
  return;
 }
}
QVariantList MapItem::trainsAt(double x,double y) const{
 // Hit radius is in screen pixels, independent of the zoom level.
 std::vector<std::pair<double,QVariantMap>> hits;
 for(const auto& value:data_.value("trains").toList()){
  const auto train=value.toMap();if(!train.value("positioned").toBool())continue;
  const auto position=trainWorldPosition(train);if(!position)continue;
  const auto delta=project(*position)-QPointF(x,y);
  const double distance=delta.x()*delta.x()+delta.y()*delta.y();
  if(distance<=100.)hits.emplace_back(distance,train);
 }
 std::stable_sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.first<b.first;});
 QVariantList result;for(const auto& hit:hits)result.append(hit.second);return result;
}
QString MapItem::signalDescription(const QVariantMap& signal) const {
 const auto text=signal.value("specificState").toString();
 return signal.value("kind").toString()+" "+signal.value("id").toString()+" | "+
  (signal.value("stateAvailable").toBool()?(text.isEmpty()?QString("État natif %1").arg(signal.value("textureState").toInt()):text):QString("État indisponible"));
}
QString MapItem::signalTextAt(double x,double y) const {
 for(const auto& hit:signalHits_)if(hit.first.contains(QPointF(x,y)))return hit.second;
 return {};
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
 // Fixed placement per signal: never let a neighbour take its screen slot.
 // Small overlaps are separated on screen, each with a leader to its own track.
 // Large clusters retain a neutral count instead of covering the whole map.
 const double symbolSize=signalSize_;
 struct Symbol {QVariantMap data;QPointF anchor;QRectF rect;QImage image;QPointF tangent;};
 std::vector<Symbol> symbols;
 for(const auto& value:data_.value("mapSignals").toList()){
  const auto m=value.toMap();auto it=index_.find(m.value("track").toString().toULongLong(nullptr,16));
  if(it==index_.end())continue;
  const auto anchor=point(nodes_[it->second]);
  if(!QRectF(-64,-64,width()+128,height()+128).contains(anchor))continue;
  QImage image;const auto path=m.value("texturePath").toString();
  if(m.value("stateAvailable").toBool()&&!path.isEmpty()){
   if(!signalImages_.contains(path)){
    if(signalImages_.size()>=256)signalImages_.clear();
    const auto decoded=loadSignalImage(path);
    if(!decoded.isNull())signalImages_.insert(path,decoded);
   }
   image=signalImages_.value(path);
  }
  const QSizeF size=image.isNull()?QSizeF(10,12):image.size().scaled(std::max(4,int(symbolSize)),std::max(4,int(symbolSize)),Qt::KeepAspectRatio);
  const bool below=m.value("balise").toBool()||m.value("direction").toInt()<0;
  const QPointF offset(-size.width()/2,below?3:-size.height()-3);
  const auto& node=nodes_[it->second];
  const auto a=index_.find(node.link_a),b=index_.find(node.link_b);
  QPointF tangent;
  if(a!=index_.end()&&b!=index_.end())tangent=point(nodes_[b->second])-point(nodes_[a->second]);
  else if(b!=index_.end())tangent=point(nodes_[b->second])-anchor;
  else if(a!=index_.end())tangent=anchor-point(nodes_[a->second]);
  const double length=std::hypot(tangent.x(),tangent.y());
  tangent=length>1e-12?tangent/length:QPointF(1,0);
  if(m.contains("trackAxisAvailable"))tangent=m.value("trackAxisAvailable").toBool()
      ?QPointF(m.value("trackAxisX").toDouble(),-m.value("trackAxisY").toDouble()):QPointF(1,0);
  symbols.push_back({m,anchor,QRectF(anchor+offset,size),image,tangent});
 }
 std::vector<size_t> parent(symbols.size());for(size_t i=0;i<parent.size();++i)parent[i]=i;
 auto root=[&](size_t i){while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}return i;};
 QHash<quint64,QList<size_t>> cells;
 for(size_t i=0;i<symbols.size();++i){
  const auto rect=symbols[i].rect.adjusted(-1,-1,1,1);
  for(int y=int(std::floor(rect.top()/64));y<=int(std::floor(rect.bottom()/64));++y)
   for(int x=int(std::floor(rect.left()/64));x<=int(std::floor(rect.right()/64));++x){
    auto& nearby=cells[(quint64(quint32(x))<<32)|quint32(y)];
    for(auto j:nearby)if(symbols[j].rect.adjusted(-1,-1,1,1).intersects(rect))parent[root(i)]=root(j);
    nearby.append(i);
   }
 }
 std::map<size_t,std::vector<size_t>> groups;
 for(size_t i=0;i<symbols.size();++i)groups[root(i)].push_back(i);
 signalHits_.clear();
 p->setFont(QFont("Segoe UI",8));
 auto drawSymbol=[&](const Symbol& symbol,const QRectF& rect){
  p->setPen(QPen(QColor("#b9b9b9"),1));
  const auto& a=symbol.anchor;
  p->drawLine(a,QPointF(std::clamp(a.x(),rect.left(),rect.right()),std::clamp(a.y(),rect.top(),rect.bottom())));
  if(!symbol.image.isNull())p->drawImage(rect,symbol.image);
  else{
   p->setBrush(QColor("#242a28"));p->drawRoundedRect(rect,2,2);p->drawText(rect,Qt::AlignCenter,"?");
  }
  signalHits_.push_back({rect.adjusted(-3,-3,3,3),signalDescription(symbol.data)});
 };
 for(const auto& [key,members]:groups){
  const auto& symbol=symbols[key];
  if(members.size()>1&&members.size()<=6){
   auto ordered=members;
   std::sort(ordered.begin(),ordered.end(),[&](size_t a,size_t b){
    return symbols[a].data.value("id").toString()<symbols[b].data.value("id").toString();
   });
   // Native fractions increase from link A toward link B. Display offsets
   // follow that axis; IDs only break ties, never define physical order.
   const auto axis=symbols[ordered.front()].tangent;
   const QPointF normal(-axis.y(),axis.x());
   const auto track=symbols[ordered.front()].data.value("track").toString();
   const bool sameTrack=std::all_of(ordered.begin(),ordered.end(),[&](size_t i){return symbols[i].data.value("track").toString()==track;});
   std::stable_sort(ordered.begin(),ordered.end(),[&](size_t a,size_t b){
    const auto order=[](const QVariantMap& m){return m.value("signalOrder",m.value("fraction")).toDouble();};
    if(sameTrack)return order(symbols[a].data)<order(symbols[b].data);
    const double pa=QPointF::dotProduct(symbols[a].anchor,axis),pb=QPointF::dotProduct(symbols[b].anchor,axis);
    if(pa!=pb)return pa<pb;
    const auto ta=symbols[a].data.value("track").toString(),tb=symbols[b].data.value("track").toString();
    if(ta!=tb)return ta<tb;
    const double sign=QPointF::dotProduct(symbols[a].tangent,axis)<0?-1.:1.;
    return sign*order(symbols[a].data)<sign*order(symbols[b].data);
   });
   QPointF center;double along=0,across=0;
   for(auto i:ordered){
    center+=symbols[i].anchor;const auto size=symbols[i].rect.size();
    along=std::max(along,std::abs(axis.x())*size.width()+std::abs(axis.y())*size.height());
    across=std::max(across,std::abs(normal.x())*size.width()+std::abs(normal.y())*size.height());
   }
   center/=double(ordered.size());
   for(int i=0;i<int(ordered.size());++i){
    const auto& item=symbols[ordered[i]];
    const QPointF position=center+axis*((i-(ordered.size()-1)/2.)*(along+18))-normal*(across/2+12);
    drawSymbol(item,QRectF(position-QPointF(item.rect.width()/2,item.rect.height()/2),item.rect.size()));
   }
   continue;
  }
  if(members.size()>1){
   QPointF center;QStringList details;
   for(auto i:members){center+=symbols[i].anchor;if(details.size()<20)details.append(signalDescription(symbols[i].data));}
   if(members.size()>20)details.append(QString("… et %1 autres signaux").arg(members.size()-20));
   center/=double(members.size());
   const QString label=QString::number(members.size());
   const QRectF badge(center+QPointF(-14,-8),QSizeF(28,16));
   p->setPen(QColor("#b9b9b9"));p->setBrush(QColor("#242a28"));p->drawRoundedRect(badge,3,3);
   p->drawText(badge,Qt::AlignCenter,label);
   signalHits_.push_back({badge,QString("%1 signaux regroupés (zoomer)\n").arg(members.size())+details.join("\n")});
   continue;
  }
  drawSymbol(symbol,symbol.rect);
 }
 p->setFont(QFont("Segoe UI",9));
 for(auto v:data_.value("trains").toList()){auto m=v.toMap();const auto position=trainWorldPosition(m);if(!position)continue;
  auto a=project(*position);if(!visible(a))continue;const bool selected=m["id"]==data_.value("selectedTrainId");
  p->setPen(Qt::NoPen);p->setBrush(QColor(selected?"#ffdf75":"#86d8c9"));p->drawEllipse(a,selected?5:2.5,selected?5:2.5);
  if(scale_>.025||selected){p->setPen(QColor("#f1eed6"));p->drawText(a+QPointF(7,14),m["name"].toString()+" · "+QString::number(m["fraction"].toDouble()*100,'f',1)+"%");}
 }
}
