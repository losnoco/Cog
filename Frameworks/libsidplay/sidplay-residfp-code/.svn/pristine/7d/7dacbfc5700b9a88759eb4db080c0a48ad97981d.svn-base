
AUTOMAKE_OPTIONS = foreign subdir-objects

if MINGW32
  W32_CPPFLAGS = -DWIN32_LEAN_AND_MEAN
endif


AM_CPPFLAGS = \
-I $(top_builddir)/src \
-I $(top_srcdir)/src \
-DPKGDATADIR="\"$(pkgdatadir)/\"" \
$(SIDPLAYFP_CFLAGS) \
$(STILVIEW_CFLAGS) \
$(ALSA_CFLAGS) \
$(PULSE_CFLAGS) \
${W32_CPPFLAGS} \
@debug_flags@


bin_PROGRAMS = \
src/sidplayfp \
src/stilview

#=========================================================
# sidplayfp

src_sidplayfp_SOURCES = \
src/IniConfig.cpp \
src/IniConfig.h \
src/args.cpp \
src/keyboard.cpp \
src/keyboard.h \
src/main.cpp \
src/menu.cpp \
src/player.cpp \
src/player.h \
src/sidcxx11.h \
src/utils.cpp \
src/utils.h \
src/audio/AudioBase.h \
src/audio/AudioConfig.h \
src/audio/AudioDrv.cpp \
src/audio/AudioDrv.h \
src/audio/IAudio.h \
src/audio/alsa/audiodrv.cpp \
src/audio/alsa/audiodrv.h \
src/audio/directx/audiodrv.cpp \
src/audio/directx/audiodrv.h \
src/audio/mmsystem/audiodrv.cpp \
src/audio/mmsystem/audiodrv.h \
src/audio/null/null.cpp \
src/audio/null/null.h \
src/audio/oss/audiodrv.cpp \
src/audio/oss/audiodrv.h \
src/audio/pulse/audiodrv.cpp \
src/audio/pulse/audiodrv.h \
src/audio/wav/WavFile.cpp \
src/audio/wav/WavFile.h \
src/ini/iniHandler.h \
src/ini/iniHandler.cpp \
src/ini/dataParser.h \
src/ini/sidfstream.h \
src/ini/types.h

src_sidplayfp_LDADD = \
$(AUDIO_LDFLAGS) \
$(SIDPLAYFP_LIBS) \
$(BUILDERS_LDFLAGS) \
$(ALSA_LIBS) \
$(PULSE_LIBS)

#=========================================================
# stilview

src_stilview_SOURCES = \
src/stilview.cpp

src_stilview_LDADD = \
$(STILVIEW_LIBS)

#=========================================================
# docs

EXTRA_DIST =  \
doc/en/sidplayfp.pod \
doc/en/sidplayfp.ini.pod \
doc/en/stilview.pod

dist_man_MANS = \
doc/en/sidplayfp.1 \
doc/en/sidplayfp.ini.5 \
doc/en/stilview.1

DISTCLEANFILES = $(dist_man_MANS)

.pod.1:
	pod2man -c "User Programs" -s 1 $< > $@

.pod.5:
	pod2man -c "File formats" -s 5 $< > $@
