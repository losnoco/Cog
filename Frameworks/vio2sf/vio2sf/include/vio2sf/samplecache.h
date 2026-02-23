#ifndef TWOSF2WAV_SAMPLECACHE_H
#define TWOSF2WAV_SAMPLECACHE_H

#include <unordered_map>
#include <vio2sf/sampledata.h>

class SampleCache {
public:
  const SampleData& getSample(vio2sf_state *st, uint32_t baseAddr, uint16_t loopStartWords, uint32_t loopLengthWords, SampleData::Format format);
  void clear();

private:
  std::unordered_map<uint64_t, SampleData> samples;
};

#endif
