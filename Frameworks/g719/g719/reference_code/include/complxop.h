/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#ifndef _COMPLXOP_H
#define _COMPLXOP_H

/*******************************************************************************
/* Local type definitions
/******************************************************************************/

typedef struct complex_struct
{
    float r;
    float i;
} complex;

void cadd(complex var1, complex var2, complex *res);
void csub(complex var1, complex var2, complex *res);
void cmpys(complex var1, float var2, complex *res);
void cmpyj(complex* v1);

#endif
