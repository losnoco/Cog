#
# Makefile for Audio Overload SDK
#
# Copyright (c) 2007-2008, R. Belmont and Richard Bannister.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
# * Neither the names of R. Belmont and Richard Bannister nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# NOTE: this makefile will auto-detect Linux and MinGW and work appropriately.  Other OSes will likely
#       need some help.

CC   = gcc
LD   = gcc
CPP  = g++
CFLAGS = -c -O3 -DPATH_MAX=1024 -DHAS_PSXCPU=1 -I. -I.. -Ieng_ssf -Ieng_qsf  -Ieng_dsf -Izlib
# set for Linux 64-bit
# CFLAGS += -DLONG_IS_64BIT=1
# set for little-endian, make "0" for big-endian
CFLAGS += -DLSB_FIRST=1

LDFLAGS = 

EXE  = aosdk
LIBS = -lm -lz

# main objects
OBJS = corlett.o

# port objects
#ifeq ($(OSTYPE),linux)
#OBJS += oss.o
#else
#OBJS += dsnd.o
#LIBS += -ldsound -ldxguid
#endif

# DSF engine
OBJS += eng_dsf/eng_dsf.o eng_dsf/dc_hw.o eng_dsf/aica.o eng_dsf/aicadsp.o eng_dsf/arm7.o eng_dsf/arm7i.o

# SSF engine
OBJS += eng_ssf/m68kcpu.o eng_ssf/m68kopac.o eng_ssf/m68kopdm.o eng_ssf/m68kopnz.o eng_ssf/m68kops.o 
OBJS += eng_ssf/scsp.o eng_ssf/scspdsp.o eng_ssf/sat_hw.o eng_ssf/eng_ssf.o

# QSF engine
OBJS += eng_qsf/eng_qsf.o eng_qsf/kabuki.o eng_qsf/qsound.o eng_qsf/z80.o eng_qsf/z80dasm.o

# PSF engine
OBJS += eng_psf/eng_psf.o eng_psf/psx.o eng_psf/psx_hw.o eng_psf/peops/spu.o 

# PSF2 extentions
OBJS += eng_psf/eng_psf2.o eng_psf/peops2/spu.o eng_psf/peops2/dma.o eng_psf/peops2/registers.o

# SPU engine (requires PSF engine)
OBJS += eng_psf/eng_spu.o

# zlib (included for max portability)
#OBJS += zlib/adler32.o zlib/compress.o zlib/crc32.o zlib/gzio.o zlib/uncompr.o zlib/deflate.o zlib/trees.o
#OBJS += zlib/zutil.o zlib/inflate.o zlib/infback.o zlib/inftrees.o zlib/inffast.o

SRCS=$(OBJS:.o=.c)

%.o: %.c
	@echo $(CC) $(CFLAGS) $< -o $@
	@$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	@echo $(CPP) $(CFLAGS) $< -o $@
	@$(CPP) $(CFLAGS) $< -o $@
	
all: $(EXE)

$(EXE): $(OBJS)
	@echo $(LD) $(LDFLAGS) -g -o $(EXE) $(OBJS) $(LIBS)
	@$(LD) $(LDFLAGS) -g -o $(EXE) $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(EXE)

