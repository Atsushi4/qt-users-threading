#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QThreadPool>
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
        threadPool.setMaxThreadCount(2); // [2]
        threadPool.setExpiryTimeout(300); // [2]
        connect(this, &MainWindow::canceled, &threadPool, &QThreadPool::clear); // [3]
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

        for (int i = 0; i < 10; ++i) {
            auto bar = bars_[i];
            bar->setValue(0);
            // [1]
            QtConcurrent::run(&threadPool, [](MainWindow *window, QSlider *bar){
                Worker worker;
                worker.connect(&worker, &Worker::progressChanged, bar, &QSlider::setValue);
                worker.connect(window, &MainWindow::canceled, &worker, &Worker::cancel, Qt::DirectConnection);
                worker.doWork();
            }, this, bar);
            // [2]
            QtConcurrent::run([this]{
                threadPool.waitForDone();
                emit runningChanged(false);
            });
        }
    }
private:
    QPushButton *cancelButton_;
    QList<QSlider*> bars_;
    QThreadPool threadPool;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

#include "main.moc"
