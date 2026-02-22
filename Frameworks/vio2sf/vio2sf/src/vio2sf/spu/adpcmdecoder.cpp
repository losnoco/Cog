#include "adpcmdecoder.h"
#include <iostream>

template <typename T, typename T2>
static inline T clamp(T2 value, T lower, T upper)
{
  return value < lower ? lower : (value > upper ? upper : value);
}

template <typename T, typename Container>
static inline T parseInt(const Container& buffer, int offset)
{
  uint64_t result = 0;
  for (int i = sizeof(T) - 1; i >= 0; --i) {
    result = (result << 8) | uint8_t(buffer[offset + i]);
  }
  return T(result);
}

static const int16_t adpcmIndex[] = { -1, -1, -1, -1, 2, 4, 6, 8, };

static const int16_t adpcmStep[] = {
      7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
     19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
     50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
   2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
   5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};
static const int16_t maxAdpcmStep = (sizeof(adpcmStep) >> 1) - 1;

AdpcmDecoder::AdpcmDecoder(int16_t initialPredictor, int16_t initialStep)
: predictor(initialPredictor), index(clamp<int8_t>(initialStep, 0, maxAdpcmStep))
{
  // initializers only
}

int16_t AdpcmDecoder::getNextSample(uint8_t value)
{
  int16_t step = adpcmStep[index];
  int32_t delta = step >> 3;
  if (value & 0x04) delta += step;
  if (value & 0x02) delta += step >> 1;
  if (value & 0x01) delta += step >> 2;
  if (value & 0x08) delta = -delta;
  /*
  // This implementation is simpler and obeys the spec, but it suffers from low-bit rounding
  // errors that introduce a slow DC drift that causes looping to produce pops.
  int32_t delta = (int32_t(value & 0x08 ? -step : step) * ((value << 1 & 0x0f) + 0x01)) >> 3;
  */
  if (predictor + delta == -0x8000) {
    predictor = -0x8000;
  } else {
    predictor = clamp<int16_t>(predictor + delta, -0x7fff, 0x7fff);
  }
  index = clamp<int8_t>(index + adpcmIndex[value & 0x07], 0, maxAdpcmStep);
  return predictor;
}

std::vector<int16_t> AdpcmDecoder::decodeFile(const std::vector<char>& data, uint32_t offset, uint32_t length)
{
  if (!length) {
    length = data.size() - offset;
  }
  AdpcmDecoder adpcm(parseInt<int16_t>(data, offset), parseInt<int16_t>(data, offset + 2));
  return adpcm.decode(data, offset + 4, length - 4);
}

std::vector<int16_t> AdpcmDecoder::decode(const std::vector<char>& data, uint32_t offset, uint32_t length)
{
  if (!length) {
    length = data.size() - offset;
  }
  std::vector<int16_t> sample;
  sample.reserve(length << 1);
  for (int i = 0; i < length; i++) {
    sample.push_back(getNextSample(data[offset + i] & 0x0f));
    sample.push_back(getNextSample(uint8_t(data[offset + i] & 0xf0) >> 4));
  }
  return sample;
}
