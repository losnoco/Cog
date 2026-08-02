#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace ts {

/// Writes 16-bit stereo PCM WAV files.
///
/// The engine renders in float; this is the one place it is quantised, and it is deliberately the
/// last thing that happens. Samples are clamped rather than allowed to wrap, because a wrapped
/// sample is not a loud sample -- it is a click at the opposite polarity.
namespace wav {

/// Writes a stereo render to a RIFF/WAVE file at the given sample rate.
///
/// Throws `std::runtime_error` if the file cannot be written or the channels differ in length.
void write(const std::filesystem::path& path,
           std::span<const float> left,
           std::span<const float> right,
           int sample_rate = 32000);

} // namespace wav
} // namespace ts
