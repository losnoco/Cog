/////////////////////////////////////////////////////////////////////////////
//
// spucore - Emulates a single SPU CORE
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "spucore.h"

////////////////////////////////////////////////////////////////////////////////
/*
** Key-on defer
**
** Bit of a hack I did to stop the clicking in Final Fantasy Tactics
** If there's a key-on while the envelope is still active, the key-on is
** deferred by this many samples while the existing channel is microramped
** down to zero (works best if it's a power of 2)
*/

//#define KEYON_DEFER_SAMPLES (64)
#define KEYON_DEFER_SAMPLES (64)

//
// Render max samples 
//
#define RENDERMAX (200)

////////////////////////////////////////////////////////////////////////////////
/*
** Static information
*/
static const sint16 gauss_shuffled_reverse_table[1024] = {
 0x12c7, 0x59b3, 0x1307, 0xffff, 0x1288, 0x59b2, 0x1347, 0xffff, 0x1249, 0x59b0, 0x1388, 0xffff, 0x120b, 0x59ad, 0x13c9, 0xffff,
 0x11cd, 0x59a9, 0x140b, 0xffff, 0x118f, 0x59a4, 0x144d, 0xffff, 0x1153, 0x599e, 0x1490, 0xffff, 0x1116, 0x5997, 0x14d4, 0xffff,
 0x10db, 0x598f, 0x1517, 0xffff, 0x109f, 0x5986, 0x155c, 0xffff, 0x1065, 0x597c, 0x15a0, 0xffff, 0x102a, 0x5971, 0x15e6, 0xffff,
 0x0ff1, 0x5965, 0x162c, 0xffff, 0x0fb7, 0x5958, 0x1672, 0xffff, 0x0f7f, 0x5949, 0x16b9, 0xffff, 0x0f46, 0x593a, 0x1700, 0xffff,
 0x0f0f, 0x592a, 0x1747, 0x0000, 0x0ed7, 0x5919, 0x1790, 0x0000, 0x0ea1, 0x5907, 0x17d8, 0x0000, 0x0e6b, 0x58f4, 0x1821, 0x0000,
 0x0e35, 0x58e0, 0x186b, 0x0000, 0x0e00, 0x58cb, 0x18b5, 0x0000, 0x0dcb, 0x58b5, 0x1900, 0x0000, 0x0d97, 0x589e, 0x194b, 0x0001,
 0x0d63, 0x5886, 0x1996, 0x0001, 0x0d30, 0x586d, 0x19e2, 0x0001, 0x0cfd, 0x5853, 0x1a2e, 0x0001, 0x0ccb, 0x5838, 0x1a7b, 0x0002,
 0x0c99, 0x581c, 0x1ac8, 0x0002, 0x0c68, 0x57ff, 0x1b16, 0x0002, 0x0c38, 0x57e2, 0x1b64, 0x0003, 0x0c07, 0x57c3, 0x1bb3, 0x0003,
 0x0bd8, 0x57a3, 0x1c02, 0x0003, 0x0ba9, 0x5782, 0x1c51, 0x0004, 0x0b7a, 0x5761, 0x1ca1, 0x0004, 0x0b4c, 0x573e, 0x1cf1, 0x0005,
 0x0b1e, 0x571b, 0x1d42, 0x0005, 0x0af1, 0x56f6, 0x1d93, 0x0006, 0x0ac4, 0x56d1, 0x1de5, 0x0007, 0x0a98, 0x56ab, 0x1e37, 0x0007,
 0x0a6c, 0x5684, 0x1e89, 0x0008, 0x0a40, 0x565b, 0x1edc, 0x0009, 0x0a16, 0x5632, 0x1f2f, 0x0009, 0x09eb, 0x5609, 0x1f82, 0x000a,
 0x09c1, 0x55de, 0x1fd6, 0x000b, 0x0998, 0x55b2, 0x202a, 0x000c, 0x096f, 0x5585, 0x207f, 0x000d, 0x0946, 0x5558, 0x20d4, 0x000e,
 0x091e, 0x5529, 0x2129, 0x000f, 0x08f7, 0x54fa, 0x217f, 0x0010, 0x08d0, 0x54ca, 0x21d5, 0x0011, 0x08a9, 0x5499, 0x222c, 0x0012,
 0x0883, 0x5467, 0x2282, 0x0013, 0x085d, 0x5434, 0x22da, 0x0015, 0x0838, 0x5401, 0x2331, 0x0016, 0x0813, 0x53cc, 0x2389, 0x0018,
 0x07ef, 0x5397, 0x23e1, 0x0019, 0x07cb, 0x5361, 0x2439, 0x001b, 0x07a7, 0x532a, 0x2492, 0x001c, 0x0784, 0x52f3, 0x24eb, 0x001e,
 0x0762, 0x52ba, 0x2545, 0x0020, 0x0740, 0x5281, 0x259e, 0x0021, 0x071e, 0x5247, 0x25f8, 0x0023, 0x06fd, 0x520c, 0x2653, 0x0025,
 0x06dc, 0x51d0, 0x26ad, 0x0027, 0x06bb, 0x5194, 0x2708, 0x0029, 0x069b, 0x5156, 0x2763, 0x002c, 0x067c, 0x5118, 0x27be, 0x002e,
 0x065c, 0x50da, 0x281a, 0x0030, 0x063e, 0x509a, 0x2876, 0x0033, 0x061f, 0x505a, 0x28d2, 0x0035, 0x0601, 0x5019, 0x292e, 0x0038,
 0x05e4, 0x4fd7, 0x298b, 0x003a, 0x05c7, 0x4f95, 0x29e7, 0x003d, 0x05aa, 0x4f52, 0x2a44, 0x0040, 0x058e, 0x4f0e, 0x2aa1, 0x0043,
 0x0572, 0x4ec9, 0x2aff, 0x0046, 0x0556, 0x4e84, 0x2b5c, 0x0049, 0x053b, 0x4e3e, 0x2bba, 0x004d, 0x0520, 0x4df7, 0x2c18, 0x0050,
 0x0506, 0x4db0, 0x2c76, 0x0054, 0x04ec, 0x4d68, 0x2cd4, 0x0057, 0x04d2, 0x4d20, 0x2d33, 0x005b, 0x04b9, 0x4cd7, 0x2d91, 0x005f,
 0x04a0, 0x4c8d, 0x2df0, 0x0063, 0x0488, 0x4c42, 0x2e4f, 0x0067, 0x0470, 0x4bf7, 0x2eae, 0x006b, 0x0458, 0x4bac, 0x2f0d, 0x006f,
 0x0441, 0x4b5f, 0x2f6c, 0x0074, 0x042a, 0x4b13, 0x2fcc, 0x0078, 0x0413, 0x4ac5, 0x302b, 0x007d, 0x03fc, 0x4a77, 0x308b, 0x0082,
 0x03e7, 0x4a29, 0x30ea, 0x0087, 0x03d1, 0x49d9, 0x314a, 0x008c, 0x03bc, 0x498a, 0x31aa, 0x0091, 0x03a7, 0x493a, 0x3209, 0x0096,
 0x0392, 0x48e9, 0x3269, 0x009c, 0x037e, 0x4898, 0x32c9, 0x00a1, 0x036a, 0x4846, 0x3329, 0x00a7, 0x0356, 0x47f4, 0x3389, 0x00ad,
 0x0343, 0x47a1, 0x33e9, 0x00b3, 0x0330, 0x474e, 0x3449, 0x00ba, 0x031d, 0x46fa, 0x34a9, 0x00c0, 0x030b, 0x46a6, 0x3509, 0x00c7,
 0x02f9, 0x4651, 0x3569, 0x00cd, 0x02e7, 0x45fc, 0x35c9, 0x00d4, 0x02d6, 0x45a6, 0x3629, 0x00db, 0x02c4, 0x4550, 0x3689, 0x00e3,
 0x02b4, 0x44fa, 0x36e8, 0x00ea, 0x02a3, 0x44a3, 0x3748, 0x00f2, 0x0293, 0x444c, 0x37a8, 0x00fa, 0x0283, 0x43f4, 0x3807, 0x0101,
 0x0273, 0x439c, 0x3867, 0x010a, 0x0264, 0x4344, 0x38c6, 0x0112, 0x0255, 0x42eb, 0x3926, 0x011b, 0x0246, 0x4292, 0x3985, 0x0123,
 0x0237, 0x4239, 0x39e4, 0x012c, 0x0229, 0x41df, 0x3a43, 0x0135, 0x021b, 0x4185, 0x3aa2, 0x013f, 0x020d, 0x412a, 0x3b00, 0x0148,
 0x0200, 0x40d0, 0x3b5f, 0x0152, 0x01f2, 0x4074, 0x3bbd, 0x015c, 0x01e5, 0x4019, 0x3c1b, 0x0166, 0x01d9, 0x3fbd, 0x3c79, 0x0171,
 0x01cc, 0x3f62, 0x3cd7, 0x017b, 0x01c0, 0x3f05, 0x3d35, 0x0186, 0x01b4, 0x3ea9, 0x3d92, 0x0191, 0x01a8, 0x3e4c, 0x3def, 0x019c,
 0x019c, 0x3def, 0x3e4c, 0x01a8, 0x0191, 0x3d92, 0x3ea9, 0x01b4, 0x0186, 0x3d35, 0x3f05, 0x01c0, 0x017b, 0x3cd7, 0x3f62, 0x01cc,
 0x0171, 0x3c79, 0x3fbd, 0x01d9, 0x0166, 0x3c1b, 0x4019, 0x01e5, 0x015c, 0x3bbd, 0x4074, 0x01f2, 0x0152, 0x3b5f, 0x40d0, 0x0200,
 0x0148, 0x3b00, 0x412a, 0x020d, 0x013f, 0x3aa2, 0x4185, 0x021b, 0x0135, 0x3a43, 0x41df, 0x0229, 0x012c, 0x39e4, 0x4239, 0x0237,
 0x0123, 0x3985, 0x4292, 0x0246, 0x011b, 0x3926, 0x42eb, 0x0255, 0x0112, 0x38c6, 0x4344, 0x0264, 0x010a, 0x3867, 0x439c, 0x0273,
 0x0101, 0x3807, 0x43f4, 0x0283, 0x00fa, 0x37a8, 0x444c, 0x0293, 0x00f2, 0x3748, 0x44a3, 0x02a3, 0x00ea, 0x36e8, 0x44fa, 0x02b4,
 0x00e3, 0x3689, 0x4550, 0x02c4, 0x00db, 0x3629, 0x45a6, 0x02d6, 0x00d4, 0x35c9, 0x45fc, 0x02e7, 0x00cd, 0x3569, 0x4651, 0x02f9,
 0x00c7, 0x3509, 0x46a6, 0x030b, 0x00c0, 0x34a9, 0x46fa, 0x031d, 0x00ba, 0x3449, 0x474e, 0x0330, 0x00b3, 0x33e9, 0x47a1, 0x0343,
 0x00ad, 0x3389, 0x47f4, 0x0356, 0x00a7, 0x3329, 0x4846, 0x036a, 0x00a1, 0x32c9, 0x4898, 0x037e, 0x009c, 0x3269, 0x48e9, 0x0392,
 0x0096, 0x3209, 0x493a, 0x03a7, 0x0091, 0x31aa, 0x498a, 0x03bc, 0x008c, 0x314a, 0x49d9, 0x03d1, 0x0087, 0x30ea, 0x4a29, 0x03e7,
 0x0082, 0x308b, 0x4a77, 0x03fc, 0x007d, 0x302b, 0x4ac5, 0x0413, 0x0078, 0x2fcc, 0x4b13, 0x042a, 0x0074, 0x2f6c, 0x4b5f, 0x0441,
 0x006f, 0x2f0d, 0x4bac, 0x0458, 0x006b, 0x2eae, 0x4bf7, 0x0470, 0x0067, 0x2e4f, 0x4c42, 0x0488, 0x0063, 0x2df0, 0x4c8d, 0x04a0,
 0x005f, 0x2d91, 0x4cd7, 0x04b9, 0x005b, 0x2d33, 0x4d20, 0x04d2, 0x0057, 0x2cd4, 0x4d68, 0x04ec, 0x0054, 0x2c76, 0x4db0, 0x0506,
 0x0050, 0x2c18, 0x4df7, 0x0520, 0x004d, 0x2bba, 0x4e3e, 0x053b, 0x0049, 0x2b5c, 0x4e84, 0x0556, 0x0046, 0x2aff, 0x4ec9, 0x0572,
 0x0043, 0x2aa1, 0x4f0e, 0x058e, 0x0040, 0x2a44, 0x4f52, 0x05aa, 0x003d, 0x29e7, 0x4f95, 0x05c7, 0x003a, 0x298b, 0x4fd7, 0x05e4,
 0x0038, 0x292e, 0x5019, 0x0601, 0x0035, 0x28d2, 0x505a, 0x061f, 0x0033, 0x2876, 0x509a, 0x063e, 0x0030, 0x281a, 0x50da, 0x065c,
 0x002e, 0x27be, 0x5118, 0x067c, 0x002c, 0x2763, 0x5156, 0x069b, 0x0029, 0x2708, 0x5194, 0x06bb, 0x0027, 0x26ad, 0x51d0, 0x06dc,
 0x0025, 0x2653, 0x520c, 0x06fd, 0x0023, 0x25f8, 0x5247, 0x071e, 0x0021, 0x259e, 0x5281, 0x0740, 0x0020, 0x2545, 0x52ba, 0x0762,
 0x001e, 0x24eb, 0x52f3, 0x0784, 0x001c, 0x2492, 0x532a, 0x07a7, 0x001b, 0x2439, 0x5361, 0x07cb, 0x0019, 0x23e1, 0x5397, 0x07ef,
 0x0018, 0x2389, 0x53cc, 0x0813, 0x0016, 0x2331, 0x5401, 0x0838, 0x0015, 0x22da, 0x5434, 0x085d, 0x0013, 0x2282, 0x5467, 0x0883,
 0x0012, 0x222c, 0x5499, 0x08a9, 0x0011, 0x21d5, 0x54ca, 0x08d0, 0x0010, 0x217f, 0x54fa, 0x08f7, 0x000f, 0x2129, 0x5529, 0x091e,
 0x000e, 0x20d4, 0x5558, 0x0946, 0x000d, 0x207f, 0x5585, 0x096f, 0x000c, 0x202a, 0x55b2, 0x0998, 0x000b, 0x1fd6, 0x55de, 0x09c1,
 0x000a, 0x1f82, 0x5609, 0x09eb, 0x0009, 0x1f2f, 0x5632, 0x0a16, 0x0009, 0x1edc, 0x565b, 0x0a40, 0x0008, 0x1e89, 0x5684, 0x0a6c,
 0x0007, 0x1e37, 0x56ab, 0x0a98, 0x0007, 0x1de5, 0x56d1, 0x0ac4, 0x0006, 0x1d93, 0x56f6, 0x0af1, 0x0005, 0x1d42, 0x571b, 0x0b1e,
 0x0005, 0x1cf1, 0x573e, 0x0b4c, 0x0004, 0x1ca1, 0x5761, 0x0b7a, 0x0004, 0x1c51, 0x5782, 0x0ba9, 0x0003, 0x1c02, 0x57a3, 0x0bd8,
 0x0003, 0x1bb3, 0x57c3, 0x0c07, 0x0003, 0x1b64, 0x57e2, 0x0c38, 0x0002, 0x1b16, 0x57ff, 0x0c68, 0x0002, 0x1ac8, 0x581c, 0x0c99,
 0x0002, 0x1a7b, 0x5838, 0x0ccb, 0x0001, 0x1a2e, 0x5853, 0x0cfd, 0x0001, 0x19e2, 0x586d, 0x0d30, 0x0001, 0x1996, 0x5886, 0x0d63,
 0x0001, 0x194b, 0x589e, 0x0d97, 0x0000, 0x1900, 0x58b5, 0x0dcb, 0x0000, 0x18b5, 0x58cb, 0x0e00, 0x0000, 0x186b, 0x58e0, 0x0e35,
 0x0000, 0x1821, 0x58f4, 0x0e6b, 0x0000, 0x17d8, 0x5907, 0x0ea1, 0x0000, 0x1790, 0x5919, 0x0ed7, 0x0000, 0x1747, 0x592a, 0x0f0f,
 0xffff, 0x1700, 0x593a, 0x0f46, 0xffff, 0x16b9, 0x5949, 0x0f7f, 0xffff, 0x1672, 0x5958, 0x0fb7, 0xffff, 0x162c, 0x5965, 0x0ff1,
 0xffff, 0x15e6, 0x5971, 0x102a, 0xffff, 0x15a0, 0x597c, 0x1065, 0xffff, 0x155c, 0x5986, 0x109f, 0xffff, 0x1517, 0x598f, 0x10db,
 0xffff, 0x14d4, 0x5997, 0x1116, 0xffff, 0x1490, 0x599e, 0x1153, 0xffff, 0x144d, 0x59a4, 0x118f, 0xffff, 0x140b, 0x59a9, 0x11cd,
 0xffff, 0x13c9, 0x59ad, 0x120b, 0xffff, 0x1388, 0x59b0, 0x1249, 0xffff, 0x1347, 0x59b2, 0x1288, 0xffff, 0x1307, 0x59b3, 0x12c7,
};

static sint32 ratelogtable[32+128];

/*
// v1.10 coefs - normalized from v1.04
static const sint32 reverb_lowpass_coefs[8] = {
  (int)((-0.036346113709214548)*(2048.0)),
  (int)(( 0.044484956332843419)*(2048.0)),
  (int)(( 0.183815456609675380)*(2048.0)),
  (int)(( 0.308045700766695740)*(2048.0)),
  (int)(( 0.308045700766695740)*(2048.0)),
  (int)(( 0.183815456609675380)*(2048.0)),
  (int)(( 0.044484956332843419)*(2048.0)),
  (int)((-0.036346113709214548)*(2048.0))
};
*/

/*
// test coefs - ganked from LAME's blackman function
// (11-point filter, but only a few points were nonzero)
static const sint32 reverb_new_lowpass_coefs[3] = {
  (int)((-0.02134438446523164)*(2048.0)),
// skip one
  (int)(( 0.27085135668587779)*(2048.0)),
  (int)(( 0.50098605555870768)*(2048.0))
// rest are symmetrical
};
*/

// PS1 reverb downsampling coefficients(as best as I could extract them at the moment, some of the even(non-zero/non-16384) ones *might* be off by 1.
// -------------
#if 1
static const sint32 reverb_psx_lowpass_coefs[11] = {
   -1,
//  0,
    2,
//  0,
  -10,
//  0,
   35,
//  0,
 -103,
//  0,
  266,
//  0,
 -616,
//  0,
 1332,
//  0,
-2960,
//  0,
10246,
16384
};
#else
static const sint32 reverb_psx_lowpass_coefs[48] = {
   -1,
    0,
    2,
    0,
  -10,
    0,
   35,
    0,
 -103,
    0,
  266,
    0,
 -616,
    0,
 1332,
    0,
-2960,
    0,
10246,
16384,
10246,
    0,
-2960,
    0,
 1332,
    0,
 -616,
    0,
  266,
    0,
 -103,
    0,
   35,
    0,
  -10,
    0,
    2,
    0,
   -1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
#endif


static const int noisetable[] = {
    1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1
};

/*
** Static init
*/
sint32 EMU_CALL spucore_init(void) {
  sint32 i;

  memset(ratelogtable, 0, sizeof(ratelogtable));
  ratelogtable[32-8] = 1;
  ratelogtable[32-7] = 1;
  ratelogtable[32-6] = 1;
  ratelogtable[32-5] = 1;
  ratelogtable[32-4] = 2;
  ratelogtable[32-3] = 2;
  ratelogtable[32-2] = 3;
  ratelogtable[32-1] = 3;
  ratelogtable[32+0] = 4;
  ratelogtable[32+1] = 5;
  ratelogtable[32+2] = 6;
  ratelogtable[32+3] = 7;
  for(i = 4; i < 128; i++) {
    uint32 n = 2*ratelogtable[32+i-4];
    if(n > 0x20000000) n = 0x20000000;
    ratelogtable[32+i] = n;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*
** State information
*/
#define SPUCORESTATE ((struct SPUCORE_STATE*)(state))

#define SAMPLE_STATE_OFF    (0)
#define SAMPLE_STATE_ENDING (1)
#define SAMPLE_STATE_ON     (2)

struct SPUCORE_SAMPLE {
  uint8 state;
  uint8 array_cleared;
  sint32 array[32];
  uint32 phase;

  uint32 block_addr;
  uint32 start_block_addr;
  //uint32 start_loop_block_addr;
  uint32 loop_block_addr;
};

#define ENVELOPE_STATE_OFF     (0)
#define ENVELOPE_STATE_ATTACK  (1)
#define ENVELOPE_STATE_DECAY   (2)
#define ENVELOPE_STATE_SUSTAIN (3)
#define ENVELOPE_STATE_RELEASE (4)

struct SPUCORE_ENVELOPE {
  uint32 reg_ad;
  uint32 reg_sr;
  sint32 level;
  sint32 delta;
  int    state;
  sint32 cachemax;
};

////////////////////////////////////////////////////////////////////////////////
/*
** Volumes with increase/decrease modes
*/
struct SPUCORE_VOLUME {
  uint16 mode;
  sint32 level;
};

static EMU_INLINE void EMU_CALL volume_setmode(struct SPUCORE_VOLUME *vol, uint16 mode) {
  vol->mode = mode;
  if(mode & 0x8000) {
  } else {
    vol->level = mode;
    vol->level <<= 17;
    vol->level >>= 1;
  }
}

static EMU_INLINE uint16 EMU_CALL volume_getmode(struct SPUCORE_VOLUME *vol) {
  return vol->mode;
}

static EMU_INLINE sint32 EMU_CALL volume_getlevel(struct SPUCORE_VOLUME *vol) {
  return (vol->level) >> 15;
}

////////////////////////////////////////////////////////////////////////////////

struct SPUCORE_CHAN {
  struct SPUCORE_VOLUME vol[2];
  uint32 voice_pitch;
  struct SPUCORE_SAMPLE   sample;
  struct SPUCORE_ENVELOPE env;
  int samples_until_pending_keyon;
};

/*
** Reverb resample state
*/
struct SPUCORE_REVERB_RESAMPLER {
  sint32  in_queue_l[64];
  sint32  in_queue_r[64];
  sint32 out_queue_l[16];
  sint32 out_queue_r[16];
  int queue_index;
};

struct SPUCORE_REVERB {
  uint32 FB_SRC_A   ;
  uint32 FB_SRC_B   ;
  uint16 IIR_ALPHA  ;
  uint16 ACC_COEF_A ;
  uint16 ACC_COEF_B ;
  uint16 ACC_COEF_C ;
  uint16 ACC_COEF_D ;
  uint16 IIR_COEF   ;
  uint16 FB_ALPHA   ;
  uint16 FB_X       ;
  uint32 IIR_DEST_A0;
  uint32 IIR_DEST_A1;
  uint32 ACC_SRC_A0 ;
  uint32 ACC_SRC_A1 ;
  uint32 ACC_SRC_B0 ;
  uint32 ACC_SRC_B1 ;
  uint32 IIR_SRC_A0 ;
  uint32 IIR_SRC_A1 ;
  uint32 IIR_DEST_B0;
  uint32 IIR_DEST_B1;
  uint32 ACC_SRC_C0 ;
  uint32 ACC_SRC_C1 ;
  uint32 ACC_SRC_D0 ;
  uint32 ACC_SRC_D1 ;
  uint32 IIR_SRC_B1 ;
  uint32 IIR_SRC_B0 ;
  uint32 MIX_DEST_A0;
  uint32 MIX_DEST_A1;
  uint32 MIX_DEST_B0;
  uint32 MIX_DEST_B1;
  uint16 IN_COEF_L  ;
  uint16 IN_COEF_R  ;

  uint32 start_address;
  uint32 end_address;

  sint32 current_address;

  sint32 safe_start_address;
  sint32 safe_end_address;
  sint32 safe_size;

  struct SPUCORE_REVERB_RESAMPLER resampler;
};

struct SPUCORE_STATE {
  uint32 flags;
  sint32 memsize;
  struct SPUCORE_CHAN chan[24];
  struct SPUCORE_REVERB reverb;
  struct SPUCORE_VOLUME mvol[2];
  sint16 evol[2];
  sint16 avol[2];
  sint16 bvol[2];
  uint32 kon;
  uint32 koff;
  uint32 fm;
  uint32 noise;
  uint32 vmix[2];
  uint32 vmixe[2];
  uint32 irq_address;
  uint32 noiseclock;
  uint32 noisecounter;
  sint32 noiseval;
  uint32 irq_decoder_clock;
  uint32 irq_triggered_cycle;
};

struct SPUCORE_IRQ_STATE {
  uint32 offset;
  uint32 triggered_cycle;
};

uint32 EMU_CALL spucore_get_state_size(void) {
  return(sizeof(struct SPUCORE_STATE));
}

/*
** Initialize SPU CORE state
*/
void EMU_CALL spucore_clear_state(void *state) {
  /*
  ** Clear to zero
  */
  memset(state, 0, sizeof(struct SPUCORE_STATE));
  /*
  ** Set other defaults
  */
  SPUCORESTATE->memsize = 0x80000;
  spucore_setreg(state, SPUREG_EEA, 0x7FFFF, 0xFFFFFFFF);
  spucore_setreg(state, SPUREG_VMIX, 0x00FFFFFF, 0xFFFFFFFF);
  spucore_setflag(state, SPUREG_FLAG_MSNDL , 1);
  spucore_setflag(state, SPUREG_FLAG_MSNDR , 1);
  spucore_setflag(state, SPUREG_FLAG_MSNDEL, 1);
  spucore_setflag(state, SPUREG_FLAG_MSNDER, 1);
  spucore_setflag(state, SPUREG_FLAG_SINL, 1);
  spucore_setflag(state, SPUREG_FLAG_SINR, 1);
  SPUCORESTATE->irq_triggered_cycle = 0xFFFFFFFF;
}

void EMU_CALL spucore_set_mem_size(void *state, uint32 size) {
  SPUCORESTATE->memsize = (sint32)size;
  spucore_setreg(state, SPUREG_EEA, size-1, 0xFFFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL make_safe_reverb_addresses(struct SPUCORE_STATE *state) {
  struct SPUCORE_REVERB *r = &(state->reverb);
  sint32 sa = r->start_address;
  sint32 ea = r->end_address;

//EMUTRACE2("[sa,ea=%X,%X]\n",sa,ea);

  ea += 0x20000;
  ea &= (~0x1FFFF);
  sa &= (~1);

  if(ea > state->memsize) ea = state->memsize;
  if(ea < 0x20000) ea = 0x20000;
  if(sa > ea) {
    sa &= 0x1FFFE;
    sa += ea;
    sa -= 0x20000;
  }

  r->safe_start_address = sa;
  r->safe_end_address   = ea;
  r->safe_size          = ea-sa;

  r->current_address &= (~1);
  if(
    (r->current_address < r->safe_start_address) ||
    (r->current_address >= r->safe_end_address)
  ) {
    r->current_address = r->safe_start_address;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** ADPCM sample decoding
*/
/*
#define SPUCORE_PREDICT_SKEL(prednum,coef1,coef2) \
static void EMU_CALL spucore_predict_##prednum(uint8 *src, sint32 *dest, uint32 shift) {  \
  uint32 i;                                                                               \
  sint32 p_a = dest[-2];                                                                  \
  sint32 p_b = dest[-1];                                                                  \
  shift += 16;                                                                            \
  for(i = 0; i < 14; i++) {                                                               \
    sint32 a = src[i^(EMU_ENDIAN_XOR(1))];                                                \
    sint32 b = (a&0xF0)<<24;                                                              \
    a = a << 28; b >>= shift; a >>= shift;                                                \
    a += ( ( ((coef1)*p_b) + ((coef2)*p_a) )+32)>>6;                                      \
    if(a > 32767) { a = 32767; } if(a < (-32768)) { a = (-32768); }                       \
    *dest++ = a;                                                                          \
    b += ( ( ((coef1)* a ) + ((coef2)*p_b) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = b;                                                                          \
    p_a = a; p_b = b;                                                                     \
  }                                                                                       \
}
*/

#define SPUCORE_PREDICT_SKEL(prednum,coef1,coef2) \
static void EMU_CALL spucore_predict_##prednum(uint16 *src, sint32 *dest, uint32 shift) { \
  uint32 i;                                                                               \
  sint32 p_a = dest[-2];                                                                  \
  sint32 p_b = dest[-1];                                                                  \
  shift += 16;                                                                            \
  for(i = 0; i < 7; i++) {                                                                \
    sint32 a = *src++;                                                                    \
    sint32 b = (a&0x00F0)<<24;                                                            \
    sint32 c = (a&0x0F00)<<20;                                                            \
    sint32 d = (a&0xF000)<<16;                                                            \
    a <<= 28;                                                                             \
    a >>= shift;                                                                          \
    b >>= shift;                                                                          \
    c >>= shift;                                                                          \
    d >>= shift;                                                                          \
    a += ( ( ((coef1)*p_b) + ((coef2)*p_a) )+32)>>6;                                      \
    if(a > 32767) { a = 32767; } if(a < (-32768)) { a = (-32768); }                       \
    *dest++ = a;                                                                          \
    b += ( ( ((coef1)* a ) + ((coef2)*p_b) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = b;                                                                          \
    c += ( ( ((coef1)* b ) + ((coef2)* a ) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = c;                                                                          \
    d += ( ( ((coef1)* c ) + ((coef2)* b ) )+32)>>6;                                      \
    if(b > 32767) { b = 32767; } if(b < (-32768)) { b = (-32768); }                       \
    *dest++ = d;                                                                          \
    p_a = c; p_b = d;                                                                     \
  }                                                                                       \
}

SPUCORE_PREDICT_SKEL(0,0,0)
SPUCORE_PREDICT_SKEL(1,60,0)
SPUCORE_PREDICT_SKEL(2,115,-52)
SPUCORE_PREDICT_SKEL(3,98,-55)
SPUCORE_PREDICT_SKEL(4,122,-60)

typedef void (EMU_CALL * spucore_predict_callback_t)(uint16*, sint32*, uint32);

static spucore_predict_callback_t spucore_predict[8] = {
  spucore_predict_0, spucore_predict_1, spucore_predict_2, spucore_predict_3,
  spucore_predict_4, spucore_predict_1, spucore_predict_2, spucore_predict_3
};

static void EMU_CALL decode_sample_block(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_SAMPLE *sample,
  int skip
) {
//  uint32 i;
  if(sample->state != SAMPLE_STATE_ON) {
    if(!sample->array_cleared) {
      memset(sample->array, 0, sizeof(sample->array));
      sample->array_cleared = 1;
    }
    sample->state = SAMPLE_STATE_OFF;
  } else {
    /* temporary hack to avoid memory problems */
    sample->block_addr &= (memmax-1);
    if((sample->block_addr + 0x10) > memmax) sample->block_addr -= 0x10;
    ram += sample->block_addr >> 1;
    /* decode */
    if(skip) {
      if(!sample->array_cleared) {
        memset(sample->array, 0, sizeof(sample->array));
        sample->array_cleared = 1;
      }
    } else {
      sample->array[0] = sample->array[28];
      sample->array[1] = sample->array[29];
      sample->array[2] = sample->array[30];
      sample->array[3] = sample->array[31];
      (spucore_predict[(ram[0]>>4)&7])(
        ram + 1,
        sample->array + 4,
        ram[0] & 0xF
      );
    }
    /* set loop start address if necessary */
    if(ram[0]&0x0400) {
      sample->loop_block_addr = sample->block_addr;
    }
    /* sample end? */
    if(ram[0]&0x0100) {
      /* loop? */
      if(ram[0]&0x0200) {
        sample->block_addr = sample->loop_block_addr;
      /* no loop, just end */
      } else {
        sample->state = SAMPLE_STATE_ENDING;
      }
    } else {
      /* advance to next block */
      sample->block_addr += 16;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Returns the number of samples actually generated
// If dest is null, it means "fast forward" - don't render anything
//
//
// output of resampler() is guaranteed clipped, as long as the output of
// decode_sample_block() is
//
static uint32 EMU_CALL resampler(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_SAMPLE *sample,
  sint32 *dest,
  uint32 n,
  uint32 phase_inc,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  uint32 irq_triggered_cycle = 0xFFFFFFFF;
  uint32 s;
  uint32 ph; //, phl;
  ph = sample->phase;
  if(!dest) {
    ph += phase_inc * n;
    s = 0;
    while(ph >= 0x1C000) {
      if(sample->state == SAMPLE_STATE_OFF) break;
      if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
        irq_triggered_cycle = s;
      }
      decode_sample_block(ram, memmax, sample, 1);
      s += phase_inc * 28;
      ph -= 0x1C000;
    }
    s = n;
  } else {
    uint32 t = 0;
    for(s = 0; s < n; s++) {
      sint32 *source_signal;
      sint16 *mygauss;
      sint32 sum;
      if(ph >= 0x1C000) {
        if(sample->state == SAMPLE_STATE_OFF) break;
        if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
          irq_triggered_cycle = t;
        }
        decode_sample_block(ram, memmax, sample, 0);
        ph -= 0x1C000;
      }
      source_signal = sample->array + (ph >> 12);
      mygauss = (sint16*) (((uint8*)gauss_shuffled_reverse_table) + ((ph & 0xFF0) >> 1));

      { sum =
          (source_signal[0] * mygauss[0]) +
          (source_signal[1] * mygauss[1]) +
          (source_signal[2] * mygauss[2]) +
          (source_signal[3] * mygauss[3]);
      }
      sum >>= 15;

      *dest++ = sum;
      ph += phase_inc;
      t += phase_inc;
    }
  }

  if(irq_state && irq_triggered_cycle != 0xFFFFFFFF) {
    irq_triggered_cycle = (irq_triggered_cycle * 768) >> 12;
    if(irq_triggered_cycle < irq_state->triggered_cycle) irq_state->triggered_cycle = irq_triggered_cycle;
  }

  sample->phase = ph;

  return s;
}

////////////////////////////////////////////////////////////////////////////////

static uint32 EMU_CALL resampler_modulated(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_SAMPLE *sample,
  sint32 *dest,
  uint32 n,
  uint32 phase_inc,
  sint32 *fmbuf,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  uint32 irq_triggered_cycle = 0xFFFFFFFF;
  uint32 s, t = 0;
  uint32 ph; //, phl;
  uint32 pimod;
  ph = sample->phase;
  if(!dest) {
    for(s = 0; s < n; s++) {
      pimod = ((*fmbuf++ + 32768) * phase_inc) / 32768;
      if(pimod > 0x3FFF) pimod = 0x3FFF;
      else if(pimod < 1) pimod = 1;
      ph += pimod;
      while(ph >= 0x1C000) {
        if(sample->state == SAMPLE_STATE_OFF) break;
        if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
          irq_triggered_cycle = t;
        }
        decode_sample_block(ram, memmax, sample, 1);
        ph -= 0x1C000;
      }
      t += pimod;
    }
  } else {
    for(s = 0; s < n; s++) {
      sint32 *source_signal;
      sint16 *mygauss;
      sint32 sum;
      if(ph >= 0x1C000) {
        if(sample->state == SAMPLE_STATE_OFF) break;
        if(irq_state && irq_state->offset - sample->block_addr < 16 && irq_triggered_cycle == 0xFFFFFFFF) {
          irq_triggered_cycle = t;
        }
        decode_sample_block(ram, memmax, sample, 0);
        ph -= 0x1C000;
      }
      source_signal = sample->array + (ph >> 12);
      mygauss = (sint16*) (((uint8*)gauss_shuffled_reverse_table) + ((ph & 0xFF0) >> 1));

      { sum =
          (source_signal[0] * mygauss[0]) +
          (source_signal[1] * mygauss[1]) +
          (source_signal[2] * mygauss[2]) +
          (source_signal[3] * mygauss[3]);
      }
      sum >>= 15;

      *dest++ = sum;
      pimod = ((*fmbuf++ + 32768) * phase_inc) / 32768;
      if(pimod > 0x3FFF) pimod = 0x3FFF;
      else if(pimod < 1) pimod = 1;
      ph += pimod;
      t += pimod;
    }
  }

  if(irq_state && irq_triggered_cycle != 0xFFFFFFFF) {
    irq_triggered_cycle = (irq_triggered_cycle * 768) >> 12;
    if(irq_triggered_cycle < irq_state->triggered_cycle) irq_state->triggered_cycle = irq_triggered_cycle;
  }

  sample->phase = ph;

  return s;
}

////////////////////////////////////////////////////////////////////////////////

#define MY_AM (((env->reg_ad)>>15)&0x01)
#define MY_AR (((env->reg_ad)>> 8)&0x7F)
#define MY_DR (((env->reg_ad)>> 4)&0x0F)
#define MY_SL (((env->reg_ad)>> 0)&0x0F)
#define MY_SM (((env->reg_sr)>>15)&0x01)
#define MY_SD (((env->reg_sr)>>14)&0x01)
#define MY_SR (((env->reg_sr)>> 6)&0x7F)
#define MY_RM (((env->reg_sr)>> 5)&0x01)
#define MY_RR (((env->reg_sr)>> 0)&0x1F)

/*
** - Sets the current envelope slope
** - Returns the max number of samples that can be processed at the current
**   slope
*/
static EMU_INLINE sint32 EMU_CALL envelope_do(struct SPUCORE_ENVELOPE *env) {
  sint32 target = 0;
  /*
  ** Clip envelope value in case it wrapped around
  */
  switch(((env->level) >> 30) & 3) {
  case 2: env->level = 0x7FFFFFFF; break;
  case 3: env->level = 0x00000000; break;
  }
  switch(env->state) {
  case ENVELOPE_STATE_ATTACK : goto attack;
  case ENVELOPE_STATE_DECAY  : goto decay;
  case ENVELOPE_STATE_SUSTAIN: goto sustain;
  case ENVELOPE_STATE_RELEASE: goto release;
  default:
    env->state = ENVELOPE_STATE_OFF;
    env->level = 0;
    env->delta = 0;
    return 1;
  }
attack:
  if(env->level == 0x7FFFFFFF) {
    env->state = ENVELOPE_STATE_DECAY;
    goto decay;
  }
  /* log */
  if(MY_AM) {
    if(env->level < 0x60000000) {
      target = 0x60000000;
      env->delta = ratelogtable[32+(MY_AR^0x7F)-0x10];
    } else {
      target = 0x7FFFFFFF;
      env->delta = ratelogtable[32+(MY_AR^0x7F)-0x18];
    }
  /* linear */
  } else {
    target = 0x7FFFFFFF;
    env->delta = ratelogtable[32+(MY_AR^0x7F)-0x10];
  }
  goto domax;

decay:
  if((((env->level)>>27)&0xF) <= ((sint32)(MY_SL))) {
    env->state = ENVELOPE_STATE_SUSTAIN;
    goto sustain;
  }
  target = env->level & (~0x07FFFFFF);
  switch(((env->level)>>28)&0x7) {
  case 0: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+0];  break;
  case 1: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+4];  break;
  case 2: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+6];  break;
  case 3: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+8];  break;
  case 4: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+9];  break;
  case 5: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+10]; break;
  case 6: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+11]; break;
  case 7: env->delta = -ratelogtable[32+(4*(MY_DR^0x1F))-0x18+12]; break;
  }
  goto domax;

sustain:
  if(!MY_SD) {
    if(env->level == 0x7FFFFFFF) {
      env->delta = 0;
      return 0x7FFFFFFF;
    }
    /* log */
    if(MY_SM) {
      if(env->level < 0x60000000) {
        target = 0x60000000;
        env->delta = ratelogtable[32+(MY_SR^0x7F)-0x10];
      } else {
        target = 0x7FFFFFFF;
        env->delta = ratelogtable[32+(MY_SR^0x7F)-0x18];
      }
    /* linear */
    } else {
      target = 0x7FFFFFFF;
      env->delta = ratelogtable[32+(MY_SR^0x7F)-0x10];
    }
  } else {
    if(env->level == 0x00000000) {
      env->delta = 0;
      return 0x7FFFFFFF;
    }
    /* log */
    if(MY_SM) {
      target = ((env->level)&(~0x0FFFFFFF));
      switch(((env->level)>>28)&0x7) {
      case 0: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+0];  break;
      case 1: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+4];  break;
      case 2: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+6];  break;
      case 3: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+8];  break;
      case 4: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+9];  break;
      case 5: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+10]; break;
      case 6: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+11]; break;
      case 7: env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x1B+12]; break;
      }
    /* linear */
    } else {
      target = 0x00000000;
      env->delta = -ratelogtable[32+(MY_SR^0x7F)-0x0F];
    }
  }
  goto domax;

release:
  if(env->level == 0) {
    env->delta = 0;
    env->state = ENVELOPE_STATE_OFF;
    return 1;
  }
  /* log */
  if(MY_RM) {
    target = ((env->level) & (~0x0FFFFFFF));
    switch(((env->level)>>28)&0x7) {
    case 0: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+0];  break;
    case 1: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+4];  break;
    case 2: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+6];  break;
    case 3: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+8];  break;
    case 4: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+9];  break;
    case 5: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+10]; break;
    case 6: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+11]; break;
    case 7: env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x18+12]; break;
    }
  /* linear */
  } else {
    target = 0;
    env->delta = -ratelogtable[32+(4*(MY_RR^0x1F))-0x0C];
  }
  goto domax;

domax:
  { sint32 max;
    if(env->delta) {
      max = (target - (env->level)) / (env->delta);
    } else {
      max = 0x7FFFFFFF;
    }
    max--;
    if(max < 1) max = 1;
    return max;
  }
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL envelope_prime(struct SPUCORE_ENVELOPE *env) {
//  if(env->state != ENVELOPE_STATE_OFF) {
//if(env->state!=ENVELOPE_STATE_RELEASE){
//    OutputDebugString("envelope re-prime\n");
//}
//  }

//EMUTRACE2("[eprime %04X %04X]",env->reg_ad_x,env->reg_sr_x);

  env->level = 1;
  env->state = ENVELOPE_STATE_ATTACK;
  env->delta = 1;

  env->cachemax = 0;
}

static void EMU_CALL envelope_release(struct SPUCORE_ENVELOPE *env) {
  if(env->state != ENVELOPE_STATE_OFF) {
    env->state = ENVELOPE_STATE_RELEASE;
  }
  env->cachemax = 0;
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL sample_prime(struct SPUCORE_SAMPLE *sample) {
  sample->state = SAMPLE_STATE_ON;
  memset(sample->array, 0, sizeof(sample->array));
  sample->array_cleared = 1;
  sample->phase = 0x1C000; // ensure it grabs the first block right away
//EMUTRACE2("[sprime %08X %08X]", sample->start_block_addr, sample->start_loop_block_addr);

  sample->block_addr = sample->start_block_addr;
  //sample->loop_block_addr = sample->start_loop_block_addr;
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL voice_on(struct SPUCORE_CHAN *c) {

  // FOR DEBUGGING PURPOSES ONLY.
//  if(c->sample.state == SAMPLE_STATE_ON)return;
//  if(c->env.state != ENVELOPE_STATE_OFF)return;
  /*
  ** Defer if already on
  */
  if(c->env.state != ENVELOPE_STATE_OFF) {
    //EMUTRACE0("alreadyon:");
    if(!(c->samples_until_pending_keyon)) {
      //EMUTRACE0("defer");
      c->samples_until_pending_keyon = KEYON_DEFER_SAMPLES;
    } else {
      //EMUTRACE0("was already deferred");
    }
  } else {
//    EMUTRACE0("prime");
    sample_prime(&(c->sample));
    envelope_prime(&(c->env));
  }
//  EMUTRACE0("\n");
}

static void EMU_CALL voice_off(struct SPUCORE_CHAN *c) {
  //EMUTRACE0("release");
  envelope_release(&(c->env));
  //EMUTRACE0("\n");
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL voices_on(struct SPUCORE_STATE *state, uint32 bits) {
  int a;
  for(a = 0; a < 24; a++) {
    if(bits & 1) {

//      EMUTRACE1("voiceon  %2d:",a);

//EMUTRACE2("[vol %08X %08X]",state->chan[a].vol[0].level,state->chan[a].vol[1].level);
//EMUTRACE2("[mvol %08X %08X]",state->mvol[0].level,state->mvol[1].level);
////EMUTRACE2("[avol %08X %08X]",state->avol[0],state->avol[1]);
//EMUTRACE2("[evol %08X %08X]",state->evol[0],state->evol[1]);

//    sint32 extvol_l = ((sint16)(state->avol[0]));
//    sint32 extvol_r = ((sint16)(state->avol[1]));

      voice_on(state->chan + a);
    }
    bits >>= 1;
  }
}

static void EMU_CALL voices_off(struct SPUCORE_STATE *state, uint32 bits) {
  int a;
  for(a = 0; a < 24; a++) {
    if(bits & 1) {
      //EMUTRACE1("voiceoff %2d:",a);
      voice_off(state->chan + a);
    }
    bits >>= 1;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** - Scales samples in a buffer using an envelope
** - Returns the actual number of samples modified
**
** All UNMODIFIED samples must then be discarded
** (they're supposed to be zero)
*/
static int EMU_CALL enveloper(
  struct SPUCORE_ENVELOPE *env,
  sint32 *buf,
  int samples
) {
  int i = 0;
  while(i < samples) {
    sint32 e, d, max;
    if(env->state == ENVELOPE_STATE_OFF) break;
    max = env->cachemax;
    if(!max) {
      max = envelope_do(env);
      env->cachemax = max;
    }
    e = env->level;
    d = env->delta;
    if(max < 1) max = 1;
    if(max > (samples - i)) { max = samples - i; }
    env->cachemax -= max;
    if(!buf) {
      i += max;
      e += max * d;
    } else {
      while(max--) {
        sint32 b = buf[i];
        b *= (e >> 16);
        b >>= 15;
        buf[i] = b;
        e += d;
        i++;
      }
    }
    env->level = e;
    env->delta = d;
  }
  return i;
}

////////////////////////////////////////////////////////////////////////////////
//
// - Calls the resampler and enveloper
// - Returns the number of samples actually generated
//   (can be 0 if the channel is silent)
//
// If buf is null, it means "fast forward" - don't render anything
//
//
// output of render_channel_raw() is guaranteed clipped as long as
// resampler() and enveloper() are also
//
static int EMU_CALL render_channel_raw(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_CHAN *c,
  sint32 *buf,
  sint32 *fmbuf,
  sint32 *nbuf,
  int samples,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  int r = samples;
  /* If the envelope is dead, don't bother anyway */
  if((c->env.state) == ENVELOPE_STATE_OFF) return 0;
  /* Do resampling */
  if (fmbuf) r = resampler_modulated(ram, memmax, &(c->sample), buf, r, c->voice_pitch, fmbuf, irq_state);
  else r = resampler(ram, memmax, &(c->sample), buf, r, c->voice_pitch, irq_state);
  if(nbuf) {
    r = samples;
    if(buf) memcpy(buf, nbuf, 4 * r);
  }
  /* Do enveloping */
  r = enveloper(&(c->env), buf, r);
  /* If we were cut short by _either_, then the envelope state must be set
  ** to OFF */
  if(r < samples) c->env.state = ENVELOPE_STATE_OFF;
  return r;
}

//
// This is the new render_channel which processes deferred key-ons
//
//
// output of render_channel_mono() is guaranteed clipped as long as
// render_channel_raw() is also
//
static int EMU_CALL render_channel_mono(
  uint16 *ram,
  uint32 memmax,
  struct SPUCORE_CHAN *c,
  sint32 *buf,
  sint32 *fmbuf,
  sint32 *nbuf,
  sint32 samples,
  struct SPUCORE_IRQ_STATE *irq_state
) {
  sint32 n;
  sint32 r, r2;
  sint32 defer_remaining;
  struct SPUCORE_IRQ_STATE spare_state;

//top:
  n = c->samples_until_pending_keyon;

  if(!n) {
    return render_channel_raw(ram, memmax, c, buf, fmbuf, nbuf, samples, irq_state);
  }

  //
  // Don't defer key-on if fast-forwarding
  // this caused a STUCK-NOTE BUG. should not use.
  //
//  if(!buf) {
//    c->samples_until_pending_keyon = 0;
//    goto top;
//  }

  if(n > samples) { n = samples; }

  /*
  ** r = how many samples we actually will process
  */
  r = render_channel_raw(ram, memmax, c, buf, fmbuf, nbuf, n, irq_state);

  defer_remaining = c->samples_until_pending_keyon;
  if(buf) {
    sint32 i;
    for(i = 0; i < r; i++) {
      (*buf) = ((*buf)*defer_remaining) / KEYON_DEFER_SAMPLES;
      buf++;
      defer_remaining--;
    }
  } else {
    defer_remaining -= r;
  }
  if(fmbuf) fmbuf += r;
  if(nbuf) nbuf += r;
  /*
  ** if render_channel_raw got cut short, then we're done anyway.
  */
  if(r < n) defer_remaining = 0;

  c->samples_until_pending_keyon = defer_remaining;
  /*
  ** Process the key-on if necessary
  */
  if(!defer_remaining) {
    sample_prime(&(c->sample));
    envelope_prime(&(c->env));
  }

  /*
  ** we already handled r samples
  */
  samples -= r;

  /*
  ** if there are any remaining samples to render, do them here
  */
  r2 = 0;
  if(samples) {
    struct SPUCORE_IRQ_STATE *s = NULL;
    if(irq_state) {
      s = &spare_state;
      spare_state.offset = irq_state->offset;
    }
    r2 = render_channel_raw(ram, memmax, c, buf, fmbuf, nbuf, samples, s);
	if(irq_state && irq_state->triggered_cycle == 0xFFFFFFFF && spare_state.triggered_cycle != 0xFFFFFFFF) irq_state->triggered_cycle = spare_state.triggered_cycle + r * 768;
  }

  return r + r2;

}

////////////////////////////////////////////////////////////////////////////////
//
// Generates a buffer worth of noise data, ganked from Eternal SPU
//

static void EMU_CALL render_noise(
  struct SPUCORE_STATE *state,
  sint32 *buf,
  sint32 samples
) {
  int n;
  uint32 noisecounter = state->noisecounter;
  sint32 noiseval = state->noiseval;
  uint32 noiseinc = (uint16)(0x8000 >> (state->noiseclock << 6));
  for(n = 0; n < samples; n++) {
    noisecounter += noiseinc;
    noiseval += noisecounter + noisecounter + noisetable[(noisecounter >> 10) & 63];
	if (noiseval < -32767) noiseval = -32767;
	else if (noiseval > 32767) noiseval = 32767;
    if (buf) *buf++ = noiseval;
  }
  state->noisecounter = noisecounter;
  state->noiseval = noiseval;
}

////////////////////////////////////////////////////////////////////////////////

#define MAKE_SINT32_COEF(x)   ((sint32)((sint16)(state->reverb.x)))
#define MAKE_REVERB_OFFSET(x) (state->reverb.x)
#define NORMALIZE_REVERB_OFFSET(x) {while(x>=state->reverb.safe_end_address){x-=state->reverb.safe_size;}while(x<state->reverb.safe_start_address){x+=state->reverb.safe_size;}}

#define RAM_PCM_SAMPLE(x) (*((sint16*)(((uint8*)ram) + (x))))
#define RAM_SINT32_SAMPLE(x) ((sint32)(RAM_PCM_SAMPLE(x)))
/*
** Multiplying -32768 by -32768 and scaling by 15 bits is not safe
** (the sign would get flipped)
** so we should clip to -32767 instead
*/
#define CLIP_PCM_1(x) {if(x>32767){x=32767;}else if(x<(-32767)){x=(-32767);}}
#define CLIP_PCM_2(a,b) {CLIP_PCM_1(a);CLIP_PCM_1(b);}
#define CLIP_PCM_4(a,b,c,d) {CLIP_PCM_1(a);CLIP_PCM_1(b);CLIP_PCM_1(c);CLIP_PCM_1(d);}

//#define CLIP_PCMDBL_1(x) {if(x>(32767*2)){x=(32767*2);}else if(x<(-(32767*2))){x=(-(32767*2));}}
//#define CLIP_PCMDBL_2(a,b) {CLIP_PCMDBL_1(a);CLIP_PCMDBL_1(b);}
//#define CLIP_PCMDBL_4(a,b,c,d) {CLIP_PCMDBL_1(a);CLIP_PCMDBL_1(b);CLIP_PCMDBL_1(c);CLIP_PCMDBL_1(d);}

////////////////////////////////////////////////////////////////////////////////
/*
** 22KHz reverb steady state step
*/
static void EMU_CALL reverb_steadystate22(struct SPUCORE_STATE *state, uint16 *ram, sint32 input_l, sint32 input_r) {
  /*
  ** Current reverb offset
  */
  sint32 current     = state->reverb.current_address;
  /*
  ** Reverb registers
  */
  sint32 fb_src_a    = MAKE_REVERB_OFFSET(FB_SRC_A);
  sint32 fb_src_b    = MAKE_REVERB_OFFSET(FB_SRC_B);
  sint32 iir_alpha   = MAKE_SINT32_COEF(IIR_ALPHA);
  sint32 acc_coef_a  = MAKE_SINT32_COEF(ACC_COEF_A);
  sint32 acc_coef_b  = MAKE_SINT32_COEF(ACC_COEF_B);
  sint32 acc_coef_c  = MAKE_SINT32_COEF(ACC_COEF_C);
  sint32 acc_coef_d  = MAKE_SINT32_COEF(ACC_COEF_D);
  sint32 iir_coef    = MAKE_SINT32_COEF(IIR_COEF);
  sint32 fb_alpha    = MAKE_SINT32_COEF(FB_ALPHA);
  sint32 fb_x        = MAKE_SINT32_COEF(FB_X);
  sint32 iir_dest_a0 = MAKE_REVERB_OFFSET(IIR_DEST_A0);
  sint32 iir_dest_a1 = MAKE_REVERB_OFFSET(IIR_DEST_A1);
  sint32 acc_src_a0  = MAKE_REVERB_OFFSET(ACC_SRC_A0);
  sint32 acc_src_a1  = MAKE_REVERB_OFFSET(ACC_SRC_A1);
  sint32 acc_src_b0  = MAKE_REVERB_OFFSET(ACC_SRC_B0);
  sint32 acc_src_b1  = MAKE_REVERB_OFFSET(ACC_SRC_B1);
  sint32 iir_src_a0  = MAKE_REVERB_OFFSET(IIR_SRC_A0);
  sint32 iir_src_a1  = MAKE_REVERB_OFFSET(IIR_SRC_A1);
  sint32 iir_dest_b0 = MAKE_REVERB_OFFSET(IIR_DEST_B0);
  sint32 iir_dest_b1 = MAKE_REVERB_OFFSET(IIR_DEST_B1);
  sint32 acc_src_c0  = MAKE_REVERB_OFFSET(ACC_SRC_C0);
  sint32 acc_src_c1  = MAKE_REVERB_OFFSET(ACC_SRC_C1);
  sint32 acc_src_d0  = MAKE_REVERB_OFFSET(ACC_SRC_D0);
  sint32 acc_src_d1  = MAKE_REVERB_OFFSET(ACC_SRC_D1);
  sint32 iir_src_b1  = MAKE_REVERB_OFFSET(IIR_SRC_B1);
  sint32 iir_src_b0  = MAKE_REVERB_OFFSET(IIR_SRC_B0);
  sint32 mix_dest_a0 = MAKE_REVERB_OFFSET(MIX_DEST_A0);
  sint32 mix_dest_a1 = MAKE_REVERB_OFFSET(MIX_DEST_A1);
  sint32 mix_dest_b0 = MAKE_REVERB_OFFSET(MIX_DEST_B0);
  sint32 mix_dest_b1 = MAKE_REVERB_OFFSET(MIX_DEST_B1);
  sint32 in_coef_l   = MAKE_SINT32_COEF(IN_COEF_L);
  sint32 in_coef_r   = MAKE_SINT32_COEF(IN_COEF_R);
  /*
  ** Alternate buffer positions
  */
  sint32 fb_src_a0;
  sint32 fb_src_a1;
  sint32 fb_src_b0;
  sint32 fb_src_b1;
  sint32 iir_dest_a0_plus;
  sint32 iir_dest_a1_plus;
  sint32 iir_dest_b0_plus;
  sint32 iir_dest_b1_plus;
  /*
  ** Intermediate results
  */
  sint32 acc0;
  sint32 acc1;
  sint32 iir_input_a0;
  sint32 iir_input_a1;
  sint32 iir_input_b0;
  sint32 iir_input_b1;
  sint32 iir_a0;
  sint32 iir_a1;
  sint32 iir_b0;
  sint32 iir_b1;
  sint32 fb_a0;
  sint32 fb_a1;
  sint32 fb_b0;
  sint32 fb_b1;
  sint32 mix_a0;
  sint32 mix_a1;
  sint32 mix_b0;
  sint32 mix_b1;

  /*
  ** Offsets
  */
  mix_dest_a0 += current; NORMALIZE_REVERB_OFFSET(mix_dest_a0);
  mix_dest_a1 += current; NORMALIZE_REVERB_OFFSET(mix_dest_a1);
  mix_dest_b0 += current; NORMALIZE_REVERB_OFFSET(mix_dest_b0);
  mix_dest_b1 += current; NORMALIZE_REVERB_OFFSET(mix_dest_b1);
  fb_src_a0 = mix_dest_a0 - fb_src_a; NORMALIZE_REVERB_OFFSET(fb_src_a0);
  fb_src_a1 = mix_dest_a1 - fb_src_a; NORMALIZE_REVERB_OFFSET(fb_src_a1);
  fb_src_b0 = mix_dest_b0 - fb_src_b; NORMALIZE_REVERB_OFFSET(fb_src_b0);
  fb_src_b1 = mix_dest_b1 - fb_src_b; NORMALIZE_REVERB_OFFSET(fb_src_b1);
  acc_src_a0 += current; NORMALIZE_REVERB_OFFSET(acc_src_a0);
  acc_src_a1 += current; NORMALIZE_REVERB_OFFSET(acc_src_a1);
  acc_src_b0 += current; NORMALIZE_REVERB_OFFSET(acc_src_b0);
  acc_src_b1 += current; NORMALIZE_REVERB_OFFSET(acc_src_b1);
  acc_src_c0 += current; NORMALIZE_REVERB_OFFSET(acc_src_c0);
  acc_src_c1 += current; NORMALIZE_REVERB_OFFSET(acc_src_c1);
  acc_src_d0 += current; NORMALIZE_REVERB_OFFSET(acc_src_d0);
  acc_src_d1 += current; NORMALIZE_REVERB_OFFSET(acc_src_d1);
  iir_src_a0 += current; NORMALIZE_REVERB_OFFSET(iir_src_a0);
  iir_src_a1 += current; NORMALIZE_REVERB_OFFSET(iir_src_a1);
  iir_src_b0 += current; NORMALIZE_REVERB_OFFSET(iir_src_b0);
  iir_src_b1 += current; NORMALIZE_REVERB_OFFSET(iir_src_b1);
  iir_dest_a0 += current; NORMALIZE_REVERB_OFFSET(iir_dest_a0);
  iir_dest_a1 += current; NORMALIZE_REVERB_OFFSET(iir_dest_a1);
  iir_dest_b0 += current; NORMALIZE_REVERB_OFFSET(iir_dest_b0);
  iir_dest_b1 += current; NORMALIZE_REVERB_OFFSET(iir_dest_b1);
  iir_dest_a0_plus = iir_dest_a0 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_a0_plus);
  iir_dest_a1_plus = iir_dest_a1 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_a1_plus);
  iir_dest_b0_plus = iir_dest_b0 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_b0_plus);
  iir_dest_b1_plus = iir_dest_b1 + 2; NORMALIZE_REVERB_OFFSET(iir_dest_b1_plus);

  /*
  ** IIR
  */
  CLIP_PCM_2(input_l,input_r);
  input_l *= in_coef_l;
  input_r *= in_coef_r;
#define OPPOSITE_IIR_ALPHA (32768-iir_alpha)
  iir_input_a0 = ((RAM_SINT32_SAMPLE(iir_src_a0) * iir_coef) + input_l) >> 15;
  iir_input_a1 = ((RAM_SINT32_SAMPLE(iir_src_a1) * iir_coef) + input_r) >> 15;
  iir_input_b0 = ((RAM_SINT32_SAMPLE(iir_src_b0) * iir_coef) + input_l) >> 15;
  iir_input_b1 = ((RAM_SINT32_SAMPLE(iir_src_b1) * iir_coef) + input_r) >> 15;
  CLIP_PCM_4(iir_input_a0,iir_input_a1,iir_input_b0,iir_input_b1);
  iir_a0 = ((iir_input_a0 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_a0) * (OPPOSITE_IIR_ALPHA))) >> 15;
  iir_a1 = ((iir_input_a1 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_a1) * (OPPOSITE_IIR_ALPHA))) >> 15;
  iir_b0 = ((iir_input_b0 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_b0) * (OPPOSITE_IIR_ALPHA))) >> 15;
  iir_b1 = ((iir_input_b1 * iir_alpha) + (RAM_SINT32_SAMPLE(iir_dest_b1) * (OPPOSITE_IIR_ALPHA))) >> 15;
  CLIP_PCM_4(iir_a0,iir_a1,iir_b0,iir_b1);

  RAM_PCM_SAMPLE(iir_dest_a0_plus) = iir_a0;
  RAM_PCM_SAMPLE(iir_dest_a1_plus) = iir_a1;
  RAM_PCM_SAMPLE(iir_dest_b0_plus) = iir_b0;
  RAM_PCM_SAMPLE(iir_dest_b1_plus) = iir_b1;

  /*
  ** Accumulators
  */
  acc0 =
    ((RAM_SINT32_SAMPLE(acc_src_a0) * acc_coef_a) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_b0) * acc_coef_b) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_c0) * acc_coef_c) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_d0) * acc_coef_d) >> 15);
  acc1 =
    ((RAM_SINT32_SAMPLE(acc_src_a1) * acc_coef_a) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_b1) * acc_coef_b) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_c1) * acc_coef_c) >> 15) +
    ((RAM_SINT32_SAMPLE(acc_src_d1) * acc_coef_d) >> 15);
  CLIP_PCM_2(acc0,acc1);

  /*
  ** Feedback
  */
  fb_a0 = RAM_SINT32_SAMPLE(fb_src_a0);
  fb_a1 = RAM_SINT32_SAMPLE(fb_src_a1);
  fb_b0 = RAM_SINT32_SAMPLE(fb_src_b0);
  fb_b1 = RAM_SINT32_SAMPLE(fb_src_b1);

  mix_a0 = acc0 - ((fb_a0*fb_alpha)>>15);
  mix_a1 = acc1 - ((fb_a1*fb_alpha)>>15);
  mix_b0 = fb_alpha*acc0;
  mix_b1 = fb_alpha*acc1;
  fb_alpha = ((sint32)((sint16)(((sint16)fb_alpha)^0x8000)));
  mix_b0 -= fb_a0*fb_alpha;
  mix_b1 -= fb_a1*fb_alpha;
  mix_b0 -= fb_b0*fb_x;
  mix_b1 -= fb_b1*fb_x;
  mix_b0>>=15;
  mix_b1>>=15;

  CLIP_PCM_4(mix_a0,mix_a1,mix_b0,mix_b1);
  RAM_PCM_SAMPLE(mix_dest_a0) = mix_a0;
  RAM_PCM_SAMPLE(mix_dest_a1) = mix_a1;
  RAM_PCM_SAMPLE(mix_dest_b0) = mix_b0;
  RAM_PCM_SAMPLE(mix_dest_b1) = mix_b1;

}

////////////////////////////////////////////////////////////////////////////////
/*
** 22KHz reverb engine
*/
static void EMU_CALL reverb_engine22(struct SPUCORE_STATE *state, uint16 *ram, sint32 *l, sint32 *r) {
  sint32 input_l = *l;
  sint32 input_r = *r;
  sint32 output_l;
  sint32 output_r;
  sint32 mix_dest_a0 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_A0);
  sint32 mix_dest_a1 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_A1);
  sint32 mix_dest_b0 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_B0);
  sint32 mix_dest_b1 = state->reverb.current_address + MAKE_REVERB_OFFSET(MIX_DEST_B1);
  NORMALIZE_REVERB_OFFSET(mix_dest_a0);
  NORMALIZE_REVERB_OFFSET(mix_dest_a1);
  NORMALIZE_REVERB_OFFSET(mix_dest_b0);
  NORMALIZE_REVERB_OFFSET(mix_dest_b1);

  /*
  ** (Scale these down for now - avoids some clipping)
  */
  input_l *= 2;
  input_r *= 2;
  input_l /= 3;
  input_r /= 3;

  /*
  ** Execute steady state step if necessary
  */
  if(state->flags & SPUREG_FLAG_REVERB_ENABLE) {
    reverb_steadystate22(state, ram, input_l, input_r);
  }

  /*
  ** Retrieve wet out L/R
  ** (pretty certain this is done AFTER the steady state step)
  */
  {
    int al = RAM_SINT32_SAMPLE(mix_dest_a0);
    int ar = RAM_SINT32_SAMPLE(mix_dest_a1);
    int bl = RAM_SINT32_SAMPLE(mix_dest_b0);
    int br = RAM_SINT32_SAMPLE(mix_dest_b1);

    output_l = al + bl;
    output_r = ar + br;
  }

  *l = output_l;
  *r = output_r;

  /*
  ** Advance reverb buffer position
  */
  state->reverb.current_address += 2;
  if(state->reverb.current_address >= state->reverb.safe_end_address) {
    state->reverb.current_address = state->reverb.safe_start_address;
  }
}

////////////////////////////////////////////////////////////////////////////////

static void EMU_CALL reverb_process(struct SPUCORE_STATE *state, uint16 *ram, sint32 *buf, int samples) {
  int q = state->reverb.resampler.queue_index;
  /*
  ** Sample loop
  */
  while(samples--) {
    /*
    ** Get an input sample
    */
    sint32 l = buf[0];
    sint32 r = buf[1];
    /*
    ** Put it in the input queue
    */
    state->reverb.resampler.in_queue_l[q & 63] = l;
    state->reverb.resampler.in_queue_r[q & 63] = r;
    /*
    ** If we're ready to create another output sample...
    */
    if(q & 1) {
      /*
      ** Lowpass/downsample
      */
#if 1
      l =
        (state->reverb.resampler.in_queue_l[(q - 38) & 63]) * reverb_psx_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_l[(q - 36) & 63]) * reverb_psx_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 34) & 63]) * reverb_psx_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_l[(q - 32) & 63]) * reverb_psx_lowpass_coefs[3] +
        (state->reverb.resampler.in_queue_l[(q - 30) & 63]) * reverb_psx_lowpass_coefs[4] +
        (state->reverb.resampler.in_queue_l[(q - 28) & 63]) * reverb_psx_lowpass_coefs[5] +
        (state->reverb.resampler.in_queue_l[(q - 26) & 63]) * reverb_psx_lowpass_coefs[6] +
        (state->reverb.resampler.in_queue_l[(q - 24) & 63]) * reverb_psx_lowpass_coefs[7] +
        (state->reverb.resampler.in_queue_l[(q - 22) & 63]) * reverb_psx_lowpass_coefs[8] +
        (state->reverb.resampler.in_queue_l[(q - 20) & 63]) * reverb_psx_lowpass_coefs[9] +
        (state->reverb.resampler.in_queue_l[(q - 19) & 63]) * reverb_psx_lowpass_coefs[10] +
        (state->reverb.resampler.in_queue_l[(q - 18) & 63]) * reverb_psx_lowpass_coefs[9] +
        (state->reverb.resampler.in_queue_l[(q - 16) & 63]) * reverb_psx_lowpass_coefs[8] +
        (state->reverb.resampler.in_queue_l[(q - 14) & 63]) * reverb_psx_lowpass_coefs[7] +
        (state->reverb.resampler.in_queue_l[(q - 12) & 63]) * reverb_psx_lowpass_coefs[6] +
        (state->reverb.resampler.in_queue_l[(q - 10) & 63]) * reverb_psx_lowpass_coefs[5] +
        (state->reverb.resampler.in_queue_l[(q - 8) & 63]) * reverb_psx_lowpass_coefs[4] +
        (state->reverb.resampler.in_queue_l[(q - 6) & 63]) * reverb_psx_lowpass_coefs[3] +
        (state->reverb.resampler.in_queue_l[(q - 4) & 63]) * reverb_psx_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_l[(q - 2) & 63]) * reverb_psx_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 0) & 63]) * reverb_psx_lowpass_coefs[0];
      r =
        (state->reverb.resampler.in_queue_r[(q - 38) & 63]) * reverb_psx_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_r[(q - 36) & 63]) * reverb_psx_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 34) & 63]) * reverb_psx_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_r[(q - 32) & 63]) * reverb_psx_lowpass_coefs[3] +
        (state->reverb.resampler.in_queue_r[(q - 30) & 63]) * reverb_psx_lowpass_coefs[4] +
        (state->reverb.resampler.in_queue_r[(q - 28) & 63]) * reverb_psx_lowpass_coefs[5] +
        (state->reverb.resampler.in_queue_r[(q - 26) & 63]) * reverb_psx_lowpass_coefs[6] +
        (state->reverb.resampler.in_queue_r[(q - 24) & 63]) * reverb_psx_lowpass_coefs[7] +
        (state->reverb.resampler.in_queue_r[(q - 22) & 63]) * reverb_psx_lowpass_coefs[8] +
        (state->reverb.resampler.in_queue_r[(q - 20) & 63]) * reverb_psx_lowpass_coefs[9] +
        (state->reverb.resampler.in_queue_r[(q - 19) & 63]) * reverb_psx_lowpass_coefs[10] +
        (state->reverb.resampler.in_queue_r[(q - 18) & 63]) * reverb_psx_lowpass_coefs[9] +
        (state->reverb.resampler.in_queue_r[(q - 16) & 63]) * reverb_psx_lowpass_coefs[8] +
        (state->reverb.resampler.in_queue_r[(q - 14) & 63]) * reverb_psx_lowpass_coefs[7] +
        (state->reverb.resampler.in_queue_r[(q - 12) & 63]) * reverb_psx_lowpass_coefs[6] +
        (state->reverb.resampler.in_queue_r[(q - 10) & 63]) * reverb_psx_lowpass_coefs[5] +
        (state->reverb.resampler.in_queue_r[(q - 8) & 63]) * reverb_psx_lowpass_coefs[4] +
        (state->reverb.resampler.in_queue_r[(q - 6) & 63]) * reverb_psx_lowpass_coefs[3] +
        (state->reverb.resampler.in_queue_r[(q - 4) & 63]) * reverb_psx_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_r[(q - 2) & 63]) * reverb_psx_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 0) & 63]) * reverb_psx_lowpass_coefs[0];
#else
      l = 0;
      r = 0;

      for (n = 47; n >= 0; n -= 8) {
        l +=
          (state->reverb.resampler.in_queue_l[(q - n + 0) & 63]) * reverb_psx_lowpass_coefs[n - 0] +
          (state->reverb.resampler.in_queue_l[(q - n + 1) & 63]) * reverb_psx_lowpass_coefs[n - 1] +
          (state->reverb.resampler.in_queue_l[(q - n + 2) & 63]) * reverb_psx_lowpass_coefs[n - 2] +
          (state->reverb.resampler.in_queue_l[(q - n + 3) & 63]) * reverb_psx_lowpass_coefs[n - 3] +
          (state->reverb.resampler.in_queue_l[(q - n + 4) & 63]) * reverb_psx_lowpass_coefs[n - 4] +
          (state->reverb.resampler.in_queue_l[(q - n + 5) & 63]) * reverb_psx_lowpass_coefs[n - 5] +
          (state->reverb.resampler.in_queue_l[(q - n + 6) & 63]) * reverb_psx_lowpass_coefs[n - 6] +
          (state->reverb.resampler.in_queue_l[(q - n + 7) & 63]) * reverb_psx_lowpass_coefs[n - 7];
        r +=
          (state->reverb.resampler.in_queue_r[(q - n + 0) & 63]) * reverb_psx_lowpass_coefs[n - 0] +
          (state->reverb.resampler.in_queue_r[(q - n + 1) & 63]) * reverb_psx_lowpass_coefs[n - 1] +
          (state->reverb.resampler.in_queue_r[(q - n + 2) & 63]) * reverb_psx_lowpass_coefs[n - 2] +
          (state->reverb.resampler.in_queue_r[(q - n + 3) & 63]) * reverb_psx_lowpass_coefs[n - 3] +
          (state->reverb.resampler.in_queue_r[(q - n + 4) & 63]) * reverb_psx_lowpass_coefs[n - 4] +
          (state->reverb.resampler.in_queue_r[(q - n + 5) & 63]) * reverb_psx_lowpass_coefs[n - 5] +
          (state->reverb.resampler.in_queue_r[(q - n + 6) & 63]) * reverb_psx_lowpass_coefs[n - 6] +
          (state->reverb.resampler.in_queue_r[(q - n + 7) & 63]) * reverb_psx_lowpass_coefs[n - 7];
      }
#endif

      l >>= 15;
      r >>= 15;

/*
      l =
        (state->reverb.resampler.in_queue_l[(q - 6) & 7]) * reverb_new_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_l[(q - 4) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 3) & 7]) * reverb_new_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_l[(q - 2) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_l[(q - 0) & 7]) * reverb_new_lowpass_coefs[0];
      l >>= 11;
      r =
        (state->reverb.resampler.in_queue_r[(q - 6) & 7]) * reverb_new_lowpass_coefs[0] +
        (state->reverb.resampler.in_queue_r[(q - 4) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 3) & 7]) * reverb_new_lowpass_coefs[2] +
        (state->reverb.resampler.in_queue_r[(q - 2) & 7]) * reverb_new_lowpass_coefs[1] +
        (state->reverb.resampler.in_queue_r[(q - 0) & 7]) * reverb_new_lowpass_coefs[0];
      r >>= 11;
*/
//l = state->reverb.resampler.in_queue_l[q & 7];
//r = state->reverb.resampler.in_queue_r[q & 7];

      /*
      ** Run the reverb engine
      */
      reverb_engine22(state, ram, &l, &r);
      /*
      ** Put the new stuff into the output queue
      */
      state->reverb.resampler.out_queue_l[q & 15] = l;
      state->reverb.resampler.out_queue_r[q & 15] = r;
    }
    /*
    ** Upsample
    */
    /*
    ** Upsample using the same technique as for ADPCM samples
    ** (may or may not be right, sounds okay though)
    */
#define gauss_table_0x000 gauss_shuffled_reverse_table[0x200]
#define gauss_table_0x100 gauss_shuffled_reverse_table[0x201]
#define gauss_table_0x200 gauss_shuffled_reverse_table[0x202]
#define gauss_table_0x300 gauss_shuffled_reverse_table[0x203]
#define gauss_table_0x080 gauss_shuffled_reverse_table[0x000]
#define gauss_table_0x180 gauss_shuffled_reverse_table[0x001]
#define gauss_table_0x280 gauss_shuffled_reverse_table[0x002]
#define gauss_table_0x380 gauss_shuffled_reverse_table[0x003]
    if(q & 1) {
      l =
        (state->reverb.resampler.out_queue_l[(q - 6) & 15]) * gauss_table_0x080 +
        (state->reverb.resampler.out_queue_l[(q - 4) & 15]) * gauss_table_0x180 +
        (state->reverb.resampler.out_queue_l[(q - 2) & 15]) * gauss_table_0x280 +
        (state->reverb.resampler.out_queue_l[(q - 0) & 15]) * gauss_table_0x380;
      l >>= 15;
      r =
        (state->reverb.resampler.out_queue_r[(q - 6) & 15]) * gauss_table_0x080 +
        (state->reverb.resampler.out_queue_r[(q - 4) & 15]) * gauss_table_0x180 +
        (state->reverb.resampler.out_queue_r[(q - 2) & 15]) * gauss_table_0x280 +
        (state->reverb.resampler.out_queue_r[(q - 0) & 15]) * gauss_table_0x380;
      r >>= 15;
    } else {
      l =
        (state->reverb.resampler.out_queue_l[(q - 7) & 15]) * gauss_table_0x000 +
        (state->reverb.resampler.out_queue_l[(q - 5) & 15]) * gauss_table_0x100 +
        (state->reverb.resampler.out_queue_l[(q - 3) & 15]) * gauss_table_0x200 +
        (state->reverb.resampler.out_queue_l[(q - 1) & 15]) * gauss_table_0x300;
      l >>= 15;
      r =
        (state->reverb.resampler.out_queue_r[(q - 7) & 15]) * gauss_table_0x000 +
        (state->reverb.resampler.out_queue_r[(q - 5) & 15]) * gauss_table_0x100 +
        (state->reverb.resampler.out_queue_r[(q - 3) & 15]) * gauss_table_0x200 +
        (state->reverb.resampler.out_queue_r[(q - 1) & 15]) * gauss_table_0x300;
      r >>= 15;
    }
    /*
    ** Advance queue position, write output, all that good stuff
    */
    q++;
    buf[0] = l;
    buf[1] = r;
    buf += 2;
  }
  state->reverb.resampler.queue_index = q;
}

////////////////////////////////////////////////////////////////////////////////

//int spucore_frq[RENDERMAX];

/*
** Renderer
*/
static void EMU_CALL render(struct SPUCORE_STATE *state, uint16 *ram, sint16 *buf, sint16 *extinput, sint32 samples, uint8 mainout, uint8 effectout) {
  uint32 chanbit;
  uint32 maskmain_l;
  uint32 maskmain_r;
  uint32 maskverb_l;
  uint32 maskverb_r;
  uint32 masknoise;
  uint32 maskfm;
  sint32 ibuf   [  RENDERMAX];
  sint32 ibufmix[2*RENDERMAX];
  sint32 ibufrvb[2*RENDERMAX];
  sint32 ibufn  [  RENDERMAX];
  sint32 ibuffm [  RENDERMAX];
  int ch, i;
  sint32 m_v_l;
  sint32 m_v_r;
  sint32 r_v_l;
  sint32 r_v_r;
  struct SPUCORE_IRQ_STATE irq_state;
  struct SPUCORE_IRQ_STATE *irq_state_ptr;

  //spucore_frq[samples]++;

  irq_state_ptr = NULL;
  irq_state.triggered_cycle = 0xFFFFFFFF;
  if (state->flags & SPUREG_FLAG_IRQ_ENABLE) {
    if (state->memsize == 0x80000 && state->irq_address < 0x1000) {
      uint32 irq_address_masked = state->irq_address & 0x3FF;
      uint32 irq_sample_offset = irq_address_masked - state->irq_decoder_clock;
      if(irq_sample_offset > 0x3FF) irq_sample_offset += 0x400;
      if (irq_sample_offset < (uint32)samples) {
        irq_state.triggered_cycle = irq_sample_offset * 768;
      }
    } else {
      irq_state_ptr = &irq_state;
      irq_state.offset = state->irq_address;
    }
  }

  state->irq_decoder_clock = (state->irq_decoder_clock + samples) & 0x3FF;

  memset(ibufmix, 0, 8 * samples);
  if(effectout) {
    memset(ibufrvb, 0, 8 * samples);
  }

  maskmain_l = 0;
  maskmain_r = 0;
  maskverb_l = 0;
  maskverb_r = 0;
  if(state->flags & SPUREG_FLAG_MSNDL) maskmain_l = state->vmix[0] & 0xFFFFFF;
  if(state->flags & SPUREG_FLAG_MSNDR) maskmain_r = state->vmix[1] & 0xFFFFFF;
  if(effectout) {
    if(state->flags & SPUREG_FLAG_MSNDEL) maskverb_l = state->vmixe[0] & 0xFFFFFF;
    if(state->flags & SPUREG_FLAG_MSNDER) maskverb_r = state->vmixe[1] & 0xFFFFFF;
  }
  masknoise = state->noise;
  render_noise(state, masknoise ? ibufn : NULL, samples);
  maskfm = state->fm & 0xFFFFFE;

  if(!mainout) { maskmain_l = 0; maskmain_r = 0; }

  for(ch = 0, chanbit = 1; ch < 24; ch++, chanbit <<= 1) {
    int r;
    sint32 v_l, v_r;
    uint32 main_l = chanbit & maskmain_l;
    uint32 main_r = chanbit & maskmain_r;
    uint32 verb_l = chanbit & maskverb_l;
    uint32 verb_r = chanbit & maskverb_r;
    sint32 *b = buf ? ibuf : NULL;
    sint32 *fm = (chanbit & maskfm) ? ibuffm : NULL;
    sint32 *noise = (chanbit & masknoise) ? ibufn : NULL;
    if(!(main_l | main_r | verb_l | verb_r)) b = NULL;
    r = render_channel_mono(
      ram, state->memsize, state->chan + ch, b, fm, noise, samples, irq_state_ptr
    );
    if(!b) {
      memset(ibuffm, 0, 4 * samples);
      continue;
    }
    memcpy(ibuffm, ibuf, 4 * r);
    if(r < samples) memset(ibuffm + r, 0, 4 * (samples-r));
    v_l = volume_getlevel(state->chan[ch].vol+0);
    v_r = volume_getlevel(state->chan[ch].vol+1);
    for(i = 0; i < r; i++) {
      sint32 q_l = (v_l * ibuf[i]) >> 16;
      sint32 q_r = (v_r * ibuf[i]) >> 16;
      if(main_l) ibufmix[2*i+0] += q_l;
      if(main_r) ibufmix[2*i+1] += q_r;
      if(verb_l) ibufrvb[2*i+0] += q_l;
      if(verb_r) ibufrvb[2*i+1] += q_r;
    }
  }

  state->irq_triggered_cycle = irq_state.triggered_cycle;

  if(!buf) return;

  /*
  ** Mix in external input, if it exists
  */
  if(extinput) {
    sint32 extvol_l = ((sint16)(state->avol[0]));
    sint32 extvol_r = ((sint16)(state->avol[1]));
    if(extvol_l == -0x8000) extvol_l = -0x7FFF;
    if(extvol_r == -0x8000) extvol_r = -0x7FFF;
    for(i = 0; i < samples; i++) {
      sint32 sin_l = extinput[2*i+0];
      sint32 sin_r = extinput[2*i+1];
      sin_l *= extvol_l;
      sin_r *= extvol_r;
      sin_l >>= 15;
      sin_r >>= 15;
      if(state->flags & SPUREG_FLAG_SINL ) ibufmix[2*i+0] += sin_l;
      if(state->flags & SPUREG_FLAG_SINR ) ibufmix[2*i+1] += sin_r;
      if(state->flags & SPUREG_FLAG_SINEL) ibufrvb[2*i+0] += sin_l;
      if(state->flags & SPUREG_FLAG_SINER) ibufrvb[2*i+1] += sin_r;
    }
  }

  /*
  ** Do reverb
  ** This handles both writing into the buffer and retrieving
  ** values out of it, resampling to/from 22KHz, etc.
  */
  if(effectout) {
    reverb_process(state, ram, ibufrvb, samples);
  }
  /*
  **
  */
  m_v_l = volume_getlevel(state->mvol+0);
  m_v_r = volume_getlevel(state->mvol+1);
  r_v_l = ((sint16)(state->evol[0]));
  r_v_r = ((sint16)(state->evol[1]));

  if(!effectout) {
    for(i = 0; i < samples; i++) {
      sint64 q_l = ibufmix[2*i+0];
      sint64 q_r = ibufmix[2*i+1];
      q_l *= (sint64)m_v_l;
      q_r *= (sint64)m_v_r;
      q_l >>= 15;
      q_r >>= 15;
      CLIP_PCM_2(q_l,q_r);
      *buf++ = (sint16)q_l;
      *buf++ = (sint16)q_r;
    }
  } else {
    for(i = 0; i < samples; i++) {
      sint64 q_l = ibufmix[2*i+0];
      sint64 q_r = ibufmix[2*i+1];
      sint64 r_l = ibufrvb[2*i+0];
      sint64 r_r = ibufrvb[2*i+1];
      q_l *= (sint64)m_v_l;
      q_r *= (sint64)m_v_r;
      r_l *= (sint64)r_v_l;
      r_r *= (sint64)r_v_r;
      q_l >>= 15;
      q_r >>= 15;
      r_l >>= 15;
      r_r >>= 15;
      q_l += r_l;
      q_r += r_r;
      CLIP_PCM_2(q_l, q_r);
      *buf++ = (sint16)q_l;
      *buf++ = (sint16)q_r;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Externally-accessible renderer
*/
void EMU_CALL spucore_render(void *state, uint16 *ram, sint16 *buf, sint16 *extinput, uint32 samples, uint8 mainout, uint8 effectout) {
  while(samples > RENDERMAX) {
    samples -= RENDERMAX;
    render(SPUCORESTATE, ram, buf, extinput, RENDERMAX, mainout, effectout);
    if(buf     ) buf      += 2 * RENDERMAX;
    if(extinput) extinput += 2 * RENDERMAX;
  }
  if(samples) render(SPUCORESTATE, ram, buf, extinput, samples, mainout, effectout);
}

////////////////////////////////////////////////////////////////////////////////
/*
** Flag get/set
*/

int EMU_CALL spucore_getflag(void *state, uint32 n) {
  return !!(SPUCORESTATE->flags & n);
}

void EMU_CALL spucore_setflag(void *state, uint32 n, int value) {
  if(value) {
    SPUCORESTATE->flags |= n;
  } else {
    SPUCORESTATE->flags &= ~n;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Register get/set
*/

uint32 EMU_CALL spucore_getreg(void *state, uint32 n) {
  switch(n) {
  case SPUREG_MVOLL:  return volume_getmode (SPUCORESTATE->mvol+0) & 0x0000FFFF;
  case SPUREG_MVOLR:  return volume_getmode (SPUCORESTATE->mvol+1) & 0x0000FFFF;
  case SPUREG_MVOLXL: return volume_getlevel(SPUCORESTATE->mvol+0) & 0x0000FFFF;
  case SPUREG_MVOLXR: return volume_getlevel(SPUCORESTATE->mvol+1) & 0x0000FFFF;
  case SPUREG_EVOLL:  return SPUCORESTATE->evol[0] & 0x0000FFFF;
  case SPUREG_EVOLR:  return SPUCORESTATE->evol[1] & 0x0000FFFF;
  case SPUREG_AVOLL:  return SPUCORESTATE->avol[0] & 0x0000FFFF;
  case SPUREG_AVOLR:  return SPUCORESTATE->avol[1] & 0x0000FFFF;
  case SPUREG_BVOLL:  return SPUCORESTATE->bvol[0] & 0x0000FFFF;
  case SPUREG_BVOLR:  return SPUCORESTATE->bvol[1] & 0x0000FFFF;
  case SPUREG_KON:    return SPUCORESTATE->kon     & 0x00FFFFFF;
  case SPUREG_KOFF:   return SPUCORESTATE->koff    & 0x00FFFFFF;
  case SPUREG_FM:     return SPUCORESTATE->fm      & 0x00FFFFFF;
  case SPUREG_NOISE:  return SPUCORESTATE->noise   & 0x00FFFFFF;
  case SPUREG_VMIXE:  return (SPUCORESTATE->vmixe[0] | SPUCORESTATE->vmixe[1]) & 0xFFFFFF;
  case SPUREG_VMIX:   return (SPUCORESTATE->vmix[0]  | SPUCORESTATE->vmix[1] ) & 0xFFFFFF;
  case SPUREG_VMIXEL: return SPUCORESTATE->vmixe[0] & 0x00FFFFFF;
  case SPUREG_VMIXER: return SPUCORESTATE->vmixe[1] & 0x00FFFFFF;
  case SPUREG_VMIXL:  return SPUCORESTATE->vmix[0]  & 0x00FFFFFF;
  case SPUREG_VMIXR:  return SPUCORESTATE->vmix[1]  & 0x00FFFFFF;
  case SPUREG_ESA:    return SPUCORESTATE->reverb.start_address;
  case SPUREG_EEA:    return SPUCORESTATE->reverb.end_address;
  case SPUREG_EAX:    return SPUCORESTATE->reverb.current_address;
  case SPUREG_IRQA:   return SPUCORESTATE->irq_address;
  case SPUREG_NOISECLOCK: return SPUCORESTATE->noiseclock;
#define SPUCORE_REVERB_GET(x) case SPUREG_REVERB_##x:return SPUCORESTATE->reverb.x;
  SPUCORE_REVERB_GET(FB_SRC_A)
  SPUCORE_REVERB_GET(FB_SRC_B)
  SPUCORE_REVERB_GET(IIR_ALPHA)
  SPUCORE_REVERB_GET(ACC_COEF_A)
  SPUCORE_REVERB_GET(ACC_COEF_B)
  SPUCORE_REVERB_GET(ACC_COEF_C)
  SPUCORE_REVERB_GET(ACC_COEF_D)
  SPUCORE_REVERB_GET(IIR_COEF)
  SPUCORE_REVERB_GET(FB_ALPHA)
  SPUCORE_REVERB_GET(FB_X)
  SPUCORE_REVERB_GET(IIR_DEST_A0)
  SPUCORE_REVERB_GET(IIR_DEST_A1)
  SPUCORE_REVERB_GET(ACC_SRC_A0)
  SPUCORE_REVERB_GET(ACC_SRC_A1)
  SPUCORE_REVERB_GET(ACC_SRC_B0)
  SPUCORE_REVERB_GET(ACC_SRC_B1)
  SPUCORE_REVERB_GET(IIR_SRC_A0)
  SPUCORE_REVERB_GET(IIR_SRC_A1)
  SPUCORE_REVERB_GET(IIR_DEST_B0)
  SPUCORE_REVERB_GET(IIR_DEST_B1)
  SPUCORE_REVERB_GET(ACC_SRC_C0)
  SPUCORE_REVERB_GET(ACC_SRC_C1)
  SPUCORE_REVERB_GET(ACC_SRC_D0)
  SPUCORE_REVERB_GET(ACC_SRC_D1)
  SPUCORE_REVERB_GET(IIR_SRC_B1)
  SPUCORE_REVERB_GET(IIR_SRC_B0)
  SPUCORE_REVERB_GET(MIX_DEST_A0)
  SPUCORE_REVERB_GET(MIX_DEST_A1)
  SPUCORE_REVERB_GET(MIX_DEST_B0)
  SPUCORE_REVERB_GET(MIX_DEST_B1)
  SPUCORE_REVERB_GET(IN_COEF_L)
  SPUCORE_REVERB_GET(IN_COEF_R)
  }
  return 0;
}

void EMU_CALL spucore_setreg(void *state, uint32 n, uint32 value, uint32 mask) {
  value &= mask;
  switch(n) {
  /* TODO: the increase/decrease modes, etc. */
  case SPUREG_MVOLL: volume_setmode(SPUCORESTATE->mvol+0, value); break;
  case SPUREG_MVOLR: volume_setmode(SPUCORESTATE->mvol+1, value); break;
  case SPUREG_EVOLL: SPUCORESTATE->evol[0] = value; break;
  case SPUREG_EVOLR: SPUCORESTATE->evol[1] = value; break;
  case SPUREG_AVOLL: SPUCORESTATE->avol[0] = value; break;
  case SPUREG_AVOLR: SPUCORESTATE->avol[1] = value; break;
  case SPUREG_BVOLL: SPUCORESTATE->bvol[0] = value; break;
  case SPUREG_BVOLR: SPUCORESTATE->bvol[1] = value; break;

  case SPUREG_KON:
    SPUCORESTATE->kon &= ~mask;
    SPUCORESTATE->kon |= value;
    voices_on(state, value);
    break;
  case SPUREG_KOFF:
    SPUCORESTATE->koff &= ~mask;
    SPUCORESTATE->koff |= value;
    voices_off(state, value);
    break;

  case SPUREG_FM:
    SPUCORESTATE->fm &= ~mask;
    SPUCORESTATE->fm |= value;
    break;
  case SPUREG_NOISE:
    SPUCORESTATE->noise &= ~mask;
    SPUCORESTATE->noise |= value;
    break;
  case SPUREG_VMIXE:
    SPUCORESTATE->vmixe[0] &= ~mask;
    SPUCORESTATE->vmixe[0] |= value;
    SPUCORESTATE->vmixe[1] &= ~mask;
    SPUCORESTATE->vmixe[1] |= value;
    break;
  case SPUREG_VMIX:
    SPUCORESTATE->vmix[0] &= ~mask;
    SPUCORESTATE->vmix[0] |= value;
    SPUCORESTATE->vmix[1] &= ~mask;
    SPUCORESTATE->vmix[1] |= value;
    break;
  case SPUREG_VMIXEL:
    SPUCORESTATE->vmixe[0] &= ~mask;
    SPUCORESTATE->vmixe[0] |= value;
    break;
  case SPUREG_VMIXER:
    SPUCORESTATE->vmixe[1] &= ~mask;
    SPUCORESTATE->vmixe[1] |= value;
    break;
  case SPUREG_VMIXL:
    SPUCORESTATE->vmix[0] &= ~mask;
    SPUCORESTATE->vmix[0] |= value;
    break;
  case SPUREG_VMIXR:
    SPUCORESTATE->vmix[1] &= ~mask;
    SPUCORESTATE->vmix[1] |= value;
    break;

  case SPUREG_ESA:
    SPUCORESTATE->reverb.start_address &= ~mask;
    SPUCORESTATE->reverb.start_address |= value;
    make_safe_reverb_addresses(state);
    SPUCORESTATE->reverb.current_address = SPUCORESTATE->reverb.safe_start_address;
    break;
  case SPUREG_EEA:
    SPUCORESTATE->reverb.end_address &= ~mask;
    SPUCORESTATE->reverb.end_address |= value;
    make_safe_reverb_addresses(state);
    break;
  case SPUREG_IRQA:
    /* TODO: actual implementation of IRQs */
    SPUCORESTATE->irq_address &= ~mask;
    SPUCORESTATE->irq_address |= value;
    break;
  case SPUREG_NOISECLOCK:
    SPUCORESTATE->noiseclock = value & 0x3F;
    break;

#define SPUCORE_REVERB_SET(x) case SPUREG_REVERB_##x:SPUCORESTATE->reverb.x&=(~mask);SPUCORESTATE->reverb.x|=value;break;
  SPUCORE_REVERB_SET(FB_SRC_A)
  SPUCORE_REVERB_SET(FB_SRC_B)
  SPUCORE_REVERB_SET(IIR_ALPHA)
  SPUCORE_REVERB_SET(ACC_COEF_A)
  SPUCORE_REVERB_SET(ACC_COEF_B)
  SPUCORE_REVERB_SET(ACC_COEF_C)
  SPUCORE_REVERB_SET(ACC_COEF_D)
  SPUCORE_REVERB_SET(IIR_COEF)
  SPUCORE_REVERB_SET(FB_ALPHA)
  SPUCORE_REVERB_SET(FB_X)
  SPUCORE_REVERB_SET(IIR_DEST_A0)
  SPUCORE_REVERB_SET(IIR_DEST_A1)
  SPUCORE_REVERB_SET(ACC_SRC_A0)
  SPUCORE_REVERB_SET(ACC_SRC_A1)
  SPUCORE_REVERB_SET(ACC_SRC_B0)
  SPUCORE_REVERB_SET(ACC_SRC_B1)
  SPUCORE_REVERB_SET(IIR_SRC_A0)
  SPUCORE_REVERB_SET(IIR_SRC_A1)
  SPUCORE_REVERB_SET(IIR_DEST_B0)
  SPUCORE_REVERB_SET(IIR_DEST_B1)
  SPUCORE_REVERB_SET(ACC_SRC_C0)
  SPUCORE_REVERB_SET(ACC_SRC_C1)
  SPUCORE_REVERB_SET(ACC_SRC_D0)
  SPUCORE_REVERB_SET(ACC_SRC_D1)
  SPUCORE_REVERB_SET(IIR_SRC_B1)
  SPUCORE_REVERB_SET(IIR_SRC_B0)
  SPUCORE_REVERB_SET(MIX_DEST_A0)
  SPUCORE_REVERB_SET(MIX_DEST_A1)
  SPUCORE_REVERB_SET(MIX_DEST_B0)
  SPUCORE_REVERB_SET(MIX_DEST_B1)
  SPUCORE_REVERB_SET(IN_COEF_L)
  SPUCORE_REVERB_SET(IN_COEF_R)
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Voice register get/set
*/

uint32 EMU_CALL spucore_getreg_voice(void *state, uint32 voice, uint32 n) {
  switch(n) {
  case SPUREG_VOICE_VOLL : return volume_getmode(SPUCORESTATE->chan[voice].vol+0);
  case SPUREG_VOICE_VOLR : return volume_getmode(SPUCORESTATE->chan[voice].vol+1);
  case SPUREG_VOICE_VOLXL: return volume_getlevel(SPUCORESTATE->chan[voice].vol+0);
  case SPUREG_VOICE_VOLXR: return volume_getlevel(SPUCORESTATE->chan[voice].vol+1);
  case SPUREG_VOICE_PITCH: return SPUCORESTATE->chan[voice].voice_pitch;
  case SPUREG_VOICE_ADSR1: return SPUCORESTATE->chan[voice].env.reg_ad;
  case SPUREG_VOICE_ADSR2: return SPUCORESTATE->chan[voice].env.reg_sr;
  case SPUREG_VOICE_ENVX :
    if(SPUCORESTATE->chan[voice].env.state == ENVELOPE_STATE_OFF) return 0;
    return (SPUCORESTATE->chan[voice].env.level) >> 16;
  case SPUREG_VOICE_SSA  : return SPUCORESTATE->chan[voice].sample.start_block_addr;
  case SPUREG_VOICE_LSAX : return SPUCORESTATE->chan[voice].sample.loop_block_addr;
  case SPUREG_VOICE_NAX  : return SPUCORESTATE->chan[voice].sample.block_addr;
  }
  return 0;
}

void EMU_CALL spucore_setreg_voice(void *state, uint32 voice, uint32 n, uint32 value, uint32 mask) {
  value &= mask;
  switch(n) {
  /* TODO: the increase/decrease modes */
  case SPUREG_VOICE_VOLL : volume_setmode(SPUCORESTATE->chan[voice].vol+0, value); break;
  case SPUREG_VOICE_VOLR : volume_setmode(SPUCORESTATE->chan[voice].vol+1, value); break;
  case SPUREG_VOICE_PITCH: SPUCORESTATE->chan[voice].voice_pitch = value; break;
  case SPUREG_VOICE_ADSR1: SPUCORESTATE->chan[voice].env.reg_ad = value; SPUCORESTATE->chan[voice].env.cachemax = envelope_do(&SPUCORESTATE->chan[voice].env); break;
  case SPUREG_VOICE_ADSR2: SPUCORESTATE->chan[voice].env.reg_sr = value; SPUCORESTATE->chan[voice].env.cachemax = envelope_do(&SPUCORESTATE->chan[voice].env); break;

  case SPUREG_VOICE_SSA:
    SPUCORESTATE->chan[voice].sample.start_block_addr &= ~mask;
    SPUCORESTATE->chan[voice].sample.start_block_addr |= value;
    break;
  case SPUREG_VOICE_LSAX:
    SPUCORESTATE->chan[voice].sample.loop_block_addr &= ~mask;
    SPUCORESTATE->chan[voice].sample.loop_block_addr |= value;
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** IRQ checking
*/
uint32 EMU_CALL spucore_cycles_until_interrupt(void *state, uint16 *ram, uint32 samples) {
  uint32 r;
  void *backup;

  if (!(SPUCORESTATE->flags & SPUREG_FLAG_IRQ_ENABLE)) return 0xFFFFFFFF;

  backup = malloc(spucore_get_state_size());
  if (!backup) return 0xFFFFFFFF;
  memcpy(backup, state, spucore_get_state_size());
  state = backup;
  SPUCORESTATE->irq_triggered_cycle = 0xFFFFFFFF;
  r = 0;
  while(samples > RENDERMAX) {
    samples -= RENDERMAX;
    render(SPUCORESTATE, ram, NULL, NULL, RENDERMAX, 0, 0);
	if (SPUCORESTATE->irq_triggered_cycle != 0xFFFFFFFF) break;
	r += RENDERMAX * 768;
  }
  if(samples && SPUCORESTATE->irq_triggered_cycle == 0xFFFFFFFF) render(SPUCORESTATE, ram, NULL, NULL, samples, 0, 0);
  r = (SPUCORESTATE->irq_triggered_cycle == 0xFFFFFFFF) ? 0xFFFFFFFF : SPUCORESTATE->irq_triggered_cycle + r;
  free(backup);
  return r;
}

////////////////////////////////////////////////////////////////////////////////
