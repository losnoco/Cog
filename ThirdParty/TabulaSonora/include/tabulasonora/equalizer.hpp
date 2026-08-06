#pragma once

#include "tabulasonora/effect_presets.hpp"

#include <span>

namespace ts {

/// The GS four-band EQ: a low shelf and a high shelf, in series, on a stereo bus.
///
/// Four *bands* in Roland's naming, two in this engine's: `40 02` exposes a frequency and a gain
/// for each of low and high, and the two channels take the same pair. The engine writes each band's
/// three coefficients to a left register and a right register with identical values
/// (`fx_eq_band_preset_apply` sends every word twice, to 0xe6-0xe8 and again to 0xf3-0xf5), so the
/// stereo image is untouched and only the spectrum moves.
///
/// This is not a send. It sits inline on the parts that switch it on with `40 4x 20`, which is why
/// it takes a stereo buffer and rewrites it rather than producing a wet signal to be mixed.
class Equalizer {
public:
    /// The flat setting: `40 02 01` and `40 02 03` centred, both corners at their lower choice.
    static constexpr int flat_gain = 0x40;

    /// Creates an equalizer over a preset set, which must outlive it.
    ///
    /// A set with no EQ coefficients — one parsed from a pinned `presets.json`, which carries none
    /// — makes every `process` a no-op rather than a flat filter. Those are the same thing in
    /// theory and not in practice: running audio through a filter whose coefficients are all
    /// default would be silently wrong, and doing nothing is visibly nothing.
    explicit Equalizer(const EffectPresets& presets) noexcept;

    /// Whether this equalizer can actually filter.
    [[nodiscard]] bool available() const noexcept { return presets_ != nullptr; }

    /// Whether the current settings would change the signal.
    ///
    /// Both gains flat means both shelves are exactly unity, so the whole stage can be skipped
    /// without any difference in output — the identity is algebraic, not an approximation.
    [[nodiscard]] bool is_flat() const noexcept
    {
        return low_gain_ == flat_gain && high_gain_ == flat_gain;
    }

    /// Sets one of the four parameters, as `40 02 00`–`03` names them.
    ///
    /// Values outside the engine's accepted ranges are ignored, as the engine ignores them: the
    /// frequency selectors take 0 or 1, and the gains take 0x34–0x4C.
    void set_low_frequency(int value) noexcept;
    void set_low_gain(int value) noexcept;
    void set_high_frequency(int value) noexcept;
    void set_high_gain(int value) noexcept;

    /// Returns the block state to power-on: flat, with both corners at their lower choice.
    void reset() noexcept;

    /// Clears the filter memory without changing the settings.
    void clear() noexcept;

    /// Filters a stereo block in place.
    void process(std::span<float> left, std::span<float> right) noexcept;

private:
    /// One shelf's two-sample state, per channel.
    struct State {
        double x1 = 0.0;
        double y1 = 0.0;
    };

    static double step(const EqBand& band, State& state, double input) noexcept;

    void refresh() noexcept;

    const EffectPresets* presets_ = nullptr;

    int low_frequency_ = 0;
    int low_gain_ = flat_gain;
    int high_frequency_ = 0;
    int high_gain_ = flat_gain;

    EqBand low_{};
    EqBand high_{};

    State low_left_;
    State low_right_;
    State high_left_;
    State high_right_;
};

} // namespace ts
