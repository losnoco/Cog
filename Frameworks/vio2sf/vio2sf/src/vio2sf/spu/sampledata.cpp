#include <vio2sf/sampledata.h>
#include "adpcmdecoder.h"
#include "interpolator.h"
#include <vio2sf/MMU.h>

SampleData::SampleData()
: std::vector<int32_t>(), baseAddr(0), loopStart(0), loopLength(0), st(0)
{
  // initializers only
}

SampleData::SampleData(vio2sf_state *_st, uint32_t baseAddr, uint16_t loopStart, uint32_t loopLength, Format format)
: std::vector<int32_t>(), baseAddr(baseAddr), loopStart(loopStart), loopLength(loopLength), st(_st)
{
  if (format == Pcm8) {
    loadPcm8();
  } else if (format == Pcm16) {
    loadPcm16();
  } else {
    loadAdpcm();
  }
}

void SampleData::loadPcm8()
{
  loopStart += 3;
  resize(loopStart + (loopLength << 2));
  for (int i = 3; i < loopStart; i++) {
    (*this)[i] = int8_t(_MMU_read08<ARMCPU_ARM7, MMU_AT_DEBUG>(st, baseAddr + i - 3)) << 8;
  }
  uint32_t length = loopStart + loopLength;
  for (int i = loopStart; i < length; i++) {
    (*this)[i] = (*this)[length + i] = int8_t(_MMU_read08<ARMCPU_ARM7, MMU_AT_DEBUG>(st, baseAddr + i - 3)) << 8;
  }
}

void SampleData::loadPcm16()
{
  loopStart >>= 1;
  loopLength >>= 1;
  loopStart += 3;
  resize(loopStart + (loopLength << 2));
  uint32_t addr = baseAddr;
  for (int i = 3; i < loopStart; i++) {
    (*this)[i] = int16_t(_MMU_read16<ARMCPU_ARM7, MMU_AT_DEBUG>(st, addr));
    addr += 2;
  }
  uint32_t length = loopStart + loopLength;
  for (int i = loopStart; i < length; i++) {
    (*this)[i] = (*this)[length + i] = int16_t(_MMU_read16<ARMCPU_ARM7, MMU_AT_DEBUG>(st, addr));
    addr += 2;
  }
}

void SampleData::loadAdpcm()
{
  uint32_t length = loopStart + loopLength;
  loopStart = ((loopStart - 4) << 1) + 11;
  loopLength <<= 1;
  resize(loopStart + (loopLength << 2));
  AdpcmDecoder adpcm(
    int16_t(_MMU_read16<ARMCPU_ARM7, MMU_AT_DEBUG>(st, baseAddr)),
    int16_t(_MMU_read16<ARMCPU_ARM7, MMU_AT_DEBUG>(st, baseAddr + 2))
  );
  int j = 11;
  for (int i = 4; i < length; i++) {
    uint8_t data = uint8_t(_MMU_read08<ARMCPU_ARM7, MMU_AT_DEBUG>(st, baseAddr + i));
    (*this)[j++] = adpcm.getNextSample(uint8_t(data & 0x0f));
    (*this)[j++] = adpcm.getNextSample(uint8_t(data & 0xf0) >> 4);
  }
  uint32_t loopEnd = loopStart + loopLength;
  for (j = loopStart; j < loopEnd; j++) {
    (*this)[j + loopLength] = (*this)[j];
  }
}

int32_t SampleData::sampleAt(double time, IInterpolator* interp) const
{
  if (!baseAddr) {
    return 0;
  }
  if (!interp) {
    return (*this)[uint32_t(time)];
  }
  return interp->interpolate(*this, time);
}
