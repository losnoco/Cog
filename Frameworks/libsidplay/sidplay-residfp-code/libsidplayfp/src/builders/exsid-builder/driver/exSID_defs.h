//
//  exSID_defs.h
//	A simple I/O library for exSID USB - private header file
//
//  (C) 2015-2016 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//

/** 
 * @file 
 * libexsid private definitions header file.
 * @note These defines are closely related to the exSID firmware.
 * Any modification that does not correspond to a related change in firmware
 * will cause the device to operate unpredictably or not at all.
 */

#ifndef exSID_defs_h
#define exSID_defs_h

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

// CLOCK_FREQ_NTSC = 1022727.14;
// CLOCK_FREQ_PAL  = 985248.4;

#if 1
#define	XS_BDRATE	2000000		///< 2Mpbs
#define	XS_ADJMLT	1		///< 1-to-1 cycle adjustement (max resolution: 1 cycle).
#else
#define	XS_BDRATE	750000		///< 750kpbs
#define	XS_ADJMLT	2		///< 2-to-1 cycle adjustement (max resolution: 2 cycles).
#endif

#define	XS_BUFFMS	10		///< ~10ms buffer seems to give best results. Penalty if too big (introduces delay in libsidplay) or too small (controller can't keep up)
#define	XS_SIDCLK	1000000		///< 1MHz (for computation only, currently hardcoded in firmware)
#define XS_RSBCLK	(XS_BDRATE/10)	///< RS232 byte clock. Each RS232 byte is 10 bits long due to start and stop bits
#define	XS_CYCCHR	(XS_SIDCLK/XS_RSBCLK)	///< SID cycles between two consecutive chars
//#define	XS_CYCCHR	((XS_SIDCLK+XS_RSBCLK-1)/XS_RSBCLK)	// ceiling
#define XS_USBLAT	1		///< FTDI latency: 1-255ms in 1ms increments
#define	XS_BUFFSZ	((((XS_RSBCLK/1000)*XS_BUFFMS)/62)*62)	///< Must be multiple of _62_ or USB won't be happy.

#define XS_MINDEL	(XS_CYCCHR)	///< Smallest possible delay (with IOCTD1).
#define	XS_CYCIO	(2*XS_CYCCHR)	///< minimum cycles between two consecutive I/Os
#define	XS_MAXADJ	7		///< maximum encodable value for post write clock adjustment: must fit on 3 bits

/* IOCTLS */
#define XS_AD_IOCTD1	0x9D	///< shortest delay (XS_MINDEL SID cycles)
#define	XS_AD_IOCTLD	0x9E	///< polled delay, amount of SID cycles to wait must be given in data

#define	XS_AD_IOCTS0	0xBD	///< select chip 0
#define XS_AD_IOCTS1	0xBE	///< select chip 1
#define XS_AD_IOCTSB	0xBF	///< select both chips. @warning Invalid for reads: unknown behaviour!

#define	XS_AD_IOCTFV	0xFD	///< Firmware version query
#define	XS_AD_IOCTHV	0xFE	///< Hardware version query
#define XS_AD_IOCTRS	0xFF	///< SID reset

#define	XS_USBVID	0x0403	///< Default FTDI VID
#define XS_USBPID	0x6001	///< Default FTDI PID
#define XS_USBDSC	"exSID USB"

#ifdef DEBUG
 #define xsdbg(format, ...)       printf("(%s) " format, __func__, ## __VA_ARGS__)
#else
 #define xsdbg(format, ...)       /* nothing */
#endif

#define xserror(format, ...)      printf("(%s) ERROR " format, __func__, ## __VA_ARGS__)

#ifdef HAVE_BUILTIN_EXPECT
 #define likely(x)       __builtin_expect(!!(x), 1)
 #define unlikely(x)     __builtin_expect(!!(x), 0)
#else
 #define likely(x)      (x)
 #define unlikely(x)    (x)
#endif

#endif /* exSID_defs_h */
