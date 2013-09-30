/////////////////////////////////////////////////////////////////////////////
//
// kabuki - Kabuki decryption
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "kabuki.h"

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint8 bitswap1(uint8 src, uint16 key, uint8 select) {
  if (select & (1 << ((key >> 0) & 7)))
    src = (src & 0xFC) | ((src & 0x01) << 1) | ((src & 0x02) >> 1);
  if (select & (1 << ((key >> 4) & 7)))
    src = (src & 0xF3) | ((src & 0x04) << 1) | ((src & 0x08) >> 1);
  if (select & (1 << ((key >> 8) & 7)))
    src = (src & 0xCF) | ((src & 0x10) << 1) | ((src & 0x20) >> 1);
  if (select & (1 << ((key >>12) & 7)))
    src = (src & 0x3F) | ((src & 0x40) << 1) | ((src & 0x80) >> 1);
  return src;
}

static EMU_INLINE uint8 bitswap2(uint8 src, uint16 key, uint8 select) {
  if (select & (1 << ((key >>12) & 7)))
    src = (src & 0xFC) | ((src & 0x01) << 1) | ((src & 0x02) >> 1);
  if (select & (1 << ((key >> 8) & 7)))
    src = (src & 0xF3) | ((src & 0x04) << 1) | ((src & 0x08) >> 1);
  if (select & (1 << ((key >> 4) & 7)))
    src = (src & 0xCF) | ((src & 0x10) << 1) | ((src & 0x20) >> 1);
  if (select & (1 << ((key >> 0) & 7)))
    src = (src & 0x3F) | ((src & 0x40) << 1) | ((src & 0x80) >> 1);
  return src;
}

static EMU_INLINE uint8 bytedecode(
  uint8 src,
  uint32 swap_key1,
  uint32 swap_key2,
  uint8 xor_key,
  uint16 select
) {
  src = bitswap1(src, swap_key1 & 0xFFFF, select & 0xFF);
  src = ((src & 0x7F) << 1) | ((src & 0x80) >> 7);
  src = bitswap2(src, swap_key1 >> 16   , select & 0xFF);
  src ^= xor_key;
  src = ((src & 0x7F) << 1) | ((src & 0x80) >> 7);
  src = bitswap2(src, swap_key2 & 0xFFFF, select >> 8  );
  src = ((src & 0x7F) << 1) | ((src & 0x80) >> 7);
  src = bitswap1(src, swap_key2 >> 16   , select >> 8  );
  return src;
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL kabuki_decode(
  uint8 *src,
  uint8 *dest_op,
  uint8 *dest_data,
  uint16 len,
  uint32 swap_key1,
  uint32 swap_key2,
  uint16 addr_key,
  uint8 xor_key
) {
  // 32K limit
  if(len > 0x8000) len = 0x8000;
  // if the swap keys are zero, that means no decryption
  if((!swap_key1) && (!swap_key2)) {
    if(len) {
      memcpy(dest_op  , src, len);
      memcpy(dest_data, src, len);
    }
  // otherwise, decrypt
  } else {
    uint16 a, select;
    for(a = 0; a < len; a++) {
      // decode opcodes
      select = a + addr_key;
      dest_op  [a] = bytedecode(src[a], swap_key1, swap_key2, xor_key, select);
      // decode data
      select = (a ^ 0x1FC0) + addr_key + 1;
      dest_data[a] = bytedecode(src[a], swap_key1, swap_key2, xor_key, select);
    }
  }
  if(len < 0x8000) {
    memset(dest_op   + len, 0xFF, 0x8000 - len);
    memset(dest_data + len, 0xFF, 0x8000 - len);
  }
}

/////////////////////////////////////////////////////////////////////////////
