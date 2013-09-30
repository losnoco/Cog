/////////////////////////////////////////////////////////////////////////////
//
// kabuki - Kabuki decryption
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __Q_KABUKI_H__
#define __Q_KABUKI_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void EMU_CALL kabuki_decode(
  uint8 *src,
  uint8 *dest_op,
  uint8 *dest_data,
  uint16 len,
  uint32 swap_key1,
  uint32 swap_key2,
  uint16 addr_key,
  uint8 xor_key
);

#ifdef __cplusplus
}
#endif

#endif
