#include "widget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QApplication>

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      m_image(100, 100, QImage::Format_ARGB32),
      m_sharedMemory("sequrerender")
{
    qDebug() << m_sharedMemory.nativeKey();
    if (!m_sharedMemory.attach()) {
        qDebug() << "Couldn't attach to existing, creating new";
        if (!m_sharedMemory.create(m_image.byteCount())) {
            QMessageBox::warning(nullptr, "Failed to create shared memory", m_sharedMemory.errorString());
        }
    }
    if (m_sharedMemory.isAttached()) {
        m_image = QImage ((uchar*)m_sharedMemory.data(), m_image.width(), m_image.height(), QImage::Format_ARGB32);
    }

    m_image.fill(Qt::red);

    setWindowFlags(Qt::Dialog);
    setLayout(new QVBoxLayout);

    QPushButton *runButton = new QPushButton("Run");
    layout()->addWidget(runButton);
    connect(runButton, &QPushButton::clicked, this, &Widget::onRunClicked);

    QPushButton *updateButton = new QPushButton("Update");
    layout()->addWidget(updateButton);
    connect(updateButton, &QPushButton::clicked, [&]() {
        m_imageLabel->setPixmap(QPixmap::fromImage(m_image));
        update();
    });

    m_imageLabel = new QLabel;
    m_imageLabel->setMinimumSize(m_image.size());
    m_imageLabel->setPixmap(QPixmap::fromImage(m_image));
    layout()->addWidget(m_imageLabel);

    QPushButton *quitButton = new QPushButton("Quit");
    layout()->addWidget(quitButton);
    connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

Widget::~Widget()
{
    if (m_sharedMemory.isAttached()) {
        m_sharedMemory.detach();
    }
}

void Widget::onRunClicked()
{
    if (!m_sharedMemory.isAttached()) {
        QMessageBox::warning(nullptr, "Not attached to shared memory", m_sharedMemory.errorString());
        return;
    }

    QStringList arguments;
    arguments << m_sharedMemory.key()
              << QString::number(m_image.width())
              << QString::number(m_image.height());
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start("/home/sandsmark/src/sequrerender-child/build-sequrerender-child-Qt_with_clang-Debug/sequrerender-child", arguments);

    if (!process.waitForFinished(10000)) {
        process.terminate();
        if (!process.waitForFinished(2000)) {
            process.kill();
            process.waitForFinished(2000);
        }
    }

    m_imageLabel->setPixmap(QPixmap::fromImage(m_image));
    update();
}
