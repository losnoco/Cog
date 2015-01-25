/*
 * Siren Encoder/Decoder library
 *
 *   @author: Youness Alaoui <kakaroto@kakaroto.homelinux.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "siren7.h"

SirenDecoder
Siren7_NewDecoder (int sample_rate, int flag)
{
  SirenDecoder decoder = (SirenDecoder) malloc (sizeof (struct stSirenDecoder));
  decoder->sample_rate = sample_rate;
  decoder->flag = flag;

#ifdef __WAV_HEADER__
  decoder->WavHeader.riff.RiffId = ME_TO_LE32 (RIFF_ID);
  decoder->WavHeader.riff.RiffSize = sizeof (PCMWavHeader) - 2 * sizeof (int);
  decoder->WavHeader.riff.RiffSize =
      ME_TO_LE32 (decoder->WavHeader.riff.RiffSize);
  decoder->WavHeader.WaveId = ME_TO_LE32 (WAVE_ID);

  decoder->WavHeader.FmtId = ME_TO_LE32 (FMT__ID);
  decoder->WavHeader.FmtSize = ME_TO_LE32 (sizeof (FmtChunk));

  decoder->WavHeader.fmt.Format = ME_TO_LE16 (0x01);
  decoder->WavHeader.fmt.Channels = ME_TO_LE16 (1);
  decoder->WavHeader.fmt.SampleRate = ME_TO_LE32 (sample_rate);
  decoder->WavHeader.fmt.ByteRate = ME_TO_LE32 (sample_rate * 2);
  decoder->WavHeader.fmt.BlockAlign = ME_TO_LE16 (2);
  decoder->WavHeader.fmt.BitsPerSample = ME_TO_LE16 (16);

  decoder->WavHeader.FactId = ME_TO_LE32 (FACT_ID);
  decoder->WavHeader.FactSize = ME_TO_LE32 (sizeof (int));
  decoder->WavHeader.Samples = ME_TO_LE32 (0);

  decoder->WavHeader.DataId = ME_TO_LE32 (DATA_ID);
  decoder->WavHeader.DataSize = ME_TO_LE32 (0);
#endif
    
  Siren7_ResetDecoder(decoder);
    
  siren_init ();
  return decoder;
}

void
Siren7_ResetDecoder(SirenDecoder decoder)
{
  memset(decoder->context, 0, sizeof(decoder->context));
  memset(decoder->backup_frame, 0, sizeof(decoder->backup_frame));

  decoder->dw1 = 1;
  decoder->dw2 = 1;
  decoder->dw3 = 1;
  decoder->dw4 = 1;
    
  memset(decoder->absolute_region_power_index, 0, sizeof(decoder->absolute_region_power_index));
  memset(decoder->decoder_standard_deviation, 0, sizeof(decoder->decoder_standard_deviation));
  memset(decoder->power_categories, 0, sizeof(decoder->power_categories));
  memset(decoder->category_balance, 0, sizeof(decoder->category_balance));
}

void
Siren7_CloseDecoder (SirenDecoder decoder)
{
  free (decoder);
}

int
Siren7_DecodeFrame (SirenDecoder decoder, unsigned char *DataIn,
    unsigned char *DataOut)
{
  int number_of_coefs,
      sample_rate_bits,
      rate_control_bits,
      rate_control_possibilities,
      checksum_bits,
      esf_adjustment,
      scale_factor, number_of_regions, sample_rate_code, bits_per_frame;
  int decoded_sample_rate_code;

  int ChecksumTable[4] = { 0x7F80, 0x7878, 0x6666, 0x5555 };
  int i, j;

  int dwRes = 0;
  int envelope_bits = 0;
  int rate_control = 0;
  int number_of_available_bits;
  int number_of_valid_coefs;
  int frame_error = 0;

  int In[60];
  float coefs[640];
  float BufferOut[640];
  int sum;
  int checksum;
  int calculated_checksum;
  int idx;
  int temp1;
  int temp2;

  bitstream bs = {0};

  dwRes =
      GetSirenCodecInfo (decoder->flag, decoder->sample_rate, &number_of_coefs,
      &sample_rate_bits, &rate_control_bits, &rate_control_possibilities,
      &checksum_bits, &esf_adjustment, &scale_factor, &number_of_regions,
      &sample_rate_code, &bits_per_frame);

  if (dwRes != 0)
    return dwRes;

#ifdef __NO_CONTROL_OR_CHECK_FIELDS__
  sample_rate_bits = 0;
  checksum_bits = 0;
  sample_rate_code = 0;
#endif

  j = bits_per_frame / 16;
    
  for (i = 0; i < j; i++)
#ifdef __BIG_ENDIAN_FRAMES__
    In[i] = ((short *) DataIn)[i];
#else
    In[i] =
        ((((short *) DataIn)[i] << 8) & 0xFF00) | ((((short *) DataIn)[i] >> 8)
        & 0x00FF);
#endif

  set_bitstream (&bs, In);

  decoded_sample_rate_code = 0;
  for (i = 0; i < sample_rate_bits; i++) {
    decoded_sample_rate_code <<= 1;
    decoded_sample_rate_code |= next_bit (&bs);
  }


  if (decoded_sample_rate_code != sample_rate_code)
    return 7;

  number_of_valid_coefs = region_size * number_of_regions;
  number_of_available_bits = bits_per_frame - sample_rate_bits - checksum_bits;


  envelope_bits =
      decode_envelope (&bs, number_of_regions, decoder->decoder_standard_deviation,
      decoder->absolute_region_power_index, esf_adjustment);

  number_of_available_bits -= envelope_bits;

  for (i = 0; i < rate_control_bits; i++) {
    rate_control <<= 1;
    rate_control |= next_bit (&bs);
  }

  number_of_available_bits -= rate_control_bits;

  categorize_regions (number_of_regions, number_of_available_bits,
      decoder->absolute_region_power_index, decoder->power_categories, decoder->category_balance);

  for (i = 0; i < rate_control; i++) {
    decoder->power_categories[decoder->category_balance[i]]++;
  }

  number_of_available_bits =
      decode_vector (decoder, &bs, number_of_regions, number_of_available_bits,
      decoder->decoder_standard_deviation, decoder->power_categories, coefs, scale_factor);


  frame_error = 0;
  if (number_of_available_bits > 0) {
    for (i = 0; i < number_of_available_bits; i++) {
      if (next_bit (&bs) == 0)
        frame_error = 1;
    }
  } else if (number_of_available_bits < 0
      && rate_control + 1 < rate_control_possibilities) {
    frame_error |= 2;
  }

  for (i = 0; i < number_of_regions; i++) {
    if (decoder->absolute_region_power_index[i] > 33
        || decoder->absolute_region_power_index[i] < -31)
      frame_error |= 4;
  }

  if (checksum_bits > 0) {
    bits_per_frame >>= 4;
    checksum = In[bits_per_frame - 1] & ((1 << checksum_bits) - 1);
    In[bits_per_frame - 1] &= ~checksum;
    sum = 0;
    idx = 0;
    do {
      sum ^= (In[idx] & 0xFFFF) << (idx % 15);
    } while (++idx < bits_per_frame);

    sum = (sum >> 15) ^ (sum & 0x7FFF);
    calculated_checksum = 0;
    for (i = 0; i < 4; i++) {
      temp1 = ChecksumTable[i] & sum;
      for (j = 8; j > 0; j >>= 1) {
        temp2 = temp1 >> j;
        temp1 ^= temp2;
      }
      calculated_checksum <<= 1;
      calculated_checksum |= temp1 & 1;
    }

    if (checksum != calculated_checksum)
      frame_error |= 8;
  }

  if (frame_error != 0) {
    for (i = 0; i < number_of_valid_coefs; i++) {
      coefs[i] = decoder->backup_frame[i];
      decoder->backup_frame[i] = 0;
    }
  } else {
    for (i = 0; i < number_of_valid_coefs; i++)
      decoder->backup_frame[i] = coefs[i];
  }


  for (i = number_of_valid_coefs; i < number_of_coefs; i++)
    coefs[i] = 0;


  dwRes = siren_rmlt_decode_samples (coefs, decoder->context, number_of_coefs, BufferOut);


  for (i = 0; i < number_of_coefs; i++) {
    if (BufferOut[i] > 32767.0)
      ((short *) DataOut)[i] = (short) ME_TO_LE16 ((short) 32767);
    else if (BufferOut[i] <= -32768.0)
      ((short *) DataOut)[i] = (short) ME_TO_LE16 ((short) 32768);
    else
      ((short *) DataOut)[i] = (short) ME_TO_LE16 ((short) BufferOut[i]);
  }

#ifdef __WAV_HEADER__
  decoder->WavHeader.Samples = ME_FROM_LE32 (decoder->WavHeader.Samples);
  decoder->WavHeader.Samples += number_of_coefs;
  decoder->WavHeader.Samples = ME_TO_LE32 (decoder->WavHeader.Samples);
  decoder->WavHeader.DataSize = ME_FROM_LE32 (decoder->WavHeader.DataSize);
  decoder->WavHeader.DataSize += number_of_coefs * 2;
  decoder->WavHeader.DataSize = ME_TO_LE32 (decoder->WavHeader.DataSize);
  decoder->WavHeader.riff.RiffSize =
      ME_FROM_LE32 (decoder->WavHeader.riff.RiffSize);
  decoder->WavHeader.riff.RiffSize += number_of_coefs * 2;
  decoder->WavHeader.riff.RiffSize =
      ME_TO_LE32 (decoder->WavHeader.riff.RiffSize);
#endif

  return 0;
}
