QT += core gui

CONFIG += c++11

LIBS += -lseccomp

TARGET = sequrerender-child
CONFIG += console
CONFIG -= app_bundle

LIBS -=  -lGL

TEMPLATE = app

SOURCES += main.cpp

INCLUDEPATH += /home/sandsmark/src/pdfium-qmake/pdfium/public
LIBS += -L/home/sandsmark/src/pdfium-qmake/build-pdfium-Qt_with_clang-Debug -lpdfium
