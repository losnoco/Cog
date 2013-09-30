/////////////////////////////////////////////////////////////////////////////
//
// vfs - Virtual filesystem management
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "vfs.h"

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//

sint32 EMU_CALL vfs_init(void) { return 0; }

/////////////////////////////////////////////////////////////////////////////
//
// State information
//
#define VFS_MAXPATH (250)
#define VFS_MAXOPENFILES (32)

struct VFS_STATE {
  psx_readfile_t readfile_cb;
  void *readfile_context;

  sint32 ofs[VFS_MAXOPENFILES];
  sint32 length[VFS_MAXOPENFILES];
  char path[VFS_MAXOPENFILES][VFS_MAXPATH];
};

#define VFSSTATE ((struct VFS_STATE*)(state))

uint32 EMU_CALL vfs_get_state_size(void) {
  return sizeof(struct VFS_STATE);
}

void EMU_CALL vfs_clear_state(void *state) {
  memset(VFSSTATE, 0, sizeof(struct VFS_STATE));
}

void EMU_CALL vfs_set_readfile(void *state, psx_readfile_t readfile, void *context) {
  VFSSTATE->readfile_cb = readfile;
  VFSSTATE->readfile_context = context;
}

/////////////////////////////////////////////////////////////////////////////
//
// check if the given fd is valid
//
static sint32 EMU_CALL isvalidfd(struct VFS_STATE *state, sint32 fd) {
  if(fd < 0 || fd >= VFS_MAXOPENFILES) return 0;
  if(state->path[fd][0] == 0) return 0;
  return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// open
//
sint32 EMU_CALL vfs_open(void *state, const char *path) {
  sint32 l;
  sint32 fd;
  char tempbuf[4];
  if(!(VFSSTATE->readfile_cb)) return -5; // EIO if no callback was set
  if(!path) return -22; // EINVAL if this was NULL for some reason
  if(!path[0]) return -2; // ENOENT if the path is empty
  l = (VFSSTATE->readfile_cb)(
    VFSSTATE->readfile_context,
    path,
    0,
    tempbuf,
    0
  );
  if(l < -1) return -5; // EIO for fatal errors
  if(l == -1) return -2; // ENOENT for not found
  // Otherwise, find a free fd and keep it
  for(fd = 0; fd < VFS_MAXOPENFILES; fd++) {
    if(VFSSTATE->path[fd][0] == 0) break;
  }
  if(fd >= VFS_MAXOPENFILES) return -24; // EMFILE too many open files
  VFSSTATE->ofs[fd] = 0;
  VFSSTATE->length[fd] = l;
  strncpy(VFSSTATE->path[fd], path, VFS_MAXPATH);
  VFSSTATE->path[fd][VFS_MAXPATH-1] = 0;
  return fd;
}

/////////////////////////////////////////////////////////////////////////////
//
// close
//
sint32 EMU_CALL vfs_close(void *state, sint32 fd) {
  if(!(VFSSTATE->readfile_cb)) return -5; // EIO if no callback was set
  if(!isvalidfd(VFSSTATE, fd)) return -9; // EBADF
  VFSSTATE->path[fd][0] = 0;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// read
//
sint32 EMU_CALL vfs_read(void *state, sint32 fd, char *buffer, sint32 length) {
  sint32 r;
  if(!(VFSSTATE->readfile_cb)) return -5; // EIO if no callback was set
  if(!isvalidfd(VFSSTATE, fd)) return -9; // EBADF
  // if length is 0, just return 0
  if(!length) return 0;
  // if we're past end-of-file, return 0
  if(VFSSTATE->ofs[fd] >= VFSSTATE->length[fd]) return 0;
  // make sure we don't read past the end
  { sint32 remain = VFSSTATE->length[fd] - VFSSTATE->ofs[fd];
    if(length > remain) length = remain;
  }
  // attempt actual read
  r = (VFSSTATE->readfile_cb)(
    VFSSTATE->readfile_context,
    VFSSTATE->path[fd],
    VFSSTATE->ofs[fd],
    buffer,
    length
  );
  if(r < -1) return -5; // EIO for fatal errors
  if(r == -1) return -2; // ENOENT for not found - weird, but should work
  VFSSTATE->ofs[fd] += r;
  return r;
}

/////////////////////////////////////////////////////////////////////////////
//
// lseek
//
sint32 EMU_CALL vfs_lseek(void *state, sint32 fd, sint32 offset, sint32 whence) {
  if(!(VFSSTATE->readfile_cb)) return -5; // EIO if no callback was set
  if(!isvalidfd(VFSSTATE, fd)) return -9; // EBADF
  switch(whence) {
  case 0: // SEEK_SET
    break;
  case 1: // SEEK_CUR
    offset += VFSSTATE->ofs[fd];
    break;
  case 2: // SEEK_END
    offset += VFSSTATE->length[fd];
    break;
  default:
    return -22; // EINVAL if whence isn't right
  }
  if(offset < 0) return -22; // EINVAL if offset ends up negative
  VFSSTATE->ofs[fd] = offset;
  return offset;
}

/////////////////////////////////////////////////////////////////////////////
