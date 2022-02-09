/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2021 Christopher Snowhill.  All rights reserved.
 * https://github.com/kode54/sflist
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SFLIST_H
#define _SFLIST_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bassmidi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sflist_presets {
	unsigned int count;
	BASS_MIDI_FONTEX *presets;
} sflist_presets;

#define sflist_max_error 1024

sflist_presets *sflist_load(const char *sflist, size_t size, const char *base_path, char *error);
void sflist_free(sflist_presets *);

const char *sflist_upgrade(const char *sflist, size_t size, char *error);
void sflist_upgrade_free(const char *);

#ifdef __cplusplus
}
#endif

#endif
