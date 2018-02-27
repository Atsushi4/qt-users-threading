#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtCore/QThread> // [1]
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
        buttonLayout->addWidget(cancelButton_);
        mainLayout->addLayout(buttonLayout);
        auto progressLayout = new QVBoxLayout{};
        for (int i = 0; i < 10; ++i) {
            auto bar = new QSlider{Qt::Horizontal};
            bar->setRange(0, 100);
            bars_ << bar;
            progressLayout->addWidget(bar);
        }
        mainLayout->addLayout(progressLayout);
        emit runningChanged(false);
    }
    ~MainWindow() Q_DECL_OVERRIDE { // [6]
        emit canceled();
        for (auto thread : threads_) thread->wait();
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
            auto worker = new Worker{};
            connect(worker, &Worker::progressChanged, bar, &QSlider::setValue);
            connect(cancelButton_, &QPushButton::clicked, worker, &Worker::cancel, Qt::DirectConnection); // [4]
            connect(worker, &Worker::finished, worker, &Worker::deleteLater);
            // [2]
            auto thread = new QThread{};
            worker->moveToThread(thread);
            connect(thread, &QThread::started, worker, &Worker::doWork);
            connect(worker, &Worker::destroyed, thread, &QThread::quit);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            thread->start(QThread::LowestPriority); // [3]
            // [2]

            // [5]
            connect(this, &MainWindow::canceled, worker, &Worker::cancel, Qt::DirectConnection);
            connect(thread, &QThread::finished, this, [this](){
                threads_.removeOne(qobject_cast<QThread*>(sender()));
                if (threads_.isEmpty()) emit runningChanged(false);
            });
            // [5]
        }
    }
private:
    QPushButton *cancelButton_;
    QList<QSlider*> bars_;
    QList<QThread*> threads_;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

#include "main.moc"
