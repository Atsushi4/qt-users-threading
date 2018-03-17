#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtConcurrent/QtConcurrentMap>
#include <QtCore/QThreadPool>
#include <QtCore/QFutureWatcher>
#include <QtCore/QFutureSynchronizer>
#include "worker.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = Q_NULLPTR) : QWidget{parent}{
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);
        auto buttonLayout = new QHBoxLayout{};
        auto runButton = new QPushButton{QStringLiteral("&Run")};
        connect(runButton, &QPushButton::clicked, this, &MainWindow::run);
        connect(this, &MainWindow::runningChanged, runButton, &QPushButton::setDisabled);
        buttonLayout->addWidget(runButton);
        cancelButton_ = new QPushButton{QStringLiteral("&Cancel")};
        connect(this, &MainWindow::runningChanged, cancelButton_, &QPushButton::setEnabled);
        connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::canceled);
        buttonLayout->addWidget(cancelButton_);
        mainLayout->addLayout(buttonLayout);
        auto progressLayout = new QVBoxLayout{};
        QThreadPool::globalInstance()->setMaxThreadCount(2);
        QThreadPool::globalInstance()->setExpiryTimeout(300);
        for (int i = 0; i < 10; ++i) {
            auto bar = new QSlider{Qt::Horizontal};
            bar->setRange(0, 100);
            bars_ << bar;
            progressLayout->addWidget(bar);
        }
        mainLayout->addLayout(progressLayout);
        emit runningChanged(false);
    }
    ~MainWindow() Q_DECL_OVERRIDE {
        emit canceled();
    }
signals:
    void runningChanged(bool running);
    void canceled();
public slots:
    void run() {
        emit runningChanged(true);
        for (int i = 0; i < 10; ++i) bars_[i]->setValue(0);

        auto watcher = new QFutureWatcher<void>{};
        // [1]
        connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]{
            watcher->deleteLater();
            emit runningChanged(false);
        });
        connect(this, &MainWindow::canceled, watcher, &QFutureWatcher<void>::cancel);
        // [1]
        watcher->setFuture(QtConcurrent::map(bars_, [this](QSlider *bar){
            Worker worker;
            worker.connect(&worker, &Worker::progressChanged, bar, &QSlider::setValue);
            worker.connect(this, &MainWindow::canceled, &worker, &Worker::cancel, Qt::DirectConnection);
            worker.doWork();
        }));
        sync.addFuture(watcher->future()); // [2]
    }
private:
    QPushButton *cancelButton_;
    QList<QSlider*> bars_;
    QFutureSynchronizer<void> sync;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

#include "main.moc"
