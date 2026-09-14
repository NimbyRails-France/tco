#pragma once
#include <QQuickPaintedItem>
#include <QVariantMap>
#include <QImage>
#include <nimby/observation.h>
#include <unordered_map>
#include <vector>
class MapItem : public QQuickPaintedItem {
 Q_OBJECT
 Q_PROPERTY(QVariantMap snapshotData READ data WRITE setData NOTIFY dataChanged)
 Q_PROPERTY(bool showCalculatedPath READ showCalculatedPath WRITE setShowCalculatedPath NOTIFY dataChanged)
public:
 explicit MapItem(QQuickItem* parent=nullptr);
 QVariantMap data() const{return data_;}
 void setData(const QVariantMap& value);
 void paint(QPainter* painter) override;
 bool showCalculatedPath() const{return showPath_;}
 void setShowCalculatedPath(bool value){if(showPath_!=value){showPath_=value;update();emit dataChanged();}}
 Q_INVOKABLE void fit();
 Q_INVOKABLE void moveView(double x,double y);
 Q_INVOKABLE void zoomAt(double factor,double x,double y);
 Q_INVOKABLE void focusTrain();
 Q_INVOKABLE QVariantList trainsAt(double x,double y) const;
signals:void dataChanged();
protected:void geometryChange(const QRectF& now,const QRectF& before) override;
private:
 QVariantMap data_;QByteArray geometry_;
 std::vector<NimbyTrackNode> nodes_;
 std::vector<unsigned> degree_;
 std::unordered_map<uint64_t,size_t> index_;
 QPointF center_;double scale_=1;bool fitted_=false,dirty_=true;
 QImage background_;
 bool showPath_=false;
 QPointF point(const NimbyTrackNode& n)const;
 void invalidate();
};
