//
//  exSID.h
//	A simple I/O library for exSID USB - interface header file
//
//  (C) 2015-2016 Thibaut VARENE
//  License: GPLv2 - http://www.gnu.org/licenses/gpl-2.0.html
//

/**
 * @file
 * libexsid interface header file.
 */

#ifndef exSID_h
#define exSID_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define	XS_VERSION	"1.2pre"

/* Chip selection values for exSID_chipselect() */
enum {
	XS_CS_CHIP0,	///< 6581
	XS_CS_CHIP1,	///< 8580
	XS_CS_BOTH,	///< Both chips. @warning Invalid for reads: unknown behaviour!
};

// public interface
int exSID_init(void);
void exSID_exit(void);
void exSID_reset(uint_least8_t volume);
uint16_t exSID_hwversion(void);
void exSID_chipselect(int chip);
void exSID_delay(uint_fast32_t cycles);
void exSID_polldelay(uint_fast32_t cycles);
void exSID_clkdwrite(uint_fast32_t cycles, uint_least8_t addr, uint8_t data);
uint8_t exSID_clkdread(uint_fast32_t cycles, uint_least8_t addr);

#define exSID_write(addr, data)	exSID_clkdwrite(0, addr, data)
#define exSID_read(addr)	exSID_clkdread(0, addr)

#ifdef __cplusplus
}
#endif
#endif /* exSID_h */
