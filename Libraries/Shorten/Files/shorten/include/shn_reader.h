/*
 *  shn_reader.h
 *  shorten_decoder
 *
 *  Created by Alex Lagutin on Sun Jul 07 2002.
 *  Copyright (c) 2002 Eckysoft All rights reserved.
 *
 */

/*  xmms-shn - a shorten (.shn) plugin for XMMS
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
 * $Id: shn.h,v 1.6 2001/12/30 05:12:04 jason Exp $
 */

#ifndef __SHN_READER_H__
#define	__SHN_READER_H__

#include <stdio.h>
#include <pthread.h>
#include "shorten.h"
#include "ringbuffer.h"


/* Derek */
typedef unsigned int uint;

/* surely no headers will be this large.  right?  RIGHT?  */
#define OUT_BUFFER_SIZE			16384
#define NUM_DEFAULT_BUFFER_BLOCKS 512L

#define SEEK_HEADER_SIZE		12
#define SEEK_TRAILER_SIZE		12
#define SEEK_ENTRY_SIZE			80

#define MASKTABSIZE 33

typedef struct _shn_decode_state
{
	uchar	*getbuf;
	uchar	*getbufp;
	int   	nbitget;
	int   	nbyteget;
	ulong	gbuffer;
	char	*writebuf;
	char	*writefub;
	int		nwritebuf;
} shn_decode_state;

typedef struct _shn_seek_header
{
	uchar	data[SEEK_HEADER_SIZE];
	ulong	version;
	ulong	shnFileSize;	
} shn_seek_header;

typedef struct _shn_seek_trailer
{
	uchar	data[SEEK_TRAILER_SIZE];
	ulong	seekTableSize;
} shn_seek_trailer;

typedef struct _shn_seek_entry
{
	uchar	data[SEEK_ENTRY_SIZE];
} shn_seek_entry;

typedef struct _shn_wave_header
{
	char			*filename,
					m_ss[16];

	uint	header_size;

	ushort	channels,
			block_align,
			bits_per_sample,
			wave_format;

	ulong	samples_per_sec,
			avg_bytes_per_sec,
			rate,
			length,
			data_size,
			total_size,
			chunk_size,
			actual_size;
  	ulong	problems;
} shn_wave_header;

class shn_reader
{
	private:
		shn_decode_state	mDecodeState;
		shn_wave_header		mWAVEHeader;
		shn_seek_header		mSeekHeader;
		shn_seek_trailer	mSeekTrailer;
		shn_seek_entry		*mSeekTable;
		int					mSeekTo;
		bool				mEOF;
		bool				mGoing;
		long				mSeekTableEntries;
		int					mBytesInBuf;
		uchar				mBuffer[OUT_BUFFER_SIZE];
		int					mBytesInHeader;
		uchar				mHeader[OUT_BUFFER_SIZE];
		bool				mFatalError;
		FILE				*mFP;
		RingBuffer			mRing;
		pthread_mutex_t		mRingLock;
		pthread_cond_t		mRunCond;
		pthread_t			mThread;
		
		ulong				masktab[MASKTABSIZE];
		int					sizeof_sample[TYPE_EOF];
		
	public:
							shn_reader();
							~shn_reader();
		int					open(const char *fn, bool should_load_seek_table = true);
		int					go();
		void				exit();
		bool				file_info(long *size, int *nch, float *rate, float *time,
										int *samplebits, bool *seekable);
		long				read(void *buf, long size);
		float				seek(float sec);
		int					shn_get_buffer_block_size(int blocks);//derek
		unsigned int		shn_get_song_length();//derek
		
	private:
		void				init();
		void				Run();
	
		int					get_wave_header();
		int					verify_header();
		void				init_decode_state();
		void				write_and_wait(int block_size);
		
		/* fixio.cpp */
		void				init_sizeof_sample();
		void				fwrite_type_init();
		void				fwrite_type_quit();
		void				fix_bitshift(slong *buffer, int nitem, int bitshift, int ftype);
		void				fwrite_type(slong **data,int ftype,int nchan,int nitem);
		
		/* vario.cpp */
		void				var_get_init();
		ulong				word_get();
		long				uvar_get(int nbin);
		ulong				ulong_get();
		long				var_get(int nbin);
		void				var_get_quit();

		/* seek.cpp */
		void				load_seek_table(const char *fn);
		int					load_separate_seek_table(const char *fn);
		shn_seek_entry 		*seek_entry_search(ulong goal, ulong min, ulong max);

		/* array.cpp */
		void				*pmalloc(ulong size);
		slong				**long2d(ulong n0, ulong n1);

		friend void			*thread_runner(shn_reader *reader)
							{
								reader->Run();
								return NULL;
							}
};

inline ulong uchar_to_ulong_le(uchar *buf)
/* converts 4 bytes stored in little-endian format to a ulong */
{
	return (ulong)((buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0]);
}

inline slong uchar_to_slong_le(uchar *buf)
/* converts 4 bytes stored in little-endian format to an slong */
{
	return (long)uchar_to_ulong_le(buf);
}

inline ushort uchar_to_ushort_le(uchar *buf)
/* converts 4 bytes stored in little-endian format to a ushort */
{
	return (ushort)((buf[1] << 8) + buf[0]);
}

#endif /*__SHN_READER_H__*/
