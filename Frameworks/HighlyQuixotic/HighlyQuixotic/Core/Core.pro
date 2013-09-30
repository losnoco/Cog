#-------------------------------------------------
#
# Project created by QtCreator 2012-12-26T20:57:48
#
#-------------------------------------------------

QT       -= core gui

TARGET = QSoundCore
TEMPLATE = lib
CONFIG += staticlib

DEFINES += EMU_COMPILE EMU_LITTLE_ENDIAN HAVE_STDINT_H

SOURCES += \
    qsound.c \
    qmix.c \
    kabuki.c \
    z80.c

HEADERS += \
    qsound.h \
    qmix.h \
    kabuki.h \
    emuconfig.h \
    z80.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    COPYING.txt \
    Readme.txt \
    m68k/m68k_in.c \
    m68k/m68kmake.c
