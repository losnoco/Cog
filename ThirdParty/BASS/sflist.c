/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2016 Christopher Snowhill.  All rights reserved.
 * https://github.com/kode54/sflist
 * https://gist.github.com/kode54/a7bb01a0db3f2e996145b77f0ca510d5
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <json-builder.h>

#include "sflist.h"

#ifndef PRId64
#ifdef _MSC_VER
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif
#endif

/* Extras needed */

static int json_equal (const json_value * a, const json_value * b);

static int json_equal_array(const json_value * a, const json_value * b)
{
   unsigned int i, j;

   if (a->u.array.length != b->u.array.length) return 0;

   for (i = 0, j = a->u.array.length; i < j; ++i)
   {
      if (!json_equal(a->u.array.values[i], b->u.array.values[i]))
         return 0;
   }

   return 1;
}

static int json_equal_object(const json_value * a, const json_value * b)
{
   unsigned int i, j;

   if (a->u.object.length != b->u.object.length) return 0;

   for (i = 0, j = a->u.object.length; i < j; ++i)
   {
      if (strcmp(a->u.object.values[i].name, b->u.object.values[i].name))
         return 0;
      if (!json_equal(a->u.object.values[i].value, b->u.object.values[i].value))
         return 0;
   }

   return 1;
}

static int json_equal (const json_value * a, const json_value * b)
{
   if (a->type != b->type) return 0;

   switch (a->type)
   {
   case json_none:
   case json_null:
      return 1;

   case json_integer:
      return a->u.integer == b->u.integer;

   case json_double:
      return a->u.dbl == b->u.dbl;

   case json_boolean:
      return !a->u.boolean == !b->u.boolean;

   case json_string:
      return !strcmp(a->u.string.ptr, b->u.string.ptr);

   case json_array:
      return json_equal_array(a, b);

   case json_object:
      return json_equal_object(a, b);
   }

   return 0;
}

static int json_signum (double val)
{
   return (val > 0.0) - (val < 0.0);
}

#define json_compare_invalid -1000

static int json_compare (const json_value * a, const json_value * b)
{
   if (a->type != b->type) return json_compare_invalid;

   switch (a->type)
   {
   case json_none:
   case json_null:
      return 0;

   case json_integer:
      return (int)(a->u.integer - b->u.integer);

   case json_double:
      return json_signum(a->u.dbl - b->u.dbl);

   case json_boolean:
      return !!a->u.boolean - !!b->u.boolean;

   case json_string:
      return strcmp(a->u.string.ptr, b->u.string.ptr);

   case json_array:
   case json_object:
      return json_compare_invalid;
   }

   return json_compare_invalid;
}

static int json_array_contains_value (const json_value * array, const json_value * value)
{
   unsigned int i, j;

   for (i = 0, j = array->u.array.length; i < j; ++i)
   {
      if (json_equal(array->u.array.values[i], value))
         return 1;
   }

   return 0;
}

json_value * json_array_merge (json_value * arrayA, json_value * arrayB)
{
   unsigned int i, j;

   if (arrayA->type != json_array || arrayB->type != json_array)
      return 0;

   for (i = 0, j = arrayB->u.array.length; i < j; ++i)
   {
      if (!json_array_contains_value(arrayA, arrayB->u.array.values[i]))
      {
         json_array_push(arrayA, arrayB->u.array.values[i]);
      }
   }

   json_builder_free(arrayB);

   return arrayA;
}

static int json_compare_callback (const void * a, const void * b)
{
   const json_value * aa = (const json_value *) a;
   const json_value * bb = (const json_value *) b;
   return json_compare(aa, bb);
}

json_value * json_array_sort (json_value * array)
{
   unsigned int i, j;

   json_type type;

   if (array->type != json_array) return 0;

   if (array->u.array.length < 2) return array;

   type = array->u.array.values[0]->type;

   for (i = 1, j = array->u.array.length; i < j; ++i)
   {
      if (array->u.array.values[i]->type != type)
         return 0;
   }

   qsort(array->u.array.values, j, sizeof(json_value *), json_compare_callback);

   return array;
}


/* Processing begins */

static size_t sflist_parse_int(const char * in, const char ** end)
{
   size_t rval = 0;
   while (in < *end) {
      if (isdigit(*in)) {
         rval = (rval * 10) + (*in - '0');
      }
      else break;
      ++in;
   }
   *end = in;
   return rval;
}

static double sflist_parse_float(const char * in, const char ** end)
{
   size_t whole = 0;
   size_t decimal = 0;
   size_t decimal_places = 0;
   double sign = 1.0;
   const char * end_orig = *end;
   const char * ptr = in;
   if (*ptr == '-') {
      ++ptr;
      sign = -1.0;
   }
   whole = sflist_parse_int(ptr, end);
   if (*end == ptr || (**end != '.' && *end < end_orig)) {
      *end = in;
      return 0.0;
   }
   if (*end < end_orig) {
      ptr = *end + 1;
      *end = end_orig;
      decimal = sflist_parse_int(ptr, end);
      if (*end == ptr || *end < end_orig) {
         *end = in;
         return 0.0;
      }
      decimal_places = *end - ptr;
   }
   return (((double)whole) + (((double)decimal) / pow(10.0, (double)decimal_places))) * sign;
}

static json_value * sflist_load_v1(const char * sflist, size_t size, char * error_buf)
{
   json_value * rval = 0;

   json_value * arr = json_array_new(0);

   json_value * channels = 0;
   json_value * patchMappings = 0;
   double gain = 0.0;

   const char * ptr = sflist;
   const char * end = sflist + size;

   unsigned int cur_line = 0;

   while (ptr < end) {
      const char * line_start = ptr;
      json_value * obj = 0;
      const char * path = 0;
      const char * pipe = 0;
      const char * lend = ptr;
      ++cur_line;
      while (lend < end && *lend && *lend != '\r' && *lend != '\n') {
         if (*lend == '|')
            pipe = lend;
         ++lend;
      }
      if (pipe)
         path = pipe + 1;
      else
         path = ptr;
      if (pipe) {
         while (ptr < pipe) {
            char c;
            const char * fend = ptr;
            const char * vend;
            while (fend < pipe && *fend != '&') ++fend;
            vend = fend;
            switch (c = *ptr++) {
               case '&':
                     continue;

               case 'c': {
                     json_value * this_channels;
                     size_t channel_low = sflist_parse_int(ptr, &vend);
                     size_t channel_high = 0;
                     size_t i;
                     if (vend == ptr || (*vend != '-' && *vend != '&' && *vend != '|')) {
                        sprintf(error_buf, "Invalid channel number (%u:%u)", cur_line, (int)(vend - line_start + 1));
                        goto error;
                     }
                     if (*vend != '-')
                        channel_high = channel_low;
                     else {
                        ptr = vend + 1;
                        vend = fend;
                        channel_high = sflist_parse_int(ptr, &vend);
                        if (vend == ptr || (*vend != '&' && *vend != '|')) {
                           sprintf(error_buf, "Invalid channel range end value (%u:%u)", cur_line, (int)(vend - line_start + 1));
                           goto error;
                        }
                     }
                     if (!channels)
                        channels = json_array_new(0);
                     this_channels = json_array_new(0);
                     for (i = channel_low; i <= channel_high; ++i)
                        json_array_push(this_channels, json_integer_new(i));
                     channels = json_array_merge(channels, this_channels);
                     ptr = fend;
                  }
                  break;

               case 'p': {
                     json_value * mapping = 0;
                     json_value * mapping_destination = 0;
                     json_value * mapping_source = 0;

                     long source_bank = -1;
                     long source_program = -1;
                     long dest_bank = -1;
                     long dest_program = -1;

                     size_t val = sflist_parse_int(ptr, &vend);
                     if (vend == ptr || (*vend != '=' && *vend != ',' && *vend != '|')) {
                        sprintf(error_buf, "Invalid preset number (%u:%u)", cur_line, (int)(vend - line_start + 1));
                        goto error;
                     }
                     dest_program = val;
                     if (*vend == ',') {
                        dest_bank = val;
                        ptr = vend + 1;
                        vend = fend;
                        val = sflist_parse_int(ptr, &vend);
                        if (vend == ptr || (*vend != '=' && *vend != '|')) {
                           sprintf(error_buf, "Invalid preset number (%u:%u)", cur_line, (int)(vend - line_start + 1));
                           goto error;
                        }
                        dest_program = val;
                     }
                     if (*vend == '=') {
                        ptr = vend + 1;
                        vend = fend;
                        val = sflist_parse_int(ptr, &vend);
                        if (vend == ptr || (*vend != ',' && *vend != '|')) {
                           sprintf(error_buf, "Invalid preset number (%u:%u)", cur_line, (int)(vend - line_start + 1));
                           goto error;
                        }
                        source_program = val;
                        if (*vend == ',') {
                           source_bank = val;
                           ptr = vend + 1;
                           vend = fend;
                           val = sflist_parse_int(ptr, &vend);
                           if (vend == ptr || (*vend != '&' && *vend != '|')) {
                              sprintf(error_buf, "Invalid preset number (%u:%u)", cur_line, (int)(vend - line_start + 1));
                              goto error;
                           }
                           source_program = val;
                        }
                     }

                     if (!patchMappings)
                        patchMappings = json_array_new(0);
                     mapping = json_object_new(0);
                     mapping_destination = json_object_new(0);
                     if (dest_bank != -1) {
                        json_object_push(mapping_destination, "bank", json_integer_new(dest_bank));
                     }
                     json_object_push(mapping_destination, "program", json_integer_new(dest_program));
                     json_object_push(mapping, "destination", mapping_destination);
                     if (source_program != -1) {
                        mapping_source = json_object_new(0);
                        if (source_bank != -1) {
                           json_object_push(mapping_source, "bank", json_integer_new(source_bank));
                        }
                        json_object_push(mapping_source, "program", json_integer_new(source_program));
                        json_object_push(mapping, "source", mapping_source);
                     }
                     json_array_push(patchMappings, mapping);

                     ptr = fend;
                  }
                  break;

               case 'g': {
                     double val = sflist_parse_float(ptr, &vend);
                     if (vend == ptr || vend < fend) {
                        sprintf(error_buf, "Invalid gain value (%u:%u)", cur_line, (int)(vend - line_start + 1));
                        goto error;
                     }
                     gain = val;
                     ptr = fend;
                  }
                  break;

               default:
                  sprintf(error_buf, "Invalid character in preset '%c' (%u:%u)", c, cur_line, (unsigned int)(ptr - line_start));
                  goto error;
            }
         }
      }
      obj = json_object_new(0);
      json_object_push(obj, "fileName", json_string_new_length((unsigned int)(lend - path), path));
      if (gain != 0.0) {
         json_object_push(obj, "gain", json_double_new(gain));
         gain = 0.0;
      }
      if (channels) {
         channels = json_array_sort(channels);
         json_object_push(obj, "channels", channels);
         channels = 0;
      }
      if (patchMappings) {
         json_object_push(obj, "patchMappings", patchMappings);
         patchMappings = 0;
      }

      json_array_push(arr, obj);

      ptr = lend;

      while (ptr < end && (*ptr == '\n' || *ptr == '\r')) ++ptr;
   }

   rval = json_object_new(1);
   json_object_push(rval, "soundFonts", arr);

   return rval;

error:
   if (channels)       json_builder_free(channels);
   if (patchMappings)  json_builder_free(patchMappings);
   if (arr)            json_builder_free(arr);
   return 0;
}

static json_value * sflist_load_v2(const char * sflist, size_t size, char * error)
{
   json_value * rval = 0;

   json_settings settings = { 0 };
   settings.value_extra = json_builder_extra;

   rval = json_parse_ex( &settings, sflist, size, error);

   return rval;
}

static const json_value * json_object_item(const json_value * object, const char * name)
{
   unsigned int i, j;

   if (object->type != json_object) return &json_value_none;

   for (i = 0, j = object->u.object.length; i < j; ++i) {
      if (!strcmp(object->u.object.values[i].name, name))
         return object->u.object.values[i].value;
   }

   return &json_value_none;
}

static void sflist_process_patchmappings(BASS_MIDI_FONTEX * out, BASS_MIDI_FONTEX * fontex, const json_value * patchMappings, unsigned int channel)
{
   unsigned int i, j;
   for (i = 0, j = patchMappings->u.array.length; i < j; ++i) {
      json_value * preset = patchMappings->u.array.values[i];
      const json_value * destination = json_object_item(preset, "destination");
      const json_value * source = json_object_item(preset, "source");
      const json_value * destination_bank = json_object_item(destination, "bank");
      const json_value * destination_program = json_object_item(destination, "program");
      const json_value * source_bank = json_object_item(source, "bank");
      const json_value * source_program = json_object_item(source, "program");
      fontex->spreset = (source_program->type == json_none) ? -1 : (int)source_program->u.integer;
      fontex->sbank = (source_bank->type == json_none) ? -1 : (int)source_bank->u.integer;
      fontex->dpreset = (destination_program->type == json_none) ? -1 : (int)destination_program->u.integer;
      fontex->dbank = (destination_bank->type == json_none) ? 0 : (int)destination_bank->u.integer;
      fontex->dbanklsb = channel;
      *out++ = *fontex;
   }
}

static sflist_presets * sflist_process(const json_value * sflist, const char * base_path, char * error_buf)
{
#ifdef _WIN32
   wchar_t path16[32768];
#endif
   char path_temp[32768];
   const char * base_path_end = base_path + strlen(base_path) - 1;
   unsigned int presets_to_allocate = 0;
   sflist_presets * rval = calloc(1, sizeof(sflist_presets));
   json_value * arr;
   unsigned int i, j, k, l, preset_number;
   HSOUNDFONT hfont = 0;
   BASS_MIDI_FONTEX fontex;

   if (!rval)
   {
      strcpy(error_buf, "Out of memory");
      goto error;
   }

   if (sflist->type != json_object || sflist->u.object.length != 1 ||
       strcmp(sflist->u.object.values[0].name, "soundFonts"))
   {
      if (sflist->type != json_object)
         strcpy(error_buf, "Base JSON item is not an object");
      else if (sflist->u.object.length != 1)
         sprintf(error_buf, "Base JSON object contains unexpected number of items (wanted 1, got %u)", sflist->u.object.length);
      else
         sprintf(error_buf, "Base JSON object contains '%s' object instead of 'soundFonts'", sflist->u.object.values[0].name);
      goto error;
   }

   arr = sflist->u.object.values[0].value;

   if (arr->type != json_array)
   {
      strcpy(error_buf, "JSON 'soundFonts' object is not an array");
      goto error;
   }

   for (i = 0, j = arr->u.array.length; i < j; ++i) {
      const json_value * obj = arr->u.array.values[i];
      const json_value * path = 0;
      const json_value * gain = 0;
      const json_value * channels = 0;
      const json_value * patchMappings = 0;
      unsigned int patches_needed = 1;
      if (obj->type != json_object) {
         sprintf(error_buf, "soundFont item #%u is not an object", i + 1);
         goto error;
      }
      path = json_object_item(obj, "fileName");
      gain = json_object_item(obj, "gain");
      channels = json_object_item(obj, "channels");
      patchMappings = json_object_item(obj, "patchMappings");
      if (path->type == json_none) {
         sprintf(error_buf, "soundFont item #%u has no 'fileName'", i + 1);
         goto error;
      }
      if (path->type != json_string) {
         sprintf(error_buf, "soundFont item #%u 'fileName' is not a string", i + 1);
         goto error;
      }
      if (gain->type != json_none && gain->type != json_integer && gain->type != json_double) {
         sprintf(error_buf, "soundFont item #%u has an invalid gain value", i + 1);
         goto error;
      }
      if (channels->type != json_none) {
         if (channels->type != json_array) {
            sprintf(error_buf, "soundFont item #%u 'channels' is not an array", i + 1);
            goto error;
         }
         for (k = 0, l = channels->u.array.length; k < l; ++k) {
            json_value * channel = channels->u.array.values[k];
            if (channel->type != json_integer) {
               sprintf(error_buf, "soundFont item #%u 'channels' #%u is not an integer", i + 1, k + 1);
               goto error;
            }
            if (channel->u.integer < 1 || channel->u.integer > 48) {
               sprintf(error_buf, "soundFont item #%u 'channels' #%u is out of range (wanted 1-48, got %" PRId64 ")", i + 1, k + 1, channel->u.integer);
               goto error;
            }
         }
         patches_needed = l;
      }
      if (patchMappings->type != json_none) {
         if (patchMappings->type != json_array) {
            sprintf(error_buf, "soundFont item #%u 'patchMappings' is not an array", i + 1);
            goto error;
         }
         for (k = 0, l = patchMappings->u.array.length; k < l; ++k) {
            unsigned int m, n;
            unsigned int source_found = 0;
            unsigned int destination_found = 0;
            json_value * mapping = patchMappings->u.array.values[k];
            if (mapping->type != json_object) {
               sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u is not an object", i + 1, k + 1);
               goto error;
            }
            for (m = 0, n = mapping->u.object.length; m < n; ++m) {
               unsigned int o, p;
               json_value * item = mapping->u.object.values[m].value;
               const char * name = mapping->u.object.values[m].name;
               unsigned int bank_found = 0;
               unsigned int program_found = 0;
               if (strcmp(name, "source") && strcmp(name, "destination")) {
                  sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u contains an invalid '%s' field", i + 1, k + 1, name);
                  goto error;
               }
               if (item->type != json_object) {
                  sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' is not an object", i + 1, k + 1, name);
                  goto error;
               }
               if (!strcmp(name, "source"))
                  ++source_found;
               else
                  ++destination_found;
               for (o = 0, p = item->u.object.length; o < p; ++o) {
                  int range_min = 0;
                  int range_max = 128;
                  json_value * item2 = item->u.object.values[o].value;
                  const char * name2 = item->u.object.values[o].name;
                  if (strcmp(name2, "bank") && strcmp(name2, "program")) {
                     sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' contains an invalid '%s' field", i + 1, k + 1, name, name2);
                     goto error;
                  }
                  if (item2->type != json_integer) {
                     sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' '%s' is not an integer", i + 1, k + 1, name, name2);
                  }
                  if (!strcmp(name2, "program")) {
                     if (!strcmp(name, "destination"))
                        range_max = 65535;
                     else
                        range_max = 127;
                  }
                  if (item2->u.integer < range_min || item2->u.integer > range_max) {
                     sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' '%s' is out of range (expected %d-%d, got %" PRId64 ")", i + 1, k + 1, name, name2, range_min, range_max, item->u.integer);
                     goto error;
                  }
                  if (!strcmp(name2, "bank"))
                     ++bank_found;
                  else
                     ++program_found;
               }
               if (!bank_found && !program_found) {
                  sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' contains no 'bank' or 'program'", i + 1, k + 1, name);
                  goto error;
               }
               if (bank_found > 1) {
                  sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' contains more than one 'bank'", i + 1, k + 1, name);
                  goto error;
               }
               if (program_found > 1) {
                  sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u '%s' contains more than one 'program'", i + 1, k + 1, name);
                  goto error;
               }
            }
            if (!destination_found) {
               sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u is missing 'destination'", i + 1, k + 1);
               goto error;
            }
            if (destination_found > 1) {
               sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u contains more than one 'destination'", i + 1, k + 1);
               goto error;
            }
            if (source_found > 1) {
               sprintf(error_buf, "soundFont item #%u 'patchMappings' #%u contains more than one 'source'", i + 1, k + 1);
            }
         }
         patches_needed *= l;
      }
      presets_to_allocate += patches_needed;
   }

   rval->count = presets_to_allocate;
   rval->presets = calloc(sizeof(BASS_MIDI_FONTEX), rval->count);

   if (!rval->presets) {
      strcpy(error_buf, "Out of memory");
      goto error;
   }

   preset_number = 0;

   for (i = arr->u.array.length, j = 0; i--; ++j) {
      const json_value * obj = arr->u.array.values[i];
      const json_value * path = json_object_item(obj, "fileName");
      const json_value * gain = json_object_item(obj, "gain");
      const json_value * channels = json_object_item(obj, "channels");
      const json_value * patchMappings = json_object_item(obj, "patchMappings");
      const void * bass_path;
      const char * path_ptr = path->u.string.ptr;
      unsigned int bass_flags = 0;
#ifdef _WIN32
      if (!(isalpha(*path_ptr) && path_ptr[1] == ':')) {
         if (strlen(path_ptr) + (base_path_end - base_path + 2) > 32767) {
            strcpy(error_buf, "Base path plus SoundFont relative path is longer than 32767 characters");
            goto error;
         }
         strcpy(path_temp, base_path);
         if (*base_path_end != '\\' && *base_path_end != '/')
            strcat(path_temp, "\\");
         strcat(path_temp, path_ptr);
         path_ptr = path_temp;
      }
      MultiByteToWideChar(CP_UTF8, 0, path_ptr, -1, path16, 32767);
      path16[32767] = '\0';
      bass_path = (void *) path16;
      bass_flags = BASS_UNICODE;
#else
      if (*path_ptr != '/') {
         if (strlen(path_ptr) + (base_path_end - base_path + 2) > 32767) {
            strcpy(error_buf, "Base path plus SoundFont relative path is longer than 32767 characters");
            goto error;
         }
         strcpy(path_temp, base_path);
         if (*base_path_end != '/')
            strcat(path_temp, "/");
         strcat(path_temp, path_ptr);
         path_ptr = path_temp;
      }
      bass_path = (void *) path_ptr;
#endif
      hfont = BASS_MIDI_FontInit(bass_path, bass_flags);
      if (!hfont) {
         int error_code = BASS_ErrorGetCode();
         if (error_code == BASS_ERROR_FILEOPEN) {
            sprintf(error_buf, "Could not open SoundFont bank '%s'", path->u.string.ptr);
            goto error;
         }
         else if (error_code == BASS_ERROR_FILEFORM) {
            sprintf(error_buf, "SoundFont bank '%s' is not a supported format", path->u.string.ptr);
            goto error;
         }
         else {
            sprintf(error_buf, "SoundFont bank '%s' failed to load with error #%u", path->u.string.ptr, error_code);
            goto error;
         }
      }
      if (gain->type != json_none) {
         double gain_value = 0.0;
         if (gain->type == json_integer) {
            gain_value = (double)gain->u.integer;
         }
         else if (gain->type == json_double) {
            gain_value = gain->u.dbl;
         }
         gain_value = pow(10.0, gain_value / 20.0);
         BASS_MIDI_FontSetVolume(hfont, gain_value);
      }
      fontex.font = hfont;
      fontex.spreset = -1;
      fontex.sbank = -1;
      fontex.dpreset = -1;
      fontex.dbank = 0;
      fontex.dbanklsb = 0;
      /* Simplest case, whole bank loading */
      if (channels->type == json_none && patchMappings->type == json_none) {
         rval->presets[preset_number++] = fontex;
      }
      else if (patchMappings->type == json_none) {
         for (k = 0, l = channels->u.array.length; k < l; ++k) {
            fontex.dbanklsb = (int)channels->u.array.values[k]->u.integer;
            rval->presets[preset_number++] = fontex;
         }
      }
      else if (channels->type == json_none) {
         sflist_process_patchmappings(rval->presets + preset_number, &fontex, patchMappings, 0);
         preset_number += patchMappings->u.array.length;
      }
      else {
         for (k = 0, l = channels->u.array.length; k < l; ++k) {
            sflist_process_patchmappings(rval->presets + preset_number, &fontex, patchMappings, (int)channels->u.array.values[k]->u.integer);
            preset_number += patchMappings->u.array.length;
         }
      }
   }

   return rval;

error:
   if (hfont) {
      BASS_MIDI_FontFree(hfont);
   }
   if (rval) {
      sflist_free(rval);
   }
   return 0;
}

static int strpbrkn_all(const char * str, size_t size, const char * chrs)
{
   const char * end = str + size;

   while (str < end && *chrs) {
      while (str < end && *str != *chrs) ++str;
      ++str, ++chrs;
   }

   return str < end;
}

sflist_presets * sflist_load(const char * sflist, size_t size, const char * base_path, char * error)
{
   sflist_presets * rval;

   json_value * list = 0;
   
   /* Handle Unicode byte order markers */
   if (size >= 2) {
      if ((sflist[0] == 0xFF && sflist[1] == 0xFE) ||
          (sflist[0] == 0xFE && sflist[1] == 0xFF)) {
         strcpy(error, "UTF-16 encoding is not supported at this time");
         return 0;
      }
      if (size >= 3 && sflist[0] == 0xEF &&
          sflist[1] == 0xBB && sflist[2] == 0xBF) {
         sflist += 3;
         size -= 3;
      }
   }

   list = sflist_load_v2(sflist, size, error);

   if (!list) {
      if (!strpbrkn_all(sflist, size, "{[]}"))
         list = sflist_load_v1(sflist, size, error);
   }

   if (!list) {
      return 0;
   }

   rval = sflist_process(list, base_path, error);

   json_builder_free(list);

   return rval;
}

void sflist_free(sflist_presets *presetlist)
{
   if (presetlist) {
      if (presetlist->presets) {
         unsigned int i, j;
         for (i = 0, j = presetlist->count; i < j; ++i) {
            HSOUNDFONT hfont = presetlist->presets[i].font;
            if (hfont) {
               BASS_MIDI_FontFree(hfont);
            }
         }
         free(presetlist->presets);
      }
      free(presetlist);
   }
}

const char * sflist_upgrade(const char * sflist, size_t size, char * error_buf)
{
   char * rval = 0;

   json_value * list = 0;

   size_t length = 0;

   const json_serialize_opts opts =
   {
      json_serialize_mode_multiline,
      0,
      3  /* indent_size */
   };

   list = sflist_load_v2(sflist, size, error_buf);

   if (!list) {
      if (!strpbrkn_all(sflist, size, "{[]}"))
         list = sflist_load_v1(sflist, size, error_buf);
   }

   if (!list) {
      return 0;
   }

   length = json_measure_ex( list, opts );

   rval = (char *) malloc( length + 1 );

   if (!rval) {
      strcpy(error_buf, "Out of memory");
      goto error;
   }

   json_serialize_ex( rval, list, opts );

   json_builder_free( list );

   rval[ length ] = '\0';

   return (const char *) rval;

error:
   if ( list ) {
      json_builder_free( list );
   }
   return 0;
}

void sflist_upgrade_free(const char *ptr)
{
   free( (void *) ptr );
}

