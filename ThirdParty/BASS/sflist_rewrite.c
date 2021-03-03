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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "sflist.h"

int main(int argc, const char ** argv)
{
   char error[sflist_max_error];
   char path_temp[32768];
   int i;
   FILE *f;
   size_t length;
   char * in;
   const char * out;

   for (i = 1; i < argc; ++i) {
      const char * end = argv[i] + strlen(argv[i]);
      if (((end - argv[i]) >= 5) && !strcmp(&end[-5], ".json")) continue;
      strcpy(path_temp, argv[i]);
      strcat(path_temp, ".json");
      f = fopen(argv[i], "r");
      fseek(f, 0, SEEK_END);
      length = ftell(f);
      fseek(f, 0, SEEK_SET);
      in = malloc(length + 1);
      if (!in) {
         fclose(f);
         fputs("Out of memory.\n", stderr);
         return 1;
      }
      if (fread(in, length, 1, f) != 1) {
         free(in);
         fclose(f);
         fprintf(stderr, "Cannot read all of file '%s'.\n", argv[i]);
         return 1;
      }
      fclose(f);
      in[length] = '\0';
      out = sflist_upgrade(in, length, error);
      free(in);
      if (!out) {
         fprintf(stderr, "Error processing '%s': %s\n", argv[i], error);
         return 1;
      }
      f = fopen(path_temp, "w");
      if (!f) {
         sflist_upgrade_free(out);
         fprintf(stderr, "Unable to open output file '%s'.\n", path_temp);
         return 1;
      }
      if (fwrite(out, strlen(out), 1, f) != 1) {
         fclose(f);
         sflist_upgrade_free(out);
         fprintf(stderr, "Unable to write to output file '%s'.\n", path_temp);
         return 1;
      }
      fclose(f);
      sflist_upgrade_free(out);
   }

   return 0;
}
