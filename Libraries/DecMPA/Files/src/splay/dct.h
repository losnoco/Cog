/*
  wrapper for dcts
  Copyright (C) 2001  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file License.txt in this package

 */


#ifndef __DCT_HEADER_H
#define __DCT_HEADER_H

// one source:
extern void initialize_dct64();

extern void dct64(REAL* out1,REAL* out2,REAL *fraction);

// one source:
extern void initialize_dct64_downsample();
extern void dct64_downsample(REAL* out1,REAL* out2,REAL *fraction);

// one source file:
extern void initialize_dct12_dct36();

extern void dct12(REAL *in,REAL *prevblk1,REAL *prevblk2,REAL *wi,REAL *out);
extern void dct36(REAL *inbuf,REAL *prevblk1,REAL *prevblk2,REAL *wi,REAL *out);

extern void initialize_win();
#endif
