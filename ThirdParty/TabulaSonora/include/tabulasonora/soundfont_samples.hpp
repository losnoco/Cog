#pragma once

#include "tabulasonora/drum_kit_table.hpp"
#include "tabulasonora/patch_directory.hpp"
#include "tabulasonora/sampler.hpp"
#include "tabulasonora/wave_descriptor.hpp"

#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace ts::sf2 {

/// How a wave's ROM data is turned into a run of PCM an SF2 sampler can play.
///
/// SF2 offers no-loop, loop and loop-until-release, and nothing else. Two of the ROM's four sampler
/// variants therefore have to be rewritten into sample data rather than described by a generator.
enum class Bake {
    /// Plays once and stops. Emitted as-is with no loop.
    one_shot,
    /// Forward loop. Emitted as-is with the loop points carried across.
    forward_loop,
    /// Plays backwards. The samples are turned round, so it becomes an ordinary one-shot.
    reverse,
    /// Bidirectional. One full round trip of the engine's own traversal is emitted as a forward
    /// loop; see `SampleSet` for why that is exact.
    ping_pong,
};

/// One run of PCM in the pooled sample data, and everything a `shdr` record needs.
struct SampleRun {
    /// The wave number this run was built from. Other waves may point at the same run.
    int wave = 0;
    /// Which rewrite produced it.
    Bake bake = Bake::one_shot;

    /// ROM provenance, so a run can be traced back without re-deriving it.
    int region = 0;
    int rom_start = 0;
    int rom_end = 0;

    /// First sample of this run within the pool.
    std::int64_t pool_offset = 0;
    /// Length of the run in samples.
    int length = 0;

    /// Loop bounds relative to `pool_offset`. Equal when the run does not loop.
    int loop_start = 0;
    int loop_end = 0;

    /// MIDI key at which the run plays untransposed.
    int root_key = 0;
    /// Correction from the descriptor's two fine-tune words, in cents.
    ///
    /// The descriptor tunes in milli-semitones and carries two words; the effective root is
    /// `WaveDescriptor::native_milli_semitones`. `root_key` takes the whole-semitone part and this
    /// takes the remainder, which is what `shdr`'s byte-wide correction can express.
    int fine_cents = 0;

    [[nodiscard]] bool loops() const noexcept { return loop_end > loop_start; }
};

/// The deduplicated set of sample runs the export needs, and the PCM pool holding them.
///
/// Three things about the deduplication are measured rather than assumed, and all three would be
/// got wrong by the obvious reading:
///
///  * **Waves are keyed on their exact ROM extent, not on a merged one.** The codec integrates from
///    zero at the exponent-block boundary below each wave's own data start, so two waves covering
///    overlapping ROM decode to PCM whose *shape* is identical and whose *level* differs by a DC
///    constant — measured at up to 0.217 on a ±1 signal across 408 overlapping pairs, with the
///    shape error exactly zero on every one of them. SF2 has no generator that can add a constant,
///    so a merged run cannot be right for both waves. Where the constant happens to be zero the
///    runs are genuinely interchangeable, which is 351 of those 408 pairs, and `share_identical`
///    recovers exactly those.
///
///  * **Ping-pong is periodic, so it bakes losslessly.** The traversal loops in the delta domain —
///    the index walks up, back down and up again while the predictor keeps *adding* deltas — so the
///    descending leg is the wave inverted and time-reversed, not its mirror. Mirroring the PCM is
///    wrong on all 612 ping-pong waves. But one round trip *is* a period of the output, provided it
///    is measured as `2 * (data_end - loop_start) + 2`: the index is unchanged when a leg flips, so
///    the turnaround sample is integrated twice at each end. At that period all 612 repeat to
///    within 1e-6 with no measurable drift. Two samples short, the pass-to-pass difference is
///    0.09–0.41, which is not a loop at all.
///
///  * **A reverse wave is already turned round by the decoder.** `DecodedWave::reversed` says the
///    samples are in playback order, so it needs no further work and becomes a plain one-shot.
class SampleSet {
public:
    /// Builds the set from the waves the export actually reaches.
    ///
    /// `waves` is the census of referenced wave numbers — see `census`. Waves the ROM cannot decode
    /// are skipped rather than emitted empty, and are reported by `skipped`.
    [[nodiscard]] static SampleSet build(const PatchDirectory& directory,
                                         Sampler& sampler,
                                         std::span<const int> waves,
                                         bool share_identical = true);

    /// Every wave number the export can reach: the mapped melodic tones and the drum kits.
    ///
    /// The five tone maps overlap heavily, so this is a set rather than a list — 3,982 preset slots
    /// across the maps resolve to far fewer distinct tones, and those tones share multisamples in
    /// turn.
    [[nodiscard]] static std::vector<int> census(const PatchDirectory& directory,
                                                 const DrumKitTable& kits);

    /// The pooled PCM, in the order the runs index it.
    [[nodiscard]] std::span<const float> pool() const noexcept { return pool_; }

    /// The runs, in pool order.
    [[nodiscard]] std::span<const SampleRun> runs() const noexcept { return runs_; }

    /// The run a wave number plays, or -1 when the wave could not be decoded.
    [[nodiscard]] int run_for_wave(int wave) const noexcept;

    /// Wave numbers that were asked for but could not be decoded.
    [[nodiscard]] std::span<const int> skipped() const noexcept { return skipped_; }

    /// How many waves resolved to a run some other wave had already produced.
    [[nodiscard]] int shared_count() const noexcept { return shared_count_; }

private:
    std::vector<float> pool_;
    std::vector<SampleRun> runs_;
    std::map<int, int> wave_to_run_;
    std::vector<int> skipped_;
    int shared_count_ = 0;
};

/// Splits a descriptor's effective root into a whole key and a cents remainder.
///
/// The descriptor tunes in milli-semitones against both fine-tune words. Rounding the key and
/// carrying the remainder in cents is what `shdr` can express; dropping the remainder detunes a
/// wave by up to half a semitone.
struct RootTuning {
    int key = 0;
    int cents = 0;
};

[[nodiscard]] RootTuning root_tuning(const WaveDescriptor& descriptor) noexcept;

/// The bake a descriptor calls for.
[[nodiscard]] Bake bake_for(const WaveDescriptor& descriptor, const DecodedWave& wave) noexcept;

/// Reads a run the way an SF2 sampler would: straight through, then wrapping within the loop.
///
/// Exposed so a bake can be checked against the engine's own playback rather than trusted.
[[nodiscard]] std::vector<float>
read_run(const SampleSet& set, const SampleRun& run, int sample_count);

/// The worst absolute error between a baked run and the engine's traversal of the same wave.
///
/// Returns a negative value when the wave cannot be decoded. A correct bake returns 0 — these are
/// the same samples in a different order, not a resampling, so there is no tolerance to allow for.
[[nodiscard]] double verify_run(const SampleSet& set,
                                const SampleRun& run,
                                const PatchDirectory& directory,
                                Sampler& sampler,
                                int sample_count);

/// The period of a ping-pong traversal, in samples.
///
/// `2 * (data_end - loop_start) + 2`. The `+ 2` is the two turnaround samples, whose deltas are
/// integrated twice because the index does not move when a leg flips.
[[nodiscard]] constexpr int ping_pong_period(int loop_start, int data_end) noexcept
{
    return (2 * (data_end - loop_start)) + 2;
}

} // namespace ts::sf2
