/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id$
 */

#ifndef _MKBSHIFT_H
#define _MKBSHIFT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(x) sys_errlist[x]
#endif

#define MAGIC			"ajkg"
#define FORMAT_VERSION		2
#define MIN_SUPPORTED_VERSION	1
#define MAX_SUPPORTED_VERSION	3
#define MAX_VERSION		7

#define UNDEFINED_UINT		-1
#define DEFAULT_BLOCK_SIZE	256
#define DEFAULT_V0NMEAN	0
#define DEFAULT_V2NMEAN	4
#define DEFAULT_MAXNLPC	0
#define DEFAULT_NCHAN		1
#define DEFAULT_NSKIP		0
#define DEFAULT_NDISCARD	0
#define NBITPERLONG		32
#define DEFAULT_MINSNR          256
#define DEFAULT_MAXRESNSTR	"32.0"
#define DEFAULT_QUANTERROR	0
#define MINBITRATE		2.5

#define MAX_LPC_ORDER	64
#define CHANSIZE	0
#define ENERGYSIZE	3
#define BITSHIFTSIZE	2
#define NWRAP		3

#define FNSIZE		2
#define FN_DIFF0	0
#define FN_DIFF1	1
#define FN_DIFF2	2
#define FN_DIFF3	3
#define FN_QUIT		4
#define FN_BLOCKSIZE	5
#define FN_BITSHIFT	6
#define FN_QLPC		7
#define FN_ZERO		8
#define FN_VERBATIM     9

#define VERBATIM_CKSIZE_SIZE 5	/* a var_put code size */
#define VERBATIM_BYTE_SIZE 8	/* code size 8 on single bytes means
				 * no compression at all */
#define VERBATIM_CHUNK_MAX 256	/* max. size of a FN_VERBATIM chunk */

#define ULONGSIZE	2
#define NSKIPSIZE	1
#define LPCQSIZE	2
#define LPCQUANT	5
#define XBYTESIZE	7

#define TYPESIZE	4
#define TYPE_AU1	0	/* original lossless ulaw                    */
#define TYPE_S8	        1	/* signed 8 bit characters                   */
#define TYPE_U8         2	/* unsigned 8 bit characters                 */
#define TYPE_S16HL	3	/* signed 16 bit shorts: high-low            */
#define TYPE_U16HL	4	/* unsigned 16 bit shorts: high-low          */
#define TYPE_S16LH	5	/* signed 16 bit shorts: low-high            */
#define TYPE_U16LH	6	/* unsigned 16 bit shorts: low-high          */
#define TYPE_ULAW	7	/* lossy ulaw: internal conversion to linear */
#define TYPE_AU2	8	/* new ulaw with zero mapping                */
#define TYPE_AU3	9	/* lossless alaw                             */
#define TYPE_ALAW 	10	/* lossy alaw: internal conversion to linear */
#define TYPE_RIFF_WAVE  11	/* Microsoft .WAV files                      */
#define TYPE_EOF	12
#define TYPE_GENERIC_ULAW 128
#define TYPE_GENERIC_ALAW 129

#define POSITIVE_ULAW_ZERO 0xff
#define NEGATIVE_ULAW_ZERO 0x7f

#undef  BOOL
#undef  TRUE
#undef  FALSE
#define BOOL  int
#define TRUE  1
#define FALSE 0

#ifndef MAX_PATH
#define MAX_PATH 2048
#endif

#ifndef	MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef	MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#if defined(unix) && !defined(linux)
#define labs abs
#endif

#define ROUNDEDSHIFTDOWN(x, n) (((n) == 0) ? (x) : ((x) >> ((n) - 1)) >> 1)

#ifndef M_LN2
#define	M_LN2	0.69314718055994530942
#endif

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

/* BUFSIZ must be a multiple of four to contain a whole number of words */
#ifdef BUFSIZ
#undef BUFSIZ
#endif
#define BUFSIZ 512

#define putc_exit(val, stream)\
{ char rval;\
  if((rval = putc((val), (stream))) != (char) (val))\
    update_exit(1, "write failed: putc returns EOF\n");\
}

extern int getc_exit_val;
#define getc_exit(stream)\
(((getc_exit_val = getc(stream)) == EOF) ? \
  update_exit(1, "read failed: getc returns EOF\n"), 0: getc_exit_val)

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#else
#  if SIZEOF_UNSIGNED_LONG == 4
#    define uint32_t unsigned long
#    define int32_t long
#  else
#    define uint32_t unsigned int
#    define int32_t int
#  endif
#  define uint16_t unsigned short
#  define uint8_t unsigned char
#  define int16_t short
#  define int8_t char
#endif

#undef	ulong
#undef	ushort
#undef	uchar
#undef	slong
#undef	sshort
#undef	schar
#define ulong	uint32_t
#define ushort	uint16_t
#define uchar	uint8_t
#define slong	int32_t
#define sshort	int16_t
#define schar	int8_t

#if defined(__STDC__) || defined(__GNUC__) || defined(sgi) || !defined(unix)
#define PROTO(ARGS)	ARGS
#else
#define PROTO(ARGS)	()
#endif

#ifdef NEED_OLD_PROTOTYPES
/*******************************************/
/* this should be in string.h or strings.h */
extern int	strcmp	PROTO ((const char*, const char*));
extern char*	strcpy	PROTO ((char*, const char*));
extern char*	strcat	PROTO ((char*, const char*));
extern int	strlen	PROTO ((const char*));

/**************************************/
/* defined in stdlib.h if you have it */
extern void*	malloc	PROTO ((unsigned long));
extern void	free	PROTO ((void*));
extern int	atoi	PROTO ((const char*));
extern void	swab	PROTO ((char*, char*, int));
extern int	fseek	PROTO ((FILE*, long, int));

/***************************/
/* other misc system calls */
extern int	unlink 	PROTO ((const char*));
extern void	exit	PROTO ((int));
#endif

/**************************/
/* defined in Sulawalaw.c */
extern int Sulaw2lineartab[];
#define Sulaw2linear(i) (Sulaw2lineartab[i])
#ifndef Sulaw2linear
extern int	Sulaw2linear PROTO((uchar));
#endif
extern uchar	Slinear2ulaw PROTO((int));

extern int Salaw2lineartab[];
#define Salaw2linear(i) (Salaw2lineartab[i])
#ifndef Salaw2linear
extern int	Salaw2linear PROTO((uchar));
#endif
extern uchar	Slinear2alaw PROTO((int));

/*********************/
/* defined in exit.c */
extern void	basic_exit PROTO ((int));
#ifdef HAVE_STDARG_H
extern void	error_exit PROTO ((char*,...));
extern void	perror_exit PROTO ((char*,...));
extern void	usage_exit PROTO ((int, char*,...));
extern void	update_exit PROTO ((int, char*,...));
#else
extern void	error_exit PROTO (());
extern void	perror_exit PROTO (());
extern void	usage_exit PROTO (());
extern void	update_exit PROTO (());
#endif

/**********************/
/* defined in array.c */
extern void* 	pmalloc PROTO ((ulong));
extern slong**	long2d PROTO ((ulong, ulong));

#endif
