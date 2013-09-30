#-------------------------------------------------
#
# Project created by QtCreator 2012-12-26T20:57:48
#
#-------------------------------------------------

QT       -= core gui

TARGET = PSXCore
TEMPLATE = lib
CONFIG += staticlib

DEFINES += EMU_COMPILE EMU_LITTLE_ENDIAN HAVE_STDINT_H

SOURCES += \
    psx.c \
    ioptimer.c \
    iop.c \
    bios.c \
    r3000dis.c \
    r3000asm.c \
    r3000.c \
    vfs.c \
    spucore.c \
    spu.c \
    mkhebios.c

HEADERS += \
    ioptimer.h \
    iop.h \
    emuconfig.h \
    bios.h \
    psx.h \
    r3000asm.h \
    r3000.h \
    vfs.h \
    spucore.h \
    spu.h \
    r3000dis.h \
    mkhebios.h \
    mkhebios_overlays.h \
    mkhebios_scripts.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    Readme.txt \
    r3000predict.txt
