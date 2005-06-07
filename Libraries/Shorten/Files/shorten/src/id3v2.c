/*  id3v2.c - functions to handle ID3v2 tags prepended to shn files
 *  Copyright (C) 2000-2004  Jason Jordan <shnutils@freeshell.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * $Id$
 */

#include <stdio.h>
#include "shorten.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define ID3V2_MAGIC "ID3"

typedef struct {
  char magic[3];
  unsigned char version[2];
  unsigned char flags[1];
  unsigned char size[4];
} _id3v2_header;

int tagcmp(char *got,char *expected)
/* compare got against expected, up to the length of expected */
{
  int i;

  for (i=0;*(expected+i);i++) {
    if (*(got+i) != *(expected+i))
      return i+1;
  }

  return 0;
}

unsigned long synchsafe_int_to_ulong(unsigned char *buf)
/* converts 4 bytes stored in synchsafe integer format to an unsigned long */
{
  return (unsigned long)(((buf[0] & 0x7f) << 21) | ((buf[1] & 0x7f) << 14) | ((buf[2] & 0x7f) << 7) | (buf[3] & 0x7f));
}

static unsigned long check_for_id3v2_tag(FILE *f)
{
  _id3v2_header id3v2_header;
  unsigned long tag_size;

  /* read an ID3v2 header's size worth of data */
  if (sizeof(_id3v2_header) != fread(&id3v2_header,1,sizeof(_id3v2_header),f)) {
    return 0;
  }

  /* verify this is an ID3v2 header */
  if (tagcmp(id3v2_header.magic,ID3V2_MAGIC) ||
      0xff == id3v2_header.version[0] || 0xff == id3v2_header.version[1] ||
      0x80 <= id3v2_header.size[0] || 0x80 <= id3v2_header.size[1] ||
      0x80 <= id3v2_header.size[2] || 0x80 <= id3v2_header.size[3])
  {
    return 0;
  }

  /* calculate and return ID3v2 tag size */
  tag_size = synchsafe_int_to_ulong(id3v2_header.size);

  return tag_size;
}

FILE *shn_open_and_discard_id3v2_tag(char *filename,int *file_has_id3v2_tag,long *id3v2_tag_size)
/* opens a file, and if it contains an ID3v2 tag, skips past it */
{ 
  FILE *f;
  unsigned long tag_size;

  if (NULL == (f = fopen(filename,"rb"))) {
    return NULL;
  }

  if (file_has_id3v2_tag)
    *file_has_id3v2_tag = 0;

  if (id3v2_tag_size)
    *id3v2_tag_size = 0;

  /* check for ID3v2 tag on input */
  if (0 == (tag_size = check_for_id3v2_tag(f))) {
    fclose(f);
    return fopen(filename,"rb");
  }

  if (file_has_id3v2_tag)
    *file_has_id3v2_tag = 2;

  if (id3v2_tag_size)
    *id3v2_tag_size = (long)(tag_size + sizeof(_id3v2_header));

  fprintf(stderr, "Discarding %lu-byte ID3v2 tag at beginning of file '%s'.",tag_size+sizeof(_id3v2_header),filename);

  if (0 != fseek(f,(long)tag_size,SEEK_CUR)) {
    fprintf(stderr, "Error while discarding ID3v2 tag in file '%s'.",filename);
    fclose(f);
    return fopen(filename,"rb");
  }

  return f;
}
