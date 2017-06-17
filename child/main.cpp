#include <QCoreApplication>
#include <QSharedMemory>
#include <QImage>
#include <QDebug>
#include <QTime>
#include <seccomp.h>

void lockDown()
{
    scmp_filter_ctx context;

    context = seccomp_init(SCMP_ACT_KILL);

    int ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    if (!ret)
        ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    if (!ret)
        ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(semop), 0);
    if (!ret)
        ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(shmctl), 0);
    if (!ret)
        ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(shmget), 0);
    if (!ret)
        ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(shmdt), 0);
    if (!ret)
        ret = seccomp_load(context);

    if (ret)
        printf("error setting seccomp\n");
}

int main(int argc, char *argv[])
{
    qsrand(QTime::currentTime().msec());

    if (argc < 4) {
        qWarning() << "Usage:" << argv[0] << "<shared memory key> <width> <height>";
        return 1;
    }

    QString key = QString::fromLocal8Bit(argv[1]);
    int width = QString::fromLocal8Bit(argv[2]).toInt();
    int height = QString::fromLocal8Bit(argv[3]).toInt();

    qDebug() << "Attaching to" << key;

    QSharedMemory sharedMemory(key);
    if (!sharedMemory.attach()) {
        qWarning() << "Failed to attach to shared memory" << sharedMemory.nativeKey() << sharedMemory.errorString();
        return 1;
    }
    qDebug() << "Attached";
    lockDown();

    QImage image((uchar*)sharedMemory.data(), width, height, QImage::Format_ARGB32);
    qDebug() << "QImage created";

    if (image.byteCount() != sharedMemory.size()) {
        qWarning() << "Image data size" << image.byteCount() << "does not match shared memory size" << sharedMemory.size();
        return 1;
    }
    qDebug() << "Checked image size";

    image.fill((Qt::GlobalColor)(qrand() % Qt::transparent));
    qDebug() << "Image filled";
    sharedMemory.detach();
    qDebug() << "Image detached";
}
