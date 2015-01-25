/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#include "cnst.h"
#include "rom.h"

/*--------------------------------------------------------------------------*/
/*  Function  recovernorm                                                   */
/*  ~~~~~~~~~~~~~~~~~~~~~                                                   */
/*                                                                          */
/*  Recover reordered quantization indices and norms                        */
/*--------------------------------------------------------------------------*/
/*  short     *idxbuf    (i)   reordered quantization indices               */
/*  short     *ynrm      (o)   recovered quantization indices               */
/*  short     *normqlg2  (o)   recovered quantized norms                    */
/*--------------------------------------------------------------------------*/
void recovernorm(
short *idxbuf,
short *ynrm,
short *normqlg2
)
{
    short i;
    short *pidx, *pnormq;


    for (i=0; i<2; i++)
    {
       pidx = ynrm + i;
       pnormq = normqlg2 + i;
       *pidx = idxbuf[i];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx = idxbuf[21-i];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx = idxbuf[22+i];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx = idxbuf[43-i];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx  = idxbuf[i+2];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx = idxbuf[19-i];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx = idxbuf[24+i];
       *pnormq = dicnlg2[*pidx];
       pidx += 2;
       pnormq += 2;
       *pidx = idxbuf[41-i];
       *pnormq = dicnlg2[*pidx];
    }

    pidx = ynrm + 16;
    pnormq = normqlg2 + 16;
    for (i=4; i<(NB_SFM/4); i++)
    {
       *pidx = idxbuf[i];
       *pnormq++ = dicnlg2[*pidx++];
       *pidx = idxbuf[21-i];
       *pnormq++ = dicnlg2[*pidx++];
       *pidx = idxbuf[22+i];
       *pnormq++ = dicnlg2[*pidx++];
       *pidx = idxbuf[43-i];
       *pnormq++ = dicnlg2[*pidx++];
    }

    return;
}
