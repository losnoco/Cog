#ifndef MKHEBIOS_H
#define MKHEBIOS_H

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

void * EMU_CALL mkhebios_create( void * ps2_bios, int *size );

void EMU_CALL mkhebios_delete( void * );

#ifdef __cplusplus
}
#endif

#endif
