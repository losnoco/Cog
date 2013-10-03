/*
 *  shn_reader.cpp
 *  shorten_decoder
 *
 *  Created by Alex Lagutin on Sun Jul 07 2002.
 *  Copyright (c) 2002 Eckysoft All rights reserved.
 *
 */

/*  shn.c - main functions for xmms-shn
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
 * $Id: shn.c,v 1.9 2001/12/30 05:12:04 jason Exp $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include "shn_reader.h"

#define WAVE_RIFF                       (0x46464952)     /* 'RIFF' in little-endian */
#define WAVE_WAVE                       (0x45564157)     /* 'WAVE' in little-endian */
#define WAVE_FMT                        (0x20746d66)     /* ' fmt' in little-endian */
#define WAVE_DATA                       (0x61746164)     /* 'data' in little-endian */
#define WAVE_FORMAT_PCM                 (0x0001)
#define CANONICAL_HEADER_SIZE           (44)

#define	CACHE_SIZE				(256*1024)	//256K

static void init_offset(slong **offset,int nchan,int nblock,int ftype)
{
  slong mean = 0;
  int  chan, i;

  /* initialise offset */
  switch(ftype)
  {
  	case TYPE_AU1:
  	case TYPE_S8:
  	case TYPE_S16HL:
  	case TYPE_S16LH:
  	case TYPE_ULAW:
  	case TYPE_AU2:
  	case TYPE_AU3:
  	case TYPE_ALAW:
    	mean = 0;
    	break;
  	case TYPE_U8:
    	mean = 0x80;
    	break;
  	case TYPE_U16HL:
  	case TYPE_U16LH:
    	mean = 0x8000;
    	break;
  	default:
        //fprintf(stderr, "Unknown file type: %d\n", ftype);
		break;
  }

  for(chan = 0; chan < nchan; chan++)
    for(i = 0; i < nblock; i++)
      offset[chan][i] = mean;
}

shn_reader::shn_reader()
{
	init();
	pthread_mutex_init(&mRingLock, NULL);
	pthread_cond_init(&mRunCond, NULL);
}

shn_reader::~shn_reader()
{
	exit();
	pthread_mutex_destroy(&mRingLock);
	pthread_cond_destroy(&mRunCond);
}

void shn_reader::init()
{
	mSeekTable	= NULL;
	mSeekTo		= -1;
	mFP			= NULL;
	mFatalError	= false;
	mGoing		= false;
	mEOF		= false;
	memset(&mDecodeState, 0, sizeof(mDecodeState));
	memset(&mWAVEHeader, 0, sizeof(mWAVEHeader));
	mRing.Init(CACHE_SIZE);
}

void shn_reader::exit()
{
	if (mGoing)
	{
		mGoing	= false;
		pthread_cond_signal(&mRunCond);
		pthread_join(mThread, NULL);
	}
	
	if (mDecodeState.getbuf)
	{
		free(mDecodeState.getbuf);
		mDecodeState.getbuf	= NULL;
	}
	if (mDecodeState.writebuf)
	{
		free(mDecodeState.writebuf);
		mDecodeState.writebuf	= NULL;
	}
	if (mDecodeState.writefub)
	{
		free(mDecodeState.writefub);
		mDecodeState.writefub	= NULL;
	}
	if (mSeekTable)
	{
		free(mSeekTable);
		mSeekTable	= NULL;
	}
	if (mFP)
	{
		fclose(mFP);
		mFP	= NULL;
	}
}

int shn_reader::open(const char *fn, bool should_load_seek_table)
{
	struct stat	sz;
	
	exit();
	init();
	
	if (stat(fn, &sz))
		return -1;
	
	if (!S_ISREG(sz.st_mode))
		return -1;

	mWAVEHeader.actual_size = (ulong)sz.st_size;
	
	mFP	= fopen(fn, "r");
	
	if (!mFP)
		return -1;
	
	if (!get_wave_header() || !verify_header())
	{
		fclose(mFP);
		mFP	= NULL;
		return -1;
	}
	
	if (should_load_seek_table)
		load_seek_table(fn);
		
	return 0;
}

int shn_reader::go()
{
	if (!mFP)
		return -1;
	
	fseek(mFP, 0, SEEK_SET);
	mGoing	= true;

	if (pthread_create(&mThread, NULL, (void *(*)(void *))thread_runner, this))
	{
		mGoing	= false;
		return -1;
	}	
	return 0;
}

long shn_reader::read(void *buf, long size)
{
	long	actread;
	
	if (!mGoing)
		return -2;
	
	pthread_mutex_lock(&mRingLock);
	actread	= mRing.ReadData((char *) buf, size);
	if (!actread)
		actread	= mEOF ? -2 : -1;
	
	pthread_mutex_unlock(&mRingLock);
	if (actread > 0)
		pthread_cond_signal(&mRunCond);

	return actread;
}

float shn_reader::seek(float seconds)
{
	if (!mSeekTable)
		return -1.0f;

	mSeekTo	= (int) seconds;
	shn_seek_entry *seek_info = seek_entry_search(mSeekTo * (ulong)mWAVEHeader.samples_per_sec,0,
								(ulong)(mSeekTableEntries - 1));
	ulong			sample = uchar_to_ulong_le(seek_info->data);
	return (float)sample/((float)mWAVEHeader.samples_per_sec);
}

int shn_reader::shn_get_buffer_block_size(int blocks)//derek
{
	int blk_size = blocks * (mWAVEHeader.bits_per_sample / 8) * mWAVEHeader.channels;
	if(blk_size > OUT_BUFFER_SIZE)
	{
		blk_size = NUM_DEFAULT_BUFFER_BLOCKS * (mWAVEHeader.bits_per_sample / 8) * mWAVEHeader.channels;
	}

	return blk_size;
}

unsigned int shn_reader::shn_get_song_length()//derek
{
	if(mWAVEHeader.length > 0)
		return (unsigned int)(1000 * mWAVEHeader.length);
	
	/* Something failed or just isn't correct */
	return (unsigned int)0;
}

bool shn_reader::file_info(long *size, int *nch, float *rate, float *time,
							int *samplebits, bool *seekable)
{
	float	frate;
	int		frame_size;
	
	if (!mFP)
		return false;
		
	if (seekable)
		*seekable	= mSeekTable ? true : false;
		
	if (size)
		*size	= mWAVEHeader.actual_size;
		
	if (nch)
		*nch	= mWAVEHeader.channels;
	
	frame_size	= (uint)mWAVEHeader.channels * (uint)mWAVEHeader.bits_per_sample / 8;
	frate	= (float)mWAVEHeader.samples_per_sec;
	
	if (rate)
		*rate	= frate;
		
	if (time)
		*time	= (float)mWAVEHeader.data_size/((float)frame_size * frate);
	
	if (samplebits)
		*samplebits	= mWAVEHeader.bits_per_sample;

	return true;
}

void shn_reader::init_decode_state()
{
	if (mDecodeState.getbuf)
		free(mDecodeState.getbuf);

	if (mDecodeState.writebuf)
		free(mDecodeState.writebuf);

	if (mDecodeState.writefub)
		free(mDecodeState.writefub);
		
	memset(&mDecodeState, 0, sizeof(mDecodeState));
}

int shn_reader::get_wave_header()
{
  int   version = FORMAT_VERSION;
  int   ftype = TYPE_EOF;
  const char  *magic = MAGIC;
  int   internal_ftype;
  int   retval = 1;

  init_decode_state();
	mBytesInHeader	= 0;
    /***********************/
    /* EXTRACT starts here */
    /***********************/

    /* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
    if(FORMAT_VERSION < 2)
    {
      for(int i = 0; i < strlen(magic); i++)
	if(getc(mFP) != magic[i])
          return 0;

      /* get version number */
      version = getc(mFP);
	  if (version == EOF)
		return 0;
    }
    else
#endif /* STRICT_FORMAT_COMPATABILITY */
    {
      int nscan = 0;

      version = MAX_VERSION + 1;
      while(version > MAX_VERSION)
      {
  	int byte = getc(mFP);
	if(byte == EOF)
          return 0;
	if(magic[nscan] != '\0' && byte == magic[nscan])
          nscan++;
	else
        if(magic[nscan] == '\0' && byte <= MAX_VERSION)
          version = byte;
	else
        {
	  if(byte == magic[0])
  	    nscan = 1;
	  else
          {
	    nscan = 0;
	  }
	  version = MAX_VERSION + 1;
	}
      }
    }

    /* check version number */
    if(version > MAX_SUPPORTED_VERSION)
      return 0;

    /* initialise the variable length file read for the compressed stream */
    var_get_init();
    if (mFatalError)
      return 0;

    /* get the internal file type */
    internal_ftype = UINT_GET(TYPESIZE);

    /* has the user requested a change in file type? */
    if(internal_ftype != ftype) {
      if(ftype == TYPE_EOF)
	ftype = internal_ftype;    /*  no problems here */
      else             /* check that the requested conversion is valid */
	if(internal_ftype == TYPE_AU1 || internal_ftype == TYPE_AU2 ||
	   internal_ftype == TYPE_AU3 || ftype == TYPE_AU1 ||ftype == TYPE_AU2 || ftype == TYPE_AU3)
          goto got_enough_data;
    }

    UINT_GET(CHANSIZE);

    /* get blocksize if version > 0 */
    if(version > 0)
    {
      int byte, nskip;
      UINT_GET((int) (log((double) DEFAULT_BLOCK_SIZE) / M_LN2));
      UINT_GET(LPCQSIZE);
      UINT_GET(0);
      nskip = UINT_GET(NSKIPSIZE);
      for(int i = 0; i < nskip; i++)
      {
		byte = uvar_get(XBYTESIZE);
      }
    }

    /* find verbatim command */
    while(1)
    {
        int cmd = uvar_get(FNSIZE);
        switch(cmd)
        {
			case FN_VERBATIM:
			{
				int cklen = uvar_get(VERBATIM_CKSIZE_SIZE);
				
				while (cklen--)
				{
					if (mBytesInHeader >= OUT_BUFFER_SIZE)
					{
						retval = 0;
						goto got_enough_data;
					}
					mBytesInBuf = 0;
					mHeader[mBytesInHeader++] = (unsigned char)uvar_get(VERBATIM_BYTE_SIZE);
				}
			}
            break;

          default:
            goto got_enough_data;
        }
    }

got_enough_data:

    /* wind up */
    var_get_quit();

    mBytesInBuf = 0;

    return retval;
}

int shn_reader::verify_header()
{
	ulong l;
	int cur = 0;

	if (mBytesInHeader < CANONICAL_HEADER_SIZE)
		return 0;
	
	if (WAVE_RIFF != uchar_to_ulong_le(&mHeader[cur]))
		return 0;
	cur += 4;

	mWAVEHeader.chunk_size = uchar_to_ulong_le(&mHeader[cur]);
	cur += 4;

	if (WAVE_WAVE != uchar_to_ulong_le(&mHeader[cur]))
		return 0;
	cur += 4;

	for (;;)
	{
		cur += 4;

		l = uchar_to_ulong_le(&mHeader[cur]);
		cur += 4;

		if (WAVE_FMT == uchar_to_ulong_le(&mHeader[cur-8]))
			break;

		cur += l;
	}

	if (l < 16)
		return 0;

	mWAVEHeader.wave_format = uchar_to_ushort_le(&mHeader[cur]);
	cur += 2;

	switch (mWAVEHeader.wave_format)
	{
		case WAVE_FORMAT_PCM:
			break;

		default:
            return 0;
	}

	mWAVEHeader.channels = uchar_to_ushort_le(&mHeader[cur]);
	cur += 2;
	mWAVEHeader.samples_per_sec = uchar_to_ulong_le(&mHeader[cur]);
	cur += 4;
	mWAVEHeader.avg_bytes_per_sec = uchar_to_ulong_le(&mHeader[cur]);
	cur += 4;
	mWAVEHeader.block_align = uchar_to_ushort_le(&mHeader[cur]);
	cur += 2;
	mWAVEHeader.bits_per_sample = uchar_to_ushort_le(&mHeader[cur]);
	cur += 2;

	if (mWAVEHeader.bits_per_sample != 8 && mWAVEHeader.bits_per_sample != 16)
		return 0;

	l -= 16;

	if (l > 0)
		cur += l;

	for (;;)
	{
		cur += 4;

		l = uchar_to_ulong_le(&mHeader[cur]);
		cur += 4;

		if (WAVE_DATA == uchar_to_ulong_le(&mHeader[cur-8]))
			break;

		cur += l;
	}

	mWAVEHeader.rate = ((uint)mWAVEHeader.samples_per_sec *
				      (uint)mWAVEHeader.channels *
				      (uint)mWAVEHeader.bits_per_sample) / 8;
	mWAVEHeader.header_size = cur;
	mWAVEHeader.data_size = l;
	mWAVEHeader.total_size = mWAVEHeader.chunk_size + 8;
	mWAVEHeader.length = mWAVEHeader.data_size / mWAVEHeader.rate;

	/* header looks ok */
	return 1;
}

void shn_reader::write_and_wait(int block_size)
{
	int bytes_to_write;
	
	if (mBytesInBuf < block_size)
		return;

	bytes_to_write = MIN(mBytesInBuf, block_size);

	if (bytes_to_write <= 0)
		return;
	
	while (mGoing && mSeekTo == -1)
	{
		long	written;
		
		pthread_mutex_lock(&mRingLock);
		written	= mRing.WriteData((char *) mBuffer, bytes_to_write);
		bytes_to_write	-= written;
		mBytesInBuf		-= written;
		if (!bytes_to_write)
		{
			pthread_mutex_unlock(&mRingLock);
			break;
		}
		pthread_cond_wait(&mRunCond, &mRingLock);
		pthread_mutex_unlock(&mRingLock);		
	}
}

void shn_reader::Run()
{
  slong  **buffer = NULL, **offset = NULL;
  slong  lpcqoffset = 0;
  int   version = FORMAT_VERSION, bitshift = 0;
  int   ftype = TYPE_EOF;
  const char  *magic = MAGIC;
  int   blocksize = DEFAULT_BLOCK_SIZE, nchan = DEFAULT_NCHAN;
  int   i, chan, nwrap, nskip = DEFAULT_NSKIP;
  int   *qlpc = NULL, maxnlpc = DEFAULT_MAXNLPC, nmean = UNDEFINED_UINT;
  int   cmd;
  int   internal_ftype;
  int   blk_size;
  int   cklen;
  uchar tmp;

restart:

  mBytesInBuf = 0;

  init_decode_state();

  blk_size = 512 * (mWAVEHeader.bits_per_sample / 8) * mWAVEHeader.channels;

    /***********************/
    /* EXTRACT starts here */
    /***********************/

    /* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
    if(FORMAT_VERSION < 2)
    {
      for(i = 0; i < strlen(magic); i++)
	if(getc(this_shn->vars.fd) != magic[i])
	{
	  mFatalError	= true;
          goto exit_thread;
        }

      /* get version number */
      version = getc(this_shn->vars.fd);
	  if (version == EOF)
	  {
		mFatalError	= true;
		goto exit_thread;
	  }
    }
    else
#endif /* STRICT_FORMAT_COMPATABILITY */
    {
      int nscan = 0;

      version = MAX_VERSION + 1;
      while(version > MAX_VERSION)
      {
  	int byte = getc(mFP);
	if(byte == EOF) {
	 mFatalError	= true;
          goto exit_thread;
        }
	if(magic[nscan] != '\0' && byte == magic[nscan])
          nscan++;
	else
        if(magic[nscan] == '\0' && byte <= MAX_VERSION)
          version = byte;
	else
        {
	  if(byte == magic[0])
  	    nscan = 1;
	  else
          {
	    nscan = 0;
	  }
	  version = MAX_VERSION + 1;
	}
      }
    }

    /* check version number */
    if(version > MAX_SUPPORTED_VERSION) {
      mFatalError	= true;
      goto exit_thread;
    }

    /* set up the default nmean, ignoring the command line state */
    nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

    /* initialise the variable length file read for the compressed stream */
    var_get_init();
    if (mFatalError)
      goto exit_thread;

    /* initialise the fixed length file write for the uncompressed stream */
    fwrite_type_init();

    /* get the internal file type */
    internal_ftype = UINT_GET(TYPESIZE);

    /* has the user requested a change in file type? */
    if(internal_ftype != ftype) {
      if(ftype == TYPE_EOF)
	ftype = internal_ftype;    /*  no problems here */
      else             /* check that the requested conversion is valid */
	if(internal_ftype == TYPE_AU1 || internal_ftype == TYPE_AU2 ||
	   internal_ftype == TYPE_AU3 || ftype == TYPE_AU1 ||ftype == TYPE_AU2 || ftype == TYPE_AU3) {
	  	mFatalError	= true;
          goto cleanup;
        }
    }

    nchan = UINT_GET(CHANSIZE);

    /* get blocksize if version > 0 */
    if(version > 0)
    {
      int byte;
      blocksize = UINT_GET((int) (log((double) DEFAULT_BLOCK_SIZE) / M_LN2));
      maxnlpc = UINT_GET(LPCQSIZE);
      nmean = UINT_GET(0);
      nskip = UINT_GET(NSKIPSIZE);
      for(i = 0; i < nskip; i++)
      {
	byte = uvar_get(XBYTESIZE);
      }
    }
    else
      blocksize = DEFAULT_BLOCK_SIZE;

    nwrap = MAX(NWRAP, maxnlpc);

    /* grab some space for the input buffer */
    buffer  = long2d((ulong) nchan, (ulong) (blocksize + nwrap));
    if (mFatalError)
      goto exit_thread;
    offset  = long2d((ulong) nchan, (ulong) MAX(1, nmean));
    if (mFatalError) {
      if (buffer) {
        free(buffer);
        buffer = NULL;
      }
      goto exit_thread;
    }

    for(chan = 0; chan < nchan; chan++)
    {
      for(i = 0; i < nwrap; i++)
      	buffer[chan][i] = 0;
      buffer[chan] += nwrap;
    }

    if(maxnlpc > 0) {
      qlpc = (int*) pmalloc((ulong) (maxnlpc * sizeof(*qlpc)));
      if (mFatalError) {
        if (buffer) {
          free(buffer);
          buffer = NULL;
        }
        if (offset) {
          free(offset);
          buffer = NULL;
        }
        goto exit_thread;
      }
    }

    if(version > 1)
      lpcqoffset = V2LPCQOFFSET;

    init_offset(offset, nchan, MAX(1, nmean), internal_ftype);

    /* get commands from file and execute them */
    chan = 0;
    while(1)
    {
        cmd = uvar_get(FNSIZE);
        if (mFatalError)
          goto cleanup;

        switch(cmd)
        {
          case FN_ZERO:
          case FN_DIFF0:
          case FN_DIFF1:
          case FN_DIFF2:
          case FN_DIFF3:
          case FN_QLPC:
          {
            slong coffset, *cbuffer = buffer[chan];
            int resn = 0, nlpc, j;

            if(cmd != FN_ZERO)
            {
              resn = uvar_get(ENERGYSIZE);
              if (mFatalError)
                goto cleanup;
              /* this is a hack as version 0 differed in definition of var_get */
              if(version == 0)
                resn--;
            }

            /* find mean offset : N.B. this code duplicated */
            if(nmean == 0)
              coffset = offset[chan][0];
            else
            {
              slong sum = (version < 2) ? 0 : nmean / 2;
              for(i = 0; i < nmean; i++)
                sum += offset[chan][i];
              if(version < 2)
                coffset = sum / nmean;
              else
                coffset = ROUNDEDSHIFTDOWN(sum / nmean, bitshift);
            }

            switch(cmd)
            {
              case FN_ZERO:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = 0;
                break;
              case FN_DIFF0:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn) + coffset;
                  if (mFatalError)
                    goto cleanup;
                }
                break;
              case FN_DIFF1:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn) + cbuffer[i - 1];
                  if (mFatalError)
                    goto cleanup;
                }
                break;
              case FN_DIFF2:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn) + (2 * cbuffer[i - 1] -	cbuffer[i - 2]);
                  if (mFatalError)
                    goto cleanup;
                }
                break;
              case FN_DIFF3:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn) + 3 * (cbuffer[i - 1] -  cbuffer[i - 2]) + cbuffer[i - 3];
                  if (mFatalError)
                    goto cleanup;
                }
                break;
              case FN_QLPC:
                nlpc = uvar_get(LPCQSIZE);
                if (mFatalError)
                  goto cleanup;

                for(i = 0; i < nlpc; i++) {
                  qlpc[i] = var_get(LPCQUANT);
                  if (mFatalError)
                    goto cleanup;
                }
                for(i = 0; i < nlpc; i++)
                  cbuffer[i - nlpc] -= coffset;
                for(i = 0; i < blocksize; i++)
                {
                  slong sum = lpcqoffset;

                  for(j = 0; j < nlpc; j++)
                    sum += qlpc[j] * cbuffer[i - j - 1];
                  cbuffer[i] = var_get(resn) + (sum >> LPCQUANT);
                  if (mFatalError)
                    goto cleanup;
                }
                if(coffset != 0)
                  for(i = 0; i < blocksize; i++)
                    cbuffer[i] += coffset;
                break;
            }

            /* store mean value if appropriate : N.B. Duplicated code */
            if(nmean > 0)
            {
              slong sum = (version < 2) ? 0 : blocksize / 2;

              for(i = 0; i < blocksize; i++)
                sum += cbuffer[i];

              for(i = 1; i < nmean; i++)
                offset[chan][i - 1] = offset[chan][i];
              if(version < 2)
                offset[chan][nmean - 1] = sum / blocksize;
              else
                offset[chan][nmean - 1] = (sum / blocksize) << bitshift;
            }

            /* do the wrap */
            for(i = -nwrap; i < 0; i++)
              cbuffer[i] = cbuffer[i + blocksize];

            fix_bitshift(cbuffer, blocksize, bitshift, internal_ftype);

            if(chan == nchan - 1)
            {
              if (!mGoing || mFatalError)
                goto cleanup;

              fwrite_type(buffer, ftype, nchan, blocksize);

              write_and_wait(blk_size);

              if (mSeekTo != -1)
              {
                shn_seek_entry *seek_info = seek_entry_search(mSeekTo * (ulong)mWAVEHeader.samples_per_sec,0,
							      (ulong)(mSeekTableEntries - 1));
				
 				pthread_mutex_lock(&mRingLock);
				mRing.Empty();
				pthread_mutex_unlock(&mRingLock);

                buffer[0][-1] = uchar_to_slong_le(seek_info->data+24);
                buffer[0][-2] = uchar_to_slong_le(seek_info->data+28);
                buffer[0][-3] = uchar_to_slong_le(seek_info->data+32);
                offset[0][0]  = uchar_to_slong_le(seek_info->data+48);
                offset[0][1]  = uchar_to_slong_le(seek_info->data+52);
                offset[0][2]  = uchar_to_slong_le(seek_info->data+56);
                offset[0][3]  = uchar_to_slong_le(seek_info->data+60);
		if (nchan > 1)
		{
	                buffer[1][-1] = uchar_to_slong_le(seek_info->data+36);
        	        buffer[1][-2] = uchar_to_slong_le(seek_info->data+40);
                	buffer[1][-3] = uchar_to_slong_le(seek_info->data+44);
	                offset[1][0]  = uchar_to_slong_le(seek_info->data+64);
        	        offset[1][1]  = uchar_to_slong_le(seek_info->data+68);
                	offset[1][2]  = uchar_to_slong_le(seek_info->data+72);
	                offset[1][3]  = uchar_to_slong_le(seek_info->data+76);
		}
                bitshift = uchar_to_ushort_le(seek_info->data+22);
                fseek(mFP,(slong)uchar_to_ulong_le(seek_info->data+8),SEEK_SET);
                fread((uchar*) mDecodeState.getbuf, 1, BUFSIZ, mFP);
                mDecodeState.getbufp = mDecodeState.getbuf + uchar_to_ushort_le(seek_info->data+14);
                mDecodeState.nbitget = uchar_to_ushort_le(seek_info->data+16);
                mDecodeState.nbyteget = uchar_to_ushort_le(seek_info->data+12);
                mDecodeState.gbuffer = uchar_to_ulong_le(seek_info->data+18);
				
                mBytesInBuf = 0;
				
				
               mSeekTo = -1;
              }

            }
            chan = (chan + 1) % nchan;
            break;
          }

          break;

          case FN_QUIT:
            /* empty out last of buffer */
            write_and_wait(mBytesInBuf);

            mEOF	= true;

            while (1)
            {
              if (!mGoing)
                goto finish;
              if (mSeekTo != -1)
              {
                var_get_quit();
                fwrite_type_quit();

                if (buffer) free((void *) buffer);
                if (offset) free((void *) offset);
                if(maxnlpc > 0 && qlpc)
                  free((void *) qlpc);

			fseek(mFP,0,SEEK_SET);
                goto restart;
              }
            }

            goto cleanup;
            break;

          case FN_BLOCKSIZE:
            blocksize = UINT_GET((int) (log((double) blocksize) / M_LN2));
            if (mFatalError)
              goto cleanup;
            break;
          case FN_BITSHIFT:
            bitshift = uvar_get(BITSHIFTSIZE);
            if (mFatalError)
              goto cleanup;
            break;
          case FN_VERBATIM:
            cklen = uvar_get(VERBATIM_CKSIZE_SIZE);
            if (mFatalError)
              goto cleanup;

            while (cklen--) {
              tmp = (uchar)uvar_get(VERBATIM_BYTE_SIZE);
              if (mFatalError)
                goto cleanup;
            }

            break;

          default:
            mFatalError	= true;
			//fprintf(stderr,"Sanity check fails trying to decode function: %d\n",cmd);
            goto cleanup;
        }
    }

cleanup:

    write_and_wait(mBytesInBuf);

finish:

    mSeekTo = -1;
    mEOF = true;

    /* wind up */
    var_get_quit();
    fwrite_type_quit();

    if (buffer) free((void *) buffer);
    if (offset) free((void *) offset);
    if(maxnlpc > 0 && qlpc)
      free((void *) qlpc);

exit_thread:

    pthread_exit(NULL);
}
