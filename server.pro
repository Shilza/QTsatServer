QT -= gui
QT += network
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp \
    server.cpp

HEADERS += \
    def.h \
    server.h
