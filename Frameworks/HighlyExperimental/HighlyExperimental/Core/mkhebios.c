#include "mkhebios.h"

/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#if 0
#define log_error do { fprintf(stderr, "he: error at line %u\n", __LINE__); } while(0)
#else
#define log_error
#endif

/***************************************************************************/

static uint16 get16lsb(const uint8 *p) {
  return (((uint16)(p[0])) <<  0) |
         (((uint16)(p[1])) <<  8);
}

static uint32 get32lsb(const uint8 *p) {
  return (((uint32)(p[0])) <<  0) |
         (((uint32)(p[1])) <<  8) |
         (((uint32)(p[2])) << 16) |
         (((uint32)(p[3])) << 24);
}

static void put16lsb(uint8 *p, uint16 n) {
  p[0] = n >> 0;
  p[1] = n >> 8;
}

static void put32lsb(uint8 *p, uint32 n) {
  p[0] = n >>  0;
  p[1] = n >>  8;
  p[2] = n >> 16;
  p[3] = n >> 24;
}

/***************************************************************************/

struct MODULE {
  uint8 name[11];
  uint16 code;
  void *ext_data;
  uint32 size;
  uint32 start;
};

static void init_mod_fields(
  struct MODULE *mod,
  const char *name,
  uint16 code,
  uint32 start,
  uint32 size
) {
  memset(mod, 0, sizeof(struct MODULE));
  strncpy(mod->name, name, 10);
  mod->code  = code;
  mod->start = start;
  mod->size  = size;
}

/***************************************************************************/

static int read_script_file(const char *script, int (*linehandler)(void *, int, const char*), void * ctx) {
  int r = 0;
  char s[1000];
  int l = 0;
  int linenum = 1;
  for(;;) {
    int c = *script++;
    if(c == 10 || c == 0) {
      while(l && s[l-1] == 32) l--;
      s[l] = 0;
      if(s[0]) {
        if(linehandler(ctx, linenum, s)) {
          r = 1;
          break;
        }
      }
      l = 0;
      if(c == 0) { r = 0; break; }
      linenum++;
      continue;
    }
    if(c < 32) c = 32;
    if(l || c > 32) {
      s[l] = c;
      if(l < (sizeof(s) - 1)) l++;
    }
  }
  return r;
}

/***************************************************************************/
/*
** Find the ROMDIR offset within the ROM.
** Returns 0 if not found (it should never be 0 in real circumstances).
*/
static uint32 find_romdir(uint8 *bios, uint32 bios_size) {
  uint32 u;
  bios_size &= ~0xF;
  for(u = 0; u < bios_size; u += 0x10) {
    if(!memcmp(bios + u, "RESET\0\0\0\0\0", 10)) return u;
  }
  return 0;
}

/***************************************************************************/
/*
** Returns true on success
*/
static int find_romdir_entry(
  uint8 *bios, uint32 bios_size,
  uint32 romdir_start,
  const uint8 *name,
  uint16 *out_code,
  uint32 *out_start,
  uint32 *out_size
) {
  uint32 ofs = 0;
  bios_size &= ~0xF;
  romdir_start &= ~0xF;
  for(; romdir_start < bios_size; romdir_start += 0x10) {
    uint8 n[11];
    uint16 c;
    uint32 s;
    // exhausted romdir?
    if(!bios[romdir_start]) return 0;
    memcpy(n, bios + romdir_start, 10);
    n[10] = 0;
    c = get16lsb(bios + romdir_start + 0xA);
    s = get32lsb(bios + romdir_start + 0xC);
    if(!strcmp(n, name)) {
      if(out_code ) *out_code  = c;
      if(out_start) *out_start = ofs;
      if(out_size ) *out_size  = s;
      log_error;
      return 1;
    }
    // round up to the nearest paragraph and advance offset
    // (confirmed the actual BIOS does this too)
    s += 0xF; s &= ~0xF;
    ofs += s;
  }
  return 0;
}

/***************************************************************************/

struct MKHEBIOS
{
  const char * iopbtconf_script;

  struct MODULE *master_module_free_list;
  int master_module_free_n;
  struct MODULE *master_module_pin_list;
  int master_module_pin_n;

  char *master_iopbtconf;

  uint8 *master_bios;
  int master_bios_size;

  struct MODULE **rom_usage_map;
  int usage_entries;

  uint8 *romdir;
  uint32 romdir_size;
  uint16 romdir_code;

  uint32 attempt_romdir_line;
  uint32 attempt_romdir_ofs;
};

static struct MODULE *master_find_module(struct MKHEBIOS *state, const char *name) {
  int n;
  for(n = 0; n < state->master_module_pin_n; n++) {
    if(!strcasecmp(state->master_module_pin_list[n].name, name)) return state->master_module_pin_list + n;
  }
  for(n = 0; n < state->master_module_free_n; n++) {
    if(!strcasecmp(state->master_module_free_list[n].name, name)) return state->master_module_free_list + n;
  }
  return NULL;
}

static struct MKHEBIOS * master_init(void) {
  struct MKHEBIOS * state = ( struct MKHEBIOS * ) malloc( sizeof( struct MKHEBIOS ) );
  if ( !state ) { log_error; return NULL; }
  memset( state, 0, sizeof( struct MKHEBIOS ) );
  return state;
}

static void master_iopbtconf_append(struct MKHEBIOS *state, const char *line) {
  char newline[1000];
  int l;
  char *p;

  strncpy(newline, line, sizeof(newline));
  newline[sizeof(newline)-1]=0;
  p = strrchr(newline,',');
  if(p) *p = 0;

  if(!state->master_iopbtconf) {
    state->master_iopbtconf = malloc(1);
    if(!state->master_iopbtconf) return;
    *state->master_iopbtconf = 0;
  }
  l = strlen(state->master_iopbtconf) + strlen(newline) + 1;
  state->master_iopbtconf = realloc(state->master_iopbtconf, l + 1);
  if(!state->master_iopbtconf) return;
  p = state->master_iopbtconf; p += strlen(p);
  strcpy(p, newline); p += strlen(p);
  p[0] = 10;
  p[1] = 0;
}

static void master_module_free_add(struct MKHEBIOS *state, struct MODULE *mod) {
  state->master_module_free_list = realloc(
    state->master_module_free_list,
    sizeof(struct MODULE) * (state->master_module_free_n + 1)
  );
  if(!state->master_module_free_list) return;
  memcpy(state->master_module_free_list + state->master_module_free_n, mod, sizeof(struct MODULE));
  state->master_module_free_n++;
}

static void master_module_pin_add(struct MKHEBIOS *state, struct MODULE *mod) {
  state->master_module_pin_list = realloc(
    state->master_module_pin_list,
    sizeof(struct MODULE) * (state->master_module_pin_n + 1)
  );
  if(!state->master_module_pin_list) return;
  memcpy(state->master_module_pin_list + state->master_module_pin_n, mod, sizeof(struct MODULE));
  state->master_module_pin_n++;
}

/***************************************************************************/
/*
** Returns nonzero on error
*/
static int master_bios_modinfo(
  struct MKHEBIOS *state,
  const char *name,
  struct MODULE *out_mod
) {
  uint32 rbase;
  int success;
  uint16 code;
  uint32 start;
  uint32 size;

  rbase = find_romdir(state->master_bios, state->master_bios_size);
  if(!rbase) { log_error; return 1; }
  success = find_romdir_entry(
    state->master_bios, state->master_bios_size,
    rbase,
    name,
    &code,
    &start,
    &size
  );
  if(!success) { log_error; return 1; }
  if(out_mod) init_mod_fields(out_mod, name, code, start, size);
  return 0;
}

/***************************************************************************/
/*
** ROM usage map (granularity 16 bytes)
*/
static void rom_usage_clear(struct MKHEBIOS * state) {
  state->usage_entries = state->master_bios_size / 0x10;
  state->rom_usage_map = realloc( state->rom_usage_map, sizeof(struct MODULE *) * state->usage_entries );
  memset(state->rom_usage_map, 0, sizeof(struct MODULE *) * state->usage_entries);
}

static void rom_usage_set(struct MKHEBIOS * state, uint32 ofs, uint32 size, struct MODULE *m) {
  uint32 slop;
//printf("rom_usage_set(%X,%X,%X)\n",ofs,size,(uint32)m);
  slop = ofs & 0xF;
  size += slop;
  ofs -= slop;
  slop = (0x10 - (size & 0xF)) & 0xF;
  size += slop;
  ofs /= 0x10;
  size /= 0x10;
  if(ofs >= state->usage_entries) return;
  if(size > state->usage_entries) size = state->usage_entries;
  if((ofs + size) > state->usage_entries) size = state->usage_entries - ofs;
  while(size--) state->rom_usage_map[ofs++] = m;
}

static uint32 rom_usage_get_free_space(struct MKHEBIOS * state, uint32 ofs) {
  uint32 i;
  ofs /= 0x10;
  i = ofs;
  for(; i < state->usage_entries; i++) if(state->rom_usage_map[i]) break;
  i -= ofs;
  i *= 0x10;
  return i;
}

static struct MODULE *rom_usage_get_module(struct MKHEBIOS * state, uint32 ofs) {
  ofs /= 0x10;
  if(ofs >= state->usage_entries) return NULL;
  return state->rom_usage_map[ofs];
}

static void rom_usage_set_all_pinned(struct MKHEBIOS * state) {
  int n;
  if(!state->master_module_pin_list) return;
  for(n = 0; n < state->master_module_pin_n; n++) {
    rom_usage_set(
      state,
      state->master_module_pin_list[n].start,
      state->master_module_pin_list[n].size,
      state->master_module_pin_list + n
    );
  }
}

/***************************************************************************/

static int any_unhandled_pinned_modules(struct MKHEBIOS * state) {
  int n;
  for(n = 0; n < state->master_module_pin_n; n++) {
    if(state->master_module_pin_list[n].start >= state->attempt_romdir_ofs) { log_error; return 1; }
  }
  return 0;
}

static uint32 closest_unhandled_pinned_module(struct MKHEBIOS * state) {
  uint32 lowest = 0xFFFFFFFF;
  int n;
  for(n = 0; n < state->master_module_pin_n; n++) {
    uint32 s = state->master_module_pin_list[n].start;
    if(s >= state->attempt_romdir_ofs) {
      if(s < lowest) lowest = s;
    }
  }
  return lowest;
}

static uint32 space_in_romdir(struct MKHEBIOS * state) { return state->romdir_size - state->attempt_romdir_line; }

static int line_from_arbitrary(struct MKHEBIOS * state, const char *name, uint16 code, uint32 size) {
  uint8 *p = state->romdir + state->attempt_romdir_line;
  if(space_in_romdir(state) < 0x10) { log_error; return 1; }
  memset  (p, 0, 0x10);
  strcpy  (p, name);
  put16lsb(p + 0xA, code);
  put32lsb(p + 0xC, size);
  state->attempt_romdir_line += 0x10;
//printf("new line name '%s' code %X size %X\n",name,code,size);
  return 0;
}

static int line_from_module(struct MKHEBIOS * state, struct MODULE *m) { return line_from_arbitrary(state, m->name, m->code, m->size); }
static int line_from_blank(struct MKHEBIOS * state, uint32 size) { return line_from_arbitrary(state, "-", 0, size); }
static int line_from_eod(struct MKHEBIOS * state) { return line_from_arbitrary(state, "", 0, 0); }

static int handle_pinned(struct MKHEBIOS * state) {
//printf("handle_pinned\n");
  for(;;) {
    struct MODULE *m = rom_usage_get_module(state, state->attempt_romdir_ofs);
    if(!m) break;
    if(line_from_module(state, m)) { log_error; return 1; }
    state->attempt_romdir_ofs += (m->size + 0xF) & (~0xF);
  }
  return 0;
}

static int seek_to_free(struct MKHEBIOS * state, uint32 s) {
//printf("seek_to_free(%X)\n",s);
  for(;;) {
    uint32 cf;
    if(handle_pinned(state)) { log_error; return 1; }
    cf = rom_usage_get_free_space(state, state->attempt_romdir_ofs);
//printf("  cf=%X\n",cf);
    if(!cf) { log_error; return 1; }
    if(cf >= s) return 0;
    if(line_from_blank(state, cf)) { log_error; return 1; }
    state->attempt_romdir_ofs += cf;
  }
  return 0;
}

/*
** Returns nonzero on error
*/
static int attempt_rebuild_romdir(struct MKHEBIOS * state) {
  int n;

  if(!state->romdir) { log_error; return 1; }
  state->romdir_size &= ~0xF;
  if(!state->romdir_size) { log_error; return 1; }

  memset(state->romdir, 0, state->romdir_size);

//printf("attempt rebuild romdir_size=0x%X\n",romdir_size);

  state->attempt_romdir_line = 0;
  state->attempt_romdir_ofs = 0;

  rom_usage_clear(state);
  rom_usage_set_all_pinned(state);

  if(seek_to_free(state, state->romdir_size)) { log_error; return 1; }
//printf("a l=%X ofs=%X\n",attempt_romdir_line,attempt_romdir_ofs);

  if(line_from_arbitrary(state, "ROMDIR", state->romdir_code, state->romdir_size)) { log_error; return 1; }
  state->attempt_romdir_ofs += state->romdir_size;
//printf("b l=%X ofs=%X\n",attempt_romdir_line,attempt_romdir_ofs);

  for(n = 0; n < state->master_module_free_n; n++) {
    uint32 size;
//printf("n%02d a l=%X ofs=%X\n",n,attempt_romdir_line,attempt_romdir_ofs);
    size = state->master_module_free_list[n].size;
//printf("n%02d b l=%X ofs=%X\n",n,attempt_romdir_line,attempt_romdir_ofs);
    if(seek_to_free(state, size)) { log_error; return 1; }
//printf("n%02d c l=%X ofs=%X\n",n,attempt_romdir_line,attempt_romdir_ofs);
    if(line_from_module(state, state->master_module_free_list + n)) { log_error; return 1; }
//printf("n%02d d l=%X ofs=%X\n",n,attempt_romdir_line,attempt_romdir_ofs);
    state->attempt_romdir_ofs += (size + 0xF) & (~0xF);
//printf("n%02d e l=%X ofs=%X\n",n,attempt_romdir_line,attempt_romdir_ofs);
  }
//printf("z l=%X ofs=%X\n",attempt_romdir_line,attempt_romdir_ofs);

  // handle any higher-up pinned modules if necessary
  for(;;) {
    uint32 c, blank;
    handle_pinned(state);
    if(!(any_unhandled_pinned_modules(state))) break;
    c = closest_unhandled_pinned_module(state);
    blank = c - state->attempt_romdir_ofs;
    if(line_from_blank(state, blank)) { log_error; return 1; }
    state->attempt_romdir_ofs += blank;
  }

  if(line_from_eod(state)) { log_error; return 1; }

  return 0;
}

/***************************************************************************/

static int find_romdir_code(struct MKHEBIOS * state) {
  struct MODULE mod;
  if(master_bios_modinfo(state, "ROMDIR", &mod)) {
    { log_error; return 1; }
  }
  state->romdir_code = mod.code;
  return 0;
}

#include "mkhebios_overlays.h"

/***************************************************************************/
/*
** Returns nonzero on error
** modname can also be of the form "modname,hexcode"
*/
static int irx(struct MKHEBIOS * state, const char *modname) {
  int overlay_index;
  uint32 code = 0xFFFFFFFF;
  char n[1000];
  int i;
  char *t;
  struct MODULE mod;
  i = 0;
  memset(&mod, 0, sizeof(struct MODULE));
  for(;;) {
    char c = *modname++;
    if(!c) break;
    if(isspace(c)) continue;
    n[i] = toupper(c);
    if(i < (sizeof(n) - 1)) i++;
  }
  n[i] = 0;
  t = strchr(n, ',');
  if(t) {
    *t = 0;
    t++;
    code = strtoul(t, NULL, 16);
    code &= 0xFFFF;
  }
  // force name to get chopped to 10 bytes
  n[10] = 0;

  goto try_file;

try_file:
  memset(&mod, 0, sizeof(struct MODULE));
  {
    for (overlay_index = 0;;overlay_index++) {
      if (modules_list[overlay_index].name == NULL) goto file_fail;
      if (!strcasecmp(modules_list[overlay_index].name, n)) break;
    }
  }
  strcpy(mod.name, n);
  mod.size = modules_list[overlay_index].size;
  mod.ext_data = malloc(mod.size);
  if(!mod.ext_data) goto giveup;
  memcpy(mod.ext_data, modules_list[overlay_index].data, mod.size);
  goto file_ok;

try_bios:
  if(master_bios_modinfo(state, n, &mod)) goto bios_fail;
  mod.ext_data = malloc(mod.size);
  if(!mod.ext_data) abort();
  memcpy(mod.ext_data, state->master_bios + mod.start, mod.size);
  goto bios_ok;

file_fail: goto try_bios;
bios_fail: goto giveup;

file_ok: goto codecheck;
bios_ok: goto codecheck;

giveup:
  { log_error; return 1; }

codecheck:
  printf(" ");
  // if we're using an override code, great
  if(code != 0xFFFFFFFF) {
    mod.code = code;
  // otherwise, try finding the BIOS code
  } else {
    struct MODULE tmpmod;
    if(master_bios_modinfo(state, n, &tmpmod)) {
      free(mod.ext_data);
      { log_error; return 1; }
    }
    mod.code = tmpmod.code;
  }

  if(mod.size < 4 || memcmp(mod.ext_data, "\x7F" "ELF", 4)) {
    free(mod.ext_data);
    { log_error; return 1; }
  }

  master_module_free_add(state, &mod);

  return 0;
}

/***************************************************************************/

static int pin(struct MKHEBIOS * state, const char *modname) {
  char n[11];
  int i, l;
  struct MODULE mod;
  strncpy(n, modname, 10);
  n[10] = 0;
  l = strlen(n);
  for(i = 0; i < l; i++) { n[i] = toupper(n[i]); }

  if(master_bios_modinfo(state, n, &mod)) {
    { log_error; return 1; }
  }

  mod.ext_data = malloc(mod.size);
  if(!mod.ext_data) abort();
  memcpy(mod.ext_data, state->master_bios + mod.start, mod.size);

  master_module_pin_add(state, &mod);

  return 0;
}

/***************************************************************************/

static int mkhebios_script_pin(struct MKHEBIOS * state, const char *args) {
  return pin(state, args);
}

/***************************************************************************/

static int mkhebios_iopbtconf_line(void *ctx, int linenum, const char *line) {
  struct MKHEBIOS * state = ( struct MKHEBIOS * ) ctx;
  int r;
  if(!line[0]) return 0;
  if(line[0] == '#') return 0;
  master_iopbtconf_append(state, line);
  if(!isalpha(line[0])) return 0;
  r = irx(state, line);
  if(r) { log_error; return 1; }
  return 0;
}

/***************************************************************************/

static int mkhebios_script_iopbtconf(struct MKHEBIOS * state, const char *args) {
  struct MODULE mod;

  if(master_bios_modinfo(state, "IOPBTCONF", &mod)) {
    { log_error; return 1; }
  }

  if(read_script_file(state->iopbtconf_script, mkhebios_iopbtconf_line, state)) { log_error; return 1; }

  mod.size = strlen(state->master_iopbtconf);
  mod.ext_data = malloc(mod.size);
  if(!mod.ext_data) { log_error; return 1; }
  memcpy(mod.ext_data, state->master_iopbtconf, mod.size);
  master_module_free_add(state, &mod);

  return 0;
}

/***************************************************************************/

static void rebuild_master_bios(struct MKHEBIOS *state) {
  uint8 *p = state->romdir;
  uint32 start = 0;
  uint32 size;
  uint16 code;

  memset(state->master_bios, 0, state->master_bios_size);

  for(;;) {
    uint8 *srcdata = NULL;
    char n[11];
//    int i;
    memcpy(n, p, 10);
    n[10] = 0;
    if(!n[0]) break;
    code = get16lsb(p + 0xA);
    size = get32lsb(p + 0xC);
//printf("named module %s\n",n);fflush(stdout);
    if(!strcmp(n, "-")) {
      srcdata = NULL;
    } else if(!strcmp(n, "ROMDIR")) {
      srcdata = state->romdir;
    } else {
      struct MODULE *pmod = master_find_module(state, n);
      if(!pmod) { return; }
      srcdata = pmod->ext_data;
    }
    if(start >= state->master_bios_size) { return; }
    if((start + size) >= state->master_bios_size) { return; }
    if(srcdata) memcpy(state->master_bios + start, srcdata, size);
    size += 0xF; size &= ~0xF; start += size;
    p += 0x10;
  }
}

/***************************************************************************/

static int mkhebios_script_rebuild(struct MKHEBIOS *state, const char *args) {
  uint32 romdir_size_min;
  uint32 romdir_size_max;

  romdir_size_min = 0x10 * (state->master_module_free_n + state->master_module_pin_n);
  romdir_size_max = 500000 & (~0xF);

  for(
    state->romdir_size = romdir_size_min;
    state->romdir_size <= romdir_size_max;
    state->romdir_size += 0x10
  ) {
    if(state->romdir) { free(state->romdir); state->romdir = NULL; }
    state->romdir = malloc(state->romdir_size);
    if(!state->romdir) { log_error; return 1; }
    if(!attempt_rebuild_romdir(state)) break;
    if(state->romdir) { free(state->romdir); state->romdir = NULL; }
  }

  if(!state->romdir) {
    log_error; return 1;
  }

  rebuild_master_bios(state);

  return 0;
}

/***************************************************************************/

static int mkhebios_script_patch(struct MKHEBIOS *state, const char *args) {
  uint32 patchstart = 0;
  uint8 patchfrom[1000];
  uint8 patchto[1000];
  uint32 patchfromny = 0;
  uint32 patchtony = 0;
  uint32 patchlen = 0;
  char n[1000];
  int i = 0;
  char *t = n;
  for(;;) {
    char c = *args++;
    if(!c) break;
    if(isspace(c)) continue;
    n[i] = toupper(c);
    if(i < (sizeof(n) - 1)) i++;
  }
  n[i] = 0;
  t = n;
  for(;;) {
    int c = *t++;
    if(c == ':') break;
         if(c >= 'A' && c <= 'F') { c -= 'A'; c += 10; }
    else if(c >= 'a' && c <= 'f') { c -= 'a'; c += 10; }
    else if(c >= '0' && c <= '9') { c -= '0'; c +=  0; }
    else { log_error; return 1; }
    patchstart <<= 4;
    patchstart += c;
  }
//  printf("patchstart %X\n",patchstart);

  for(;;) {
    int c = *t++;
    if(c == ':') break;
         if(c >= 'A' && c <= 'F') { c -= 'A'; c += 10; }
    else if(c >= 'a' && c <= 'f') { c -= 'a'; c += 10; }
    else if(c >= '0' && c <= '9') { c -= '0'; c +=  0; }
    else { log_error; return 1; }
    patchfrom[patchfromny/2] <<= 4;
    patchfrom[patchfromny/2] += c;
    patchfromny++;
  }

  for(;;) {
    int c = *t++;
    if(c == 0) break;
         if(c >= 'A' && c <= 'F') { c -= 'A'; c += 10; }
    else if(c >= 'a' && c <= 'f') { c -= 'a'; c += 10; }
    else if(c >= '0' && c <= '9') { c -= '0'; c +=  0; }
    else { log_error; return 1; }
    patchto[patchtony/2] <<= 4;
    patchto[patchtony/2] += c;
    patchtony++;
  }

  if(patchfromny != patchtony) { log_error; return 1; }
  patchlen = patchfromny / 2;
  if(!patchlen) { log_error; return 1; }

  if(patchstart >= state->master_bios_size) { log_error; return 1; }
  if((patchstart + patchlen) > state->master_bios_size) { log_error; return 1; }

  if(memcmp(state->master_bios + patchstart, patchfrom, patchlen)) {
    log_error; return 1;
  }

  memcpy(state->master_bios + patchstart, patchto, patchlen);

  return 0;
}

/***************************************************************************/

static int mkhebios_script_asc(struct MKHEBIOS *state, const char *args) {
  uint32 ascstart = 0;

  for(;;) {
    int c = *args++;
    if(!c) { log_error; return 1; }
    if(isspace(c)) continue;
    if(c == ':') break;
         if(c >= 'A' && c <= 'F') { c -= 'A'; c += 10; }
    else if(c >= 'a' && c <= 'f') { c -= 'a'; c += 10; }
    else if(c >= '0' && c <= '9') { c -= '0'; c +=  0; }
    else { log_error; return 1; }
    ascstart <<= 4;
    ascstart += c;
  }

  for(;;) {
    int c = *args++;
    if(!c) { log_error; return 1; }
    if(isspace(c)) continue;
    if(c == '\"') break;
  }

  for(;;) {
    int c = *args++;
    if(!c) { log_error; return 1; }
    if(c == '\"') break;
    state->master_bios[ascstart % state->master_bios_size] = c;
    ascstart++;
  }
  state->master_bios[ascstart % state->master_bios_size] = 0;

  return 0;
}

/***************************************************************************/
/*
** Returns nonzero on error
*/
int mkhebios_script_line(void *ctx, int linenum, const char *line) {
  struct MKHEBIOS *state = ( struct MKHEBIOS * ) ctx;
  if(line[0] == '#') return 0;

  { int cmdlen = 0;
    int ofs_to_args = 0;
    while(line[cmdlen] && isalpha(line[cmdlen])) cmdlen++;
    ofs_to_args = cmdlen;
    while(line[ofs_to_args] && isspace(line[ofs_to_args])) ofs_to_args++;

    switch(cmdlen) {
    case 3:
      if(!memcmp(line, "pin", 3)) return mkhebios_script_pin(state, line + ofs_to_args);
      if(!memcmp(line, "asc", 3)) return mkhebios_script_asc(state, line + ofs_to_args);
      break;
    case 4:
      if(!memcmp(line, "load", 4)) return 0; /*mkhebios_script_load(line + ofs_to_args);*/
      if(!memcmp(line, "save", 4)) return 0; /*mkhebios_script_save(line + ofs_to_args);*/
      break;
    case 5:
      if(!memcmp(line, "patch", 5)) return mkhebios_script_patch(state, line + ofs_to_args);
      break;
    case 7:
      if(!memcmp(line, "rebuild", 7)) return mkhebios_script_rebuild(state, line + ofs_to_args);
      break;
    /*case 8:
      if(!memcmp(line, "deadbeef", 8)) return mkhebios_script_deadbeef(line + ofs_to_args);
      break;*/
    case 9:
      if(!memcmp(line, "iopbtconf", 9)) return mkhebios_script_iopbtconf(state, line + ofs_to_args);
      break;
    }
  }
  log_error; return 1;
}

/***************************************************************************/

#include "mkhebios_scripts.h"

void * EMU_CALL mkhebios_create( void * ps2_bios, int *size )
{
  int rval;

  void * bios_out;

  struct MKHEBIOS * state;

  if ( *size != 0x400000 ) { log_error; return NULL; }

  state = master_init();
  if ( !state ) { log_error; return NULL; }

  state->master_bios = (uint8 *) malloc( 0x400000 );
  if ( !state->master_bios ) {
    free( state );
    log_error;
    return NULL;
  }

  memcpy( state->master_bios, ps2_bios, 0x400000 );
  state->master_bios_size = 0x400000;

  state->iopbtconf_script = script_iopbtconf;

  rval = read_script_file( script_hebios, mkhebios_script_line, state );

  if ( rval ) {
    bios_out = NULL;
    free( state->master_bios );
    log_error;
  }
  else {
    bios_out = realloc( state->master_bios, 0x400000 / 8 );
    if ( !bios_out ) log_error;
    *size = 0x400000 / 8;
  }

  if ( state->master_iopbtconf ) free( state->master_iopbtconf );

  if ( state->master_module_free_list ) {
    for ( rval = 0; rval < state->master_module_free_n; rval++ ) {
      free( state->master_module_free_list[ rval ].ext_data );
    }
    free( state->master_module_free_list );
  }

  if ( state->master_module_pin_list ) {
    for ( rval = 0; rval < state->master_module_pin_n; rval++ ) {
      free( state->master_module_pin_list[ rval ].ext_data );
    }
    free( state->master_module_pin_list );
  }

  if ( state->romdir ) free( state->romdir );

  free( state );

  return bios_out;
}

void EMU_CALL mkhebios_delete( void * iop_bios )
{
  free( iop_bios );
}

/***************************************************************************/
