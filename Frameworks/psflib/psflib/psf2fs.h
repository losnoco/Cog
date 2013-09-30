#ifndef PSF2FS_H
#define PSF2FS_H

#include "psflib.h"

#ifdef __cplusplus
extern "C" {
#endif

void * psf2fs_create();

void psf2fs_delete( void * );

int psf2fs_load_callback(void * psf2vfs, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size);

int psf2fs_virtual_readfile(void *psf2vfs, const char *path, int offset, char *buffer, int length);

#ifdef __cplusplus
}
#endif

#endif
