#-------------------------------------------------
#
# Project created by QtCreator 2012-12-26T20:57:48
#
#-------------------------------------------------

QT       -= core gui

TARGET = SegaCore
TEMPLATE = lib
CONFIG += staticlib

# C68K: C68K_NO_JUMP_TABLE
# M68K: USE_M68K and LSB_FIRST if host is little endian
# Starscream: USE_STARSCREAM

DEFINES += EMU_COMPILE EMU_LITTLE_ENDIAN HAVE_STDINT_H USE_M68K LSB_FIRST HAVE_MPROTECT

SOURCES += \
    sega.c \
    dcsound.c \
    satsound.c \
    yam.c \
    arm.c \
    m68k/m68kops.c \
    m68k/m68kcpu.c

HEADERS += \
    sega.h \
    dcsound.h \
    satsound.h \
    emuconfig.h \
    yam.h \
    arm.h \
    m68k/m68kconf.h \
    m68k/m68kcpu.h \
    m68k/m68k.h \
    m68k/m68kops.h \
    m68k/macros.h
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
