#pragma once

#include "tabulasonora/segment_envelope.hpp"

#include <span>

namespace ts::sf2 {

/// SF2's centibel silence floor, as the target reader defines it.
///
/// 960, not the 1000 that "-100 dB" in the specification suggests. It matters twice over, because
/// both the decay and the release generator are times to travel the *whole* floor and are scaled
/// down by how far the envelope actually goes — so the constant is baked into every duration this
/// emits, not just into the sustain level.
inline constexpr double cb_silence = 960.0;

/// The floor a generator time may not go below, and the ceiling it may not exceed.
///
/// -12000 timecents is about a millisecond, which is SF2's way of saying "instant".
inline constexpr int min_timecents = -12000;
inline constexpr int max_timecents = 8000;

/// An SF2 DAHDSR, in the generator units the file stores.
struct Dahdsr {
    int delay = min_timecents;
    int attack = min_timecents;
    int hold = min_timecents;
    /// Time to fall the full `cb_silence`, not the time to reach `sustain`.
    int decay = min_timecents;
    /// Attenuation below the peak, in centibels.
    int sustain = 0;
    /// Time to fall the full `cb_silence` from the peak, not from `sustain`.
    int release = min_timecents;
};

/// A fitted volume envelope and how badly it fits.
struct VolumeFit {
    Dahdsr envelope;

    /// The gain the SF2 envelope's 1.0 stands for.
    ///
    /// SF2 normalises the volume envelope to unity at its peak, so a partial whose envelope never
    /// reaches full scale has to fold the shortfall into `initialAttenuation` instead. Ignoring it
    /// makes every such partial play too loud by exactly this factor.
    double peak = 1.0;

    /// Worst gain difference against the engine's own envelope, **as a fraction of the peak**.
    ///
    /// The first two milliseconds are excluded. The engine's amplitude attack is instantaneous and
    /// SF2's shortest is about a millisecond, so including that window reports the format's floor
    /// rather than the fit -- every partial with a zero-length first segment would score an error
    /// of exactly 1.0 and nothing else would be visible.
    double worst_error = 0.0;

    /// Root-mean-square difference over the same window, also as a fraction of the peak.
    double rms_error = 0.0;

    /// Segments of the source envelope that actually moved.
    ///
    /// Two or fewer fit; three or four do not, and the extra stages are folded into the decay. This
    /// is the number that says whether a partial is representable at all.
    int moving_segments = 0;
};

/// Converts seconds to SF2 timecents, clamped to the range a generator can hold.
[[nodiscard]] int to_timecents(double seconds) noexcept;

/// Converts SF2 timecents back to seconds.
[[nodiscard]] double from_timecents(int timecents) noexcept;

/// Evaluates a DAHDSR the way the target reader does, returning gain in [0, 1].
///
/// `note_off` is the time the release begins, or a negative value while the note is held. The
/// attack is linear in *gain* and everything after it is linear in centibels, which is SF2's shape
/// and is not the engine's — the engine picks per segment between linear and a fast-approach curve.
/// That mismatch is part of what `VolumeFit::worst_error` measures.
[[nodiscard]] double evaluate(const Dahdsr& envelope,
                              double seconds,
                              double note_off,
                              double release_start_cb) noexcept;

/// Fits a four-segment engine envelope onto SF2's DAHDSR.
///
/// The rule, stated so a change to it shows up in a diff:
///
///  * The **peak** is the largest segment target. SF2 normalises to it, so it is reported rather
///    than encoded, and the caller folds it into the attenuation.
///  * **Attack** is the time from note-on to reaching that peak — every segment up to and
///    including the peak's collapses into one ramp.
///  * **Hold** covers any segments immediately after the peak that stay within half a percent of
///    it, which is what a genuine hold stage looks like in the data.
///  * **Decay** runs from the end of that hold to the last segment's end, and **sustain** is the
///    last segment's target relative to the peak.
///  * **Release** is the release segment, rescaled because SF2's generator measures a fall of the
///    whole `cb_silence` rather than the fall actually taken.
///
/// What this loses is every intermediate stage between the peak and the final target: a
/// three-stage decay becomes one. Half the mapped library has such an envelope, so the loss is the
/// common case rather than the exception, and `worst_error` is how it is kept honest.
[[nodiscard]] VolumeFit fit_volume(const SegmentEnvelope& source,
                                   int sample_rate,
                                   double hold_seconds,
                                   double delay_seconds = 0.0);

/// A fitted modulation envelope, normalised to [0, 1] with its span reported separately.
///
/// The modulation envelope is a different animal from the volume one and the differences are all
/// load-bearing. Its sustain generator is a *decrease* in 0.1% units rather than an attenuation in
/// centibels; it is linear in level throughout rather than in centibels; and its attack is a convex
/// curve rather than a straight line. Its decay and release generators are still scaled — decay by
/// how far it falls, release by the level it starts from — but by level fractions, not by
/// centibels.
struct ModulationFit {
    Dahdsr envelope;

    /// The trajectory's lowest and highest values, in whatever unit the source used.
    ///
    /// The SF2 envelope runs 0 to 1 over this span, so a caller turns `high - low` into the depth
    /// generator and `low` into the static value the depth is added to.
    double low = 0.0;
    double high = 0.0;

    /// Whether the envelope does anything at all.
    bool active = false;

    double worst_error = 0.0;
    double rms_error = 0.0;
    int moving_segments = 0;

    [[nodiscard]] double span() const noexcept { return high - low; }
};

/// Evaluates a modulation envelope the way the target reader does, returning level in [0, 1].
[[nodiscard]] double evaluate_modulation(const Dahdsr& envelope,
                                         double seconds,
                                         double note_off) noexcept;

/// Fits an arbitrary four-segment trajectory onto SF2's modulation envelope.
///
/// `targets` and `ends_seconds` are the segment destinations and their cumulative end times;
/// `start` is the value at note-on, which for the filter envelope is its peak rather than zero —
/// the engine's filter envelope opens instantly and falls, where SF2's rises from nothing. That
/// case comes out as a zero-length attack, which is what it should be.
[[nodiscard]] ModulationFit fit_modulation(std::span<const double> targets,
                                           std::span<const double> ends_seconds,
                                           double start,
                                           double release_target,
                                           double release_seconds,
                                           double hold_seconds);

} // namespace ts::sf2
