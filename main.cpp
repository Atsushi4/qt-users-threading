#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>
#include "worker.h"

template <typename ...Args>
class NandemoRunner : public QRunnable {
public:
    NandemoRunner(std::function<void(Args...)> func, Args... args) : func{std::bind(func, args...)} {}
    void run() Q_DECL_OVERRIDE {func();}
private:
    std::function<void()> func;
};

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

        // [4]
        for (int i = 0; i < 10; ++i) {
            auto bar = bars_[i];
            bar->setValue(0);
            threadPool.start(new NandemoRunner<MainWindow*, QSlider*>{[](MainWindow *window, QSlider *bar){
                Worker worker;
                worker.connect(&worker, &Worker::progressChanged, bar, &QSlider::setValue);
                worker.connect(window, &MainWindow::canceled, &worker, &Worker::cancel, Qt::DirectConnection);
                worker.doWork();
            }, this, bar}, i); // [6]
            // [5]
            QThreadPool::globalInstance()->start(new NandemoRunner<>{[this]{
                    threadPool.waitForDone();
                    emit runningChanged(false);
                }
            });
        }
        // [4]
    }
private:
    QPushButton *cancelButton_;
    QList<QSlider*> bars_;
    QThreadPool threadPool; // [1]
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

#include "main.moc"
