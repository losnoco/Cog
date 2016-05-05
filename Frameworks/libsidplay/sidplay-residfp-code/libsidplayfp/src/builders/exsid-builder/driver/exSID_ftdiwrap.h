//
//  exSID_ftdiwrap.h
//	A FTDI access wrapper for exSID USB - header file
//
//  (C) 2016 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//

/**
 * @file
 * FTDI access wrapper header file.
 */

#ifndef exSID_ftdiwrap_h
#define exSID_ftdiwrap_h

/** Allocate new ftdi handle. */
typedef void * (* xSfw_new_p)(void);

/** Free ftdi handle. */
typedef void (* xSfw_free_p)(void * ftdi);

/**
 * Write data to FTDI.
 * @param ftdi ftdi handle.
 * @param buf write buffer.
 * @param size number of bytes to write.
 * @return number of bytes written or negative error value.
 * @note there are performance constrains on the size of the buffer, see documentation.
 */
typedef int (* xSfw_write_data_p)(void * ftdi, const unsigned char * buf, int size);

/**
 * Read data from FTDI.
 * @param ftdi ftdi handle.
 * @param buf read buffer.
 * @param size number of bytes to read.
 * @return number of bytes read or negative error value.
 * @note there are performance constrains on the size of the buffer, see documentation.
 */
typedef int (* xSfw_read_data_p)(void * ftdi, unsigned char * buf, int size);

/**
 * Open device by description.
 * @param ftdi pointer to ftdi handle.
 * @param vid target vendor id. Ignored by ftd2xx.
 * @param pid target product id. Ignored by ftd2xx.
 * @param desc Description string.
 * @param serial target product serial. Ignored by ftd2xx.
 * @return 0 on success or negative error value.
 * @warning This is the only function to use a pointer to ftdi handle, this is
 *	rendered necessary because of libftd2xx silly way of doing things.
 */
typedef int (* xSfw_usb_open_desc_p)(void ** ftdi, int vid, int pid, const char * desc, const char * serial);

/** Purge FTDI buffers. */
typedef int (* xSfw_usb_purge_buffers_p)(void * ftdi);

/** Close FTDI device. */
typedef int (* xSfw_usb_close_p)(void * ftdi);

/**
 * Get error string.
 * @param ftdi ftdi handle.
 * @return human-readable error string.
 * @note only supported with libftdi.
 */
typedef char * (* xSfw_get_error_string_p)(void * ftdi);

#ifndef	XSFW_WRAPDECL
#define XSFW_EXTERN extern
#else
#define	XSFW_EXTERN /* nothing */
#endif

#define	XSFW_PROTODEF(a)	XSFW_EXTERN a ## _p a

XSFW_PROTODEF(xSfw_new);		///< Handle allocation callback
XSFW_PROTODEF(xSfw_free);		///< Handle deallocation callback
XSFW_PROTODEF(xSfw_write_data);		///< Data write callback
XSFW_PROTODEF(xSfw_read_data);		///< Data read callback
XSFW_PROTODEF(xSfw_usb_open_desc);	///< Device open callback
XSFW_PROTODEF(xSfw_usb_purge_buffers);	///< Device buffers purge callback
XSFW_PROTODEF(xSfw_usb_close);		///< Device close callback
XSFW_PROTODEF(xSfw_get_error_string);	///< Human readable error string callback
int xSfw_usb_setup(void *ftdi, int baudrate, int latency);

int xSfw_dlopen();
void xSfw_dlclose();

#endif /* exSID_ftdiwrap_h */
