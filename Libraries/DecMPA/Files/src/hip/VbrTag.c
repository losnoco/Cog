/*  DecMPA decoding routines from Lame/HIP (Myers W. Carpenter)	

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	For more information look at the file License.txt in this package.

  
	email: hazard_hd@users.sourceforge.net
*/

#include "VbrTag.h"
#include <stdio.h>

/* stuff from lame's tables.c */
const int  bitrate_table    [3] [16] = {
    { 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1 },
    { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 },
    { 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1 },
};

const int  samplerate_table [3]  [4] = { 
    { 22050, 24000, 16000, -1 },      /* MPEG 2 */
    { 44100, 48000, 32000, -1 },      /* MPEG 1 */  
    { 11025, 12000,  8000, -1 },      /* MPEG 2.5 */
};
/* end stuff from lame's tables.c */


/* stuff from VbrTag.c/.h */
const static char	VBRTag[]={"Xing"};
const static char	VBRTag2[]={"Info"};

/* from lame's VbrTag.c */
static int 
ExtractI4(unsigned char *buf)
{
	int x;
	/* big endian extract */
	x = buf[0];
	x <<= 8;
	x |= buf[1];
	x <<= 8;
	x |= buf[2];
	x <<= 8;
	x |= buf[3];
	return x;
}

/* from lame's VbrTag.c */
int 
GetVbrTag(VBRTAGDATA *pTagData,  unsigned char *buf)
{
	int			i, head_flags;
	int			h_bitrate,h_id, h_mode, h_sr_index;
        int enc_delay,enc_padding; 

	/* get Vbr header data */
	pTagData->flags = 0;

	/* get selected MPEG header data */
	h_id       = (buf[1] >> 3) & 1;
	h_sr_index = (buf[2] >> 2) & 3;
	h_mode     = (buf[3] >> 6) & 3;
        h_bitrate  = ((buf[2]>>4)&0xf);
	h_bitrate = bitrate_table[h_id][h_bitrate];

        /* check for FFE syncword */
        if ((buf[1]>>4)==0xE) 
            pTagData->samprate = samplerate_table[2][h_sr_index];
        else
            pTagData->samprate = samplerate_table[h_id][h_sr_index];
        //	if( h_id == 0 )
        //		pTagData->samprate >>= 1;

	/*  determine offset of header */
	if( h_id )
	{
                /* mpeg1 */
		if( h_mode != 3 )	buf+=(32+4);
		else				buf+=(17+4);
	}
	else
	{
                /* mpeg2 */
		if( h_mode != 3 ) buf+=(17+4);
		else              buf+=(9+4);
	}

	if( buf[0] != VBRTag[0] && buf[0] != VBRTag2[0] ) return 0;    /* fail */
	if( buf[1] != VBRTag[1] && buf[1] != VBRTag2[1]) return 0;    /* header not found*/
	if( buf[2] != VBRTag[2] && buf[2] != VBRTag2[2]) return 0;
	if( buf[3] != VBRTag[3] && buf[3] != VBRTag2[3]) return 0;

	buf+=4;

	pTagData->h_id = h_id;

	head_flags = pTagData->flags = ExtractI4(buf); buf+=4;      /* get flags */

	if( head_flags & FRAMES_FLAG )
	{
		pTagData->frames   = ExtractI4(buf); buf+=4;
	}

	if( head_flags & BYTES_FLAG )
	{
		pTagData->bytes = ExtractI4(buf); buf+=4;
	}

	if( head_flags & TOC_FLAG )
	{
		if( pTagData->toc != NULL )
		{
			for(i=0;i<NUMTOCENTRIES;i++)
				pTagData->toc[i] = buf[i];
		}
		buf+=NUMTOCENTRIES;
	}

	pTagData->vbr_scale = -1;

	if( head_flags & VBR_SCALE_FLAG )
	{
		pTagData->vbr_scale = ExtractI4(buf); buf+=4;
	}

	pTagData->headersize = 
	  ((h_id+1)*72000*h_bitrate) / pTagData->samprate;

        buf+=21;
        enc_delay = buf[0] << 4;
        enc_delay += buf[1] >> 4;
        enc_padding= (buf[1] & 0x0F)<<8;
        enc_padding += buf[2];
        // check for reasonable values (this may be an old Xing header,
        // not a INFO tag)
        if (enc_delay<0 || enc_delay > 3000) enc_delay=-1;
        if (enc_padding<0 || enc_padding > 3000) enc_padding=-1;

        pTagData->enc_delay=enc_delay;
        pTagData->enc_padding=enc_padding;

#ifdef HIP_DEBUG
	fprintf(stderr,"exit GetVbrTag:\n");
	fprintf(stderr,"  tag         = %s\n",VBRTag);
	fprintf(stderr,"  head_flags  = %d\n",head_flags);
	fprintf(stderr,"  bytes       = %d\n",pTagData->bytes);
	fprintf(stderr,"  frames      = %d\n",pTagData->frames);
	fprintf(stderr,"  VBR Scale   = %d\n",pTagData->vbr_scale);
  fprintf(stderr,"  enc_delay   = %i \n",enc_delay);
  fprintf(stderr,"  enc_padding = %i \n",enc_padding);
	fprintf(stderr,"  toc         =\n");
	if( pTagData->toc != NULL )
	{
		for(i=0;i<NUMTOCENTRIES;i++)
		{
			if( (i%10) == 0 ) fprintf(stderr,"\n");
			fprintf(stderr," %3d", (int)(pTagData->toc[i]));
		}
	}
	fprintf(stderr,"\n");
#endif
	return 1;       /* success */
}
