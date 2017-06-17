QT += core gui

CONFIG += c++11

LIBS += -lseccomp

TARGET = sequrerender-child
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp
