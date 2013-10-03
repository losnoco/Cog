/////////////////////////////////////////////////////////////////////////////
//
// bios - Holds BIOS image and can retrieve environment data
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "bios.h"

/////////////////////////////////////////////////////////////////////////////

static uint8 *image = {0};
static uint32 image_size = 0;

uint8* EMU_CALL bios_get_image_native(void) { return image; }
uint32 EMU_CALL bios_get_imagesize(void) { return image_size; }

/////////////////////////////////////////////////////////////////////////////
//
// Static init
//
void EMU_CALL bios_set_image(uint8 *_image, uint32 _size) {
#ifdef EMU_BIG_ENDIAN
  uint32 i;
  for (i = 0; i < _size; i += 4) {
    uint8 a = _image[i + 0];
    uint8 b = _image[i + 1];
    _image[i + 0] = _image[i + 3];
    _image[i + 1] = _image[i + 2];
    _image[i + 2] = b;
    _image[i + 3] = a;
  }
#endif
  image = _image;
  image_size = _size;
}

/////////////////////////////////////////////////////////////////////////////
//
// Find environment variables
// Returns nonzero on error
//
sint32 EMU_CALL bios_getenv(
  const char *name,
  char *dest,
  sint32 dest_l
) {
  uint8 *romnative = bios_get_image_native();
  uint8 whole_env_area[129];
  uint8 *env;
  const uint8 *banner = (const uint8 *) "Highly Experimental";
  sint32 banner_l = (sint32) strlen((char *)banner);
  sint32 name_l = (sint32) strlen(name);
  sint32 i;

  for(i = 0; i < 128; i++) {
    sint32 byteofs = 0x80 + (i ^ (EMU_ENDIAN_XOR(3)));
    whole_env_area[i] = romnative[byteofs];
  }
  whole_env_area[128] = 0;

  env = whole_env_area;

  if(!dest_l) return 1;
  if(memcmp(env, banner, banner_l)) return 1;
  env += banner_l;

  for(;;) {
    sint32 isquote = 0;
    uint8 *varnamestart;
    uint8 *varnameend;
    uint8 *varvalstart;
    uint8 *varvalend;
    sint32 varnamelen;
    sint32 varvallen;
    // find a variable name
    for(;; env++) { uint8 c = *env; if(!c) return 1; if(c != ' ') break; }
    varnamestart = env;
    // find where it ends
    for(;; env++) { uint8 c = *env; if(!c) return 1; if(c == '=') break; }
    varnameend = env;
    // eat trailing spaces
    while(
      (varnameend > varnamestart) &&
      (varnameend[-1] == ' ')
    ) varnameend--;
    // skip equals sign
    env++;
    // compute length
    varnamelen = (sint32)(varnameend - varnamestart);
    // find the value
    for(;; env++) { uint8 c = *env; if(!c) return 1; if(c != ' ') break; }
    varvalstart = env;
    // if it's a quote, handle it as such
    if(*env == '\"') { varvalstart++; env++; isquote = 1; }
    // seek to the end of the value
    for(;; env++) {
      uint8 c = *env;
      if(!c) break;
      if(isquote && c == '\"') break;
      if((!isquote) && c == ' ') break;
    }
    varvalend = env;
    // skip trailing quote if it's there
    if(isquote && *env == '\"') env++;
    // compute length
    varvallen = (sint32)(varvalend - varvalstart);

    // now, if this is the variable name we want...
    if(varnamelen == name_l && !memcmp(varnamestart, name, name_l)) {
      // return it in the destination buffer
      if(varvallen > (dest_l - 1)) varvallen = (dest_l - 1);
      if(varvallen) memcpy(dest, varvalstart, varvallen);
      dest[varvallen] = 0;
      return 0;
    }

  }

  return 1;
}

/////////////////////////////////////////////////////////////////////////////
