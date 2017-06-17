#include "widget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QApplication>
#include <QDir>

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      m_image(1404, 1872, QImage::Format_ARGB32),
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

    QDir dir(qApp->applicationDirPath() + "/../child/");
    if (!dir.entryList().contains("sequrerender-child")) {
        qWarning() << "Failed to find child executable";
        return;
    }

    QProcess process;

    QStringList arguments;
    arguments << m_sharedMemory.key()
              << QString::number(m_image.width())
              << QString::number(m_image.height());
    process.setArguments(arguments);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LD_LIBRARY_PATH", "/home/sandsmark/src/pdfium-qmake/build-pdfium-Qt_with_clang-Debug"); // Add an environment variable
    process.setProcessEnvironment(env);

    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setProgram(dir.absoluteFilePath("sequrerender-child"));

    qDebug() << "Starting" << process.program() << process.arguments();

    process.start();

    if (!process.waitForFinished(100000)) {
        qDebug() << "Timed out, trying to terminate";
        process.terminate();
        if (!process.waitForFinished(2000)) {
        qDebug() << "Timed out, trying to kill";
            process.kill();
            process.waitForFinished(2000);
        }
    }
    qDebug() << process.exitCode();

    if (process.error() != QProcess::UnknownError) {
        qWarning() << process.errorString() << process.exitCode() << process.readAllStandardError() << process.readAllStandardOutput();
    }

    m_imageLabel->setPixmap(QPixmap::fromImage(m_image));
    update();
}
