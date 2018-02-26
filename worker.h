#ifndef WORKER_H
#define WORKER_H
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <atomic>

class Worker : public QObject {
    Q_OBJECT
public:
    explicit Worker(QObject *parent = Q_NULLPTR) : QObject{parent} {qDebug() << "new" << this;}
    ~Worker() Q_DECL_OVERRIDE {qDebug() << "delete" << this;}
signals:
    void progressChanged(int done);
    void finished();
public slots:
    void doWork() {
        // work few seconds
        for (int i = 0; i <= 100; ++i) {
            if (canceled_) break;
            QThread::msleep(rand() * 10 / RAND_MAX);
            emit progressChanged(i);
        }
        emit finished();
    }
    void cancel(){canceled_ = true; qDebug() << "canceled" << this;}
private:
    std::atomic_bool canceled_ = false;
};
#endif // WORKER_H
