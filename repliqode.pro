#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T18:07:12
#
#-------------------------------------------------

QT       += core gui widgets

LIBS +=  -lr_code -lr_comp -lr_exec

exists(config.pri) {
    include(config.pri)
}

# Hack to get QtCreator to pick it up
INCLUDEPATH += $$(INCLUDEPATH)

TARGET = repliqode
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp \
    hivewidget.cpp \
    replicodehandler.cpp \
    window.cpp \
    replicodehighlighter.cpp \
    streamredirector.cpp

HEADERS  += \
    hivewidget.h \
    replicodehandler.h \
    window.h \
    replicodehighlighter.h \
    streamredirector.h

# Copy in some examples
copydata.commands = $(COPY) \
                    $$PWD/std.replicode \
                    $$PWD/user.classes.replicode \
                    $$PWD/example-all-objects.image \
                    $$PWD/example-only-models.image \
                    $$OUT_PWD
first.depends = $(first) copydata
QMAKE_EXTRA_TARGETS += first copydata
