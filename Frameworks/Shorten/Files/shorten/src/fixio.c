/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id: fixio.c 19 2005-06-07 04:16:15Z vspader $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shorten.h"
#include "bitshift.h"

#define CAPMAXSCHAR(x)  ((x > 127) ? 127 : x)
#define CAPMAXUCHAR(x)  ((x > 255) ? 255 : x)
#define CAPMAXSHORT(x)  ((x > 32767) ? 32767 : x)
#define CAPMAXUSHORT(x) ((x > 65535) ? 65535 : x)

static int sizeof_sample[TYPE_EOF];

void init_sizeof_sample() {
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

void fwrite_type_init(shn_file *this_shn) {
  init_sizeof_sample();
  this_shn->decode_state->writebuf  = (schar*) NULL;
  this_shn->decode_state->writefub  = (schar*) NULL;
  this_shn->decode_state->nwritebuf = 0;
}

void fwrite_type_quit(shn_file *this_shn) {
  if(this_shn->decode_state->writebuf != NULL) {
    free(this_shn->decode_state->writebuf);
    this_shn->decode_state->writebuf = NULL;
  }
  if(this_shn->decode_state->writefub != NULL) {
    free(this_shn->decode_state->writefub);
    this_shn->decode_state->writefub = NULL;
  }
}

/* convert from signed ints to a given type and write */
void fwrite_type(slong **data,int ftype,int nchan,int nitem,shn_file *this_shn)
{
  int hiloint = 1, hilo = !(*((char*) &hiloint));
  int i, nwrite = 0, datasize = sizeof_sample[ftype], chan;
  slong *data0 = data[0];
  int bufAvailable = OUT_BUFFER_SIZE - this_shn->vars.bytes_in_buf;

  if(this_shn->decode_state->nwritebuf < nchan * nitem * datasize) {
    this_shn->decode_state->nwritebuf = nchan * nitem * datasize;
    if(this_shn->decode_state->writebuf != NULL) free(this_shn->decode_state->writebuf);
    if(this_shn->decode_state->writefub != NULL) free(this_shn->decode_state->writefub);
    this_shn->decode_state->writebuf = (schar*) pmalloc((ulong) this_shn->decode_state->nwritebuf,this_shn);
    if (!this_shn->decode_state->writebuf)
      return;
    this_shn->decode_state->writefub = (schar*) pmalloc((ulong) this_shn->decode_state->nwritebuf,this_shn);
    if (!this_shn->decode_state->writefub)
      return;
  }

  switch(ftype) {
  case TYPE_AU1: /* leave the conversion to fix_bitshift() */
  case TYPE_AU2: {
    uchar *writebufp = (uchar*) this_shn->decode_state->writebuf;
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
    uchar *writebufp = (uchar*) this_shn->decode_state->writebuf;
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
    schar *writebufp = (schar*) this_shn->decode_state->writebuf;
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
    short *writebufp = (short*) this_shn->decode_state->writebuf;
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
    ushort *writebufp = (ushort*) this_shn->decode_state->writebuf;
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
    uchar *writebufp = (uchar*) this_shn->decode_state->writebuf;
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
    uchar *writebufp = (uchar*) this_shn->decode_state->writebuf;
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
    uchar *writebufp = (uchar*) this_shn->decode_state->writebuf;
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
      memcpy((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf],(const void *)this_shn->decode_state->writebuf,datasize*nchan*nitem);
      this_shn->vars.bytes_in_buf += datasize*nchan*nitem;
      nwrite = nitem;
    }
    else
      shn_debug(this_shn->config, "Buffer overrun in fwrite_type() [case 1]: %d bytes to read, but only %d bytes are available",datasize*nchan*nitem,bufAvailable);
    break;
  case TYPE_S16HL:
  case TYPE_U16HL:
    if(hilo)
    {
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf],(const void *)this_shn->decode_state->writebuf,datasize*nchan*nitem);
        this_shn->vars.bytes_in_buf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
        shn_debug(this_shn->config, "Buffer overrun in fwrite_type() [case 2]: %d bytes to read, but only %d bytes are available",datasize*nchan*nitem,bufAvailable);
    }
    else
    {
      swab(this_shn->decode_state->writebuf, this_shn->decode_state->writefub, datasize * nchan * nitem);
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf],(const void *)this_shn->decode_state->writefub,datasize*nchan*nitem);
        this_shn->vars.bytes_in_buf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
        shn_debug(this_shn->config, "Buffer overrun in fwrite_type() [case 3]: %d bytes to read, but only %d bytes are available",datasize*nchan*nitem,bufAvailable);
    }
    break;
  case TYPE_S16LH:
  case TYPE_U16LH:
    if(hilo)
    {
      swab(this_shn->decode_state->writebuf, this_shn->decode_state->writefub, datasize * nchan * nitem);
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf],(const void *)this_shn->decode_state->writefub,datasize*nchan*nitem);
        this_shn->vars.bytes_in_buf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
        shn_debug(this_shn->config, "Buffer overrun in fwrite_type() [case 4]: %d bytes to read, but only %d bytes are available",datasize*nchan*nitem,bufAvailable);
    }
    else
    {
      if (datasize*nchan*nitem <= bufAvailable) {
        memcpy((void *)&this_shn->vars.buffer[this_shn->vars.bytes_in_buf],(const void *)this_shn->decode_state->writebuf,datasize*nchan*nitem);
        this_shn->vars.bytes_in_buf += datasize*nchan*nitem;
        nwrite = nitem;
      }
      else
        shn_debug(this_shn->config, "Buffer overrun in fwrite_type() [case 5]: %d bytes to read, but only %d bytes are available",datasize*nchan*nitem,bufAvailable);
    }
    break;
  }

  if(nwrite != nitem)
    shn_error_fatal(this_shn,"Failed to write decompressed stream -\npossible corrupt or truncated file");
}

/*************/
/* bitshifts */
/*************/

void fix_bitshift(buffer, nitem, bitshift, ftype) slong *buffer; int nitem,
       bitshift, ftype; {
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
