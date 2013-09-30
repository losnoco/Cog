/////////////////////////////////////////////////////////////////////////////
//
// vfs - Virtual filesystem management
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_VFS_H__
#define __PSX_VFS_H__

#include "emuconfig.h"

#include "psx.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 EMU_CALL vfs_init(void);

uint32 EMU_CALL vfs_get_state_size(void);
void   EMU_CALL vfs_clear_state(void *state);

void   EMU_CALL vfs_set_readfile(void *state, psx_readfile_t readfile, void *context);

sint32 EMU_CALL vfs_open (void *state, const char *path);
sint32 EMU_CALL vfs_close(void *state, sint32 fd);
sint32 EMU_CALL vfs_read (void *state, sint32 fd, char *buffer, sint32 length);
sint32 EMU_CALL vfs_lseek(void *state, sint32 fd, sint32 offset, sint32 whence);

#ifdef __cplusplus
}
#endif

#endif
