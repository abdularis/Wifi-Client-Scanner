#-------------------------------------------------
#
# Project created by QtCreator 2017-02-27T20:06:05
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = wcscanner
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    rawsocket.cpp \
    sniffer.cpp

HEADERS  += mainwindow.h \
    rawsocket.h \
    sniffer.h

FORMS    += mainwindow.ui

OTHER_FILES += \
    mlist/db
