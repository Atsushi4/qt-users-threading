#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
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
signals:
    void runningChanged(bool running);
public slots:
    void run() {
        emit runningChanged(true);
        for (int i = 0; i < 10; ++i) bars_[i]->setValue(0);

        for (int i = 0; i < 10; ++i) {
            auto bar = bars_[i];
            bar->setValue(0);
            Worker worker;
            connect(&worker, &Worker::progressChanged, bar, &QSlider::setValue);
            connect(cancelButton_, &QPushButton::clicked, &worker, &Worker::cancel);
            worker.doWork();
        }
        emit runningChanged(false);
    }
private:
    QPushButton *cancelButton_;
    QList<QSlider*> bars_;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

#include "main.moc"
