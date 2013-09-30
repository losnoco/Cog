/*
** Starscream 680x0 emulation library
** Copyright 1997, 1998, 1999 Neill Corlett
**
** Refer to STARDOC.TXT for terms of use, API reference, and directions on
** how to compile.
*/

#ifndef __STARCPU_H__
#define __STARCPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#define STARSCREAM_CALL __fastcall

/* Remember to byte-swap these regions. (read STARDOC.TXT for details) */
struct STARSCREAM_PROGRAMREGION {
  unsigned lowaddr;
  unsigned highaddr;
  void    *offset;
};

struct STARSCREAM_DATAREGION {
  unsigned lowaddr;
  unsigned highaddr;
  void    *memorycall;
  void    *userdata;
};

#define STARSCREAM_IDENTIFIERS(SNC,SN)                                             \
                                                                                   \
int         STARSCREAM_CALL SN##_init             (void);                          \
const char* STARSCREAM_CALL SN##_get_version      (void);                          \
unsigned    STARSCREAM_CALL SN##_get_state_size   (void);                          \
void        STARSCREAM_CALL SN##_clear_state      (void *state);                   \
void        STARSCREAM_CALL SN##_set_memory_maps  (void *state, void **info);      \
unsigned    STARSCREAM_CALL SN##_reset            (void *state);                   \
unsigned    STARSCREAM_CALL SN##_execute          (void *state, int n);            \
unsigned    STARSCREAM_CALL SN##_getreg           (void *state, int n);            \
int         STARSCREAM_CALL SN##_interrupt        (void *state, int veclevel);     \
void        STARSCREAM_CALL SN##_flush_interrupts (void *state);                   \
unsigned    STARSCREAM_CALL SN##_read_odometer    (void *state);                   \
void        STARSCREAM_CALL SN##_break            (void *state);                   \

#define STARSCREAM_REG_DATA    (0)
#define STARSCREAM_REG_ADDRESS (8)
#define STARSCREAM_REG_ASP     (16)
#define STARSCREAM_REG_PC      (17)
#define STARSCREAM_REG_SR      (18)

STARSCREAM_IDENTIFIERS(S68000,s68000)
STARSCREAM_IDENTIFIERS(S68010,s68010)

#ifdef __cplusplus
}
#endif

#endif
