/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*  Function  reordvct                                                      */
/*  ~~~~~~~~~~~~~~~~~~                                                      */
/*                                                                          */
/*  Rearrange a vector in decreasing order                                  */
/*--------------------------------------------------------------------------*/
/*  short     *y    (i/o) vector to rearrange                               */
/*  short     N     (i)   dimensions                                        */
/*  short     *idx  (o)   reordered vector index                            */
/*--------------------------------------------------------------------------*/
void reordvct(short *y, short N, short *idx)
{
    short i, j, k, n, im, temp;

    n = N - 1;
    for (i=0; i<n; i++)
    {
       im = i;
       k = i + 1;
       for (j=k; j<N; j++)
       {
          temp = y[im] - y[j];
          if (temp<0)
          {
            im = j;
          }
       }
       temp = y[i];
       y[i] = y[im];
       y[im] = temp;
       j = idx[i];
       idx[i] = idx[im];
       idx[im] = j;
    }

    return;
}
