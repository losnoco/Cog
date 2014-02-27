#ifndef PSFLIB_H
#define PSFLIB_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psf_file_callbacks
{
    /* list of characters which act as path separators, null terminated */
    const char * path_separators;

    /* accepts UTF-8 encoding, returns file handle */
    void * (* fopen )(const char *);

    /* reads to specified buffer, returns count of size bytes read */
    size_t (* fread )(void *, size_t size, size_t count, void * handle);

    /* returns zero on success, -1 on error */
    int    (* fseek )(void * handle, int64_t, int);

    /* returns zero on success, -1 on error */
    int    (* fclose)(void * handle);

    /* returns current file offset */
    long   (* ftell )(void * handle);
} psf_file_callbacks;

/* Receives exe and reserved bodies, with deepest _lib->_lib->_lib etc head first, followed
 * by the specified file itself, then followed by numbered library chains.
 *
 * Example:
 *
 * outermost file, a.psf, has _lib=b.psf and _lib2=c.psf tags; b.psf has _lib=d.psf:
 *
 * the callback will be passed the contents of d.psf, then b.psf, then a.psf, then c.psf
 *
 * Returning non-zero indicates an error.
 */
typedef int (* psf_load_callback)(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size);

/* Receives the name/value pairs from the outermost file, one at a time.
 *
 * Returning non-zero indicates an error.
 */
typedef int (* psf_info_callback)(void * context, const char * name, const char * value);

/* Loads the PSF chain starting with uri, opened using file_callbacks, passes the tags,
 * if any, to the optional info_target callback, then passes all loaded data to load_target
 * with the highest priority file first.
 *
 * allowed_version may be set to zero to probe the file version, but is not recommended when
 * actually loading files into an emulator state.
 *
 * Both load_target and info_target are optional.
 *
 * Returns negative on error, PSF version on success.
 */
int psf_load( const char * uri, const psf_file_callbacks * file_callbacks, uint8_t allowed_version,
              psf_load_callback load_target, void * load_context, psf_info_callback info_target, void * info_context, int info_want_nested_tags );

#ifdef __cplusplus
}
#endif

#endif // PSFLIB_H
