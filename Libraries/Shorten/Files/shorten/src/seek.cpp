/*
 *  seek.cpp
 *  shorten_decoder
 *
 *  Created by Alex Lagutin on Tue Jul 09 2002.
 *  Copyright (c) 2002 Eckysoft All rights reserved.
 *
 */

/*  seek.c - functions related to real-time seeking
 *  Copyright (C) 2000-2001  Jason Jordan (shnutils@freeshell.org)
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
 * $Id: seek.c,v 1.4 2001/12/30 05:12:04 jason Exp $
 */

#include <string.h>
#include "shn_reader.h"

#define	SEEK_HEADER_SIGNATURE		"SEEK"
#define	SEEK_TRAILER_SIGNATURE		"SHNAMPSK"
#define	SEEK_SUFFIX					".skt"

shn_seek_entry *shn_reader::seek_entry_search(ulong goal, ulong min, ulong max)
{
	ulong	med = (min + max) / 2;
	shn_seek_entry	*middle = &mSeekTable[med];
	ulong	sample = uchar_to_ulong_le(middle->data);

	if (goal < sample)
		return seek_entry_search(goal, min, med-1);
	if (goal > sample + 25600)
		return seek_entry_search(goal, med+1, max);

	return middle;
}

void shn_reader::load_seek_table(const char *fn)
{
	FILE	*fp;
	int		found = 0;
	
	if (!(fp = fopen(fn, "r")))
		return;
	
	fseek(fp, -SEEK_TRAILER_SIZE, SEEK_END);
	if (fread(mSeekTrailer.data, 1, SEEK_TRAILER_SIZE, fp) == SEEK_TRAILER_SIZE)
	{
		mSeekTrailer.seekTableSize = uchar_to_ulong_le(mSeekTrailer.data);
		if (!memcmp(&mSeekTrailer.data[4], SEEK_TRAILER_SIGNATURE, strlen(SEEK_TRAILER_SIGNATURE)))
		{
			fseek(fp, -mSeekTrailer.seekTableSize, SEEK_END);
			mSeekTrailer.seekTableSize -= (SEEK_HEADER_SIZE + SEEK_TRAILER_SIZE);
			if (fread(mSeekHeader.data, 1, SEEK_HEADER_SIZE, fp) == SEEK_HEADER_SIZE)
			{
				long	entries = mSeekTrailer.seekTableSize/sizeof(shn_seek_entry);
				
				if ((mSeekTable = (shn_seek_entry *) malloc(sizeof(shn_seek_entry)*entries)))
				{
					if ((long)fread(mSeekTable, sizeof(shn_seek_entry), entries, fp) == entries)
					{
						//fprintf(stderr, "Successfully loaded seek table appended to file: '%s'\n", fn);
						mSeekTableEntries	= entries;
						found	= 1;
					}
					else
					{
						free(mSeekTable);
						mSeekTable	= NULL;
					}
				}
			}
		}
	}
	fclose(fp);

	if (found)
		return;
		
	{
		// try load from separate seek file
		char	*slash, *ext;
		char	seek_fn[MAX_PATH];

		strcpy(seek_fn, fn);
		slash = strrchr(seek_fn, '/');
		ext = strrchr((slash ? slash : seek_fn), '.');
		if (ext)
			*ext = 0;
		strcat(seek_fn, SEEK_SUFFIX);
		load_separate_seek_table(seek_fn);
	}
}

int shn_reader::load_separate_seek_table(const char *fn)
{
	FILE	*fp;
	long	seek_table_len, entries;
	int		result = 0;

	//fprintf(stderr, "Looking for seek table in separate file: '%s'\n", fn);

	if (!(fp = fopen(fn, "r")))
		return 0;

	fseek(fp, 0, SEEK_END);
	seek_table_len	= (long)ftell(fp) - SEEK_HEADER_SIZE;
	entries			= seek_table_len/sizeof(shn_seek_entry);
	fseek(fp, 0, SEEK_SET);

	if (fread(mSeekHeader.data, 1, SEEK_HEADER_SIZE, fp) == SEEK_HEADER_SIZE)
	{
		mSeekHeader.version		= uchar_to_ulong_le(&mSeekHeader.data[4]);
		mSeekHeader.shnFileSize	= uchar_to_ulong_le(&mSeekHeader.data[8]);
		if (!memcmp(mSeekHeader.data, SEEK_HEADER_SIGNATURE, strlen(SEEK_HEADER_SIGNATURE)))
		{
			if (mSeekHeader.shnFileSize == mWAVEHeader.actual_size)
			{
				if ((mSeekTable = (shn_seek_entry *) malloc(sizeof(shn_seek_entry)*entries)))
				{
					if ((long)fread(mSeekTable, sizeof(shn_seek_entry), entries, fp) == entries)
					{
						//fprintf(stderr, "Successfully loaded seek table in separate file: '%s'\n", fn);
						mSeekTableEntries	= entries;
					}
					else
					{
						free(mSeekTable);
						mSeekTable	= NULL;
					}
				}
			}
			else
			{
				//fprintf(stderr, "Seek table .shn file size and actual .shn file size differ\n");
			}
		}
	}

	fclose(fp);
	return result;
}
