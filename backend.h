#pragma once
#include <QObject>
#include <QThread>
#include <QVariantMap>
#include <QTimer>
#include <atomic>
#include <mutex>

struct ViewFilter { int scope=0, page=0; uint64_t station=0, track=0, revision=0, train=0; };

class ReaderThread : public QThread {
    Q_OBJECT
public:
    std::atomic<uint64_t> request{0}; // Generation + requested PID, consumed by worker only.
    std::mutex viewMutex;
    ViewFilter view;
    std::atomic<bool> pending{false}; // At most one queued UI update.
    explicit ReaderThread(QObject* parent=nullptr):QThread(parent){}
signals:
    void received(uint64_t request, QVariantMap data);
protected:
    void run() override;
};
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap data READ data NOTIFY changed)
public:
    explicit Backend(QObject* parent=nullptr);
    ~Backend() override;
    QVariantMap data() const {return data_;}
    Q_INVOKABLE void connectGame(const QString& pid);
    Q_INVOKABLE void disconnectGame();
    Q_INVOKABLE void setView(int scope, const QString& station, const QString& track, int page);
    Q_INVOKABLE void selectTrain(const QString& train);
signals:
    void changed();
private:
    ReaderThread worker_;
    QVariantMap data_;
    uint64_t generation_=0;
    QTimer pathExpiry_;
};
