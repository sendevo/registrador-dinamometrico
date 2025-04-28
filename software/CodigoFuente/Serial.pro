include(M:\Proyectos\Registrador\Qextserialport\qextserialport-1.2rc\src\qextserialport.pri)

QT += core
QT += widgets

CONFIG -= app_bundle
#CONFIG += console
CONFIG += static

TARGET = RegistradorSoft

TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp

HEADERS += mainwindow.h
