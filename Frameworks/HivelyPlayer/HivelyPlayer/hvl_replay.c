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
#include "hvl_tables.h"

#undef min
#define min(a,b) (((a) < (b)) ? (a) : (b))

int8 waves[WAVES_SIZE];

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

static int32 clipshifted8(int32 in)
{
  int16 top = (int16)(in >> 16);
  if (top > 127) in = 127 << 16;
  else if (top < -128) in = -128 << 16;
  return in;
}

void hvl_GenFilterWaves( int8 *buf, int8 *lowbuf, int8 *highbuf )
{
  const int16 * mid_table = &filter_thing[0];
  const int16 * low_table = &filter_thing[1395];

  int32 freq;
  int32 i;

  for( i=0, freq = 25; i<31; i++, freq += 9 )
  {
    uint32 wv;
    int8  *a0 = buf;

    for( wv=0; wv<6+6+0x20+1; wv++ )
    {
      int32 in, fre, high, mid, low;
      uint32  j;

      mid  = *mid_table++ << 8;
      low = *low_table++ << 8;

      for( j=0; j<=lentab[wv]; j++ )
      {
        in   = a0[j] << 16;
        high = clipshifted8( in - mid - low );
        fre  = (high >> 8) * freq;
        mid  = clipshifted8(mid + fre);
        fre  = (mid  >> 8) * freq;
        low  = clipshifted8(low + fre);
        *highbuf++ = high >> 16;
        *lowbuf++  = low  >> 16;
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
  hvl_GenFilterWaves( &waves[WO_TRIANGLE_04], &waves[WO_LOWPASSES], &waves[WO_HIGHPASSES] );
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

  ht->ht_WaveformTab[0]  = &waves[WO_TRIANGLE_04];
  ht->ht_WaveformTab[1]  = &waves[WO_SAWTOOTH_04];
  ht->ht_WaveformTab[3]  = &waves[WO_WHITENOISE];

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
    
    case 0x3: // Tone portamento
      if( FXParam != 0 ) voice->vc_PeriodSlideSpeed = FXParam;
    case 0x5: // Tone portamento + volume slide
      
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
    SquarePtr = &waves[WO_SQUARES+(voice->vc_FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3)];
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
