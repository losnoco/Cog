#include "samplecache.h"

static inline constexpr uint64_t makeKey(uint32_t base, uint16_t loop, uint32_t length)
{
  return
    // has to be 32-bit aligned, so 2 low bits aren't needed
    // has to fit in the memory map, so high 7 bits aren't needed
    (uint64_t(base & 0x01FFFFFC) >> 2) |
    // loop uses the full 16 bits
    uint64_t(loop << 23) |
    // max length is 21 bits
    (uint64_t(length & 0x1FFFFF) << 39);
}

const SampleData& SampleCache::getSample(vio2sf_state *st, uint32_t baseAddr, uint16_t loopStartWords, uint32_t loopLengthWords, SampleData::Format format)
{
  uint64_t key = makeKey(baseAddr, loopStartWords, loopLengthWords);
  auto iter = samples.find(key);
  if (iter == samples.end()) {
    iter = samples.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(st, baseAddr, loopStartWords << 2, (loopStartWords + loopLengthWords) << 2, format)
    ).first;
  }
  return iter->second;
}

void SampleCache::clear()
{
  samples.clear();
}
