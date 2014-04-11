/*
** Changes for the 1.4 release are commented. You can do
** a search for "1.4" and merge them into your own replay
** code.
**
** Changes for 1.5 are marked also.
**
** ... as are those for 1.6
**
** ... and for 1.8
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hvl_replay.h"

// USE_HVL_FILTER_FOR_HVL - see hvl_replay.h - define there to use original hvl filter gen function for hvl files

#undef min
#define min(a,b) (((a) < (b)) ? (a) : (b))

const int32 stereopan_left[]  = { 128,  96,  64,  32,   0 };
const int32 stereopan_right[] = { 128, 160, 193, 225, 255 };

/*
** Waves
*/
#define WHITENOISELEN (0x280*3)

#define WO_LOWPASSES   0
#define WO_TRIANGLE_04 (WO_LOWPASSES+((0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31))
#define WO_TRIANGLE_08 (WO_TRIANGLE_04+0x04)
#define WO_TRIANGLE_10 (WO_TRIANGLE_08+0x08)
#define WO_TRIANGLE_20 (WO_TRIANGLE_10+0x10)
#define WO_TRIANGLE_40 (WO_TRIANGLE_20+0x20)
#define WO_TRIANGLE_80 (WO_TRIANGLE_40+0x40)
#define WO_SAWTOOTH_04 (WO_TRIANGLE_80+0x80)
#define WO_SAWTOOTH_08 (WO_SAWTOOTH_04+0x04)
#define WO_SAWTOOTH_10 (WO_SAWTOOTH_08+0x08)
#define WO_SAWTOOTH_20 (WO_SAWTOOTH_10+0x10)
#define WO_SAWTOOTH_40 (WO_SAWTOOTH_20+0x20)
#define WO_SAWTOOTH_80 (WO_SAWTOOTH_40+0x40)
#define WO_SQUARES     (WO_SAWTOOTH_80+0x80)
#define WO_WHITENOISE  (WO_SQUARES+(0x80*0x20))
#define WO_HIGHPASSES  (WO_WHITENOISE+WHITENOISELEN)
#define WAVES_SIZE     (WO_HIGHPASSES+((0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31))

int8 waves[WAVES_SIZE];
#ifdef USE_HVL_FILTER_FOR_HVL
int8 ahxwaves[WAVES_SIZE];
#endif

const int16 vib_tab[] =
{ 
  0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
  253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
  0,-24,-49,-74,-97,-120,-141,-161,-180,-197,-212,-224,-235,-244,-250,-253,-255,
  -253,-250,-244,-235,-224,-212,-197,-180,-161,-141,-120,-97,-74,-49,-24
};

const uint16 period_tab[] =
{
  0x0000, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970,
  0x08E8, 0x0868, 0x07F0, 0x0780, 0x0714, 0x06B0, 0x0650, 0x05F4,
  0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0,
  0x038A, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C,
  0x023A, 0x021A, 0x01FC, 0x01E0, 0x01C5, 0x01AC, 0x0194, 0x017D,
  0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
  0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097,
  0x008F, 0x0087, 0x007F, 0x0078, 0x0071
};

uint32 panning_left[256], panning_right[256];

void hvl_GenPanningTables( void )
{
  uint32 i;
  float64 aa, ab;

  // Sine based panning table
  aa = (3.14159265f*2.0f)/4.0f;   // Quarter of the way through the sinewave == top peak
  ab = 0.0f;                      // Start of the climb from zero

  for( i=0; i<256; i++ )
  {
    panning_left[i]  = (uint32)(sin(aa)*255.0f);
    panning_right[i] = (uint32)(sin(ab)*255.0f);
    
    aa += (3.14159265*2.0f/4.0f)/256.0f;
    ab += (3.14159265*2.0f/4.0f)/256.0f;
  }
  panning_left[255] = 0;
  panning_right[0] = 0;
}

void hvl_GenSawtooth( int8 *buf, uint32 len )
{
  uint32 i;
  int32  val, add;
  
  add = 256 / (len-1);
  val = -128;
  
  for( i=0; i<len; i++, val += add )
    *buf++ = (int8)val;  
}

void hvl_GenTriangle( int8 *buf, uint32 len )
{
  uint32 i;
  int32  d2, d5, d1, d4;
  int32  val;
  int8   *buf2;
  
  d2  = len;
  d5  = len >> 2;
  d1  = 128/d5;
  d4  = -(d2 >> 1);
  val = 0;
  
  for( i=0; i<d5; i++ )
  {
    *buf++ = val;
    val += d1;
  }
  *buf++ = 0x7f;

  if( d5 != 1 )
  {
    val = 128;
    for( i=0; i<d5-1; i++ )
    {
      val -= d1;
      *buf++ = val;
    }
  }
  
  buf2 = buf + d4;
  for( i=0; i<d5*2; i++ )
  {
    int8 c;
    
    c = *buf2++;
    if( c == 0x7f )
      c = 0x80;
    else
      c = -c;
    
    *buf++ = c;
  }
}

void hvl_GenSquare( int8 *buf )
{
  uint32 i, j;
  
  for( i=1; i<=0x20; i++ )
  {
    for( j=0; j<(0x40-i)*2; j++ )
      *buf++ = 0x80;
    for( j=0; j<i*2; j++ )
      *buf++ = 0x7f;
  }
}

static float64 clip( float64 x )
{
  if( x > 127.f )
    x = 127.f;
  else if( x < -128.f )
    x = -128.f;
  return x;
}

#ifdef USE_HVL_FILTER_FOR_HVL
void hvl_GenFilterWaves( int8 *buf, int8 *lowbuf, int8 *highbuf )
{
  static const uint16 lentab[45] = { 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    (0x280*3)-1 };

  float64 freq;
  uint32  temp;
  
  for( temp=0, freq=8.3115f; temp<31; temp++, freq+=3.f )
  {
    uint32 wv;
    int8   *a0 = buf;
    
    for( wv=0; wv<6+6+0x20+1; wv++ )
    {
      float64 fre, high, mid, low;
      uint32  i;
      
      mid = 0.f;
      low = 0.f;
      fre = freq * 1.17250f / 100.0f;
      
      for( i=0; i<=lentab[wv]; i++ )
      {
        high  = a0[i] - mid - low;
        high  = clip( high );
        mid  += high * fre;
        mid   = clip( mid );
        low  += mid * fre;
        low   = clip( low );
      }
      
      for( i=0; i<=lentab[wv]; i++ )
      {
        high  = a0[i] - mid - low;
        high  = clip( high );
        mid  += high * fre;
        mid   = clip( mid );
        low  += mid * fre;
        low   = clip( low );
        *lowbuf++  = (int8)low;
        *highbuf++ = (int8)high;
      }
      
      a0 += lentab[wv]+1;
    }
  }
}
#endif

static const int16 filter_thing[] =
{
    0xFB77, 0xEEC3, 0xE407, 0xCCDA, 0x027B, 0x33C7, 0x088D, 0x1901,
    0x2351, 0x3F02, 0x3494, 0x14F0, 0x18CD, 0x319B, 0x4A69, 0x6336,
    0x7700, 0x7F00, 0x7F00, 0x7F03, 0x7B89, 0x743C, 0x6A16, 0x5DFC,
    0x50BB, 0x4304, 0x3692, 0x2C6F, 0x242F, 0x1D77, 0x17FE, 0x138A,
    0x0FEA, 0x0CF6, 0x0A8E, 0x0882, 0x06DB, 0x0587, 0x0475, 0x038D,
    0x02CC, 0x0233, 0x01BC, 0x014B, 0xFD67, 0xF7DE, 0xE7E6, 0xDBED,
    0xCACA, 0x3101, 0x2591, 0x0F6F, 0x2099, 0x2BEE, 0x4836, 0x1B05,
    0x0F08, 0x21BB, 0x4377, 0x6533, 0x7DA3, 0x7F00, 0x7EC7, 0x780E,
    0x6B20, 0x5A61, 0x47DD, 0x362D, 0x28BD, 0x1EA3, 0x1709, 0x1153,
    0x0D07, 0x09CB, 0x075D, 0x056D, 0x03FF, 0x02D0, 0x0212, 0x0161,
    0x0104, 0x00AD, 0x0060, 0x0020, 0xFFEE, 0xFFC9, 0xFFB1, 0xFFA4,
    0xFFA1, 0xFCBA, 0xF363, 0xE37E, 0xCF9E, 0xE43D, 0x367A, 0x1965,
    0x1752, 0x23AD, 0x3A63, 0x41F1, 0x17C1, 0x0BE8, 0x2AA9, 0x5553,
    0x7A8B, 0x7F00, 0x7D44, 0x70C0, 0x5C86, 0x4508, 0x2FC9, 0x2115,
    0x16E6, 0x0FDA, 0x0AF9, 0x0798, 0x0542, 0x0384, 0x0259, 0x0173,
    0x00DF, 0x0089, 0x0040, 0x0007, 0xFFDE, 0xFFC6, 0xFFBB, 0xFFBA,
    0xFFC1, 0xFFCC, 0xFFD9, 0xFFE6, 0xFFF2, 0xFFFB, 0x1378, 0xEE84,
    0xE05A, 0xC5D4, 0x0B4E, 0x31B3, 0x1313, 0x1F4A, 0x2616, 0x45DF,
    0x2E0E, 0x13EB, 0x09D8, 0x3397, 0x672F, 0x7F00, 0x7EC9, 0x7012,
    0x564D, 0x3949, 0x2460, 0x1719, 0x0EAA, 0x0950, 0x05E9, 0x038F,
    0x0224, 0x014A, 0x008F, 0x0003, 0xFFAA, 0xFF7E, 0xFF75, 0xFF83,
    0xFF9F, 0xFFBF, 0xFFDD, 0xFFF5, 0x0006, 0x000F, 0x0013, 0x0013,
    0x0010, 0x000C, 0x0008, 0x1ADD, 0xE985, 0xDC57, 0xC2A3, 0x25E9,
    0x2A8D, 0x103D, 0x269A, 0x2A91, 0x4B24, 0x1FD9, 0x10BD, 0x0865,
    0x3C85, 0x779A, 0x7F00, 0x760C, 0x599E, 0x377B, 0x2031, 0x12AD,
    0x0AD6, 0x0649, 0x03A5, 0x01F5, 0x00DC, 0x0051, 0x0023, 0x0002,
    0xFFEE, 0xFFE6, 0xFFE7, 0xFFEC, 0xFFF3, 0xFFF9, 0xFFFF, 0x0002,
    0x0004, 0x0004, 0x0003, 0x0002, 0x0001, 0x0000, 0x0000, 0xFFFF,
    0x097F, 0xE4D4, 0xD636, 0xC6FE, 0x31B0, 0x2314, 0x0E82, 0x2A8C,
    0x314E, 0x4C62, 0x1B03, 0x0EA1, 0x0750, 0x4573, 0x7F00, 0x7F6E,
    0x66AE, 0x3FAE, 0x219D, 0x11BE, 0x095D, 0x04F1, 0x0257, 0x011B,
    0x002D, 0xFFA4, 0xFF73, 0xFF7D, 0xFFA3, 0xFFCF, 0xFFF2, 0x0008,
    0x0012, 0x0012, 0x000E, 0x0008, 0x0003, 0x0000, 0xFFFE, 0xFFFD,
    0xFFFE, 0xFFFE, 0xFFFF, 0x0000, 0x0000, 0xF1BA, 0xE0B8, 0xCE39,
    0xD4B0, 0x3539, 0x1CAE, 0x0D02, 0x2C42, 0x3A0B, 0x4951, 0x1954,
    0x0CF7, 0x067C, 0x4E61, 0x7F00, 0x77EB, 0x5274, 0x2978, 0x13D3,
    0x0979, 0x0487, 0x01DD, 0x00C4, 0x0001, 0xFFA3, 0xFF93, 0xFFAE,
    0xFFD4, 0xFFF4, 0x0007, 0x000E, 0x000D, 0x0009, 0x0004, 0x0000,
    0xFFFE, 0xFFFE, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0xE1AB, 0xDD5D, 0xC593, 0xE91A, 0x34EE, 0x17FB,
    0x0BAC, 0x2C14, 0x429E, 0x40DA, 0x1781, 0x0BA3, 0x05D1, 0x574F,
    0x7F00, 0x6DB3, 0x3CD8, 0x1A34, 0x0B48, 0x04DB, 0x0217, 0x00BC,
    0x0020, 0xFFD1, 0xFFC0, 0xFFD1, 0xFFEA, 0xFFFD, 0x0007, 0x0008,
    0x0005, 0x0003, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xDC89,
    0xDAC4, 0xBDC0, 0xFEB1, 0x32C9, 0x14D5, 0x0A90, 0x2BB8, 0x4936,
    0x3581, 0x1551, 0x0A8F, 0x0547, 0x603D, 0x7F00, 0x5FEC, 0x2A63,
    0x1059, 0x064E, 0x026E, 0x00B8, 0x000F, 0xFFC7, 0xFFC5, 0xFFDE,
    0xFFF7, 0x0005, 0x0008, 0x0006, 0x0002, 0x0000, 0xFFFF, 0xFFFF,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0xDE80, 0xD8C5, 0xB789, 0x1114,
    0x2F9E, 0x12C9, 0x09A8, 0x2BDE, 0x4D5B, 0x2BA2, 0x1359, 0x09A9,
    0x04D4, 0x692B, 0x7F00, 0x5057, 0x1D06, 0x09F6, 0x036B, 0x00D4,
    0x0033, 0xFFE2, 0xFFD5, 0xFFE7, 0xFFFA, 0x0003, 0x0005, 0x0003,
    0x0001, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0xE4D0, 0xD70B, 0xB2E4, 0x1EB8, 0x2BD7, 0x1161, 0x08E7,
    0x2D67, 0x4F9C, 0x2510, 0x11C9, 0x08E8, 0x0474, 0x7219, 0x7C55,
    0x3F6B, 0x133C, 0x05D5, 0x01C4, 0x0056, 0xFFF9, 0xFFE0, 0xFFEC,
    0xFFFB, 0x0002, 0x0003, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xED7D, 0xD522,
    0xB289, 0x2800, 0x27CE, 0x1049, 0x0842, 0x30EC, 0x50A9, 0x2153,
    0x1082, 0x0842, 0x0421, 0x7B07, 0x73E8, 0x2E8C, 0x0C60, 0x0349,
    0x0079, 0x0011, 0xFFEA, 0xFFEE, 0xFFFB, 0x0002, 0x0002, 0x0001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0xF6E3, 0xD287, 0xB4A8, 0x2DF5, 0x23D7,
    0x0F53, 0x07B3, 0x3641, 0x50A6, 0x1F47, 0x0F66, 0x07B3, 0x03D9,
    0x7F00, 0x6B22, 0x20FE, 0x079D, 0x01C1, 0x002D, 0xFFF5, 0xFFF0,
    0xFFFB, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x001D, 0xCEB8, 0xB9CD, 0x3192, 0x2037, 0x0E6D, 0x0736, 0x3D2E,
    0x4F2F, 0x1DEA, 0x0E6C, 0x0736, 0x039B, 0x7F00, 0x622C, 0x188C,
    0x04DD, 0x00F6, 0xFFB9, 0xFFB2, 0xFFEF, 0x0008, 0x0007, 0x0001,
    0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x08B8, 0xC94F, 0xC47E,
    0x338E, 0x1D17, 0x0D96, 0x06C8, 0x4401, 0x4BD3, 0x1CA4, 0x0D90,
    0x06C8, 0x0364, 0x7F00, 0x5811, 0x1100, 0x02DB, 0x0012, 0xFF8B,
    0xFFD8, 0x0008, 0x0009, 0x0002, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x1065, 0xC224, 0xD328, 0x3460, 0x1A87, 0x0CD1,
    0x0667, 0x4B18, 0x469A, 0x1B42, 0x0CCC, 0x0667, 0x0333, 0x7F00,
    0x4CC9, 0x0A92, 0x017C, 0xFF6C, 0xFFAA, 0x0002, 0x000D, 0x0003,
    0xFFFE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x16E7,
    0xBA2A, 0xDB0C, 0x344D, 0x187E, 0x0C20, 0x060F, 0x5204, 0x402F,
    0x19D8, 0x0C1E, 0x060F, 0x0308, 0x7F00, 0x40F9, 0x0781, 0x00DD,
    0xFFA1, 0xFFD9, 0x0005, 0x0005, 0x0000, 0xFFFF, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x1C0C, 0xB0D2, 0xE7CE, 0x337D,
    0x16EA, 0x0B82, 0x05C1, 0x5814, 0x399A, 0x1881, 0x0B82, 0x05C1,
    0x02E1, 0x7F00, 0x3535, 0x04EF, 0x0074, 0xFFCB, 0xFFF1, 0x0004,
    0x0002, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x1FB5, 0xAD77, 0xF515, 0x3209, 0x15AE, 0x0AF4, 0x057A,
    0x5CA5, 0x340E, 0x174A, 0x0AF4, 0x057A, 0x02BD, 0x7F00, 0x29BF,
    0x0308, 0xFFC8, 0xFFC8, 0x0004, 0x0004, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2170, 0xB20F,
    0x01FC, 0x300B, 0x14AF, 0x0A73, 0x0539, 0x6215, 0x2FE7, 0x1634,
    0x0A73, 0x053A, 0x029D, 0x7F00, 0x1EE1, 0x01B1, 0xFFDC, 0xFFEA,
    0x0003, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x2128, 0xB6D5, 0x0E58, 0x2D9F, 0x13D8,
    0x09FE, 0x04FF, 0x68E7, 0x2CD8, 0x153A, 0x09FD, 0x04FF, 0x027F,
    0x7F00, 0x14ED, 0x00D4, 0xFFA1, 0x0000, 0x0004, 0xFFFF, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x1F29, 0xA209, 0x1989, 0x2AEA, 0x1313, 0x0992, 0x04C9, 0x6FC3,
    0x2AA6, 0x1456, 0x0992, 0x04C9, 0x0264, 0x7F00, 0x0C3B, 0x0053,
    0xFFDD, 0x0002, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1BB0, 0x8888, 0x235E,
    0x2819, 0x1258, 0x092F, 0x0498, 0x7023, 0x28FE, 0x1384, 0x092F,
    0x0497, 0x024C, 0x7F00, 0x0780, 0xFF65, 0xFFF3, 0x0004, 0xFFFF,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x1740, 0x808D, 0x2BF1, 0x255C, 0x11A7, 0x08D4,
    0x046A, 0x7006, 0x2781, 0x12C3, 0x08D4, 0x046A, 0x0235, 0x7F00,
    0x0423, 0xFFB7, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1215,
    0x800F, 0x338F, 0x22E6, 0x10FF, 0x087F, 0x0440, 0x6F1E, 0x262F,
    0x120F, 0x087F, 0x043F, 0x0220, 0x7F00, 0x01B2, 0xFFEA, 0x0001,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0C3C, 0x8000, 0x3B79, 0x20EE,
    0x1062, 0x0831, 0x0419, 0x780C, 0x24DF, 0x1168, 0x0831, 0x0419,
    0x020C, 0x7F00, 0x004B, 0xFFFA, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0541, 0x8000, 0x417D, 0x1FAB, 0x0FD0, 0x07E9, 0x03F4,
    0x7F00, 0x2398, 0x10CE, 0x07E9, 0x03F4, 0x01FA, 0x7E81, 0x0188,
    0x0005, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFD3E, 0x8000,
    0x45D7, 0x1F45, 0x0F49, 0x07A4, 0x03D2, 0x7F00, 0x228B, 0x103D,
    0x07A5, 0x03D2, 0x01E9, 0x79D0, 0x0687, 0x007A, 0x000A, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0xF416, 0x8000, 0x49EB, 0x1FE3, 0x0ED7,
    0x0765, 0x03B2, 0x7F00, 0x21A5, 0x0FB6, 0x0765, 0x03B1, 0x01D9,
    0x74CF, 0x0C02, 0x013C, 0x0034, 0x000B, 0x0003, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xE94C, 0x8000, 0x4D8B, 0x21B2, 0x0E9B, 0x0729, 0x0395, 0x7F00,
    0x1F2E, 0x0F31, 0x0729, 0x0394, 0x01CB, 0x6F7D, 0x11D7, 0x02DB,
    0x00CE, 0x0042, 0x0017, 0x0008, 0x0001, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xDBED, 0x8000, 0x506B,
    0x24C0, 0x0F01, 0x06F8, 0x0379, 0x7F00, 0x1956, 0x0E68, 0x06F0,
    0x0379, 0x01BF, 0x69DB, 0x17E0, 0x0563, 0x0139, 0x0087, 0x0041,
    0x0021, 0x0011, 0x0007, 0x0004, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0xCE57, 0x04A4, 0x0526, 0xFB66, 0xEF30, 0x9930,
    0xC5AD, 0xF94C, 0xFA32, 0x09BE, 0x0E1B, 0x5703, 0x6B3A, 0x83A1,
    0x8C1C, 0x996E, 0xAB98, 0xC1F8, 0xDAA8, 0xF375, 0x0C42, 0x2499,
    0x3BB1, 0x50ED, 0x63E3, 0x744E, 0x7F00, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7FEC, 0x7F58, 0x7FEE, 0x7F36, 0x7F93, 0x7FDA, 0x22B6,
    0x06E2, 0x01FA, 0xF97F, 0xD0B0, 0x9FBF, 0xDEBF, 0xF750, 0x00F7,
    0x0CD8, 0x26C6, 0x64BB, 0x70ED, 0x86B8, 0x9666, 0xAF0A, 0xCF0C,
    0xF0C8, 0x127D, 0x32F3, 0x5058, 0x6982, 0x7DD7, 0x7F00, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7FFA, 0x7FB8, 0x7F87,
    0x7F00, 0x7F99, 0x7F00, 0x7F4B, 0x7F7E, 0x7F9D, 0x7FA9, 0x7FA7,
    0x7F9B, 0x7F88, 0x7F70, 0x7F56, 0x1827, 0x085D, 0xFD8A, 0xF58E,
    0xAAA0, 0xB87E, 0xE9B1, 0xF78F, 0x089F, 0x1097, 0x44D0, 0x676B,
    0x7417, 0x8ABF, 0xA3D4, 0xCA53, 0xF4FD, 0x1F5D, 0x4681, 0x6766,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7FE2,
    0x7F97, 0x7F64, 0x7FF4, 0x7F25, 0x7F5A, 0x7F75, 0x7F7C, 0x7F74,
    0x7F62, 0x7F4C, 0x7F34, 0x7F1E, 0x7F0C, 0x7EFE, 0x7EF4, 0x7EEE,
    0x7EEC, 0x0E14, 0x08C8, 0xFA29, 0xEA14, 0x9750, 0xCB17, 0xED77,
    0xFA92, 0x0D73, 0x1B3D, 0x5BC7, 0x6C4C, 0x7626, 0x8FB7, 0xB465,
    0xE741, 0x1ACD, 0x4A39, 0x7012, 0x7F00, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F9B, 0x7FE7, 0x7FF5, 0x7FDA,
    0x7FA9, 0x7F70, 0x7F3C, 0x7F11, 0x7EF4, 0x7EE2, 0x7EDC, 0x7EDD,
    0x7EE2, 0x7EE9, 0x7EF1, 0x7EF8, 0x7EFD, 0x7F01, 0x1F29, 0x07B7,
    0xF8BB, 0xD9E8, 0x9ADB, 0xD85A, 0xEF6F, 0x0056, 0x105E, 0x2993,
    0x6293, 0x6F41, 0x779A, 0x95A1, 0xC7C2, 0x0448, 0x3E6E, 0x6CE0,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7FDE, 0x7F77, 0x7FFF,
    0x7F13, 0x7F29, 0x7F2D, 0x7F27, 0x7F1B, 0x7F0F, 0x7F04, 0x7EFD,
    0x7EF9, 0x7EF8, 0x7EF9, 0x7EFA, 0x7EFC, 0x7EFE, 0x7EFF, 0x7F00,
    0x7F00, 0x7F00, 0x7EFF, 0x38C1, 0x056D, 0xF814, 0xC623, 0xA713,
    0xE15B, 0xF142, 0x07C2, 0x1262, 0x38E2, 0x63C5, 0x715B, 0x78AE,
    0x9C7C, 0xDBB2, 0x2144, 0x5DAB, 0x7F00, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7F00, 0x7FC4, 0x7FFA, 0x7FD7, 0x7F8E, 0x7F43, 0x7F0A,
    0x7EEA, 0x7EDE, 0x7EE0, 0x7EE8, 0x7EF2, 0x7EFB, 0x7F00, 0x7F03,
    0x7F03, 0x7F02, 0x7F01, 0x7F00, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x44FF, 0x022D, 0xF638, 0xB1EC, 0xB3D3, 0xE6DC, 0xF2F4, 0x1027,
    0x1555, 0x4964, 0x65A0, 0x7308, 0x7983, 0xA447, 0xEFC0, 0x3BF3,
    0x755D, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7FA2, 0x7FBC,
    0x7F8E, 0x7F4A, 0x7F13, 0x7EF2, 0x7EE6, 0x7EE8, 0x7EF0, 0x7EF8,
    0x7EFE, 0x7F01, 0x7F02, 0x7F01, 0x7F00, 0x7EFF, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x3F9E, 0xFE6E, 0xF23E,
    0xA271, 0xBEFF, 0xEA01, 0xF459, 0x186B, 0x1AB5, 0x58FD, 0x6858,
    0x745C, 0x7A2D, 0xAD04, 0x0454, 0x552E, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7FB7, 0x7F40, 0x7F6E, 0x7F58, 0x7F2D, 0x7F08, 0x7EF5,
    0x7EF0, 0x7EF4, 0x7EF9, 0x7EFD, 0x7F00, 0x7F00, 0x7F00, 0x7EFF,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x347C, 0xFAB9, 0xED47, 0x9AA4, 0xC870, 0xEBE1,
    0xF572, 0x1E07, 0x2265, 0x6469, 0x6AD4, 0x7570, 0x7AB7, 0xB6B2,
    0x16F0, 0x681C, 0x7F00, 0x7F00, 0x7F00, 0x7FEB, 0x7F45, 0x7F6C,
    0x7F4A, 0x7F1C, 0x7EFC, 0x7EF1, 0x7EF3, 0x7EF9, 0x7EFE, 0x7F00,
    0x7F00, 0x7F00, 0x7EFF, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x28BB,
    0xF78E, 0xE420, 0x99DD, 0xD05D, 0xED36, 0xF656, 0x2213, 0x2B5A,
    0x6A73, 0x6CB8, 0x7655, 0x7B2A, 0xC151, 0x2A7D, 0x790E, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F48, 0x7F3B, 0x7F18, 0x7EFF, 0x7EF6,
    0x7EF7, 0x7EFB, 0x7EFE, 0x7F00, 0x7F00, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x1E75, 0xF544, 0xD834, 0x9DAA,
    0xD6E9, 0xEE5D, 0xF717, 0x24B5, 0x34A1, 0x6D21, 0x6E35, 0x7717,
    0x7B8B, 0xCCE1, 0x3DC8, 0x7F00, 0x7F00, 0x7F00, 0x7FCB, 0x7F26,
    0x7F30, 0x7F16, 0x7F00, 0x7EF9, 0x7EFA, 0x7EFC, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x16D0, 0xF3F4, 0xCBF1, 0xA3BF, 0xDC27, 0xEF79, 0xF7BC,
    0x26A4, 0x3DE3, 0x6E84, 0x6F7A, 0x77BC, 0x7BDE, 0xD962, 0x4F0B,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F26, 0x7F16, 0x7F02, 0x7EFB,
    0x7EFB, 0x7EFE, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x11F0, 0xF371,
    0xC1E6, 0xAAA0, 0xE033, 0xF08A, 0xF84C, 0x28A1, 0x4735, 0x6FC4,
    0x7098, 0x784C, 0x7C25, 0xE441, 0x5DE5, 0x7F00, 0x7F00, 0x7FE9,
    0x7F17, 0x7F16, 0x7F05, 0x7EFC, 0x7EFC, 0x7EFD, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x0F7C, 0xF342, 0xB6DF, 0xB189, 0xE338,
    0xF187, 0xF8C9, 0x2B06, 0x509A, 0x7144, 0x7193, 0x78C8, 0x7C63,
    0xED84, 0x6A6B, 0x7F00, 0x7F00, 0x7F88, 0x7F73, 0x7F12, 0x7EEF,
    0x7EF4, 0x7EFE, 0x7F00, 0x7F00, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x0F12, 0xF2B4, 0xAA33, 0xB822, 0xE56F, 0xF269, 0xF936, 0x2EE9,
    0x59B0, 0x72F5, 0x726E, 0x7936, 0x7C9A, 0xF717, 0x75F2, 0x7F00,
    0x7F00, 0x7FBB, 0x7F39, 0x7EF0, 0x7EF0, 0x7EFD, 0x7F01, 0x7F00,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x1051, 0xF0E3, 0xA18C,
    0xBE4C, 0xE714, 0xF330, 0xF998, 0x33B3, 0x61CB, 0x74A7, 0x7332,
    0x7998, 0x7CCB, 0x00FB, 0x7FF6, 0x7F00, 0x7FED, 0x7F8C, 0x7EFC,
    0x7EEA, 0x7EFB, 0x7F01, 0x7F00, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x12E5, 0xECCF, 0x98BC, 0xC3FC, 0xE858, 0xF3E1,
    0xF9EF, 0x39A7, 0x6850, 0x7630, 0x73E0, 0x79EF, 0x7CF7, 0x0B2F,
    0x7F00, 0x7F00, 0x7F91, 0x7F44, 0x7EF7, 0x7EF5, 0x7EFE, 0x7F00,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x167B,
    0xE76E, 0x93CA, 0xC92C, 0xE964, 0xF47D, 0xFA3E, 0x40A5, 0x6D08,
    0x778A, 0x747D, 0x7A3E, 0x7D1E, 0x15B3, 0x7F00, 0x7F00, 0x7F50,
    0x7F1D, 0x7EF9, 0x7EFB, 0x7EFF, 0x7EFF, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x1AF2, 0xDD31, 0x9374, 0xCDD7,
    0xEA52, 0xF50B, 0xFA85, 0x484D, 0x700F, 0x78BA, 0x750B, 0x7A84,
    0x7D42, 0x2088, 0x7F00, 0x7F00, 0x7F6F, 0x7EFF, 0x7EF6, 0x7EFE,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x1FAB, 0xD337, 0x963B, 0xD1F3, 0xEB33, 0xF58C, 0xFAC5,
    0x4D79, 0x721D, 0x79CB, 0x758C, 0x7AC5, 0x7D62, 0x2BAD, 0x7F00,
    0x7F00, 0x7F31, 0x7EFC, 0x7EFC, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x241F, 0xCC6D,
    0x9B30, 0xD57A, 0xEC0B, 0xF601, 0xFB00, 0x5244, 0x73C4, 0x7AC4,
    0x7601, 0x7B00, 0x7D7F, 0x3722, 0x7F00, 0x7FA9, 0x7F11, 0x7EF6,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x280C, 0xBE21, 0xA145, 0xD867, 0xECDA,
    0xF66D, 0xFB36, 0x55B1, 0x7546, 0x7BA8, 0x766D, 0x7B36, 0x7D9A,
    0x42E8, 0x7F00, 0x7F45, 0x7F02, 0x7EFC, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x2B18, 0xA865, 0xA7BA, 0xDAC1, 0xED9F, 0xF6CF, 0xFB67, 0x5207,
    0x76C6, 0x7C7B, 0x76CF, 0x7B67, 0x7DB3, 0x4EFE, 0x7F00, 0x7F4F,
    0x7EF5, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x2D13, 0xB2BE, 0xAE1E,
    0xDC9C, 0xEE56, 0xF72B, 0xFB95, 0x4C1E, 0x7845, 0x7D3D, 0x772B,
    0x7B95, 0x7DC9, 0x5B64, 0x7F00, 0x7F17, 0x7EFC, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x2E1B, 0xCE29, 0xB467, 0xDE0E, 0xEEFF, 0xF77F,
    0xFBBF, 0x4843, 0x79BD, 0x7DF0, 0x777F, 0x7BBE, 0x7DDE, 0x681B,
    0x7F00, 0x7F02, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x2E32,
    0xE25E, 0xBB9A, 0xDF40, 0xEF9C, 0xF7CD, 0xFBE6, 0x68ED, 0x7B21,
    0x7E96, 0x77CD, 0x7BE6, 0x7DF1, 0x7522, 0x7F49, 0x7EFE, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF,
    0x7EFF, 0x7EFF, 0x7EFF, 0x7EFF, 0x2D4F, 0xF4E0, 0xC241, 0xE05F,
    0xF02D, 0xF817, 0xFC0A, 0x77E5, 0x7C6A, 0x7F31, 0x7817, 0x7C0B,
    0x7E05, 0x7F00, 0x7F0C, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x2B1D, 0xEB4A, 0xC91D, 0xE1A6, 0xF0B7, 0xF85A, 0xFC2C,
    0x6D81, 0x7DA5, 0x7FC1, 0x785A, 0x7C2D, 0x7E16, 0x7F00, 0x7FD9,
    0x7F18, 0x7F01, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x2782, 0xE163,
    0xCFFB, 0xE34C, 0xF13A, 0xF89A, 0xFC4C, 0x615E, 0x7ED9, 0x7F00,
    0x7899, 0x7C4B, 0x7E26, 0x7F00, 0x7F00, 0x7F44, 0x7F0F, 0x7F03,
    0x7F00, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x2237, 0xD728, 0xD6A1, 0xE592, 0xF1CA,
    0xF8D5, 0xFC6B, 0x5DFA, 0x7F58, 0x7F00, 0x78D6, 0x7C6A, 0x7E35,
    0x7F00, 0x7F00, 0x7FD9, 0x7F4F, 0x7F19, 0x7F08, 0x7F02, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE, 0x7EFE,
    0x1A9B, 0xCC9C, 0xDCAB, 0xE8B3, 0xF2A3, 0xF910, 0xFC86, 0x7B5E,
    0x7DF6, 0x7F00, 0x790C, 0x7C86, 0x7E44, 0x7F00, 0x7F00, 0x7F00,
    0x7F74, 0x7F3D, 0x7F1D, 0x7F0E, 0x7F06, 0x7F02, 0x7F01, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00,
    0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x7F00, 0x12E3
};

static int32 clipshifted8(int32 in)
{
  int16 top = (int16)(in >> 16);
  if (top > 127) in = 127 << 16;
  else if (top < -128) in = -128 << 16;
  return in;
}

static void hvl_AhxGenFilterWaves( int8 *buf, int8 *lowbuf, int8 *highbuf )
{
  static const uint16 lentab[45] = { 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    (0x280*3)-1 };

  const int16 * mid_table = &filter_thing[0];
  const int16 * low_table = &filter_thing[1395];

  int32 d5;
  int32 temp;
  
  for( temp=0, d5 = 25; temp<31; temp++, d5 += 9 )
  {
    uint32 wv;
    int8   *a0 = buf;
    
    for( wv=0; wv<6+6+0x20+1; wv++ )
    {
      int32 d2, d3;
      uint32  i;
      
      d2 = *mid_table++ << 8;
      d3 = *low_table++ << 8;
      
      for( i=0; i<=lentab[wv]; i++ )
      {
        int32 d0, d1, d4;
        d0 = (int32)a0[i] << 16;
        d1 = clipshifted8( d0 - d2 - d3 );
        *highbuf++ = d1 >> 16;
        d4 = d1 >> 8;
        d4 *= d5;
        d2 = clipshifted8(d2 + d4);
        d4 = d2 >> 8;
        d4 *= d5;
        d3 = clipshifted8(d3 + d4);
        *lowbuf++ = d3 >> 16;
      }
      
      a0 += lentab[wv]+1;
    }
  }
}

void hvl_GenWhiteNoise( int8 *buf, uint32 len )
{
  uint32 ays;

  ays = 0x41595321;

  do {
    uint16 ax, bx;
    int8 s;

    s = ays;

    if( ays & 0x100 )
    {
      s = 0x7F;

      if( ays & 0x8000 )
        s = 0x80;
    }

    *buf++ = s;
    len--;

    ays = (ays >> 5) | (ays << 27);
    ays = (ays & 0xffffff00) | ((ays & 0xff) ^ 0x9a);
    bx  = ays;
    ays = (ays << 2) | (ays >> 30);
    ax  = ays;
    bx  += ax;
    ax  ^= bx;
    ays  = (ays & 0xffff0000) | ax;
    ays  = (ays >> 3) | (ays << 29);
  } while( len );
}

void hvl_reset_some_stuff( struct hvl_tune *ht )
{
  uint32 i;

  for( i=0; i<MAX_CHANNELS; i++ )
  {
    ht->ht_Voices[i].vc_Delta=~0;
    ht->ht_Voices[i].vc_OverrideTranspose=1000;  // 1.5
    ht->ht_Voices[i].vc_SamplePos=ht->ht_Voices[i].vc_Track=ht->ht_Voices[i].vc_Transpose=ht->ht_Voices[i].vc_NextTrack = ht->ht_Voices[i].vc_NextTranspose = 0;
    ht->ht_Voices[i].vc_ADSRVolume=ht->ht_Voices[i].vc_InstrPeriod=ht->ht_Voices[i].vc_TrackPeriod=ht->ht_Voices[i].vc_VibratoPeriod=ht->ht_Voices[i].vc_NoteMaxVolume=ht->ht_Voices[i].vc_PerfSubVolume=ht->ht_Voices[i].vc_TrackMasterVolume=0;
    ht->ht_Voices[i].vc_NewWaveform=ht->ht_Voices[i].vc_Waveform=ht->ht_Voices[i].vc_PlantSquare=ht->ht_Voices[i].vc_PlantPeriod=ht->ht_Voices[i].vc_IgnoreSquare=0;
    ht->ht_Voices[i].vc_TrackOn=ht->ht_Voices[i].vc_FixedNote=ht->ht_Voices[i].vc_VolumeSlideUp=ht->ht_Voices[i].vc_VolumeSlideDown=ht->ht_Voices[i].vc_HardCut=ht->ht_Voices[i].vc_HardCutRelease=ht->ht_Voices[i].vc_HardCutReleaseF=0;
    ht->ht_Voices[i].vc_PeriodSlideSpeed=ht->ht_Voices[i].vc_PeriodSlidePeriod=ht->ht_Voices[i].vc_PeriodSlideLimit=ht->ht_Voices[i].vc_PeriodSlideOn=ht->ht_Voices[i].vc_PeriodSlideWithLimit=0;
    ht->ht_Voices[i].vc_PeriodPerfSlideSpeed=ht->ht_Voices[i].vc_PeriodPerfSlidePeriod=ht->ht_Voices[i].vc_PeriodPerfSlideOn=ht->ht_Voices[i].vc_VibratoDelay=ht->ht_Voices[i].vc_VibratoCurrent=ht->ht_Voices[i].vc_VibratoDepth=ht->ht_Voices[i].vc_VibratoSpeed=0;
    ht->ht_Voices[i].vc_SquareOn=ht->ht_Voices[i].vc_SquareInit=ht->ht_Voices[i].vc_SquareLowerLimit=ht->ht_Voices[i].vc_SquareUpperLimit=ht->ht_Voices[i].vc_SquarePos=ht->ht_Voices[i].vc_SquareSign=ht->ht_Voices[i].vc_SquareSlidingIn=ht->ht_Voices[i].vc_SquareReverse=0;
    ht->ht_Voices[i].vc_FilterOn=ht->ht_Voices[i].vc_FilterInit=ht->ht_Voices[i].vc_FilterLowerLimit=ht->ht_Voices[i].vc_FilterUpperLimit=ht->ht_Voices[i].vc_FilterPos=ht->ht_Voices[i].vc_FilterSign=ht->ht_Voices[i].vc_FilterSpeed=ht->ht_Voices[i].vc_FilterSlidingIn=ht->ht_Voices[i].vc_IgnoreFilter=0;
    ht->ht_Voices[i].vc_PerfCurrent=ht->ht_Voices[i].vc_PerfSpeed=ht->ht_Voices[i].vc_WaveLength=ht->ht_Voices[i].vc_NoteDelayOn=ht->ht_Voices[i].vc_NoteCutOn=0;
    ht->ht_Voices[i].vc_AudioPeriod=ht->ht_Voices[i].vc_AudioVolume=ht->ht_Voices[i].vc_VoiceVolume=ht->ht_Voices[i].vc_VoicePeriod=ht->ht_Voices[i].vc_VoiceNum=ht->ht_Voices[i].vc_WNRandom=0;
    ht->ht_Voices[i].vc_SquareWait=ht->ht_Voices[i].vc_FilterWait=ht->ht_Voices[i].vc_PerfWait=ht->ht_Voices[i].vc_NoteDelayWait=ht->ht_Voices[i].vc_NoteCutWait=0;
    ht->ht_Voices[i].vc_PerfList=0;
    ht->ht_Voices[i].vc_RingSamplePos=ht->ht_Voices[i].vc_RingDelta=ht->ht_Voices[i].vc_RingPlantPeriod=ht->ht_Voices[i].vc_RingAudioPeriod=ht->ht_Voices[i].vc_RingNewWaveform=ht->ht_Voices[i].vc_RingWaveform=ht->ht_Voices[i].vc_RingFixedPeriod=ht->ht_Voices[i].vc_RingBasePeriod=0;

    ht->ht_Voices[i].vc_RingMixSource = NULL;
    ht->ht_Voices[i].vc_RingAudioSource = NULL;

    memset(&ht->ht_Voices[i].vc_SquareTempBuffer,0,0x80);
    memset(&ht->ht_Voices[i].vc_ADSR,0,sizeof(struct hvl_envelope));
    memset(&ht->ht_Voices[i].vc_VoiceBuffer,0,0x281);
    memset(&ht->ht_Voices[i].vc_RingVoiceBuffer,0,0x281);
  }
  
  for( i=0; i<MAX_CHANNELS; i++ )
  {
    ht->ht_Voices[i].vc_WNRandom          = 0x280;
    ht->ht_Voices[i].vc_VoiceNum          = i;
    ht->ht_Voices[i].vc_TrackMasterVolume = 0x40;
    ht->ht_Voices[i].vc_TrackOn           = 1;
    ht->ht_Voices[i].vc_MixSource         = ht->ht_Voices[i].vc_VoiceBuffer;

	ht->ht_Voices[i].vc_LastAmp[0]        = 0;
	ht->ht_Voices[i].vc_LastAmp[1]        = 0;
	ht->ht_Voices[i].vc_LastClock[0]      = 0;
	ht->ht_Voices[i].vc_LastClock[1]      = 0;
  }

  hvl_blip_clear(ht->ht_BlipBuffers[0]);
  hvl_blip_clear(ht->ht_BlipBuffers[1]);
}

BOOL hvl_InitSubsong( struct hvl_tune *ht, uint32 nr )
{
  uint32 PosNr, i;

  if( nr > ht->ht_SubsongNr )
    return FALSE;

  ht->ht_SongNum = nr;
  
  PosNr = 0;
  if( nr ) PosNr = ht->ht_Subsongs[nr-1];
  
  ht->ht_PosNr          = PosNr;
  ht->ht_PosJump        = 0;
  ht->ht_PatternBreak   = 0;
  ht->ht_NoteNr         = 0;
  ht->ht_PosJumpNote    = 0;
  ht->ht_Tempo          = 6;
  ht->ht_StepWaitFrames	= 0;
  ht->ht_GetNewPosition = 1;
  ht->ht_SongEndReached = 0;
  ht->ht_PlayingTime    = 0;
  
  for( i=0; i<MAX_CHANNELS; i+=4 )
  {
    ht->ht_Voices[i+0].vc_Pan          = ht->ht_defpanleft;
    ht->ht_Voices[i+0].vc_SetPan       = ht->ht_defpanleft; // 1.4
    ht->ht_Voices[i+0].vc_PanMultLeft  = panning_left[ht->ht_defpanleft];
    ht->ht_Voices[i+0].vc_PanMultRight = panning_right[ht->ht_defpanleft];
    ht->ht_Voices[i+1].vc_Pan          = ht->ht_defpanright;
    ht->ht_Voices[i+1].vc_SetPan       = ht->ht_defpanright; // 1.4
    ht->ht_Voices[i+1].vc_PanMultLeft  = panning_left[ht->ht_defpanright];
    ht->ht_Voices[i+1].vc_PanMultRight = panning_right[ht->ht_defpanright];
    ht->ht_Voices[i+2].vc_Pan          = ht->ht_defpanright;
    ht->ht_Voices[i+2].vc_SetPan       = ht->ht_defpanright; // 1.4
    ht->ht_Voices[i+2].vc_PanMultLeft  = panning_left[ht->ht_defpanright];
    ht->ht_Voices[i+2].vc_PanMultRight = panning_right[ht->ht_defpanright];
    ht->ht_Voices[i+3].vc_Pan          = ht->ht_defpanleft;
    ht->ht_Voices[i+3].vc_SetPan       = ht->ht_defpanleft;  // 1.4
    ht->ht_Voices[i+3].vc_PanMultLeft  = panning_left[ht->ht_defpanleft];
    ht->ht_Voices[i+3].vc_PanMultRight = panning_right[ht->ht_defpanleft];
  }

  hvl_reset_some_stuff( ht );
  
  return TRUE;
}

void hvl_InitReplayer( void )
{
  hvl_GenPanningTables();
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_04], 0x04 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_08], 0x08 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_10], 0x10 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_20], 0x20 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_40], 0x40 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_80], 0x80 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_04], 0x04 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_08], 0x08 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_10], 0x10 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_20], 0x20 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_40], 0x40 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_80], 0x80 );
  hvl_GenSquare( &waves[WO_SQUARES] );
  hvl_GenWhiteNoise( &waves[WO_WHITENOISE], WHITENOISELEN );
#ifdef USE_HVL_FILTER_FOR_HVL
  memcpy( &ahxwaves[WO_TRIANGLE_04], &waves[WO_TRIANGLE_04], WO_HIGHPASSES - WO_TRIANGLE_04 );
  hvl_GenFilterWaves( &waves[WO_TRIANGLE_04], &waves[WO_LOWPASSES], &waves[WO_HIGHPASSES] );
  hvl_AhxGenFilterWaves( &ahxwaves[WO_TRIANGLE_04], &ahxwaves[WO_LOWPASSES], &ahxwaves[WO_HIGHPASSES] );
#else
  hvl_AhxGenFilterWaves( &waves[WO_TRIANGLE_04], &waves[WO_LOWPASSES], &waves[WO_HIGHPASSES] );
#endif
}

struct hvl_tune *hvl_load_ahx( uint8 *buf, uint32 buflen, uint32 defstereo, uint32 freq )
{
  uint8  *bptr;
  TEXT   *nptr;
  uint32  i, j, k, l, posn, insn, ssn, hs, trkn, trkl;
  struct hvl_tune *ht;
  struct  hvl_plsentry *ple;
  int32 defgain[] = { 71, 72, 76, 85, 100 };

  if ( buflen < 14 )
    return NULL;
  
  posn = ((buf[6]&0x0f)<<8)|buf[7];
  insn = buf[12];
  ssn  = buf[13];
  trkl = buf[10];
  trkn = buf[11];

  hs  = sizeof( struct hvl_tune );
  hs += sizeof( struct hvl_position ) * posn;
  hs += sizeof( struct hvl_instrument ) * (insn+1);
  hs += sizeof( uint16 ) * ssn;
  hs += hvl_blip_size( 256 ) * 2;

  // Calculate the size of all instrument PList buffers
  bptr = &buf[14];
  bptr += ssn*2;    // Skip past the subsong list
  bptr += posn*4*2; // Skip past the positions
  bptr += trkn*trkl*3;
  if((buf[6]&0x80)==0) bptr += trkl*3;

  // *NOW* we can finally calculate PList space
  for( i=1; i<=insn; i++ )
  {
    if ( bptr + 21 > buf + buflen )
      return NULL;
    
    hs += bptr[21] * sizeof( struct hvl_plsentry );
    bptr += 22 + bptr[21]*4;
  }

  ht = malloc( hs );
  if( !ht )
  {
    return NULL;
  }

#ifdef USE_HVL_FILTER_FOR_HVL
  ht->ht_IsHt            = 0;
#endif
  ht->ht_Frequency       = freq;
  ht->ht_FreqF           = (float64)freq;
  
  ht->ht_Positions      = (struct hvl_position *)(&ht[1]);
  ht->ht_Instruments    = (struct hvl_instrument *)(&ht->ht_Positions[posn]);
  ht->ht_Subsongs       = (uint16 *)(&ht->ht_Instruments[(insn+1)]);
  ht->ht_BlipBuffers[0] = (hvl_blip_t *)(&ht->ht_Subsongs[ssn]);
  ht->ht_BlipBuffers[1] = (hvl_blip_t *)((uint8 *)(ht->ht_BlipBuffers[0]) + hvl_blip_size(256));
  ple                   = (struct hvl_plsentry *)(((uint8 *)ht->ht_BlipBuffers[1]) + hvl_blip_size(256));

  hvl_blip_new_inplace(ht->ht_BlipBuffers[0], 256);
  hvl_blip_new_inplace(ht->ht_BlipBuffers[1], 256);
  hvl_blip_set_rates(ht->ht_BlipBuffers[0], 65536, 1);
  hvl_blip_set_rates(ht->ht_BlipBuffers[1], 65536, 1);

#ifdef USE_HVL_FILTER_FOR_HVL
  ht->ht_WaveformTab[0]  = &ahxwaves[WO_TRIANGLE_04];
  ht->ht_WaveformTab[1]  = &ahxwaves[WO_SAWTOOTH_04];
  ht->ht_WaveformTab[3]  = &ahxwaves[WO_WHITENOISE];
#else
  ht->ht_WaveformTab[0]  = &waves[WO_TRIANGLE_04];
  ht->ht_WaveformTab[1]  = &waves[WO_SAWTOOTH_04];
  ht->ht_WaveformTab[3]  = &waves[WO_WHITENOISE];
#endif

  ht->ht_Channels        = 4;
  ht->ht_PositionNr      = posn;
  ht->ht_Restart         = (buf[8]<<8)|buf[9];
  ht->ht_SpeedMultiplier = ((buf[6]>>5)&3)+1;
  ht->ht_TrackLength     = trkl;
  ht->ht_TrackNr         = trkn;
  ht->ht_InstrumentNr    = insn;
  ht->ht_SubsongNr       = ssn;
  ht->ht_defstereo       = defstereo;
  ht->ht_defpanleft      = stereopan_left[ht->ht_defstereo];
  ht->ht_defpanright     = stereopan_right[ht->ht_defstereo];
  ht->ht_mixgain         = (defgain[ht->ht_defstereo]*256)/100;
  
  if( ht->ht_Restart >= ht->ht_PositionNr )
    ht->ht_Restart = ht->ht_PositionNr-1;

  // Do some validation  
  if( ( ht->ht_PositionNr > 1000 ) ||
      ( ht->ht_TrackLength > 64 ) ||
      ( ht->ht_InstrumentNr > 64 ) )
  {
    free( ht );
    return NULL;
  }

  bptr = &buf[(buf[4]<<8)|buf[5]];
  i = min( 128, buf + buflen - bptr );

  strncpy( ht->ht_Name, (TEXT *)bptr, i );
  if ( i < 128 ) ht->ht_Name[ i ] = 0;
  nptr = (TEXT *)bptr+strlen( ht->ht_Name )+1;
  if ( nptr > buf + buflen )
  {
    free( ht );
    return NULL;
  }

  bptr = &buf[14];
  
  // Subsongs
  for( i=0; i<ht->ht_SubsongNr; i++ )
  {
    ht->ht_Subsongs[i] = (bptr[0]<<8)|bptr[1];
    if( ht->ht_Subsongs[i] >= ht->ht_PositionNr )
      ht->ht_Subsongs[i] = 0;
    bptr += 2;
  }
  
  // Position list
  for( i=0; i<ht->ht_PositionNr; i++ )
  {
    for( j=0; j<4; j++ )
    {
      ht->ht_Positions[i].pos_Track[j]     = *bptr++;
      ht->ht_Positions[i].pos_Transpose[j] = *(int8 *)bptr++;
    }
  }
  
  // Tracks
  for( i=0; i<=ht->ht_TrackNr; i++ )
  {
    if( ( ( buf[6]&0x80 ) == 0x80 ) && ( i == 0 ) )
    {
      for( j=0; j<ht->ht_TrackLength; j++ )
      {
        ht->ht_Tracks[i][j].stp_Note       = 0;
        ht->ht_Tracks[i][j].stp_Instrument = 0;
        ht->ht_Tracks[i][j].stp_FX         = 0;
        ht->ht_Tracks[i][j].stp_FXParam    = 0;
        ht->ht_Tracks[i][j].stp_FXb        = 0;
        ht->ht_Tracks[i][j].stp_FXbParam   = 0;
      }
      continue;
    }
    
    for( j=0; j<ht->ht_TrackLength; j++ )
    {
      ht->ht_Tracks[i][j].stp_Note       = (bptr[0]>>2)&0x3f;
      ht->ht_Tracks[i][j].stp_Instrument = ((bptr[0]&0x3)<<4) | (bptr[1]>>4);
      ht->ht_Tracks[i][j].stp_FX         = bptr[1]&0xf;
      ht->ht_Tracks[i][j].stp_FXParam    = bptr[2];
      ht->ht_Tracks[i][j].stp_FXb        = 0;
      ht->ht_Tracks[i][j].stp_FXbParam   = 0;
      bptr += 3;
    }
  }
  
  // Instruments
  for( i=1; i<=ht->ht_InstrumentNr; i++ )
  {
    if( nptr < (TEXT *)(buf+buflen) )
    {
      strncpy( ht->ht_Instruments[i].ins_Name, nptr, 128 );
      nptr += strlen( nptr )+1;
    } else {
      ht->ht_Instruments[i].ins_Name[0] = 0;
    }
    
    ht->ht_Instruments[i].ins_Volume      = bptr[0];
    ht->ht_Instruments[i].ins_FilterSpeed = ((bptr[1]>>3)&0x1f)|((bptr[12]>>2)&0x20);
    ht->ht_Instruments[i].ins_WaveLength  = bptr[1]&0x07;

    ht->ht_Instruments[i].ins_Envelope.aFrames = bptr[2];
    ht->ht_Instruments[i].ins_Envelope.aVolume = bptr[3];
    ht->ht_Instruments[i].ins_Envelope.dFrames = bptr[4];
    ht->ht_Instruments[i].ins_Envelope.dVolume = bptr[5];
    ht->ht_Instruments[i].ins_Envelope.sFrames = bptr[6];
    ht->ht_Instruments[i].ins_Envelope.rFrames = bptr[7];
    ht->ht_Instruments[i].ins_Envelope.rVolume = bptr[8];
    
    ht->ht_Instruments[i].ins_FilterLowerLimit     = bptr[12]&0x7f;
    ht->ht_Instruments[i].ins_VibratoDelay         = bptr[13];
    ht->ht_Instruments[i].ins_HardCutReleaseFrames = (bptr[14]>>4)&0x07;
    ht->ht_Instruments[i].ins_HardCutRelease       = bptr[14]&0x80?1:0;
    ht->ht_Instruments[i].ins_VibratoDepth         = bptr[14]&0x0f;
    ht->ht_Instruments[i].ins_VibratoSpeed         = bptr[15];
    ht->ht_Instruments[i].ins_SquareLowerLimit     = bptr[16];
    ht->ht_Instruments[i].ins_SquareUpperLimit     = bptr[17];
    ht->ht_Instruments[i].ins_SquareSpeed          = bptr[18];
    ht->ht_Instruments[i].ins_FilterUpperLimit     = bptr[19]&0x3f;
    ht->ht_Instruments[i].ins_PList.pls_Speed      = bptr[20];
    ht->ht_Instruments[i].ins_PList.pls_Length     = bptr[21];
    
    ht->ht_Instruments[i].ins_PList.pls_Entries    = ple;
    ple += bptr[21];
    
    bptr += 22;
    for( j=0; j<ht->ht_Instruments[i].ins_PList.pls_Length; j++ )
    {
      k = (bptr[0]>>5)&7;
      if( k == 6 ) k = 12;
      if( k == 7 ) k = 15;
      l = (bptr[0]>>2)&7;
      if( l == 6 ) l = 12;
      if( l == 7 ) l = 15;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[1]      = k;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[0]      = l;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Waveform   = ((bptr[0]<<1)&6) | (bptr[1]>>7);
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Fixed      = (bptr[1]>>6)&1;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Note       = bptr[1]&0x3f;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] = bptr[2];
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] = bptr[3];

      // 1.6: Strip "toggle filter" commands if the module is
      //      version 0 (pre-filters). This is what AHX also does.
      if( ( buf[3] == 0 ) && ( l == 4 ) && ( (bptr[2]&0xf0) != 0 ) )
        ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] &= 0x0f;
      if( ( buf[3] == 0 ) && ( k == 4 ) && ( (bptr[3]&0xf0) != 0 ) )
        ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] &= 0x0f; // 1.8

      bptr += 4;
    }
  }
  
  hvl_InitSubsong( ht, 0 );
  return ht;
}

struct hvl_tune *hvl_LoadTune( const uint8 *buf, uint32 buflen, uint32 freq, uint32 defstereo )
{
  struct hvl_tune *ht;
  uint8  *bptr;
  TEXT   *nptr;
  uint32  i, j, posn, insn, ssn, chnn, hs, trkl, trkn;
  FILE *fh;
  struct  hvl_plsentry *ple;

  if ( !buf || buflen < 4 )
    return NULL;

  if( ( buf[0] == 'T' ) &&
      ( buf[1] == 'H' ) &&
      ( buf[2] == 'X' ) &&
      ( buf[3] < 3 ) )
    return hvl_load_ahx( buf, buflen, defstereo, freq );

  if( ( buf[0] != 'H' ) ||
      ( buf[1] != 'V' ) ||
      ( buf[2] != 'L' ) ||
      ( buf[3] > 1 ) )
  {
    return NULL;
  }
  
  if ( buflen < 14 )
    return NULL;

  posn = ((buf[6]&0x0f)<<8)|buf[7];
  insn = buf[12];
  ssn  = buf[13];
  chnn = (buf[8]>>2)+4;
  trkl = buf[10];
  trkn = buf[11];

  hs  = sizeof( struct hvl_tune );
  hs += sizeof( struct hvl_position ) * posn;
  hs += sizeof( struct hvl_instrument ) * (insn+1);
  hs += sizeof( uint16 ) * ssn;
  hs += hvl_blip_size(256) * 2;

  // Calculate the size of all instrument PList buffers
  bptr = &buf[16];
  bptr += ssn*2;       // Skip past the subsong list
  bptr += posn*chnn*2; // Skip past the positions

  // Skip past the tracks
  // 1.4: Fixed two really stupid bugs that cancelled each other
  //      out if the module had a blank first track (which is how
  //      come they were missed.
  for( i=((buf[6]&0x80)==0x80)?1:0; i<=trkn; i++ )
    for( j=0; j<trkl; j++ )
    {
      if ( bptr > buf + buflen )
        return NULL;
      
      if( bptr[0] == 0x3f )
      {
        bptr++;
        continue;
      }
      bptr += 5;
    }

  // *NOW* we can finally calculate PList space
  for( i=1; i<=insn; i++ )
  {
    if ( bptr + 21 > buf + buflen )
      return NULL;
    
    hs += bptr[21] * sizeof( struct hvl_plsentry );
    bptr += 22 + bptr[21]*5;
  }
  
  ht = malloc( hs );    
  if( !ht )
  {
    return NULL;
  }
  
#ifdef USE_HVL_FILTER_FOR_HVL
  ht->ht_IsHt            = 1;
#endif
  ht->ht_Version         = buf[3]; // 1.5
  ht->ht_Frequency       = freq;
  ht->ht_FreqF           = (float64)freq;
  
  ht->ht_Positions       = (struct hvl_position *)(&ht[1]);
  ht->ht_Instruments     = (struct hvl_instrument *)(&ht->ht_Positions[posn]);
  ht->ht_Subsongs        = (uint16 *)(&ht->ht_Instruments[(insn+1)]);
  ht->ht_BlipBuffers[0]  = (hvl_blip_t *)(&ht->ht_Subsongs[ssn]);
  ht->ht_BlipBuffers[1]  = (hvl_blip_t *)(((uint8 *)ht->ht_BlipBuffers[0]) + hvl_blip_size(256));
  ple                    = (struct hvl_plsentry *)(((uint8 *)ht->ht_BlipBuffers[1]) + hvl_blip_size(256));

  hvl_blip_new_inplace(ht->ht_BlipBuffers[0], 256);
  hvl_blip_new_inplace(ht->ht_BlipBuffers[1], 256);
  hvl_blip_set_rates(ht->ht_BlipBuffers[0], 65536, 1);
  hvl_blip_set_rates(ht->ht_BlipBuffers[1], 65536, 1);

  ht->ht_WaveformTab[0]  = &waves[WO_TRIANGLE_04];
  ht->ht_WaveformTab[1]  = &waves[WO_SAWTOOTH_04];
  ht->ht_WaveformTab[3]  = &waves[WO_WHITENOISE];

  ht->ht_PositionNr      = posn;
  ht->ht_Channels        = (buf[8]>>2)+4;
  ht->ht_Restart         = ((buf[8]&3)<<8)|buf[9];
  ht->ht_SpeedMultiplier = ((buf[6]>>5)&3)+1;
  ht->ht_TrackLength     = buf[10];
  ht->ht_TrackNr         = buf[11];
  ht->ht_InstrumentNr    = insn;
  ht->ht_SubsongNr       = ssn;
  ht->ht_mixgain         = (buf[14]<<8)/100;
  ht->ht_defstereo       = buf[15];
  ht->ht_defpanleft      = stereopan_left[ht->ht_defstereo];
  ht->ht_defpanright     = stereopan_right[ht->ht_defstereo];
  
  if( ht->ht_Restart >= ht->ht_PositionNr )
    ht->ht_Restart = ht->ht_PositionNr-1;

  // Do some validation  
  if( ( ht->ht_PositionNr > 1000 ) ||
      ( ht->ht_TrackLength > 64 ) ||
      ( ht->ht_InstrumentNr > 64 ) )
  {
    free( ht );
    return NULL;
  }

  bptr = &buf[(buf[4]<<8)|buf[5]];
  if ( bptr > buf + buflen )
  {
    free( ht );
    return NULL;
  }
  i = min( 128, buf + buflen - bptr );

  strncpy( ht->ht_Name, (TEXT *)bptr, i );
  if ( i < 128 ) ht->ht_Name[ i ] = 0;
  nptr = (TEXT *)bptr+strlen( ht->ht_Name )+1;
  if ( nptr > buf + buflen )
  {
    free( ht );
    return NULL;
  }

  bptr = &buf[16];
  
  // Subsongs
  for( i=0; i<ht->ht_SubsongNr; i++ )
  {
    ht->ht_Subsongs[i] = (bptr[0]<<8)|bptr[1];
    bptr += 2;
  }
  
  // Position list
  for( i=0; i<ht->ht_PositionNr; i++ )
  {
    for( j=0; j<ht->ht_Channels; j++ )
    {
      ht->ht_Positions[i].pos_Track[j]     = *bptr++;
      ht->ht_Positions[i].pos_Transpose[j] = *(int8 *)bptr++;
    }
  }
  
  // Tracks
  for( i=0; i<=ht->ht_TrackNr; i++ )
  {
    if( ( ( buf[6]&0x80 ) == 0x80 ) && ( i == 0 ) )
    {
      for( j=0; j<ht->ht_TrackLength; j++ )
      {
        ht->ht_Tracks[i][j].stp_Note       = 0;
        ht->ht_Tracks[i][j].stp_Instrument = 0;
        ht->ht_Tracks[i][j].stp_FX         = 0;
        ht->ht_Tracks[i][j].stp_FXParam    = 0;
        ht->ht_Tracks[i][j].stp_FXb        = 0;
        ht->ht_Tracks[i][j].stp_FXbParam   = 0;
      }
      continue;
    }
    
    for( j=0; j<ht->ht_TrackLength; j++ )
    {
      if( bptr[0] == 0x3f )
      {
        ht->ht_Tracks[i][j].stp_Note       = 0;
        ht->ht_Tracks[i][j].stp_Instrument = 0;
        ht->ht_Tracks[i][j].stp_FX         = 0;
        ht->ht_Tracks[i][j].stp_FXParam    = 0;
        ht->ht_Tracks[i][j].stp_FXb        = 0;
        ht->ht_Tracks[i][j].stp_FXbParam   = 0;
        bptr++;
        continue;
      }
      
      ht->ht_Tracks[i][j].stp_Note       = bptr[0];
      ht->ht_Tracks[i][j].stp_Instrument = bptr[1];
      ht->ht_Tracks[i][j].stp_FX         = bptr[2]>>4;
      ht->ht_Tracks[i][j].stp_FXParam    = bptr[3];
      ht->ht_Tracks[i][j].stp_FXb        = bptr[2]&0xf;
      ht->ht_Tracks[i][j].stp_FXbParam   = bptr[4];
      bptr += 5;
    }
  }
  
  
  // Instruments
  for( i=1; i<=ht->ht_InstrumentNr; i++ )
  {
    if( nptr < (TEXT *)(buf+buflen) )
    {
      strncpy( ht->ht_Instruments[i].ins_Name, nptr, 128 );
      nptr += strlen( nptr )+1;
    } else {
      ht->ht_Instruments[i].ins_Name[0] = 0;
    }
    
    ht->ht_Instruments[i].ins_Volume      = bptr[0];
    ht->ht_Instruments[i].ins_FilterSpeed = ((bptr[1]>>3)&0x1f)|((bptr[12]>>2)&0x20);
    ht->ht_Instruments[i].ins_WaveLength  = bptr[1]&0x07;

    ht->ht_Instruments[i].ins_Envelope.aFrames = bptr[2];
    ht->ht_Instruments[i].ins_Envelope.aVolume = bptr[3];
    ht->ht_Instruments[i].ins_Envelope.dFrames = bptr[4];
    ht->ht_Instruments[i].ins_Envelope.dVolume = bptr[5];
    ht->ht_Instruments[i].ins_Envelope.sFrames = bptr[6];
    ht->ht_Instruments[i].ins_Envelope.rFrames = bptr[7];
    ht->ht_Instruments[i].ins_Envelope.rVolume = bptr[8];
    
    ht->ht_Instruments[i].ins_FilterLowerLimit     = bptr[12]&0x7f;
    ht->ht_Instruments[i].ins_VibratoDelay         = bptr[13];
    ht->ht_Instruments[i].ins_HardCutReleaseFrames = (bptr[14]>>4)&0x07;
    ht->ht_Instruments[i].ins_HardCutRelease       = bptr[14]&0x80?1:0;
    ht->ht_Instruments[i].ins_VibratoDepth         = bptr[14]&0x0f;
    ht->ht_Instruments[i].ins_VibratoSpeed         = bptr[15];
    ht->ht_Instruments[i].ins_SquareLowerLimit     = bptr[16];
    ht->ht_Instruments[i].ins_SquareUpperLimit     = bptr[17];
    ht->ht_Instruments[i].ins_SquareSpeed          = bptr[18];
    ht->ht_Instruments[i].ins_FilterUpperLimit     = bptr[19]&0x3f;
    ht->ht_Instruments[i].ins_PList.pls_Speed      = bptr[20];
    ht->ht_Instruments[i].ins_PList.pls_Length     = bptr[21];
    
    ht->ht_Instruments[i].ins_PList.pls_Entries    = ple;
    ple += bptr[21];
    
    bptr += 22;
    for( j=0; j<ht->ht_Instruments[i].ins_PList.pls_Length; j++ )
    {
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[0] = bptr[0]&0xf;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[1] = (bptr[1]>>3)&0xf;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Waveform = bptr[1]&7;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Fixed = (bptr[2]>>6)&1;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Note  = bptr[2]&0x3f;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] = bptr[3];
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] = bptr[4];
      bptr += 5;
    }
  }
  
  hvl_InitSubsong( ht, 0 );
  return ht;
}

void hvl_FreeTune( struct hvl_tune *ht )
{
  if( !ht ) return;
  free( ht );
}

void hvl_process_stepfx_1( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam )
{
  switch( FX )
  {
    case 0x0:  // Position Jump HI
      if( ((FXParam&0x0f) > 0) && ((FXParam&0x0f) <= 9) )
        ht->ht_PosJump = FXParam & 0xf;
      break;

    case 0x5:  // Volume Slide + Tone Portamento
    case 0xa:  // Volume Slide
      voice->vc_VolumeSlideDown = FXParam & 0x0f;
      voice->vc_VolumeSlideUp   = FXParam >> 4;
      break;
    
    case 0x7:  // Panning
      if( FXParam > 127 )
        FXParam -= 256;
      voice->vc_Pan          = (FXParam+128);
      voice->vc_SetPan       = (FXParam+128); // 1.4
      voice->vc_PanMultLeft  = panning_left[voice->vc_Pan];
      voice->vc_PanMultRight = panning_right[voice->vc_Pan];
      break;
    
    case 0xb: // Position jump
      ht->ht_PosJump      = ht->ht_PosJump*100 + (FXParam & 0x0f) + (FXParam >> 4)*10;
      ht->ht_PatternBreak = 1;
      if( ht->ht_PosJump <= ht->ht_PosNr )
        ht->ht_SongEndReached = 1;
      break;
    
    case 0xd: // Pattern break
      ht->ht_PosJump      = ht->ht_PosNr+1;
      ht->ht_PosJumpNote  = (FXParam & 0x0f) + (FXParam>>4)*10;
      ht->ht_PatternBreak = 1;
      if( ht->ht_PosJumpNote >  ht->ht_TrackLength )
        ht->ht_PosJumpNote = 0;
      break;
    
    case 0xe: // Extended commands
      switch( FXParam >> 4 )
      {
        case 0xc: // Note cut
          if( (FXParam & 0x0f) < ht->ht_Tempo )
          {
            voice->vc_NoteCutWait = FXParam & 0x0f;
            if( voice->vc_NoteCutWait )
            {
              voice->vc_NoteCutOn      = 1;
              voice->vc_HardCutRelease = 0;
            }
          }
          break;
          
          // 1.6: 0xd case removed
      }
      break;
    
    case 0xf: // Speed
      ht->ht_Tempo = FXParam;
      if( FXParam == 0 )
        ht->ht_SongEndReached = 1;
      break;
  }  
}

void hvl_process_stepfx_2( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam, int32 *Note )
{
  switch( FX )
  {
    case 0x9: // Set squarewave offset
      voice->vc_SquarePos    = FXParam >> (5 - voice->vc_WaveLength);
//      voice->vc_PlantSquare  = 1;
      voice->vc_IgnoreSquare = 1;
      break;
    
    case 0x5: // Tone portamento + volume slide
    case 0x3: // Tone portamento
      if( FXParam != 0 ) voice->vc_PeriodSlideSpeed = FXParam;
      
      if( *Note )
      {
        int32 new, diff;

        new   = period_tab[*Note];
        diff  = period_tab[voice->vc_TrackPeriod];
        diff -= new;
        new   = diff + voice->vc_PeriodSlidePeriod;
        
        if( new )
          voice->vc_PeriodSlideLimit = -diff;
      }
      voice->vc_PeriodSlideOn        = 1;
      voice->vc_PeriodSlideWithLimit = 1;
      *Note = 0;
      break;      
  } 
}

void hvl_process_stepfx_3( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam )
{
  int32 i;
  
  switch( FX )
  {
    case 0x01: // Portamento up (period slide down)
      voice->vc_PeriodSlideSpeed     = -FXParam;
      voice->vc_PeriodSlideOn        = 1;
      voice->vc_PeriodSlideWithLimit = 0;
      break;
    case 0x02: // Portamento down
      voice->vc_PeriodSlideSpeed     = FXParam;
      voice->vc_PeriodSlideOn        = 1;
      voice->vc_PeriodSlideWithLimit = 0;
      break;
    case 0x04: // Filter override
      if( ( FXParam == 0 ) || ( FXParam == 0x40 ) ) break;
      if( FXParam < 0x40 )
      {
        voice->vc_IgnoreFilter = FXParam;
        break;
      }
      if( FXParam > 0x7f ) break;
      voice->vc_FilterPos = FXParam - 0x40;
      break;
    case 0x0c: // Volume
      FXParam &= 0xff;
      if( FXParam <= 0x40 )
      {
        voice->vc_NoteMaxVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0x50) < 0 ) break;  // 1.6

      if( FXParam <= 0x40 )
      {
        for( i=0; i<ht->ht_Channels; i++ )
          ht->ht_Voices[i].vc_TrackMasterVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0xa0-0x50) < 0 ) break; // 1.6

      if( FXParam <= 0x40 )
        voice->vc_TrackMasterVolume = FXParam;
      break;

    case 0xe: // Extended commands;
      switch( FXParam >> 4 )
      {
        case 0x1: // Fineslide up
          voice->vc_PeriodSlidePeriod -= (FXParam & 0x0f); // 1.8
          voice->vc_PlantPeriod = 1;
          break;
        
        case 0x2: // Fineslide down
          voice->vc_PeriodSlidePeriod += (FXParam & 0x0f); // 1.8
          voice->vc_PlantPeriod = 1;
          break;
        
        case 0x4: // Vibrato control
          voice->vc_VibratoDepth = FXParam & 0x0f;
          break;
        
        case 0x0a: // Fine volume up
          voice->vc_NoteMaxVolume += FXParam & 0x0f;
          
          if( voice->vc_NoteMaxVolume > 0x40 )
            voice->vc_NoteMaxVolume = 0x40;
          break;
        
        case 0x0b: // Fine volume down
          voice->vc_NoteMaxVolume -= FXParam & 0x0f;
          
          if( voice->vc_NoteMaxVolume < 0 )
            voice->vc_NoteMaxVolume = 0;
          break;
        
        case 0x0f: // Misc flags (1.5)
          if( ht->ht_Version < 1 ) break;
          switch( FXParam & 0xf )
          {
            case 1:
              voice->vc_OverrideTranspose = voice->vc_Transpose;
              break;
          }
          break;
      } 
      break;
  }
}

void hvl_process_step( struct hvl_tune *ht, struct hvl_voice *voice )
{
  int32  Note, Instr, donenotedel;
  struct hvl_step *Step;
  
  if( voice->vc_TrackOn == 0 )
    return;
  
  voice->vc_VolumeSlideUp = voice->vc_VolumeSlideDown = 0;
  
  Step = &ht->ht_Tracks[ht->ht_Positions[ht->ht_PosNr].pos_Track[voice->vc_VoiceNum]][ht->ht_NoteNr];
  
  Note    = Step->stp_Note;
  Instr   = Step->stp_Instrument;
  
  // --------- 1.6: from here --------------

  donenotedel = 0;

  // Do notedelay here
  if( ((Step->stp_FX&0xf)==0xe) && ((Step->stp_FXParam&0xf0)==0xd0) )
  {
    if( voice->vc_NoteDelayOn )
    {
      voice->vc_NoteDelayOn = 0;
      donenotedel = 1;
    } else {
      if( (Step->stp_FXParam&0x0f) < ht->ht_Tempo )
      {
        voice->vc_NoteDelayWait = Step->stp_FXParam & 0x0f;
        if( voice->vc_NoteDelayWait )
        {
          voice->vc_NoteDelayOn = 1;
          return;
        }
      }
    }
  }

  if( (donenotedel==0) && ((Step->stp_FXb&0xf)==0xe) && ((Step->stp_FXbParam&0xf0)==0xd0) )
  {
    if( voice->vc_NoteDelayOn )
    {
      voice->vc_NoteDelayOn = 0;
    } else {
      if( (Step->stp_FXbParam&0x0f) < ht->ht_Tempo )
      {
        voice->vc_NoteDelayWait = Step->stp_FXbParam & 0x0f;
        if( voice->vc_NoteDelayWait )
        {
          voice->vc_NoteDelayOn = 1;
          return;
        }
      }
    }
  }

  // --------- 1.6: to here --------------

  if( Note ) voice->vc_OverrideTranspose = 1000; // 1.5

  hvl_process_stepfx_1( ht, voice, Step->stp_FX&0xf,  Step->stp_FXParam );  
  hvl_process_stepfx_1( ht, voice, Step->stp_FXb&0xf, Step->stp_FXbParam );
  
  if( ( Instr ) && ( Instr <= ht->ht_InstrumentNr ) )
  {
    struct hvl_instrument *Ins;
    int16  SquareLower, SquareUpper, d6, d3, d4;
    
    /* 1.4: Reset panning to last set position */
    voice->vc_Pan          = voice->vc_SetPan;
    voice->vc_PanMultLeft  = panning_left[voice->vc_Pan];
    voice->vc_PanMultRight = panning_right[voice->vc_Pan];

    voice->vc_PeriodSlideSpeed = voice->vc_PeriodSlidePeriod = voice->vc_PeriodSlideLimit = 0;

    voice->vc_PerfSubVolume    = 0x40;
    voice->vc_ADSRVolume       = 0;
    voice->vc_Instrument       = Ins = &ht->ht_Instruments[Instr];
    voice->vc_SamplePos        = 0;
    
    voice->vc_ADSR.aFrames     = Ins->ins_Envelope.aFrames;
	voice->vc_ADSR.aVolume     = voice->vc_ADSR.aFrames ? Ins->ins_Envelope.aVolume*256/voice->vc_ADSR.aFrames : Ins->ins_Envelope.aVolume * 256; // XXX
    voice->vc_ADSR.dFrames     = Ins->ins_Envelope.dFrames;
	voice->vc_ADSR.dVolume     = voice->vc_ADSR.dFrames ? (Ins->ins_Envelope.dVolume-Ins->ins_Envelope.aVolume)*256/voice->vc_ADSR.dFrames : Ins->ins_Envelope.dVolume * 256; // XXX
    voice->vc_ADSR.sFrames     = Ins->ins_Envelope.sFrames;
    voice->vc_ADSR.rFrames     = Ins->ins_Envelope.rFrames;
	voice->vc_ADSR.rVolume     = voice->vc_ADSR.rFrames ? (Ins->ins_Envelope.rVolume-Ins->ins_Envelope.dVolume)*256/voice->vc_ADSR.rFrames : Ins->ins_Envelope.rVolume * 256; // XXX
    
    voice->vc_WaveLength       = Ins->ins_WaveLength;
    voice->vc_NoteMaxVolume    = Ins->ins_Volume;
    
    voice->vc_VibratoCurrent   = 0;
    voice->vc_VibratoDelay     = Ins->ins_VibratoDelay;
    voice->vc_VibratoDepth     = Ins->ins_VibratoDepth;
    voice->vc_VibratoSpeed     = Ins->ins_VibratoSpeed;
    voice->vc_VibratoPeriod    = 0;
    
    voice->vc_HardCutRelease   = Ins->ins_HardCutRelease;
    voice->vc_HardCut          = Ins->ins_HardCutReleaseFrames;
    
    voice->vc_IgnoreSquare = voice->vc_SquareSlidingIn = 0;
    voice->vc_SquareWait   = voice->vc_SquareOn        = 0;
    
    SquareLower = Ins->ins_SquareLowerLimit >> (5 - voice->vc_WaveLength);
    SquareUpper = Ins->ins_SquareUpperLimit >> (5 - voice->vc_WaveLength);
    
    if( SquareUpper < SquareLower )
    {
      int16 t = SquareUpper;
      SquareUpper = SquareLower;
      SquareLower = t;
    }
    
    voice->vc_SquareUpperLimit = SquareUpper;
    voice->vc_SquareLowerLimit = SquareLower;
    
    voice->vc_IgnoreFilter    = voice->vc_FilterWait = voice->vc_FilterOn = 0;
    voice->vc_FilterSlidingIn = 0;

    d6 = Ins->ins_FilterSpeed;
    d3 = Ins->ins_FilterLowerLimit;
    d4 = Ins->ins_FilterUpperLimit;
    
    if( d3 & 0x80 ) d6 |= 0x20;
    if( d4 & 0x80 ) d6 |= 0x40;
    
    voice->vc_FilterSpeed = d6;
    d3 &= ~0x80;
    d4 &= ~0x80;
    
    if( d3 > d4 )
    {
      int16 t = d3;
      d3 = d4;
      d4 = t;
    }
    
    voice->vc_FilterUpperLimit = d4;
    voice->vc_FilterLowerLimit = d3;
    voice->vc_FilterPos        = 32;
    
    voice->vc_PerfWait  = voice->vc_PerfCurrent = 0;
    voice->vc_PerfSpeed = Ins->ins_PList.pls_Speed;
    voice->vc_PerfList  = &voice->vc_Instrument->ins_PList;
    
    voice->vc_RingMixSource   = NULL;   // No ring modulation
    voice->vc_RingSamplePos   = 0;
    voice->vc_RingPlantPeriod = 0;
    voice->vc_RingNewWaveform = 0;
  }
  
  voice->vc_PeriodSlideOn = 0;
  
  hvl_process_stepfx_2( ht, voice, Step->stp_FX&0xf,  Step->stp_FXParam,  &Note );  
  hvl_process_stepfx_2( ht, voice, Step->stp_FXb&0xf, Step->stp_FXbParam, &Note );

  if( Note )
  {
    voice->vc_TrackPeriod = Note;
    voice->vc_PlantPeriod = 1;
  }
  
  hvl_process_stepfx_3( ht, voice, Step->stp_FX&0xf,  Step->stp_FXParam );  
  hvl_process_stepfx_3( ht, voice, Step->stp_FXb&0xf, Step->stp_FXbParam );  
}

void hvl_plist_command_parse( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam )
{
  switch( FX )
  {
    case 0:
      if( ( FXParam > 0 ) && ( FXParam < 0x40 ) )
      {
        if( voice->vc_IgnoreFilter )
        {
          voice->vc_FilterPos    = voice->vc_IgnoreFilter;
          voice->vc_IgnoreFilter = 0;
        } else {
          voice->vc_FilterPos    = FXParam;
        }
        voice->vc_NewWaveform = 1;
      }
      break;

    case 1:
      voice->vc_PeriodPerfSlideSpeed = FXParam;
      voice->vc_PeriodPerfSlideOn    = 1;
      break;

    case 2:
      voice->vc_PeriodPerfSlideSpeed = -FXParam;
      voice->vc_PeriodPerfSlideOn    = 1;
      break;
    
    case 3:
      if( voice->vc_IgnoreSquare == 0 )
        voice->vc_SquarePos = FXParam >> (5-voice->vc_WaveLength);
      else
        voice->vc_IgnoreSquare = 0;
      break;
    
    case 4:
      if( FXParam == 0 )
      {
        voice->vc_SquareInit = (voice->vc_SquareOn ^= 1);
        voice->vc_SquareSign = 1;
      } else {

        if( FXParam & 0x0f )
        {
          voice->vc_SquareInit = (voice->vc_SquareOn ^= 1);
          voice->vc_SquareSign = 1;
          if(( FXParam & 0x0f ) == 0x0f )
            voice->vc_SquareSign = -1;
        }
        
        if( FXParam & 0xf0 )
        {
          voice->vc_FilterInit = (voice->vc_FilterOn ^= 1);
          voice->vc_FilterSign = 1;
          if(( FXParam & 0xf0 ) == 0xf0 )
            voice->vc_FilterSign = -1;
        }
      }
      break;
    
    case 5:
      voice->vc_PerfCurrent = FXParam;
      break;
    
    case 7:
      // Ring modulate with triangle
      if(( FXParam >= 1 ) && ( FXParam <= 0x3C ))
      {
        voice->vc_RingBasePeriod = FXParam;
        voice->vc_RingFixedPeriod = 1;
      } else if(( FXParam >= 0x81 ) && ( FXParam <= 0xBC )) {
        voice->vc_RingBasePeriod = FXParam-0x80;
        voice->vc_RingFixedPeriod = 0;
      } else {
        voice->vc_RingBasePeriod = 0;
        voice->vc_RingFixedPeriod = 0;
        voice->vc_RingNewWaveform = 0;
        voice->vc_RingAudioSource = NULL; // turn it off
        voice->vc_RingMixSource   = NULL;
        break;
      }    
      voice->vc_RingWaveform    = 0;
      voice->vc_RingNewWaveform = 1;
      voice->vc_RingPlantPeriod = 1;
      break;
    
    case 8:  // Ring modulate with sawtooth
      if(( FXParam >= 1 ) && ( FXParam <= 0x3C ))
      {
        voice->vc_RingBasePeriod = FXParam;
        voice->vc_RingFixedPeriod = 1;
      } else if(( FXParam >= 0x81 ) && ( FXParam <= 0xBC )) {
        voice->vc_RingBasePeriod = FXParam-0x80;
        voice->vc_RingFixedPeriod = 0;
      } else {
        voice->vc_RingBasePeriod = 0;
        voice->vc_RingFixedPeriod = 0;
        voice->vc_RingNewWaveform = 0;
        voice->vc_RingAudioSource = NULL;
        voice->vc_RingMixSource   = NULL;
        break;
      }

      voice->vc_RingWaveform    = 1;
      voice->vc_RingNewWaveform = 1;
      voice->vc_RingPlantPeriod = 1;
      break;

    /* New in HivelyTracker 1.4 */    
    case 9:    
      if( FXParam > 127 )
        FXParam -= 256;
      voice->vc_Pan          = (FXParam+128);
      voice->vc_PanMultLeft  = panning_left[voice->vc_Pan];
      voice->vc_PanMultRight = panning_right[voice->vc_Pan];
      break;

    case 12:
      if( FXParam <= 0x40 )
      {
        voice->vc_NoteMaxVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0x50) < 0 ) break;

      if( FXParam <= 0x40 )
      {
        voice->vc_PerfSubVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0xa0-0x50) < 0 ) break;
      
      if( FXParam <= 0x40 )
        voice->vc_TrackMasterVolume = FXParam;
      break;
    
    case 15:
      voice->vc_PerfSpeed = voice->vc_PerfWait = FXParam;
      break;
  } 
}

void hvl_process_frame( struct hvl_tune *ht, struct hvl_voice *voice )
{
  static uint8 Offsets[] = {0x00,0x04,0x04+0x08,0x04+0x08+0x10,0x04+0x08+0x10+0x20,0x04+0x08+0x10+0x20+0x40};

  if( voice->vc_TrackOn == 0 )
    return;

  if( voice->vc_NoteDelayOn )
  {
    if( voice->vc_NoteDelayWait <= 0 )
      hvl_process_step( ht, voice );
    else
      voice->vc_NoteDelayWait--;
  }
  
  if( voice->vc_HardCut )
  {
    int32 nextinst;
    
    if( ht->ht_NoteNr+1 < ht->ht_TrackLength )
      nextinst = ht->ht_Tracks[voice->vc_Track][ht->ht_NoteNr+1].stp_Instrument;
    else
      nextinst = ht->ht_Tracks[voice->vc_NextTrack][0].stp_Instrument;
    
    if( nextinst )
    {
      int32 d1;
      
      d1 = ht->ht_Tempo - voice->vc_HardCut;
      
      if( d1 < 0 ) d1 = 0;
    
      if( !voice->vc_NoteCutOn )
      {
        voice->vc_NoteCutOn       = 1;
        voice->vc_NoteCutWait     = d1;
        voice->vc_HardCutReleaseF = -(d1-ht->ht_Tempo);
      } else {
        voice->vc_HardCut = 0;
      }
    }
  }
    
  if( voice->vc_NoteCutOn )
  {
    if( voice->vc_NoteCutWait <= 0 )
    {
      voice->vc_NoteCutOn = 0;
        
      if( voice->vc_HardCutRelease )
      {
        voice->vc_ADSR.rVolume = -(voice->vc_ADSRVolume - (voice->vc_Instrument->ins_Envelope.rVolume << 8)) / voice->vc_HardCutReleaseF;
        voice->vc_ADSR.rFrames = voice->vc_HardCutReleaseF;
        voice->vc_ADSR.aFrames = voice->vc_ADSR.dFrames = voice->vc_ADSR.sFrames = 0;
      } else {
        voice->vc_NoteMaxVolume = 0;
      }
    } else {
      voice->vc_NoteCutWait--;
    }
  }
    
  // ADSR envelope
  if( voice->vc_ADSR.aFrames )
  {
    voice->vc_ADSRVolume += voice->vc_ADSR.aVolume;
      
    if( --voice->vc_ADSR.aFrames <= 0 )
      voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.aVolume << 8;

  } else if( voice->vc_ADSR.dFrames ) {
    
    voice->vc_ADSRVolume += voice->vc_ADSR.dVolume;
      
    if( --voice->vc_ADSR.dFrames <= 0 )
      voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.dVolume << 8;
    
  } else if( voice->vc_ADSR.sFrames ) {
    
    voice->vc_ADSR.sFrames--;
    
  } else if( voice->vc_ADSR.rFrames ) {
    
    voice->vc_ADSRVolume += voice->vc_ADSR.rVolume;
    
    if( --voice->vc_ADSR.rFrames <= 0 )
      voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.rVolume << 8;
  }

  // VolumeSlide
  voice->vc_NoteMaxVolume = voice->vc_NoteMaxVolume + voice->vc_VolumeSlideUp - voice->vc_VolumeSlideDown;

  if( voice->vc_NoteMaxVolume < 0 )
    voice->vc_NoteMaxVolume = 0;
  else if( voice->vc_NoteMaxVolume > 0x40 )
    voice->vc_NoteMaxVolume = 0x40;

  // Portamento
  if( voice->vc_PeriodSlideOn )
  {
    if( voice->vc_PeriodSlideWithLimit )
    {
      int32  d0, d2;
      
      d0 = voice->vc_PeriodSlidePeriod - voice->vc_PeriodSlideLimit;
      d2 = voice->vc_PeriodSlideSpeed;
      
      if( d0 > 0 )
        d2 = -d2;
      
      if( d0 )
      {
        int32 d3;
         
        d3 = (d0 + d2) ^ d0;
        
        if( d3 >= 0 )
          d0 = voice->vc_PeriodSlidePeriod + d2;
        else
          d0 = voice->vc_PeriodSlideLimit;
        
        voice->vc_PeriodSlidePeriod = d0;
        voice->vc_PlantPeriod = 1;
      }
    } else {
      voice->vc_PeriodSlidePeriod += voice->vc_PeriodSlideSpeed;
      voice->vc_PlantPeriod = 1;
    }
  }
  
  // Vibrato
  if( voice->vc_VibratoDepth )
  {
    if( voice->vc_VibratoDelay <= 0 )
    {
      voice->vc_VibratoPeriod = (vib_tab[voice->vc_VibratoCurrent] * voice->vc_VibratoDepth) >> 7;
      voice->vc_PlantPeriod = 1;
      voice->vc_VibratoCurrent = (voice->vc_VibratoCurrent + voice->vc_VibratoSpeed) & 0x3f;
    } else {
      voice->vc_VibratoDelay--;
    }
  }
  
  // PList
  if( voice->vc_PerfList != 0 )
  {
    if( voice->vc_Instrument && voice->vc_PerfCurrent < voice->vc_Instrument->ins_PList.pls_Length )
    {
      if( --voice->vc_PerfWait <= 0 )
      {
        uint32 i;
        int32 cur;
        
        cur = voice->vc_PerfCurrent++;
        voice->vc_PerfWait = voice->vc_PerfSpeed;
        
        if( voice->vc_PerfList->pls_Entries[cur].ple_Waveform )
        {
          voice->vc_Waveform             = voice->vc_PerfList->pls_Entries[cur].ple_Waveform-1;
          voice->vc_NewWaveform          = 1;
          voice->vc_PeriodPerfSlideSpeed = voice->vc_PeriodPerfSlidePeriod = 0;
        }
        
        // Holdwave
        voice->vc_PeriodPerfSlideOn = 0;
        
        for( i=0; i<2; i++ )
          hvl_plist_command_parse( ht, voice, voice->vc_PerfList->pls_Entries[cur].ple_FX[i]&0xff, voice->vc_PerfList->pls_Entries[cur].ple_FXParam[i]&0xff );
        
        // GetNote
        if( voice->vc_PerfList->pls_Entries[cur].ple_Note )
        {
          voice->vc_InstrPeriod = voice->vc_PerfList->pls_Entries[cur].ple_Note;
          voice->vc_PlantPeriod = 1;
          voice->vc_FixedNote   = voice->vc_PerfList->pls_Entries[cur].ple_Fixed;
        }
      }
    } else {
      if( voice->vc_PerfWait )
        voice->vc_PerfWait--;
      else
        voice->vc_PeriodPerfSlideSpeed = 0;
    }
  }
  
  // PerfPortamento
  if( voice->vc_PeriodPerfSlideOn )
  {
    voice->vc_PeriodPerfSlidePeriod -= voice->vc_PeriodPerfSlideSpeed;
    
    if( voice->vc_PeriodPerfSlidePeriod )
      voice->vc_PlantPeriod = 1;
  }
  
  if( voice->vc_Waveform == 3-1 && voice->vc_SquareOn )
  {
    if( --voice->vc_SquareWait <= 0 )
    {
      int32 d1, d2, d3;
      
      d1 = voice->vc_SquareLowerLimit;
      d2 = voice->vc_SquareUpperLimit;
      d3 = voice->vc_SquarePos;
      
      if( voice->vc_SquareInit )
      {
        voice->vc_SquareInit = 0;
        
        if( d3 <= d1 )
        {
          voice->vc_SquareSlidingIn = 1;
          voice->vc_SquareSign = 1;
        } else if( d3 >= d2 ) {
          voice->vc_SquareSlidingIn = 1;
          voice->vc_SquareSign = -1;
        }
      }
      
      // NoSquareInit
      if( d1 == d3 || d2 == d3 )
      {
        if( voice->vc_SquareSlidingIn )
          voice->vc_SquareSlidingIn = 0;
        else
          voice->vc_SquareSign = -voice->vc_SquareSign;
      }
      
      d3 += voice->vc_SquareSign;
      voice->vc_SquarePos   = d3;
      voice->vc_PlantSquare = 1;
      voice->vc_SquareWait  = voice->vc_Instrument->ins_SquareSpeed;
    }
  }
  
  if( voice->vc_FilterOn && --voice->vc_FilterWait <= 0 )
  {
    uint32 i, FMax;
    int32 d1, d2, d3;
    
    d1 = voice->vc_FilterLowerLimit;
    d2 = voice->vc_FilterUpperLimit;
    d3 = voice->vc_FilterPos;
    
    if( voice->vc_FilterInit )
    {
      voice->vc_FilterInit = 0;
      if( d3 <= d1 )
      {
        voice->vc_FilterSlidingIn = 1;
        voice->vc_FilterSign      = 1;
      } else if( d3 >= d2 ) {
        voice->vc_FilterSlidingIn = 1;
        voice->vc_FilterSign      = -1;
      }
    }
    
    // NoFilterInit
    FMax = (voice->vc_FilterSpeed < 3) ? (5-voice->vc_FilterSpeed) : 1;

    for( i=0; i<FMax; i++ )
    {
      if( ( d1 == d3 ) || ( d2 == d3 ) )
      {
        if( voice->vc_FilterSlidingIn )
          voice->vc_FilterSlidingIn = 0;
        else
          voice->vc_FilterSign = -voice->vc_FilterSign;
      }
      d3 += voice->vc_FilterSign;
    }
    
    if( d3 < 1 )  d3 = 1;
    if( d3 > 63 ) d3 = 63;
    voice->vc_FilterPos   = d3;
    voice->vc_NewWaveform = 1;
    voice->vc_FilterWait  = voice->vc_FilterSpeed - 3;
    
    if( voice->vc_FilterWait < 1 )
      voice->vc_FilterWait = 1;
  }

  if( voice->vc_Waveform == 3-1 || voice->vc_PlantSquare )
  {
    // CalcSquare
    uint32  i;
    int32   Delta;
    int8   *SquarePtr;
    int32  X;
#ifdef USE_HVL_FILTER_FOR_HVL
    int8 *thewaves = (ht->ht_IsHt) ? waves : ahxwaves;

    SquarePtr = &thewaves[WO_SQUARES+(voice->vc_FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3)];
#else
    SquarePtr = &waves[WO_SQUARES+(voice->vc_FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3)];
#endif
    X = voice->vc_SquarePos << (5 - voice->vc_WaveLength);
    
    if( X > 0x20 )
    {
      X = 0x40 - X;
      voice->vc_SquareReverse = 1;
    }
    
    // OkDownSquare
    if( X > 0 )
      SquarePtr += (X-1) << 7;
    
    Delta = 32 >> voice->vc_WaveLength;
    ht->ht_WaveformTab[2] = voice->vc_SquareTempBuffer;
    
    for( i=0; i<(1<<voice->vc_WaveLength)*4; i++ )
    {
      voice->vc_SquareTempBuffer[i] = *SquarePtr;
      SquarePtr += Delta;
    }
    
    voice->vc_NewWaveform = 1;
    voice->vc_Waveform    = 3-1;
    voice->vc_PlantSquare = 0;
  }
  
  if( voice->vc_Waveform == 4-1 )
    voice->vc_NewWaveform = 1;
  
  if( voice->vc_RingNewWaveform )
  {
    int8 *rasrc;
    
    if( voice->vc_RingWaveform > 1 ) voice->vc_RingWaveform = 1;
    
    rasrc = ht->ht_WaveformTab[voice->vc_RingWaveform];
    rasrc += Offsets[voice->vc_WaveLength];
    
    voice->vc_RingAudioSource = rasrc;
  }    
        
  
  if( voice->vc_NewWaveform )
  {
    int8 *AudioSource;

    AudioSource = ht->ht_WaveformTab[voice->vc_Waveform];

    if( voice->vc_Waveform != 3-1 )
      AudioSource += (voice->vc_FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3);

    if( voice->vc_Waveform < 3-1)
    {
      // GetWLWaveformlor2
      AudioSource += Offsets[voice->vc_WaveLength];
    }

    if( voice->vc_Waveform == 4-1 )
    {
      // AddRandomMoving
      AudioSource += ( voice->vc_WNRandom & (2*0x280-1) ) & ~1;
      // GoOnRandom
      voice->vc_WNRandom += 2239384;
      voice->vc_WNRandom  = ((((voice->vc_WNRandom >> 8) | (voice->vc_WNRandom << 24)) + 782323) ^ 75) - 6735;
    }

    voice->vc_AudioSource = AudioSource;
  }
  
  // Ring modulation period calculation
  if( voice->vc_RingAudioSource )
  {
    voice->vc_RingAudioPeriod = voice->vc_RingBasePeriod;
  
    if( !(voice->vc_RingFixedPeriod) )
    {
      if( voice->vc_OverrideTranspose != 1000 )  // 1.5
        voice->vc_RingAudioPeriod += voice->vc_OverrideTranspose + voice->vc_TrackPeriod - 1;
      else
        voice->vc_RingAudioPeriod += voice->vc_Transpose + voice->vc_TrackPeriod - 1;
    }
  
    if( voice->vc_RingAudioPeriod > 5*12 )
      voice->vc_RingAudioPeriod = 5*12;
  
    if( voice->vc_RingAudioPeriod < 0 )
      voice->vc_RingAudioPeriod = 0;
  
    voice->vc_RingAudioPeriod = period_tab[voice->vc_RingAudioPeriod];

    if( !(voice->vc_RingFixedPeriod) )
      voice->vc_RingAudioPeriod += voice->vc_PeriodSlidePeriod;

    voice->vc_RingAudioPeriod += voice->vc_PeriodPerfSlidePeriod + voice->vc_VibratoPeriod;

    if( voice->vc_RingAudioPeriod > 0x0d60 )
      voice->vc_RingAudioPeriod = 0x0d60;

    if( voice->vc_RingAudioPeriod < 0x0071 )
      voice->vc_RingAudioPeriod = 0x0071;
  }
  
  // Normal period calculation
  voice->vc_AudioPeriod = voice->vc_InstrPeriod;
  
  if( !(voice->vc_FixedNote) )
  {
    if( voice->vc_OverrideTranspose != 1000 ) // 1.5
      voice->vc_AudioPeriod += voice->vc_OverrideTranspose + voice->vc_TrackPeriod - 1;
    else
      voice->vc_AudioPeriod += voice->vc_Transpose + voice->vc_TrackPeriod - 1;
  }
    
  if( voice->vc_AudioPeriod > 5*12 )
    voice->vc_AudioPeriod = 5*12;
  
  if( voice->vc_AudioPeriod < 0 )
    voice->vc_AudioPeriod = 0;
  
  voice->vc_AudioPeriod = period_tab[voice->vc_AudioPeriod];
  
  if( !(voice->vc_FixedNote) )
    voice->vc_AudioPeriod += voice->vc_PeriodSlidePeriod;

  voice->vc_AudioPeriod += voice->vc_PeriodPerfSlidePeriod + voice->vc_VibratoPeriod;    

  if( voice->vc_AudioPeriod > 0x0d60 )
    voice->vc_AudioPeriod = 0x0d60;

  if( voice->vc_AudioPeriod < 0x0071 )
    voice->vc_AudioPeriod = 0x0071;
  
  voice->vc_AudioVolume = (((((((voice->vc_ADSRVolume >> 8) * voice->vc_NoteMaxVolume) >> 6) * voice->vc_PerfSubVolume) >> 6) * voice->vc_TrackMasterVolume) >> 6);
}

void hvl_set_audio( struct hvl_voice *voice, float64 freqf )
{
  if( voice->vc_TrackOn == 0 )
  {
    voice->vc_VoiceVolume = 0;
    return;
  }
  
  voice->vc_VoiceVolume = voice->vc_AudioVolume;
  
  if( voice->vc_PlantPeriod )
  {
    float64 freq2;
	uint32  idelta;
    
    voice->vc_PlantPeriod = 0;
    voice->vc_VoicePeriod = voice->vc_AudioPeriod;
    
    freq2 = Period2Freq( voice->vc_AudioPeriod );
	idelta = (uint32)(freqf / freq2 * 65536.f);

    if( idelta == 0 ) idelta = 1;
    voice->vc_Delta = idelta;
  }
  
  if( voice->vc_NewWaveform )
  {
    int8 *src;
    
    src = voice->vc_AudioSource;
    
    if( voice->vc_Waveform == 4-1 )
    {
      memcpy( &voice->vc_VoiceBuffer[0], src, 0x280 );
    } else {
      uint32 i, WaveLoops;

      WaveLoops = (1 << (5 - voice->vc_WaveLength)) * 5;

      for( i=0; i<WaveLoops; i++ )
        memcpy( &voice->vc_VoiceBuffer[i*4*(1<<voice->vc_WaveLength)], src, 4*(1<<voice->vc_WaveLength) );
    }

    voice->vc_VoiceBuffer[0x280] = voice->vc_VoiceBuffer[0];
    voice->vc_MixSource          = voice->vc_VoiceBuffer;
  }

  /* Ring Modulation */
  if( voice->vc_RingPlantPeriod )
  {
    float64 freq2;
    uint32  idelta;
    
    voice->vc_RingPlantPeriod = 0;
    freq2 = Period2Freq( voice->vc_RingAudioPeriod );
    idelta = (uint32)(freqf / freq2 * 65536.f);
    
    if( idelta == 0 ) idelta = 1;
    voice->vc_RingDelta = idelta;
  }
  
  if( voice->vc_RingNewWaveform )
  {
    int8 *src;
    uint32 i, WaveLoops;
    
    src = voice->vc_RingAudioSource;

    WaveLoops = (1 << (5 - voice->vc_WaveLength)) * 5;

    for( i=0; i<WaveLoops; i++ )
      memcpy( &voice->vc_RingVoiceBuffer[i*4*(1<<voice->vc_WaveLength)], src, 4*(1<<voice->vc_WaveLength) );

    voice->vc_RingVoiceBuffer[0x280] = voice->vc_RingVoiceBuffer[0];
    voice->vc_RingMixSource          = voice->vc_RingVoiceBuffer;
  }
}

void hvl_play_irq( struct hvl_tune *ht )
{
  uint32 i;

  if( ht->ht_StepWaitFrames <= 0 )
  {
    if( ht->ht_GetNewPosition )
    {
      int32 nextpos = (ht->ht_PosNr+1==ht->ht_PositionNr)?0:(ht->ht_PosNr+1);

      for( i=0; i<ht->ht_Channels; i++ )
      {
        ht->ht_Voices[i].vc_Track         = ht->ht_Positions[ht->ht_PosNr].pos_Track[i];
        ht->ht_Voices[i].vc_Transpose     = ht->ht_Positions[ht->ht_PosNr].pos_Transpose[i];
        ht->ht_Voices[i].vc_NextTrack     = ht->ht_Positions[nextpos].pos_Track[i];
        ht->ht_Voices[i].vc_NextTranspose = ht->ht_Positions[nextpos].pos_Transpose[i];
      }
      ht->ht_GetNewPosition = 0;
    }
    
    for( i=0; i<ht->ht_Channels; i++ )
      hvl_process_step( ht, &ht->ht_Voices[i] );
    
    ht->ht_StepWaitFrames = ht->ht_Tempo;
  }
  
  for( i=0; i<ht->ht_Channels; i++ )
    hvl_process_frame( ht, &ht->ht_Voices[i] );

  ht->ht_PlayingTime++;
  if( ht->ht_Tempo > 0 && --ht->ht_StepWaitFrames <= 0 )
  {
    if( !ht->ht_PatternBreak )
    {
      ht->ht_NoteNr++;
      if( ht->ht_NoteNr >= ht->ht_TrackLength )
      {
        ht->ht_PosJump      = ht->ht_PosNr+1;
        ht->ht_PosJumpNote  = 0;
        ht->ht_PatternBreak = 1;
      }
    }
    
    if( ht->ht_PatternBreak )
    {
      ht->ht_PatternBreak = 0;
      ht->ht_PosNr        = ht->ht_PosJump;
      ht->ht_NoteNr       = ht->ht_PosJumpNote;
      if( ht->ht_PosNr == ht->ht_PositionNr )
      {
        ht->ht_SongEndReached = 1;
        ht->ht_PosNr          = ht->ht_Restart;
      }
      ht->ht_PosJumpNote  = 0;
      ht->ht_PosJump      = 0;

      ht->ht_GetNewPosition = 1;
    }
  }

  for( i=0; i<ht->ht_Channels; i++ )
    hvl_set_audio( &ht->ht_Voices[i], ht->ht_Frequency );
}

void hvl_mixchunk( struct hvl_tune *ht, uint32 samples, int8 *buf1, int8 *buf2, int32 bufmod )
{
  int8   *src[MAX_CHANNELS];
  int8   *rsrc[MAX_CHANNELS];
  uint32  delta[MAX_CHANNELS];
  uint32  rdelta[MAX_CHANNELS];
  int32   vol[MAX_CHANNELS];
  uint32  pos[MAX_CHANNELS];
  uint32  rpos[MAX_CHANNELS];
  uint32  cnt;
  int32   panl[MAX_CHANNELS];
  int32   panr[MAX_CHANNELS];
//  uint32  vu[MAX_CHANNELS];
  int32   last_amp[MAX_CHANNELS][2];
  uint32  last_clock[MAX_CHANNELS][2];
  uint32  clock, rclock, next_clock, current_clock, target_clock;
  int32   j, blip_deltal, blip_deltar, last_ampl, last_ampr;
  uint32  i, chans, loops;
  
  chans = ht->ht_Channels;
  for( i=0; i<chans; i++ )
  {
    delta[i] = ht->ht_Voices[i].vc_Delta;
    vol[i]   = ht->ht_Voices[i].vc_VoiceVolume;
    pos[i]   = ht->ht_Voices[i].vc_SamplePos;
    src[i]   = ht->ht_Voices[i].vc_MixSource;
    panl[i]  = ht->ht_Voices[i].vc_PanMultLeft;
    panr[i]  = ht->ht_Voices[i].vc_PanMultRight;
    
    /* Ring Modulation */
    rdelta[i]= ht->ht_Voices[i].vc_RingDelta;
    rpos[i]  = ht->ht_Voices[i].vc_RingSamplePos;
    rsrc[i]  = ht->ht_Voices[i].vc_RingMixSource;

	last_amp[i][0]   = ht->ht_Voices[i].vc_LastAmp[0];
	last_amp[i][1]   = ht->ht_Voices[i].vc_LastAmp[1];
	last_clock[i][0] = ht->ht_Voices[i].vc_LastClock[0];
	last_clock[i][1] = ht->ht_Voices[i].vc_LastClock[1];
    
//    vu[i] = 0;
  }
  
  do
  {
    loops = samples;
	if (loops > 256) loops = 256;

    samples -= loops;

	target_clock = loops << 16;
    
    // Inner loop
      for( i=0; i<chans; i++ )
      {
		if( delta[i] == ~0 ) continue;
		last_ampl = last_amp[i][0];
		last_ampr = last_amp[i][1];
		clock = last_clock[i][0];
        rclock = last_clock[i][1];
		current_clock = clock;
		if( rsrc[i] && rclock < clock ) current_clock = rclock;
		while( current_clock < target_clock )
		{
		  next_clock = clock + delta[i];
		  if( rsrc[i] && rclock + rdelta[i] < next_clock )
			next_clock = rclock + rdelta[i];
		  j = src[i][pos[i]];
		  if( clock < next_clock )
		  {
			clock += delta[i];
			pos[i] = (pos[i] + 1) % 0x280;
		  }
		  if( rsrc[i] )
          {
			j = (j*rsrc[i][rpos[i]])>>7;
			if( rclock < next_clock )
			{
			  rclock += rdelta[i];
			  rpos[i] = (rpos[i] + 1) % 0x280;
			}
          }
		  j *= vol[i];

//        if( abs( j ) > vu[i] ) vu[i] = abs( j );

          blip_deltal = ((j * panl[i]) >> 7) - last_ampl;
          blip_deltar = ((j * panr[i]) >> 7) - last_ampr;
		  last_ampl += blip_deltal;
		  last_ampr += blip_deltar;
          if( blip_deltal ) hvl_blip_add_delta( ht->ht_BlipBuffers[0], current_clock, blip_deltal );
          if( blip_deltar ) hvl_blip_add_delta( ht->ht_BlipBuffers[1], current_clock, blip_deltar );

		  current_clock = next_clock;
		}
		clock -= target_clock;
		if( rsrc[i] ) rclock -= target_clock;
		last_clock[i][0] = clock;
		last_clock[i][1] = rclock;
		last_amp[i][0] = last_ampl;
		last_amp[i][1] = last_ampr;
      }

      hvl_blip_end_frame( ht->ht_BlipBuffers[0], target_clock );
      hvl_blip_end_frame( ht->ht_BlipBuffers[1], target_clock );

      hvl_blip_read_samples( ht->ht_BlipBuffers[0], (int*)buf1, loops, ht->ht_mixgain );
      hvl_blip_read_samples( ht->ht_BlipBuffers[1], (int*)buf2, loops, ht->ht_mixgain );

	  buf1 += bufmod * loops;
	  buf2 += bufmod * loops;
  } while( samples > 0 );

  for( i=0; i<chans; i++ )
  {
    ht->ht_Voices[i].vc_SamplePos = pos[i];
    ht->ht_Voices[i].vc_RingSamplePos = rpos[i];
	ht->ht_Voices[i].vc_LastAmp[0] = last_amp[i][0];
	ht->ht_Voices[i].vc_LastAmp[1] = last_amp[i][1];
	ht->ht_Voices[i].vc_LastClock[0] = last_clock[i][0];
	ht->ht_Voices[i].vc_LastClock[1] = last_clock[i][1];
//    ht->ht_Voices[i].vc_VUMeter = vu[i];
  }
}

void hvl_DecodeFrame( struct hvl_tune *ht, int8 *buf1, int8 *buf2, int32 bufmod )
{
  uint32 samples, loops;
  
  samples = ht->ht_Frequency/50/ht->ht_SpeedMultiplier;
  loops   = ht->ht_SpeedMultiplier;
  
  do
  {
    hvl_play_irq( ht );
    hvl_mixchunk( ht, samples, buf1, buf2, bufmod );
    buf1 += samples * bufmod;
    buf2 += samples * bufmod;
    loops--;
  } while( loops );
}
