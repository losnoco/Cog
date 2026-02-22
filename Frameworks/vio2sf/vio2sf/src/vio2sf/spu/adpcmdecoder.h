#ifndef ADPCMDECODER_H
#define ADPCMDECODER_H

#include <vector>
#include <cstdint>

class AdpcmDecoder
{
private:
  int16_t predictor;
  int8_t index;

public:
  AdpcmDecoder(int16_t initialPredictor, int16_t initialStep);
  int16_t getNextSample(uint8_t value);

  std::vector<int16_t> decode(const std::vector<char>& data, uint32_t offset = 0, uint32_t length = 0);
  static std::vector<int16_t> decodeFile(const std::vector<char>& data, uint32_t offset = 0, uint32_t length = 0);
};

#endif
