//
//  exSID.c
//	A simple I/O library for exSID USB
//
//  (C) 2015-2016 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html

/**
 * @file
 * exSID USB I/O library
 * @author Thibaut VARENE
 * @date 2015-2016
 * @version 1.2
 */

#include "exSID.h"
#include "exSID_defs.h"
#include "exSID_ftdiwrap.h"
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef	DEBUG
static long accdrift = 0;
static unsigned long accioops = 0;
static unsigned long accdelay = 0;
static unsigned long acccycle = 0;
#endif

static int ftdi_status;
static void * ftdi = NULL;

/**
 * cycles is uint_fast32_t. Technically, clkdrift should be int_fast64_t though
 * overflow should not happen under normal conditions.
 * negative values mean we're lagging, positive mean we're ahead.
 * See it as a number of SID clocks queued to be spent.
 */
static int_fast32_t clkdrift = 0;

static inline void _exSID_write(uint_least8_t addr, uint8_t data, int flush);

/**
 * Write routine to send data to the device.
 * @note BLOCKING.
 * @param buff pointer to a byte array of data to send
 * @param size number of bytes to send
 */
static inline void _xSwrite(const unsigned char * buff, int size)
{
	ftdi_status = xSfw_write_data(ftdi, buff, size);
#ifdef	DEBUG
	if (unlikely(ftdi_status < 0)) {
		xserror("Error ftdi_write_data(%d): %s\n",
			ftdi_status, xSfw_get_error_string(ftdi));
	}
	if (unlikely(ftdi_status != size)) {
		xserror("ftdi_write_data only wrote %d (of %d) bytes\n",
			ftdi_status, size);
	}
#endif
}

/**
 * Read routine to get data from the device.
 * @note BLOCKING.
 * @param buff pointer to a byte array that will be filled with read data
 * @param size number of bytes to read
 */
static inline void _xSread(unsigned char * buff, int size)
{
	ftdi_status = xSfw_read_data(ftdi, buff, size);
#ifdef	DEBUG
	if (unlikely(ftdi_status < 0)) {
	        xserror("Error ftdi_read_data(%d): %s\n",
			ftdi_status, xSfw_get_error_string(ftdi));
	}
	if (unlikely(ftdi_status != size)) {
		xserror("ftdi_read_data only read %d (of %d) bytes\n",
			ftdi_status, size);
	}
#endif
}

/**
 * Single byte output routine.
 * Fills a static buffer with bytes to send to the device until the buffer is
 * full or a forced write is triggered. Compensates for drift if XS_BDRATE isn't
 * a multiple of of XS_SIDCLK.
 * @note No drift compensation is performed on read operations.
 * @param byte byte to send
 * @param flush force write flush if non-zero
 */
static void _xSoutb(uint8_t byte, int flush)
{
	static unsigned char bufchar[XS_BUFFSZ];
	static int i = 0;

	bufchar[i++] = (unsigned char)byte;

	/* if XS_BDRATE isn't a multiple of XS_SIDCLK we will drift:
	   every XS_BDRATE/(remainder of XS_SIDCLK/XS_BDRATE) we lose one SID cycle.
	   Compensate here */
#if (XS_SIDCLK % XS_RSBCLK)
	if (!(i % (XS_RSBCLK/(XS_SIDCLK%XS_RSBCLK))))
		clkdrift--;
#endif
	if (likely((i < XS_BUFFSZ) && !flush))
		return;

	_xSwrite(bufchar, i);
	i = 0;
}

/**
 * Device init routine.
 * Must be called once before any operation is attempted on the device.
 * Opens the named device, and sets various parameters: baudrate, parity, flow
 * control and USB latency, and finally clears the RX and TX buffers.
 * @return 0 on success, !0 otherwise.
 */
int exSID_init(void)
{
	unsigned char dummy;

	if (ftdi) {
		xserror("Device already open!\n");
		return -1;
	}

	if (xSfw_dlopen()) {
		xserror("dl error\n");
		return -1;
	}

	if (xSfw_new) {
		ftdi = xSfw_new();
		if (!ftdi) {
			xserror("ftdi_new failed\n");
			return -1;
		}
	}

	ftdi_status = xSfw_usb_open_desc(&ftdi, XS_USBVID, XS_USBPID, XS_USBDSC, NULL);
	if (ftdi_status < 0) {
		xserror("Failed to open device: %d (%s)\n",
			ftdi_status, xSfw_get_error_string(ftdi));
		if (xSfw_free)
			xSfw_free(ftdi);
		ftdi = NULL;
		return -1;
	}
	ftdi_status = xSfw_usb_setup(ftdi, XS_BDRATE, XS_USBLAT);
	if (ftdi_status < 0) {
		xserror("Failed to setup device\n");
		return -1;
	}

	// success - device with device description "exSIDUSB" is open
	xsdbg("Device opened\n");
	
	xSfw_usb_purge_buffers(ftdi); // Purge both Rx and Tx buffers

	// Wait for device ready by trying to read FV and wait for the answer
	// XXX Broken with libftdi due to non-blocking read :-/
	_xSoutb(XS_AD_IOCTFV, 1);
	_xSread(&dummy, 1);

	xsdbg("Rock'n'roll!\n");

#ifdef	DEBUG
	exSID_hwversion();
	xsdbg("XS_BDRATE: %dkpbs, XS_BUFFSZ: %d bytes\n", XS_BDRATE/1000, XS_BUFFSZ);
	xsdbg("XS_CYCCHR: %d, XS_CYCIO: %d, compensation every %d cycle(s)\n",
		XS_CYCCHR, XS_CYCIO, (XS_SIDCLK % XS_RSBCLK) ? (XS_RSBCLK/(XS_SIDCLK%XS_RSBCLK)) : 0);
#endif

	return 0;
}

/**
 * Device exit routine.
 * Must be called to release the device.
 * Resets the SIDs and clears RX/TX buffers, releases all resources allocated
 * in exSID_init().
 */
void exSID_exit(void)
{
	if (ftdi) {
		exSID_reset(0);

		xSfw_usb_purge_buffers(ftdi); // Purge both Rx and Tx buffers

		ftdi_status = xSfw_usb_close(ftdi);
		if (ftdi_status < 0)
			xserror("unable to close ftdi device: %d (%s)\n",
				ftdi_status, xSfw_get_error_string(ftdi));

		if (xSfw_free)
			xSfw_free(ftdi);
		ftdi = NULL;

#ifdef	DEBUG
		xsdbg("mean jitter: %.1f cycle(s) over %lu I/O ops\n",
			((float)accdrift/accioops), accioops);
		xsdbg("bandwidth used for I/O ops: %lu%% (approx)\n",
			100-(accdelay*100/acccycle));
		accdrift = accioops = accdelay = acccycle = 0;
#endif
	}

	clkdrift = 0;
	xSfw_dlclose();
}


/**
 * SID reset routine.
 * Performs a hardware reset on the SIDs.
 * @note since the reset procedure in firmware will stall the device for more than
 * XS_CYCCHR, reset forcefully waits for enough time before resuming execution
 * via a call to usleep();
 * @param volume volume to set the SIDs to after reset.
 */
void exSID_reset(uint_least8_t volume)
{
	xsdbg("rvol: %" PRIxLEAST8 "\n", volume);

	_xSoutb(XS_AD_IOCTRS, 0);	// this will take more than XS_CYCCHR
	_exSID_write(0x18, volume, 1);	// this only needs 2 bytes which matches the input buffer of the PIC so all is well

	clkdrift = 0;
	usleep(1000);	// sleep for 1ms
}

/**
 * SID chipselect routine.
 * Selects which SID will play the tunes. If neither CHIP0 or CHIP1 is chosen,
 * both SIDs will operate together. Accounts for elapsed cycles.
 * @param chip SID selector value, see exSID.h.
 */
void exSID_chipselect(int chip)
{
	clkdrift -= XS_CYCCHR;

	if (XS_CS_CHIP0 == chip)
		_xSoutb(XS_AD_IOCTS0, 0);
	else if (XS_CS_CHIP1 == chip)
		_xSoutb(XS_AD_IOCTS1, 0);
	else
		_xSoutb(XS_AD_IOCTSB, 0);
}

/**
 * Hardware and firmware version of the device.
 * Queries the device for the hardware revision and current firmware version
 * and returns both in the form of a 16bit integer: MSB is an ASCII
 * character representing the hardware revision (e.g. 0x42 = "B"), and LSB
 * is a number representing the firmware version in decimal integer.
 * Does NOT account for elapsed cycles.
 * @return version information as described above.
 */
uint16_t exSID_hwversion(void)
{
	unsigned char inbuf[2];
	uint16_t out = 0;

	_xSoutb(XS_AD_IOCTHV, 0);
	_xSoutb(XS_AD_IOCTFV, 1);
	_xSread(inbuf, 2);
	out = inbuf[0] << 8 | inbuf[1];	// ensure proper order regardless of endianness

	xsdbg("HV: %c, FV: %hhu\n", inbuf[0], inbuf[1]);

	return out;
}

/**
 * Poll-based blocking (long) delay.
 * Calls to IOCTLD polled delay, for "very" long delays (thousands of SID clocks).
 * Total delay should be 3*CYCCHR + WAITCNT(500 + 1) (see PIC firmware), and for
 * better performance, ideally the requested delay time should be close to a multiple
 * of XS_USBLAT milliseconds.
 * @warning NOT CYCLE ACCURATE
 * @param cycles how many SID clocks to wait for.
 */
void exSID_polldelay(uint_fast32_t cycles)
{
#define	SBPDOFFSET	(3*XS_CYCCHR)
#define	SBPDMULT	501
	int delta;
	int multiple;	// minimum 1 full loop
	unsigned char dummy;

	multiple = cycles - SBPDOFFSET;
	delta = multiple % SBPDMULT;
	multiple /= SBPDMULT;

	//xsdbg("ldelay: %d, multiple: %d, delta: %d\n", cycles, multiple, delta);

#ifdef	DEBUG
	if (unlikely((multiple <=0) || (multiple > 255)))
		xserror("Wrong delay!\n");
#endif

	// send delay command and flush queue
	_exSID_write(XS_AD_IOCTLD, (unsigned char)multiple, 1);

	// wait for answer with blocking read
	_xSread(&dummy, 1);

	// deal with remainder
	exSID_delay(delta);

#ifdef	DEBUG
	acccycle += (cycles - delta);
	accdelay += (cycles - delta);
#endif
}

/**
 * Private delay loop.
 * @note will block every time a device write is triggered, blocking time will be
 * equal to the number of bytes written times XS_MINDEL.
 * @param cycles how many SID clocks to loop for.
 */
static inline void _xSdelay(uint_fast32_t cycles)
{
#ifdef	DEBUG
	accdelay += cycles;
#endif
	while (likely(cycles >= XS_MINDEL)) {
		_xSoutb(XS_AD_IOCTD1, 0);
		cycles -= XS_MINDEL;
		clkdrift -= XS_MINDEL;
	}
#ifdef	DEBUG
	accdelay -= cycles;
#endif
}

/**
 * Write-based delay.
 * Calls _xSdelay() while leaving enough lead time for an I/O operation.
 * @param cycles how many SID clocks to loop for.
 */
void exSID_delay(uint_fast32_t cycles)
{
	clkdrift += cycles;

#ifdef	DEBUG
	acccycle += cycles;
#endif

	if (unlikely(clkdrift <= XS_CYCIO))	// never delay for less than a full write would need
		return;	// too short

	_xSdelay(clkdrift - XS_CYCIO);
}

/**
 * Private write routine for a tuple address + data.
 * @param addr target address to write to.
 * @param data data to write at that address.
 * @param flush if non-zero, force immediate flush to device.
 */
static inline void _exSID_write(uint_least8_t addr, uint8_t data, int flush)
{
	_xSoutb((unsigned char)addr, 0);
	_xSoutb((unsigned char)data, flush);
}

/**
 * Timed write routine, attempts cycle-accurate writes.
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than XS_CYCIO apart and the leftover delay is <= XS_MAXADJ*XS_ADJMLT SID clock cycles.
 * @param cycles how many SID clocks to wait before the actual data write.
 * @param addr target address.
 * @param data data to write at that address.
 */
void exSID_clkdwrite(uint_fast32_t cycles, uint_least8_t addr, uint8_t data)
{
	static int adj = 0;

#ifdef	DEBUG
	if (unlikely(addr > 0x18)) {
		xserror("Invalid write: %.2" PRIxLEAST8 "\n", addr);
		exSID_delay(cycles);
		return;
	}
#endif

	// actual write will cost XS_CYCIO. Delay for cycles - XS_CYCIO then account for the write
	clkdrift += cycles;
	if (clkdrift > XS_CYCIO)
		_xSdelay(clkdrift - XS_CYCIO);

	clkdrift -= XS_CYCIO;	// write is going to consume XS_CYCIO clock ticks

#ifdef	DEBUG
	if (clkdrift >= XS_CYCCHR)
		xserror("Impossible drift adjustment! %" PRIdFAST32 " cycles\n", clkdrift);
	else if (clkdrift < 0)
		accdrift += clkdrift;
#endif

	/* if we are still going to be early, delay actual write by up to XS_MAXAD*XS_ADJMLT ticks
	At this point it is guaranted that clkdrift will be < XS_MINDEL (== XS_CYCCHR). */
	if (likely(clkdrift >= 0)) {
		adj = clkdrift % (XS_MAXADJ*XS_ADJMLT+1);
		/* if XS_MAXADJ*XS_ADJMLT is >= clkdrift, modulo will give the same results
		   as the correct test:
		   adj = (clkdrift < XS_MAXADJ*XS_ADJMLT ? clkdrift : XS_MAXADJ*XS_ADJMLT)
		   but without an extra conditional branch. If is is < clkdrift, then it
		   seems to provide better results by evening jitter accross writes. So
		   it's the preferred solution for all cases. */
		adj /= XS_ADJMLT;
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
#ifdef	DEBUG
		accdrift += (clkdrift - adj*XS_ADJMLT);
#endif
		//xsdbg("drft: %d, adj: %d, addr: %.2hhx, data: %.2hhx\n", clkdrift, adj*XS_ADJMLT, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	acccycle += cycles;
	accioops++;
#endif

	//xsdbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
	_exSID_write(addr, data, 0);
}

/**
 * Private read routine for a given address.
 * @param addr target address to read from.
 * @param flush if non-zero, force immediate flush to device.
 * @return data read from address.
 */
static inline uint8_t _exSID_read(uint_least8_t addr, int flush)
{
	static unsigned char data;

	_xSoutb(addr, flush);	// XXX
	_xSread(&data, flush);	// blocking

	xsdbg("addr: %.2" PRIxLEAST8 ", data: %.2hhx\n", addr, data);
	return data;
}

/**
 * BLOCKING Timed read routine, attempts cycle-accurate reads.
 * This function will be cycle-accurate provided that no two consecutive reads or writes
 * are less than XS_CYCIO apart and leftover delay is <= XS_MAXADJ*XS_ADJMLT SID clock cycles.
 * Read result will only be available after a full XS_CYCIO, giving clkdread() the same
 * run time as clkdwrite(). There's a 2-cycle negative adjustment in the code because
 * that's the actual offset from the write calls ('/' denotes falling clock edge latch),
 * which the following ASCII tries to illustrate: <br />
 * Write looks like this in firmware:
 * > ...|_/_|...
 * ...end of data byte read | cycle during which write is enacted / next cycle | etc... <br />
 * Read looks like this in firmware:
 * > ...|_|_|_/_|_|...
 * ...end of address byte read | 2 cycles for address processing | cycle during which SID is read /
 *	then half a cycle later the CYCCHR-long data TX starts, cycle completes | another cycle | etc... <br />
 * This explains why reads happen a relative 2-cycle later than then should with
 * respect to writes.
 * @note The actual time the read will take to complete depends
 * on the USB bus activity and settings. It *should* complete in XS_USBLAT ms, but
 * not less, meaning that read operations are bound to introduce timing inaccuracy.
 * As such, this function is only really provided as a proof of concept but should
 * better be avoided.
 * @param cycles how many SID clocks to wait before the actual data read.
 * @param addr target address.
 * @return data read from address.
 */
uint8_t exSID_clkdread(uint_fast32_t cycles, uint_least8_t addr)
{
	static int adj = 0;

#ifdef	DEBUG
	if (unlikely((addr < 0x19) || (addr > 0x1C))) {
		xserror("Invalid read: %.2" PRIxLEAST8 "\n", addr);
		exSID_delay(cycles);
		return 0xFF;
	}
#endif

	// actual read will happen after XS_CYCCHR. Delay for cycles - XS_CYCCHR then account for the read
	clkdrift += -2;		// 2-cycle offset adjustement, see function documentation.
	clkdrift += cycles;
	if (clkdrift > XS_CYCCHR)
		_xSdelay(clkdrift - XS_CYCCHR);

	clkdrift -= XS_CYCCHR;	// read request is going to consume XS_CYCCHR clock ticks

#ifdef	DEBUG
	if (clkdrift > XS_CYCCHR)
		xserror("Impossible drift adjustment! %" PRIdFAST32 " cycles\n", clkdrift);
	else if (clkdrift < 0) {
		accdrift += clkdrift;
		xsdbg("Late read request! %" PRIdFAST32 " cycles\n", clkdrift);
	}
#endif

	// if we are still going to be early, delay actual read by up to XS_MAXADJ*XS_ADJMLT ticks
	if (likely(clkdrift >= 0)) {
		adj = clkdrift % (XS_MAXADJ*XS_ADJMLT+1);	// see clkdwrite()
		adj /= XS_ADJMLT;
		addr = (unsigned char)(addr | (adj << 5));	// final delay encoded in top 3 bits of address
#ifdef	DEBUG
		accdrift += (clkdrift - adj*XS_ADJMLT);
#endif
		//xsdbg("drft: %d, adj: %d, addr: %.2hhx, data: %.2hhx\n", clkdrift, adj*XS_ADJMLT, (char)(addr | (adj << 5)), data);
	}

#ifdef	DEBUG
	acccycle += cycles;
	accioops++;
#endif

	// after read has completed, at least another XS_CYCCHR will have been spent
	clkdrift -= XS_CYCCHR;

	//xsdbg("delay: %d, clkdrift: %d\n", cycles, clkdrift);
	return _exSID_read(addr, 1);
}
