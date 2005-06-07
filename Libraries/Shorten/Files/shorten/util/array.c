/******************************************************************************
*                                                                             *
*  Copyright (C) 1992-1995 Tony Robinson                                      *
*                                                                             *
*  See the file doc/LICENSE.shorten for conditions on distribution and usage  *
*                                                                             *
******************************************************************************/

/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include "mkbshift.h"

void *pmalloc(size) ulong size; {
  void *ptr;

#if defined(DOS_MALLOC_FEATURE) && !defined(_WINDOWS)	/* mrhmod */
  fprintf(stderr, "requesting %ld bytes: ", size);
#endif
  ptr = malloc(size);
#if defined(DOS_MALLOC_FEATURE) && !defined(_WINDOWS)	/* mrhmod */
  if(ptr == NULL)
    fprintf(stderr, "denied\n");
  else
    fprintf(stderr, "accepted\n");
#endif

  if(ptr == NULL)
    perror_exit("call to malloc(%ld) failed in pmalloc()", size);

  return(ptr);
}

slong **long2d(n0, n1) ulong n0, n1; {
  slong **array0;

  if((array0 = (slong**) pmalloc((ulong) (n0 * sizeof(slong*) +
					 n0 * n1 * sizeof(slong)))) != NULL ) {
    slong *array1 = (slong*) (array0 + n0);
    int i;

    for(i = 0; i < n0; i++)
      array0[i] = array1 + i * n1;
  }
  return(array0);
}

float **float2d(n0, n1) ulong n0, n1; {
  float **array0;

  if((array0 = (float**) pmalloc((ulong) (n0 * sizeof(float*) +
					 n0 * n1 * sizeof(float)))) != NULL ) {
    float *array1 = (float*) (array0 + n0);
    int i;

    for(i = 0; i < n0; i++)
      array0[i] = array1 + i * n1;
  }
  return(array0);
}
