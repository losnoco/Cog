/*
 *  fixio.cpp
 *  shorten_decoder
 *
 *  Created by Alex Lagutin on Tue Jul 09 2002.
 *  Copyright (c) 2002 Eckysoft All rights reserved.
 *
 */

/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id: fixio.c,v 1.6 2001/12/30 05:12:04 jason Exp $
 */

#include <string.h>
#include "shn_reader.h"
#include "bitshift.h"

#define CAPMAXSCHAR(x)  ((x > 127) ? 127 : x)
#define CAPMAXUCHAR(x)  ((x > 255) ? 255 : x)
#define CAPMAXSHORT(x)  ((x > 32767) ? 32767 : x)
#define CAPMAXUSHORT(x) ((x > 65535) ? 65535 : x)

extern "C"
{
	int	Sulaw2linear(uchar);
	uchar	Slinear2ulaw(int);
	uchar Slinear2alaw(int);
}

void shn_reader::init_sizeof_sample() {
  sizeof_sample[TYPE_AU1]   = sizeof(uchar);
  sizeof_sample[TYPE_S8]    = sizeof(schar);
  sizeof_sample[TYPE_U8]    = sizeof(uchar);
  sizeof_sample[TYPE_S16HL] = sizeof(ushort);
  sizeof_sample[TYPE_U16HL] = sizeof(ushort);
  sizeof_sample[TYPE_S16LH] = sizeof(ushort);
  sizeof_sample[TYPE_U16LH] = sizeof(ushort);
  sizeof_sample[TYPE_ULAW]  = sizeof(uchar);
  sizeof_sample[TYPE_AU2]   = sizeof(uchar);
  sizeof_sample[TYPE_AU3]   = sizeof(uchar);
  sizeof_sample[TYPE_ALAW]  = sizeof(uchar);
}

/***************/
/* fixed write */
/***************/

void shn_reader::fwrite_type_init()
{
  init_sizeof_sample();
  mDecodeState.writebuf  = (char*) NULL;
  mDecodeState.writefub  = (char*) NULL;
  mDecodeState.nwritebuf = 0;
}

void shn_reader::fwrite_type_quit() {
  if(mDecodeState.writebuf != NULL) {
    free(mDecodeState.writebuf);
    mDecodeState.writebuf = NULL;
  }
  if(mDecodeState.writefub != NULL) {
    free(mDecodeState.writefub);
    mDecodeState.writefub = NULL;
  }
}

/* convert from signed ints to a given type and write */
void shn_reader::fwrite_type(slong **data,int ftype,int nchan,int nitem)
{
  int hiloint = 1, hilo = !(*((char*) &hiloint));
  int i, nwrite = 0, datasize = sizeof_sample[ftype], chan;
  slong *data0 = data[0];
  int bufAvailable = OUT_BUFFER_SIZE - mBytesInBuf;

  if(mDecodeState.nwritebuf < nchan * nitem * datasize) {
    mDecodeState.nwritebuf = nchan * nitem * datasize;
    if(mDecodeState.writebuf != NULL) free(mDecodeState.writebuf);
    if(mDecodeState.writefub != NULL) free(mDecodeState.writefub);
    mDecodeState.writebuf = (char*) pmalloc((ulong) mDecodeState.nwritebuf);
    if (!mDecodeState.writebuf)
      return;
    mDecodeState.writefub = (char*) pmalloc((ulong) mDecodeState.nwritebuf);
    if (!mDecodeState.writefub)
      return;
  }

  switch(ftype) {
  case TYPE_AU1: /* leave the conversion to fix_bitshift() */
  case TYPE_AU2: {
    uchar *writebufp = (uchar*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = data0[i];
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ = data[chan][i];
    break;
  }
  case TYPE_U8: {
    uchar *writebufp = (uchar*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = CAPMAXUCHAR(data0[i]);
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ =  CAPMAXUCHAR(data[chan][i]);
    break;
  }
  case TYPE_S8: {
    schar *writebufp = (schar*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = CAPMAXSCHAR(data0[i]);
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ = CAPMAXSCHAR(data[chan][i]);
    break;
  }
  case TYPE_S16HL:
  case TYPE_S16LH: {
    short *writebufp = (short*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = CAPMAXSHORT(data0[i]);
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ = CAPMAXSHORT(data[chan][i]);
    break;
  }
  case TYPE_U16HL:
  case TYPE_U16LH: {
    ushort *writebufp = (ushort*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = CAPMAXUSHORT(data0[i]);
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ = CAPMAXUSHORT(data[chan][i]);
    break;
  }
  case TYPE_ULAW: {
    uchar *writebufp = (uchar*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = Slinear2ulaw(CAPMAXSHORT((data0[i] << 3)));
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ = Slinear2ulaw(CAPMAXSHORT((data[chan][i] << 3)));
    break;
  }
  case TYPE_AU3: {
    uchar *writebufp = (uchar*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	if(data0[i] < 0)
	  *writebufp++ = (127 - data0[i]) ^ 0xd5;
	else
	  *writebufp++ = (data0[i] + 128) ^ 0x55;
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  if(data[chan][i] < 0)
	    *writebufp++ = (127 - data[chan][i]) ^ 0xd5;
	  else
	    *writebufp++ = (data[chan][i] + 128) ^ 0x55;
    break;
  }
  case TYPE_ALAW: {
    uchar *writebufp = (uchar*) mDecodeState.writebuf;
    if(nchan == 1)
      for(i = 0; i < nitem; i++)
	*writebufp++ = Slinear2alaw(CAPMAXSHORT((data0[i] << 3)));
    else
      for(i = 0; i < nitem; i++)
	for(chan = 0; chan < nchan; chan++)
	  *writebufp++ = Slinear2alaw(CAPMAXSHORT((data[chan][i] << 3)));
    break;
  }
  }

  switch(ftype) {
  case TYPE_AU1:
  case TYPE_S8:
  case TYPE_U8:
  case TYPE_ULAW:
  case TYPE_AU2:
  case TYPE_AU3:
  case TYPE_ALAW:
    if (datasize*nchan*nitem <= bufAvailable) {
      memcpy((void *)&mBuffer[mBytesInBuf],(const void *)mDecodeState.writebuf,datasize*nchan*nitem);
      mBytesInBuf += datasize*nchan*nitem;
      nwrite = nitem;
    }
    else
	{
     // fprintf(stderr, "Buffer overrun in fwrite_type() [case 1]: %d bytes to read, but only %d bytes are available\n",datasize*nchan*nitem,bufAvailable);
	}
    break;
  case TYPE_S16HL:
  case TYPE_U16HL:
    if(hilo)
    {
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&mBuffer[mBytesInBuf],(const void *)mDecodeState.writebuf,datasize*nchan*nitem);
        mBytesInBuf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
	  {
        //fprintf(stderr, "Buffer overrun in fwrite_type() [case 2]: %d bytes to read, but only %d bytes are available\n",datasize*nchan*nitem,bufAvailable);
		}
    }
    else
    {
      swab(mDecodeState.writebuf, mDecodeState.writefub, datasize * nchan * nitem);
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&mBuffer[mBytesInBuf],(const void *)mDecodeState.writefub,datasize*nchan*nitem);
        mBytesInBuf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
	  {
        //fprintf(stderr, "Buffer overrun in fwrite_type() [case 3]: %d bytes to read, but only %d bytes are available\n",datasize*nchan*nitem,bufAvailable);
		}
    }
    break;
  case TYPE_S16LH:
  case TYPE_U16LH:
    if(hilo)
    {
      swab(mDecodeState.writebuf, mDecodeState.writefub, datasize * nchan * nitem);
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&mBuffer[mBytesInBuf],(const void *)mDecodeState.writefub,datasize*nchan*nitem);
        mBytesInBuf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
	  {
        //fprintf(stderr, "Buffer overrun in fwrite_type() [case 4]: %d bytes to read, but only %d bytes are available\n",datasize*nchan*nitem,bufAvailable);
		}
    }
    else
    {
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&mBuffer[mBytesInBuf],(const void *)mDecodeState.writebuf,datasize*nchan*nitem);
        mBytesInBuf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
	  {
        //fprintf(stderr, "Buffer overrun in fwrite_type() [case 5]: %d bytes to read, but only %d bytes are available\n",datasize*nchan*nitem,bufAvailable);
		}
    }
    break;
  }

  if(nwrite != nitem)
  {
    mFatalError	= true;
		//fprintf(stderr,"Failed to write decompressed stream -\npossible corrupt or truncated file\n");
	}
}

/*************/
/* bitshifts */
/*************/

void shn_reader::fix_bitshift(slong *buffer, int nitem, int bitshift, int ftype)
{
  int i;

  if(ftype == TYPE_AU1)
    for(i = 0; i < nitem; i++)
      buffer[i] = ulaw_outward[bitshift][buffer[i] + 128];
  else if(ftype == TYPE_AU2)
    for(i = 0; i < nitem; i++) {
      if(buffer[i] >= 0)
	buffer[i] = ulaw_outward[bitshift][buffer[i] + 128];
      else if(buffer[i] == -1)
	buffer[i] =  NEGATIVE_ULAW_ZERO;
      else
	buffer[i] = ulaw_outward[bitshift][buffer[i] + 129];
    }
  else
    if(bitshift != 0)
      for(i = 0; i < nitem; i++)
	buffer[i] <<= bitshift;
}
