#ifndef _READER_PUT_H
#define _READER_PUT_H

#ifdef BUILD_VGMSTREAM
#include "../streamtypes.h"
#else
#include <libvgmstream/streamtypes.h>
#endif

void put_u8(uint8_t* buf, uint8_t v);
void put_u16le(uint8_t* buf, uint16_t v);
void put_u32le(uint8_t* buf, uint32_t v);
void put_u16be(uint8_t* buf, uint16_t v);
void put_u32be(uint8_t* buf, uint32_t v);

/* alias of the above */ //TODO: improve
#define put_s8 put_u8
#define put_s16le put_u16le
#define put_s32le put_u32le
#define put_s16be put_u16be
#define put_s32be put_u32be

#endif
