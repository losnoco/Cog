#ifndef TWOSF2WAV_INTERPOLATOR_H
#define TWOSF2WAV_INTERPOLATOR_H

#include <vector>
#include <cstdint>

class IInterpolator
{
public:
  virtual ~IInterpolator() {}

  virtual int32_t interpolate(const std::vector<int32_t>& data, double time) const = 0;

  static IInterpolator* allInterpolators[4];
};

class LinearInterpolator : public IInterpolator
{
public:
  virtual int32_t interpolate(const std::vector<int32_t>& data, double time) const;
};

class CosineInterpolator : public IInterpolator
{
public:
  CosineInterpolator();

  virtual int32_t interpolate(const std::vector<int32_t>& data, double time) const;

private:
  double lut[8192];
};

class SharpIInterpolator : public IInterpolator
{
public:
  virtual int32_t interpolate(const std::vector<int32_t>& data, double time) const;
};


#endif
