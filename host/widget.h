#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSharedMemory>
#include <QLabel>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void onRunClicked();

private:
    QImage m_image;
    QSharedMemory m_sharedMemory;
    QLabel *m_imageLabel;
};

#endif // WIDGET_H
