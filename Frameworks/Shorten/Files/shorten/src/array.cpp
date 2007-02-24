/*
 *  array.cpp
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
 * $Id: array.c,v 1.6 2001/12/30 05:12:04 jason Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "shn_reader.h"

void *shn_reader::pmalloc(ulong size) {
  void *ptr;

  ptr = malloc(size);

  if(ptr == NULL)
  	mFatalError	= true;

  return(ptr);
}

slong **shn_reader::long2d(ulong n0, ulong n1) {
  slong **array0 = NULL;

  if((array0 = (slong**) pmalloc((ulong) (n0 * sizeof(slong*) +
					 n0 * n1 * sizeof(slong)))) != NULL ) {
    slong *array1 = (slong*) (array0 + n0);
    ulong i;

    for(i = 0; i < n0; i++)
      array0[i] = array1 + i * n1;
  }
  return(array0);
}

