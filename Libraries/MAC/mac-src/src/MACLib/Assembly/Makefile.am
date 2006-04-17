AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libmacasm.la

COMMON_SOURCES = \
	Assembly.h

LINK = $(LIBTOOL) --tag=CXX --mode=link $(CXX) -o $@

NASM = @NASM@

if ARCH_X86

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh
SUFFIXES = .nas .lo

.nas.lo:
	$(LIBTOOL) --mode=compile --tag=ASM $(STRIP_FPIC) $(NASM) -f elf -d OBJ_FORMAT_elf $< -o $@

EXTRA_SOURCES = \
	Assembly.nas \
	Tools.inc
else

EXTRA_SOURCES = \
	common.cpp

EXTRA_DISTS = \
	Assembly.nas \
	Tools.inc

endif


libmacasm_la_SOURCES = $(COMMON_SOURCES) $(EXTRA_SOURCES)

