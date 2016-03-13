#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T18:07:12
#
#-------------------------------------------------

QT       += core gui widgets

LIBS +=  -lr_code -lr_comp -lr_exec

# Hack to get QtCreator to pick it up
INCLUDEPATH += $$(INCLUDEPATH)

TARGET = repliqode
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp \
    hivewidget.cpp \
    replicodehandler.cpp \
    cHighlighterReplicode.cpp \
    window.cpp

HEADERS  += \
    hivewidget.h \
    replicodehandler.h \
    cHighlighterReplicode.h \
    window.h
