#include "psf2fs.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <zlib.h>

/////////////////////////////////////////////////////////////////////////////

#define MYMAXPATH (1024)

struct SOURCE_FILE {
  uint8_t * reserved_data;
  int reserved_size;
  struct SOURCE_FILE *next;
};

struct DIR_ENTRY {
  char name[37];
  struct DIR_ENTRY *subdir;
  int length;
  int block_size;
  struct SOURCE_FILE *source;
  int *offset_table;
  struct DIR_ENTRY *next;
};

struct CACHEBLOCK {
  struct SOURCE_FILE *from_source;
  int   from_offset;
  char *uncompressed_data;
  int   uncompressed_size;
};

struct PSF2FS {
  struct SOURCE_FILE *sources;
  struct DIR_ENTRY *dir;
  struct CACHEBLOCK cacheblock;

  int adderror;
};

/////////////////////////////////////////////////////////////////////////////

static void source_cleanup_free(struct SOURCE_FILE *source) {
  while(source) {
    struct SOURCE_FILE *next = source->next;
    if(source->reserved_data) free( source->reserved_data );
    free( source );
    source = next;
  }
}

static void dir_cleanup_free(struct DIR_ENTRY *dir) {
  while(dir) {
    struct DIR_ENTRY *next = dir->next;
    if(dir->subdir) dir_cleanup_free(dir->subdir);
    free( dir );
    dir = next;
  }
}

static void cache_cleanup(struct CACHEBLOCK *cacheblock) {
  if(!cacheblock) return;
  if(cacheblock->uncompressed_data) free( cacheblock->uncompressed_data );
}

/////////////////////////////////////////////////////////////////////////////

void *psf2fs_create(void) {
  struct PSF2FS *fs;
  fs = ( struct PSF2FS * ) malloc( sizeof( struct PSF2FS ) );
  if(!fs) return NULL;
  memset(fs, 0, sizeof(struct PSF2FS));
  return fs;
}

/////////////////////////////////////////////////////////////////////////////

void psf2fs_delete(void *psf2fs) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  if(fs->sources) source_cleanup_free(fs->sources);
  if(fs->dir) dir_cleanup_free(fs->dir);
  cache_cleanup(&(fs->cacheblock));
  free( fs );
}

/////////////////////////////////////////////////////////////////////////////

static int isdirsep(char c) { return (c == '/' || c == '\\' || c == '|' || c == ':'); }

/////////////////////////////////////////////////////////////////////////////

static unsigned read32lsb(const uint8_t * foo) {
  return (
    ((foo[0] & 0xFF) <<  0) |
    ((foo[1] & 0xFF) <<  8) |
    ((foo[2] & 0xFF) << 16) |
    ((foo[3] & 0xFF) << 24)
  );
}

/////////////////////////////////////////////////////////////////////////////

static int __memicmp(const char * a, const char * b, int length)
{
  int o, p;
  for (o = 0; o < length; o++) {
    p = tolower(a[o]) - tolower(b[o]);
    if (p) return p;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

static struct DIR_ENTRY *finddirentry(
  struct DIR_ENTRY *dir,
  const char *name,
  int name_l
) {
  if(name_l > 36) return NULL;
  while(dir) {
    if(!__memicmp(dir->name, name, name_l) && dir->name[name_l] == 0) return dir;
    dir = dir->next;
  }
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Make a DIR_ENTRY list for a given file and Reserved offset.
// Recurses subdirectories also.
// All entries are set to point to the given SOURCE_FILE.
//
static struct DIR_ENTRY *makearchivedir(
  struct PSF2FS *fs,
  int offset,
  struct SOURCE_FILE *source
) {
  struct DIR_ENTRY *dir = NULL;
  const uint8_t *file = source->reserved_data;
  int n, num;
  if(offset < 0) goto corrupt;
  if(offset >= source->reserved_size) { goto corrupt; }
  if((offset + 4) > source->reserved_size) { goto corrupt; }
  num = read32lsb(file + offset);
  offset += 4;
  if(num < 0) goto corrupt;
  for(n = 0; n < num; n++) {
    int o, u, b;
    if((offset + 48) > source->reserved_size) { goto corrupt; }
    { struct DIR_ENTRY *entry = ( struct DIR_ENTRY * ) malloc( sizeof( struct DIR_ENTRY ) );
      if(!entry) goto outofmemory;
      memset(entry, 0, sizeof(struct DIR_ENTRY));
      entry->next = dir;
      dir = entry;
    }
    memcpy(dir->name, file + offset, 36);
    o = read32lsb(file + offset + 36);
    u = read32lsb(file + offset + 40);
    b = read32lsb(file + offset + 44);
    offset += 48;
    if(o < 0) goto corrupt;
    if(u < 0) goto corrupt;
    if(b < 0) goto corrupt;
    if(o && o < offset) {
//      char s[100];
//      sprintf(s,"q[o=%08X offset=%08X]",o,offset);
//      errormessageadd(fs, s);
      goto corrupt;
    }
    // if this new entry describes a subdirectory:
    if(u == 0 && b == 0 && o != 0) {
      dir->subdir = makearchivedir(fs, o, source);
      if(fs->adderror) goto error;
//      if(!dir->subdir) goto error;
    // if this new entry describes a zero-length file:
    } else if(u == 0 || b == 0 || o == 0) {
      // fields were zero anyway
    // if this new entry describes a real source file:
    } else {
      int i;
      int blocks = (u + (b-1)) / b;
      int dataofs = o + 4 * blocks;
      if(dataofs >= source->reserved_size) { goto corrupt; }
      // record the info
      dir->length = u;
      dir->block_size = b;
      dir->source = source;
      dir->offset_table = (int *) malloc( ( blocks + 1 ) * sizeof( int ) );
      if(!dir->offset_table) goto outofmemory;
      for(i = 0; i < blocks; i++) {
        int cbs;
        if((o + 4) > source->reserved_size) { goto corrupt; }
        cbs = read32lsb(file + o);
        o += 4;
        dir->offset_table[i] = dataofs;
        dataofs += cbs;
      }
      dir->offset_table[i] = dataofs;
    }
  }
//success:
  return dir;

corrupt:
  goto error;
outofmemory:
  goto error;
error:
  dir_cleanup_free(dir);
  fs->adderror = 1;
  return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
// Merge two SOURCE_FILE lists.
// Guaranteed to succeed and not to free anything.
//
static struct SOURCE_FILE *mergesource(
  struct SOURCE_FILE *to,
  struct SOURCE_FILE *from
) {
  struct SOURCE_FILE *to_tail;
  if(!to && !from) return NULL;
  if(!to) {
    struct SOURCE_FILE *t;
    t = to; to = from; from = t;
  }
  to_tail = to;
  while(to_tail->next) { to_tail = to_tail->next; }
  to_tail->next = from;
  return to;
}

/////////////////////////////////////////////////////////////////////////////
//
// Merge two DIR_ENTRY lists.
// Guaranteed to succeed. May free some structures.
//
static struct DIR_ENTRY *mergedir(
  struct DIR_ENTRY *to,
  struct DIR_ENTRY *from
) {
  // will traverse "from", and add to "to".
  while(from) {
    struct DIR_ENTRY *entry_to;
    struct DIR_ENTRY *entry_from;
    entry_from = from;
    from = from->next;
    // delink entry_from
    entry_from->next = NULL;
    // look for a duplicate entry in "to"
    entry_to = finddirentry(to, entry_from->name, (int)strlen(entry_from->name));
    // if there is one, do something fancy and then free entry_from.
    if(entry_to) {
      // if both are subdirs, merge the subdirs
      if((entry_to->subdir) && (entry_from->subdir)) {
        entry_to->subdir = mergedir(entry_to->subdir, entry_from->subdir);
        entry_from->subdir = NULL;
      // if both are files, copy over the info
      } else if((!(entry_to->subdir)) && (!(entry_from->subdir))) {
        entry_to->length = entry_from->length;
        entry_to->block_size = entry_from->block_size;
        entry_to->source = entry_from->source;
        if(entry_to->offset_table) free( entry_to->offset_table );
        entry_to->offset_table = entry_from->offset_table;
        entry_from->offset_table = NULL;
      // if one's a subdir but the other's not, we lose "from". this is fine.
      }
      dir_cleanup_free(entry_from);
      entry_from = NULL;
    // otherwise, just relink to the top of "to"
    } else {
      entry_from->next = to;
      to = entry_from;
    }
  }
  return to;
}

/////////////////////////////////////////////////////////////////////////////
//
// only modifies *psource and *pdir on success
//
static int addarchive(
  struct PSF2FS *fs,
  const uint8_t *reserved_data,
  int reserved_size,
  struct SOURCE_FILE **psource,
  struct DIR_ENTRY **pdir
) {
  struct SOURCE_FILE *source = *psource;
  struct DIR_ENTRY *dir = *pdir;
  // these relate to the current file
  struct SOURCE_FILE *this_source = NULL;
  struct DIR_ENTRY *this_dir = NULL;

  // default to no error
  fs->adderror = 0;

  // create a source entry for this psf2
  this_source = ( struct SOURCE_FILE * ) malloc( sizeof( struct SOURCE_FILE ) );
  if(!this_source) goto outofmemory;
  this_source->reserved_data = ( uint8_t * ) malloc( reserved_size );
  if(!this_source->reserved_data) goto outofmemory;
  memcpy(this_source->reserved_data, reserved_data, reserved_size);
  this_source->reserved_size = reserved_size;
  this_source->next = NULL;
  this_dir = makearchivedir(fs, 0, this_source);
  if(fs->adderror) goto error;

  // success
  // now merge everything
  *psource = mergesource(source, this_source);
  *pdir = mergedir(dir, this_dir);
//success:
  return 0;

outofmemory:
  goto error;
error:
  if(dir) dir_cleanup_free(dir);
  if(source) source_cleanup_free(source);
  if(this_dir) dir_cleanup_free(this_dir);
  if(this_source) source_cleanup_free(this_source);
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
//
//
//
int psf2fs_load_callback(void * psf2fs, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  (void)exe;
  (void)exe_size;
  return addarchive(fs, reserved, (int)reserved_size, &(fs->sources), &(fs->dir));
}

/////////////////////////////////////////////////////////////////////////////
//
//
//
static int virtual_read(struct PSF2FS *fs, struct DIR_ENTRY *entry, int offset, char *buffer, int length) {
  int length_read = 0;
  int r;
  unsigned long destlen;
  if(offset >= entry->length) return 0;
  if((offset + length) > entry->length) length = entry->length - offset;
  while(length_read < length) {
    // get info on the current block
    int blocknum = offset / entry->block_size;
    int ofs_within_block = offset % entry->block_size;
    int canread;
    int block_zofs  = entry->offset_table[blocknum];
    int block_zsize = entry->offset_table[blocknum+1] - block_zofs;
    int block_usize;
    if(block_zofs <= 0 || block_zofs >= entry->source->reserved_size) goto bounds;
    if((block_zofs+block_zsize) > entry->source->reserved_size) goto bounds;

    // get the actual uncompressed size of this block
    block_usize = entry->length - (blocknum * entry->block_size);
    if(block_usize > entry->block_size) block_usize = entry->block_size;

    // if it's not already in the cache block, read it
    if(
      (fs->cacheblock.from_offset != block_zofs) ||
      (fs->cacheblock.from_source != entry->source)
    ) {
      // invalidate cache without freeing buffer
      fs->cacheblock.from_source = NULL;

      // make sure there's a buffer allocated
      // but only reallocate if the size is different
      if(fs->cacheblock.uncompressed_size != block_usize) {
        fs->cacheblock.uncompressed_size = 0;
        if(fs->cacheblock.uncompressed_data) {
          free( fs->cacheblock.uncompressed_data );
          fs->cacheblock.uncompressed_data = NULL;
        }
        fs->cacheblock.uncompressed_data = ( char * ) malloc( block_usize );
        if(!fs->cacheblock.uncompressed_data) goto outofmemory;
        fs->cacheblock.uncompressed_size = block_usize;
      }
      destlen = block_usize;
      // attempt decompress
      r = uncompress((unsigned char *) fs->cacheblock.uncompressed_data, &destlen, (const unsigned char *) entry->source->reserved_data + block_zofs, block_zsize);
      if(r != Z_OK || destlen != block_usize) {
//        char s[999];
//        sprintf(s,"zdata=%02X %02X %02X blockz=%d blocku=%d destlenout=%d", zdata[0], zdata[1], zdata[2], block_zsize, block_usize, destlen);
//        errormessageadd(fs, s);
        goto error;
      }
    }

    // at this point, we can read whatever we want out of the cacheblock
    canread = fs->cacheblock.uncompressed_size - ofs_within_block;
    if(canread > (length - length_read)) canread = length - length_read;

    // copy
    memcpy(buffer, fs->cacheblock.uncompressed_data + ofs_within_block, canread);

    // advance pointers/counters
    offset += canread;
    length_read += canread;
    buffer += canread;
  }

//success:
  return length_read;

bounds:
  goto error;
outofmemory:
  goto error;
error:
  // if cacheblock was invalidated, we can free it
  if(!fs->cacheblock.from_source) {
    fs->cacheblock.uncompressed_size = 0;
    if(fs->cacheblock.uncompressed_data) {
      free( fs->cacheblock.uncompressed_data );
      fs->cacheblock.uncompressed_data = NULL;
    }
  }
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
//
//
//
int psf2fs_virtual_readfile(void *psf2fs, const char *path, int offset, char *buffer, int length) {
  struct PSF2FS *fs = (struct PSF2FS*)psf2fs;
  struct DIR_ENTRY *entry = fs->dir;


  if(!path) goto invalidarg;
  if(offset < 0) goto invalidarg;
  if(!buffer) goto invalidarg;
  if(length < 0) goto invalidarg;

  for(;;) {
    int l;
    int need_dir;
    if(!entry) goto pathnotfound;
    while(isdirsep(*path)) path++;
    for(l = 0;; l++) {
      if(!path[l]) { need_dir = 0; break; }
      if(isdirsep(path[l])) { need_dir = 1; break; }
    }
    entry = finddirentry(entry, path, l);
    if(!entry) goto pathnotfound;
    if(!need_dir) break;
    entry = entry->subdir;
    path += l;
  }

  // if we "found" a file but it's a directory, then we didn't find it
  if(entry->subdir) goto pathnotfound;

  // special case: if requested length is 0, return the total file length
  if(!length) return entry->length;

  // otherwise, read from source
  return virtual_read(fs, entry, offset, buffer, length);

pathnotfound:
  goto error;
invalidarg:
  goto error;
error:
  return -1;
}

/////////////////////////////////////////////////////////////////////////////
