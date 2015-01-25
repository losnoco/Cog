/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "complxop.h"

/*--------------------------------------------------------------------------*/
/*  Function  cadd                                                          */
/*  ~~~~~~~~~~~~~~                                                          */
/*                                                                          */
/*  Add complex variables                                                   */
/*--------------------------------------------------------------------------*/
/*  complex   var1 (i)  variable 1                                          */
/*  complex   var2 (i)  variable 2                                          */
/*  complex   *res (o)  result of addition                                  */
/*--------------------------------------------------------------------------*/
void cadd(complex var1, complex var2, complex *res)
{
    res->r = var1.r + var2.r;
    res->i = var1.i + var2.i;
}

/*--------------------------------------------------------------------------*/
/*  Function  csub                                                          */
/*  ~~~~~~~~~~~~~~                                                          */
/*                                                                          */
/*  Subtract complex variables                                              */
/*--------------------------------------------------------------------------*/
/*  complex   var1 (i)  variable 1                                          */
/*  complex   var2 (i)  variable 2                                          */
/*  complex   *res (o)  result of subtraction                               */
/*--------------------------------------------------------------------------*/
void csub(complex var1, complex var2, complex *res)
{
    res->r = var1.r - var2.r;
    res->i = var1.i - var2.i;
}

/*--------------------------------------------------------------------------*/
/*  Function  cmpys                                                         */
/*  ~~~~~~~~~~~~~~~                                                         */
/*                                                                          */
/*  Multiply complex variable with a scalar                                 */
/*--------------------------------------------------------------------------*/
/*  complex   var1 (i)  variable 1                                          */
/*  float     var2 (i)  variable 2                                          */
/*  complex   *res (o)  result                                              */
/*--------------------------------------------------------------------------*/
void cmpys(complex var1, float var2, complex *res)
{
    res->r = var1.r * var2;
    res->i = var1.i * var2;
}

/*--------------------------------------------------------------------------*/
/*  Function  cmpyj                                                         */
/*  ~~~~~~~~~~~~~~~                                                         */
/*                                                                          */
/*  Find orthogonal complex variable                                        */
/*--------------------------------------------------------------------------*/
/*  complex   *v1  (i/o)  variable                                          */
/*--------------------------------------------------------------------------*/
void cmpyj(complex* v1)
{
    float a;

    a = v1->r;
    v1->r = -v1->i;
    v1->i =  a;
}

