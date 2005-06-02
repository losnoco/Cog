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


#ifndef _MPGLIB_H_
#define _MPGLIB_H_

/*
#ifndef NOANALYSIS
extern plotting_data *mpg123_pinfo;
#endif
*/

struct buf {
    unsigned char *pnt;
    long size;
    long pos;
    struct buf *next;
    struct buf *prev;
};

struct framebuf {
	struct buf *buf;
	long pos;
	struct frame *next;
	struct frame *prev;
};

typedef struct mpstr_tag {
	struct buf *head,*tail;
        int vbr_header;               /* 1 if valid Xing vbr header detected */
        int num_frames;               /* set if vbr header present */
        int enc_delay;                /* set if vbr header present */
        int enc_padding;              /* set if vbr header present */
        int header_parsed;
        int side_parsed;  
        int data_parsed;  
        int free_format;             /* 1 = free format frame */
        int old_free_format;        /* 1 = last frame was free format */
	int bsize;
	int framesize;
	int ssize;
	int dsize;
        int fsizeold;
        int fsizeold_nopadding;
	struct frame fr;
        unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
	real hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	unsigned long header;
	int bsnum;
	real synth_buffs[2][2][0x110];
        int  synth_bo;
        int  sync_bitstream;
	
} MPSTR, *PMPSTR;


#if ( defined(_MSC_VER) || defined(__BORLANDC__) )
	typedef int BOOL; /* windef.h contains the same definition */
#else
	#define BOOL int
#endif

#define MP3_ERR -1
#define MP3_OK  0
#define MP3_NEED_MORE 1



#endif /* _MPGLIB_H_ */
