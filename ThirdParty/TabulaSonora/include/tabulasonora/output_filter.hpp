#pragma once

#include <cstddef>
#include <span>

namespace ts {

/// The module's output stage — `tg_output_filter` @`18008aca0`.
///
/// `TG_Process` runs this over every 32-sample chunk on its way out, and it runs **whether or not
/// the host rate matches the engine's**: at 32 kHz the ratio it carries is exactly 1 and it still
/// filters. That is worth knowing because it sits in every latency measured between this engine and
/// the module, and was the first suspect for the 20 samples left over once the event pipeline
/// accounted for 128 of them.
///
/// The shape is a half-sample allpass feeding a linear interpolator:
///
/// ```
/// mid = allpass(x)                       // one pole, one zero, k = 1/3
/// out = frac < 0.5 ? lerp(prev, mid, 2 * frac)
///                  : lerp(mid, next, 2 * frac - 1)
/// ```
///
/// So the allpass supplies the half-way point between two input samples and the interpolator picks
/// a side of it. The coefficient is `1/3` at every host rate tried -- 22.05, 32, 44.1 and 48 kHz
/// all read it back unchanged -- so it is a constant of the filter rather than a function of the
/// conversion; only the ratio moves, and it is `32000 / host`.
///
/// **At 1:1 this reduces to a one-sample delay.** The phase accumulator sits at zero, so the
/// interpolation weight is zero, the output is the previous input, and the allpass contributes
/// nothing to what is heard. That is not a reason to leave it out -- it is a real sample of latency
/// against the oracle, and the filter stops being a plain delay the moment the ratio is not one.
class OutputFilter {
public:
    /// The allpass coefficient, constant across host rates.
    static constexpr double allpass_coefficient = 1.0 / 3.0;

    /// The engine's own rate; the ratio is this over the host's.
    static constexpr int engine_rate = 32000;

    void reset() noexcept
    {
        state_left_ = 0.0;
        state_right_ = 0.0;
        previous_left_ = 0.0;
        previous_right_ = 0.0;
        phase_ = 0.0;
    }

    /// Sets the conversion ratio from the host's sample rate.
    void set_host_rate(int host_rate) noexcept
    {
        ratio_ = host_rate > 0 ? static_cast<double>(engine_rate) / host_rate : 1.0;
    }

    [[nodiscard]] double ratio() const noexcept { return ratio_; }

    /// Passes one stereo sample through, returning the pair the host receives.
    ///
    /// One in, one out: at the engine's own rate the ratio is one, which is the only case this
    /// engine runs in — it renders at 32 kHz and leaves any real conversion to whatever consumes
    /// it. A ratio other than one makes the output count differ from the input's, and the render
    /// path would have to carry that difference before this could serve it.
    [[nodiscard]] std::pair<float, float> process(float left, float right) noexcept
    {
        const double in_left = static_cast<double>(left);
        const double in_right = static_cast<double>(right);

        // The allpass, per channel: the state takes the input less the fed-back state, and the
        // output leads with the coefficient. Half a sample of delay, which is the midpoint the
        // interpolation below is taken against.
        const double held_left = state_left_;
        const double held_right = state_right_;
        state_left_ = in_left - (allpass_coefficient * held_left);
        state_right_ = in_right - (allpass_coefficient * held_right);
        const double mid_left = (state_left_ * allpass_coefficient) + held_left;
        const double mid_right = (state_right_ * allpass_coefficient) + held_right;

        double out_left = 0.0;
        double out_right = 0.0;
        if (phase_ >= 0.5) {
            const double t = (phase_ + phase_) - 1.0;
            out_left = ((in_left - mid_left) * t) + mid_left;
            out_right = ((in_right - mid_right) * t) + mid_right;
        } else {
            const double t = phase_ + phase_;
            out_left = ((mid_left - previous_left_) * t) + previous_left_;
            out_right = ((mid_right - previous_right_) * t) + previous_right_;
        }

        previous_left_ = in_left;
        previous_right_ = in_right;

        phase_ += ratio_;
        while (phase_ >= 1.0) {
            phase_ -= 1.0;
        }

        return {static_cast<float>(out_left), static_cast<float>(out_right)};
    }

private:
    double ratio_ = 1.0;
    double state_left_ = 0.0;
    double state_right_ = 0.0;
    double previous_left_ = 0.0;
    double previous_right_ = 0.0;
    double phase_ = 0.0;
};

} // namespace ts
