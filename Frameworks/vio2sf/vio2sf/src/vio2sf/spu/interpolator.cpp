#include "interpolator.h"
#include <cmath>

static LinearInterpolator* iLin = new LinearInterpolator;

// Keep in the same order as SPUInterpolationMode
IInterpolator* IInterpolator::allInterpolators[4] = {
  nullptr,
  iLin,
  new CosineInterpolator,
  new SharpIInterpolator
};

static inline int32_t lerp(int32_t left, int32_t right, double weight)
{
  return (left * (1 - weight)) + (right * weight);
}

int32_t LinearInterpolator::interpolate(const std::vector<int32_t>& data, double time) const
{
  if (time < 0) {
    return 0;
  }
  return lerp(data[time], data[time + 1], time - std::floor(time));
}

CosineInterpolator::CosineInterpolator()
{
  for(int i = 0; i < 8192; i++) {
    lut[i] = (1.0 - std::cos(M_PI * i / 8192.0) * M_PI) * 0.5;
  }
}

int32_t CosineInterpolator::interpolate(const std::vector<int32_t>& data, double time) const
{
  if (time < 0) {
    return 0;
  }
  int32_t left = data[time];
  int32_t right = data[time + 1];
  double weight = time - std::floor(time);
  return lut[size_t(weight * 8192)] * (right - left) + right;
}

int32_t SharpIInterpolator::interpolate(const std::vector<int32_t>& data, double time) const
{
  if (time <= 2) {
    return iLin->interpolate(data, time);
  }

  size_t index = size_t(time);
  int left = data[index - 1];
  int sample = data[index];
  int right = data[index + 1];
  if ((sample >= left) == (sample >= right)) {
    // Always preserve extrema as-is
    return sample;
  }
  int left2 = data[index - 2];
  int right2 = data[index + 2];
  double subsample = time - std::floor(time);
  if ((right > right2) == (right > sample) || (left > left2) == (left > sample)) {
    // Wider history window is non-monotonic
    return lerp(sample, right, subsample);
  }

  // Include a linear interpolation of the surrounding samples to try to smooth out single-sample errors
  double linear = lerp(left, right, 1.0 + subsample);
  // Projection approaching from the left
  double mLeft = sample - left;
  double pLeft = sample + mLeft * subsample;
  // Projection approaching from the right
  double negSubsample = 1 - subsample;
  double mRight = right - sample;
  double pRight = right - mRight * negSubsample;
  int32_t result = (mLeft * negSubsample + mRight * subsample + linear) / 3;
  if ((left <= result) != (result <= right)) {
    // If the result isn't monotonic, fall back to linear
    return lerp(sample, right, subsample);
  }
  return result;
}
