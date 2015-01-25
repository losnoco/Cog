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


SirenEncoder
Siren7_NewEncoder (int sample_rate, int flag)
{
  SirenEncoder encoder = (SirenEncoder) malloc (sizeof (struct stSirenEncoder));
  encoder->sample_rate = sample_rate;
  encoder->flag = flag;

#ifdef __WAV_HEADER__
  encoder->WavHeader.riff.RiffId = ME_TO_LE32 (RIFF_ID);
  encoder->WavHeader.riff.RiffSize = sizeof (SirenWavHeader) - 2 * sizeof (int);
  encoder->WavHeader.riff.RiffSize =
      ME_TO_LE32 (encoder->WavHeader.riff.RiffSize);
  encoder->WavHeader.WaveId = ME_TO_LE32 (WAVE_ID);

  encoder->WavHeader.FmtId = ME_TO_LE32 (FMT__ID);
  encoder->WavHeader.FmtSize = ME_TO_LE32 (sizeof (SirenFmtChunk));

  encoder->WavHeader.fmt.fmt.Format = ME_TO_LE16 (0x028E);
  encoder->WavHeader.fmt.fmt.Channels = ME_TO_LE16 (1);
  encoder->WavHeader.fmt.fmt.SampleRate = ME_TO_LE32 (sample_rate);
  encoder->WavHeader.fmt.fmt.ByteRate = ME_TO_LE32 (sample_rate * 16 / 3);
  encoder->WavHeader.fmt.fmt.BlockAlign = ME_TO_LE16 (40);
  encoder->WavHeader.fmt.fmt.BitsPerSample = ME_TO_LE16 (0);
  encoder->WavHeader.fmt.ExtraSize = ME_TO_LE16 (2);
  encoder->WavHeader.fmt.DctLength = ME_TO_LE16 (flag == 1 ? 320 : 640);

  encoder->WavHeader.FactId = ME_TO_LE32 (FACT_ID);
  encoder->WavHeader.FactSize = ME_TO_LE32 (sizeof (int));
  encoder->WavHeader.Samples = ME_TO_LE32 (0);

  encoder->WavHeader.DataId = ME_TO_LE32 (DATA_ID);
  encoder->WavHeader.DataSize = ME_TO_LE32 (0);
#endif

  memset (encoder->context, 0, sizeof (encoder->context));

  memset (encoder->absolute_region_power_index, 0, sizeof(encoder->absolute_region_power_index));
  memset (encoder->power_categories, 0, sizeof(encoder->power_categories));
  memset (encoder->category_balance, 0, sizeof(encoder->category_balance));
  memset (encoder->drp_num_bits, 0, sizeof(encoder->drp_num_bits));
  memset (encoder->drp_code_bits, 0, sizeof(encoder->drp_code_bits));
  memset (encoder->region_mlt_bit_counts, 0, sizeof(encoder->region_mlt_bit_counts));
  memset (encoder->region_mlt_bits, 0, sizeof(encoder->region_mlt_bits));

  siren_init ();
  return encoder;
}

void
Siren7_CloseEncoder (SirenEncoder encoder)
{
  free (encoder);
}



int
Siren7_EncodeFrame (SirenEncoder encoder, unsigned char *DataIn,
    unsigned char *DataOut)
{
  int number_of_coefs,
      sample_rate_bits,
      rate_control_bits,
      rate_control_possibilities,
      checksum_bits,
      esf_adjustment,
      scale_factor, number_of_regions, sample_rate_code, bits_per_frame;
  int sample_rate = encoder->sample_rate;

  int ChecksumTable[4] = { 0x7F80, 0x7878, 0x6666, 0x5555 };
  int i, j;

  int dwRes = 0;
  short out_word;
  int bits_left;
  int current_word_bits_left;
  int region_bit_count;
  unsigned int current_word;
  unsigned int sum;
  unsigned int checksum;
  int temp1 = 0;
  int temp2 = 0;
  int region;
  int idx = 0;
  int envelope_bits = 0;
  int rate_control;
  int number_of_available_bits;

  float coefs[640];
  float In[640];
  short BufferOut[60];
  float *context = encoder->context;

  dwRes =
  GetSirenCodecInfo (encoder->flag, sample_rate, &number_of_coefs, &sample_rate_bits,
                     &rate_control_bits, &rate_control_possibilities, &checksum_bits,
                     &esf_adjustment, &scale_factor, &number_of_regions, &sample_rate_code,
                     &bits_per_frame);
    
  if (dwRes != 0)
    return dwRes;
    
#ifdef __NO_CONTROL_OR_CHECK_FIELDS__
  sample_rate_bits = 0;
  checksum_bits = 0;
  sample_rate_code = 0;
#endif
    
  for (i = 0; i < number_of_coefs; i++)
    In[i] = (float) ((short) ME_FROM_LE16 (((short *) DataIn)[i]));

  dwRes = siren_rmlt_encode_samples (In, context, number_of_coefs, coefs);

  if (dwRes != 0)
    return dwRes;

  envelope_bits =
      compute_region_powers (number_of_regions, coefs, encoder->drp_num_bits,
      encoder->drp_code_bits, encoder->absolute_region_power_index, esf_adjustment);

  number_of_available_bits =
      bits_per_frame - rate_control_bits - envelope_bits - sample_rate_bits -
      checksum_bits;

  categorize_regions (number_of_regions, number_of_available_bits,
      encoder->absolute_region_power_index, encoder->power_categories, encoder->category_balance);

  for (region = 0; region < number_of_regions; region++) {
    encoder->absolute_region_power_index[region] += 24;
    encoder->region_mlt_bit_counts[region] = 0;
  }

  rate_control =
      quantize_mlt (number_of_regions, rate_control_possibilities,
      number_of_available_bits, coefs, encoder->absolute_region_power_index,
      encoder->power_categories, encoder->category_balance, encoder->region_mlt_bit_counts,
      encoder->region_mlt_bits);

  idx = 0;
  bits_left = 16 - sample_rate_bits;
  out_word = sample_rate_code << (16 - sample_rate_bits);
  encoder->drp_num_bits[number_of_regions] = rate_control_bits;
  encoder->drp_code_bits[number_of_regions] = rate_control;
  for (region = 0; region <= number_of_regions; region++) {
    i = encoder->drp_num_bits[region] - bits_left;
    if (i < 0) {
      out_word += encoder->drp_code_bits[region] << -i;
      bits_left -= encoder->drp_num_bits[region];
    } else {
      BufferOut[idx++] = out_word + (encoder->drp_code_bits[region] >> i);
      bits_left += 16 - encoder->drp_num_bits[region];
      out_word = encoder->drp_code_bits[region] << bits_left;
    }
  }

  for (region = 0; region < number_of_regions && (16 * idx) < bits_per_frame;
      region++) {
    current_word_bits_left = region_bit_count = encoder->region_mlt_bit_counts[region];
    if (current_word_bits_left > 32)
      current_word_bits_left = 32;

    current_word = encoder->region_mlt_bits[region * 4];
    i = 1;
    while (region_bit_count > 0 && (16 * idx) < bits_per_frame) {
      if (current_word_bits_left < bits_left) {
        bits_left -= current_word_bits_left;
        out_word +=
            (current_word >> (32 - current_word_bits_left)) << bits_left;
        current_word_bits_left = 0;
      } else {
        BufferOut[idx++] =
            (short) (out_word + (current_word >> (32 - bits_left)));
        current_word_bits_left -= bits_left;
        current_word <<= bits_left;
        bits_left = 16;
        out_word = 0;
      }
      if (current_word_bits_left == 0) {
        region_bit_count -= 32;
        current_word = encoder->region_mlt_bits[(region * 4) + i++];
        current_word_bits_left = region_bit_count;
        if (current_word_bits_left > 32)
          current_word_bits_left = 32;
      }
    }
  }


  while ((16 * idx) < bits_per_frame) {
    BufferOut[idx++] = (short) ((0xFFFF >> (16 - bits_left)) + out_word);
    bits_left = 16;
    out_word = 0;
  }

  if (checksum_bits > 0) {
    BufferOut[idx - 1] &= (-1 << checksum_bits);
    sum = 0;
    idx = 0;
    do {
      sum ^= (BufferOut[idx] & 0xFFFF) << (idx % 15);
    } while ((16 * ++idx) < bits_per_frame);

    sum = (sum >> 15) ^ (sum & 0x7FFF);
    checksum = 0;
    for (i = 0; i < 4; i++) {
      temp1 = ChecksumTable[i] & sum;
      for (j = 8; j > 0; j >>= 1) {
        temp2 = temp1 >> j;
        temp1 ^= temp2;
      }
      checksum <<= 1;
      checksum |= temp1 & 1;
    }
    BufferOut[idx - 1] |= ((1 << checksum_bits) - 1) & checksum;
  }

  j = bits_per_frame / 16;
  for (i = 0; i < j; i++)
#ifdef __BIG_ENDIAN_FRAMES__
    ((short *) DataOut)[i] = BufferOut[i];
#else
    ((short *) DataOut)[i] =
        ((BufferOut[i] << 8) & 0xFF00) | ((BufferOut[i] >> 8) & 0x00FF);
#endif

#ifdef __WAV_HEADER__
  encoder->WavHeader.Samples = ME_FROM_LE32 (encoder->WavHeader.Samples);
  encoder->WavHeader.Samples += number_of_coefs;
  encoder->WavHeader.Samples = ME_TO_LE32 (encoder->WavHeader.Samples);
  encoder->WavHeader.DataSize = ME_FROM_LE32 (encoder->WavHeader.DataSize);
  encoder->WavHeader.DataSize += bits_per_Frame / 8;
  encoder->WavHeader.DataSize = ME_TO_LE32 (encoder->WavHeader.DataSize);
  encoder->WavHeader.riff.RiffSize =
      ME_FROM_LE32 (encoder->WavHeader.riff.RiffSize);
  encoder->WavHeader.riff.RiffSize += bits_per_Frame / 8;
  encoder->WavHeader.riff.RiffSize =
      ME_TO_LE32 (encoder->WavHeader.riff.RiffSize);
#endif

  return 0;
}
