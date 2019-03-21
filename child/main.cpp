#include <QCoreApplication>
#include <QSharedMemory>
#include <QImage>
#include <QDebug>
#include <QTime>
#include <seccomp.h>

#include <fcntl.h>

#include <fpdfview.h>

#define ALLOW_SYSCALL(sysscall) if (!ret) ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(sysscall), 0);

void lockDown()
{
    scmp_filter_ctx context;

    context = seccomp_init(SCMP_ACT_KILL);

    int ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);

    //ALLOW_SYSCALL(access         )
    //ALLOW_SYSCALL(arch_prctl     )
    ALLOW_SYSCALL(brk            )
    ALLOW_SYSCALL(close          )
    ALLOW_SYSCALL(exit_group     )
    ALLOW_SYSCALL(fstat          )
    ALLOW_SYSCALL(getpid         )
    ALLOW_SYSCALL(getrlimit      )
    ALLOW_SYSCALL(lseek          )
    ALLOW_SYSCALL(mmap           )
    //ALLOW_SYSCALL(mprotect       )
    ALLOW_SYSCALL(munmap         )
    //ALLOW_SYSCALL(pread64        )
    //ALLOW_SYSCALL(read           )
    ALLOW_SYSCALL(rt_sigaction   )
    ALLOW_SYSCALL(rt_sigprocmask )
    ALLOW_SYSCALL(semop          )
    ALLOW_SYSCALL(set_robust_list)
    ALLOW_SYSCALL(set_tid_address)
    ALLOW_SYSCALL(shmctl         )
    ALLOW_SYSCALL(shmdt          )
    ALLOW_SYSCALL(shmget         )
    //ALLOW_SYSCALL(write          )

    // Only allow writes to stderr (qDebug)
    if (!ret) {
            ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(write), 1,
                            SCMP_CMP(0, SCMP_CMP_EQ, 2));
    }

    // Only allow reads from pdf file
    if (!ret) {
            ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(read), 1,
                            SCMP_CMP(0, SCMP_CMP_EQ, 3));
    }
    // Only allow reads from pdf file
    if (!ret) {
            ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 1,
                            SCMP_CMP(0, SCMP_CMP_EQ, 3));
    }
    if (!ret) {
            ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 1,
                            SCMP_CMP(0, SCMP_CMP_EQ, 3));
    }
    if (!ret) {
            ret = seccomp_rule_add(context, SCMP_ACT_ALLOW, SCMP_SYS(pread64), 1,
                            SCMP_CMP(0, SCMP_CMP_EQ, 3));
    }

    if (!ret)
        ret = seccomp_load(context);

    if (ret)
        printf("error setting seccomp\n");
}

static int s_readCallback(void* param,
        unsigned long pos,
        unsigned char* buf,
        unsigned long size)
{
    QFile *inFile = static_cast<QFile*>(param);
    if (pos != inFile->pos()) {
        inFile->seek(pos);
    }
    if (inFile->read(buf, size) < 0) {
        return 0
    } else {
        return 1;
    }
}

FPDF_DOCUMENT loadPdf(const QString &path)
{
    FPDF_DOCUMENT m_pdfDocument;
    qDebug() << "Opening document";
    m_pdfDocument = FPDF_LoadDocument(path.toLocal8Bit().constData(), nullptr);
    qDebug() << "Document opened";
    return m_pdfDocument;
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

    QByteArray m_path = "/home/sandsmark/Downloads/Testweb_reMarkable.pdf";


    qDebug() << "Attaching to" << key;

    QSharedMemory sharedMemory(key);
    if (!sharedMemory.attach()) {
        qWarning() << "Failed to attach to shared memory" << sharedMemory.nativeKey() << sharedMemory.errorString();
        return 1;
    }
    qDebug() << "Attached";


    FPDF_SetSandBoxPolicy(FPDF_POLICY_MACHINETIME_ACCESS, true);

    FPDF_LIBRARY_CONFIG_ config;
    config.version = 2;
    config.m_pUserFontPaths = nullptr;
    config.m_pIsolate = nullptr;
    config.m_v8EmbedderSlot = 0;

    qDebug() << "Initializing";
    FPDF_InitLibraryWithConfig(&config);
    qDebug() << "Initialized";


    lockDown();
    qDebug() << "Locked down";

    FPDF_DOCUMENT document = loadPdf(m_path);
    if (!document) {
        qWarning() << "Failed to load pdf";
        sharedMemory.detach();
        return 1;
    }
    int m_pageCount = FPDF_GetPageCount(document);
    qDebug() << "page count" << m_pageCount;

    QImage image((uchar*)sharedMemory.data(), width, height, QImage::Format_ARGB32);
    qDebug() << "QImage created";

    if (image.byteCount() != sharedMemory.size()) {
        qWarning() << "Image data size" << image.byteCount() << "does not match shared memory size" << sharedMemory.size();
        sharedMemory.detach();
        return 1;
    }

    image.fill(Qt::transparent);
    // TODO: FPDF_QuickDrawPage?

    FPDF_PAGE pdfPage = FPDF_LoadPage(document, 0);
    if (!pdfPage) {
        qWarning() << "Failed to load pdf page";
        sharedMemory.detach();
        return 1;
    }

    int renderFlags = FPDF_GRAYSCALE;
    int pdfImageFormat = FPDFBitmap_Gray;

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(image.width(), image.height(),
                                             pdfImageFormat,
                                             image.scanLine(0),
                                             image.bytesPerLine());
    FPDF_RenderPageBitmap(bitmap, pdfPage, 0, 0, image.width(), image.height(), 0, renderFlags);
    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(pdfPage);

    qDebug() << "Image filled";
    sharedMemory.detach();
    qDebug() << "Image detached";

    return 0;
}
