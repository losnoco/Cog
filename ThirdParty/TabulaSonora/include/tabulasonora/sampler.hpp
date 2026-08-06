#pragma once

#include "tabulasonora/interpolator.hpp"
#include "tabulasonora/wave_descriptor.hpp"
#include "tabulasonora/wave_rom.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace ts {

/// Which sampler variant plays a wave.
enum class SamplerMode {
    /// Plays once and stops. The predictor is zeroed at the end.
    one_shot,
    /// Forward loop. Rewinds the delta index and keeps the predictor.
    loop,
    /// Bidirectional. Walks up to the end, turns around, and walks back to the loop point.
    ping_pong,
};

/// A wave decoded from the ROM, with everything playback needs.
struct DecodedWave {
    /// The decoded samples.
    std::vector<float> samples;
    /// The shifted per-sample deltas, needed to rebuild a ping-pong traversal.
    std::vector<std::int32_t> steps;
    /// The predictor's value entering the data start — the integral of the preamble deltas from
    /// the 32-sample exponent-block boundary below it, where the engine's decoder zeroes.
    ///
    /// Every decoded sample already includes it; it is kept so the ping-pong rebuild can
    /// integrate from the same origin. An unaligned wave rides this as a constant DC — the
    /// verification article's "module has DC", measured live at exactly −0.041015625 on
    /// `Crash Cym.1` and −0.139 on the sine-kick zone.
    std::int32_t seed = 0;
    /// Index of the loop point.
    int loop_start = 0;
    /// Number of decodable samples.
    int data_end = 0;
    /// Which sampler variant applies.
    SamplerMode mode = SamplerMode::one_shot;

    /// The buffer a forward loop reads from; empty for the other modes.
    ///
    /// The 4-tap window reaches two samples past the read index, so the buffer must carry the
    /// samples the loop wraps to rather than whatever follows it. It is the wave plus a few wrapped
    /// samples — a fixed size, independent of how long a note holds — so it is built once with the
    /// wave and shared by every voice that plays it.
    std::vector<float> loop_buffer;

    /// Whether `samples` has been turned round because the wave plays backwards.
    ///
    /// Playback does not consult this: the samples are already in the order they are read, so a
    /// reverse wave is an ordinary one-shot from here on. It is recorded because a buffer that no
    /// longer matches the order `steps` is in would otherwise be silently confusing — the steps
    /// stay forward, and nothing that reads them applies to a reverse wave.
    bool reversed = false;

    /// The forward loop's period in samples.
    ///
    /// Inclusive of the data end: one pass plays `loop_start..data_end`. Dropping that last sample
    /// is inaudible on a long loop but detunes a short single-cycle one outright — a 63 against 64
    /// sample period is 27 cents sharp.
    [[nodiscard]] int loop_period() const noexcept { return data_end - loop_start + 1; }

    /// Whether a forward loop is actually in effect.
    [[nodiscard]] bool is_looping() const noexcept
    {
        return mode == SamplerMode::loop && data_end > loop_start + 1 && !loop_buffer.empty();
    }
};

/// Wave playback: decode, then resample at a moving rate while honouring the loop.
///
/// Looping happens in the delta domain — the index rewinds while the predictor carries on — so
/// loops, ping-pong and reverse are all seamless by construction with no crossfade anywhere.
///
/// Decoded waves are cached and never evicted. The working set is bounded by how many distinct
/// waves a song touches, and every voice playing a wave shares one copy, so the cache is also what
/// keeps a dense passage from decoding the same megabyte repeatedly.
///
/// Not thread-safe: `decode` mutates the cache. Both renderers drive it from one thread.
class Sampler {
public:
    /// Creates the sampler over a ROM and resampler, both of which must outlive it.
    Sampler(const WaveRom& rom, const Interpolator& interpolator)
        : rom_(&rom), interpolator_(&interpolator)
    {
    }

    /// Decodes a wave, caching the result.
    ///
    /// Returns `nullptr` when the descriptor has no usable data. The returned pointer stays valid
    /// for the sampler's lifetime.
    [[nodiscard]] const DecodedWave* decode(const WaveDescriptor& descriptor);

    /// Plays a wave at a constant rate.
    [[nodiscard]] std::vector<float>
    play(const DecodedWave& wave, int sample_count, double ratio) const;

    /// Plays a wave at a rate that changes per sample; a shorter input holds its last value.
    ///
    /// Vibrato and pitch envelopes are applied by integrating the instantaneous rate into the read
    /// position, so the first sample always reads position zero.
    [[nodiscard]] std::vector<float>
    play_variable(const DecodedWave& wave, int sample_count, std::span<const double> ratios) const;

private:
    [[nodiscard]] std::unique_ptr<DecodedWave> decode_core(const WaveDescriptor& descriptor) const;
    [[nodiscard]] std::vector<float> play_at(const DecodedWave& wave,
                                             std::span<const double> positions) const;

    /// Builds the buffer a forward loop reads from, with the wrapped samples appended.
    [[nodiscard]] static std::vector<float> build_loop_buffer(const DecodedWave& wave);

    /// Rebuilds a ping-pong traversal by integrating the deltas along the back-and-forth path.
    [[nodiscard]] static std::vector<float> build_ping_pong_buffer(const DecodedWave& wave,
                                                                   int required);

    const WaveRom* rom_;
    const Interpolator* interpolator_;

    /// Keyed on the descriptor fields that determine the bytes: region, data start, physical end.
    /// `unique_ptr` so a rehash cannot move a wave a caller is still holding.
    std::unordered_map<std::uint64_t, std::unique_ptr<DecodedWave>> cache_;
};

} // namespace ts
