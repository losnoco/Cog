/*--------------------------------------------------------------------------*/
/* ITU-T G.722.1 Fullband Extension Annex X. Source Code                    */
/* © 2008 Ericsson AB. and Polycom Inc.                                     */
/* All rights reserved.                                                     */
/*--------------------------------------------------------------------------*/

#ifndef _CNST_H
#define _CNST_H


#define CODEC_VERSION   "0.5b"

#define MAX16B    (short)0x7fff
#define MIN16B    (short)0x8000
#define EPSILON   0.000000000000001f
#define PI        3.141592653589793238462f
#define TRUE      1
#define FALSE     0

#define FRAME_LENGTH                960
#define MAX_SEGMENT_LENGTH          480
#define MAX_BITS_PER_FRAME          2560
#define NUM_TIME_SWITCHING_BLOCKS   4
#define NUM_MAP_BANDS               20
#define FREQ_LENGTH                 800

#define G192_SYNC_GOOD_FRAME (unsigned short) 0x6B21 
#define G192_SYNC_BAD_FRAME  (unsigned short) 0x6B20 
#define G192_BIT0            (unsigned short) 0x007F 
#define G192_BIT1            (unsigned short) 0x0081 

#define  MLT960_LENGTH              960
#define  MLT960_LENGTH_DIV_2        480
#define  MLT960_LENGTH_MINUS_1      959
#define  MLT240_LENGTH              240
#define  MLT240_LENGTH_MINUS_1      239

#define  STOP_BAND      800
#define  STOP_BAND4     200

#define  SFM_G1         16
#define  SFM_G2         8
#define  SFM_G1G2       24
#define  SFM_G3         12
#define  SFM_N          36
#define  SFM_GX         8
#define  NB_SFM         44
#define  WID_G1         8
#define  WID_G2         16
#define  WID_G3         24
#define  WID_GX         32
#define  NUMC_G1        128
#define  NUMC_G1G2      256
#define  NUMC_N         544
#define  NB_VECT1       1
#define  NB_VECT2       2
#define  NB_VECT3       3
#define  NB_VECTX       4

#define  NUMC_G23       256
#define  NUMC_G1SUB     32
#define  NUMC_G1G2SUB   64
#define  NUMC_G1G2G3SUB 136

#define  QBIT_MAX1      5
#define  QBIT_MAX2      9
#define  OFFSETf        0.015625f
#define  FCT_LVQ1f      1.1f
#define  FCT_LVQ2f      6.0f
#define  FCT_LVQ2fQ13   1365.333333f
#define  MAX_DIST       100000000.0f
#define  N_LEADER1      10

#define  FLAGS_BITS     3
#define  NORM0_BITS     5
#define  NORMI_BITS     5
#define  NUMNRMIBITS    215

#define  NOALLGROUPS    0
#define  ALLGROUPS      1

#define  NOHUFCODE      0
#define  HUFCODE        1


#include "rom.h"
#endif

