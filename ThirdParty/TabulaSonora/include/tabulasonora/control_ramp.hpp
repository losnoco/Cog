#pragma once

#include <algorithm>
#include <cstdint>
#include <span>

namespace ts {

/// The engine's anti-zipper control smoother — `voice_ctrl_ramp_b` @`18005e990`.
///
/// A part's volume is not handed to the mixer the moment CC#7 moves. Every voice carries a second
/// per-sample gain buffer beside its TVA envelope, and `render_block` fills it through this ramp
/// from the target `voice_volume_apply` computes. Without it a volume slide is a staircase of
/// instantaneous gain jumps, one per event, each a waveform discontinuity — audible as a click on
/// any sustained part. DOOM's `D_DOOM.MUS` is the case that exposes it: two pad channels crossfaded
/// by nothing but CC#7, restepped every seven ticks (~50 ms) for four minutes.
///
/// **The approach is exponential, not linear.** Each update closes a fixed *fraction* of the
/// remaining distance:
///
/// ```
/// error = (int16)((target << 10) - accumulator) >> 13)
/// step  = error * rate, forced to at least one accumulator LSB either way
/// ```
///
/// so with the volume path's `rate` of `0xCC` an update closes 204/8192 — 2.49% — of what is left,
/// and updates land every eight samples. That is a time constant of 317 samples, 9.9 ms at 32 kHz,
/// and a settle around 45 ms. The `minimum_step` floor is what guarantees arrival: a proportional
/// step alone would stall once the error shifted down to zero.
///
/// Measured against the oracle rather than assumed. `scdec volramp` reads the engine's own gain
/// buffer through a CC#7 127->0 step and it falls by a *constant ratio* of 0.90403 per 32-sample
/// call — which is `(1 - 204/8192)^4`, four updates a call. A constant ratio is the signature of a
/// step rescaled from the live error; a fixed decrement would fall linearly. `voice_ctrl_ramp_a`,
/// the envelope's smoother, is the fixed-decrement one.
///
/// This is the sibling of `voice_ctrl_ramp_a`, which smooths the *envelope* gain and is a plain
/// linear ramp. The two are not interchangeable and the engine runs both, one per gain buffer.
///
/// **The rest state dithers, and that is the engine's own behaviour.** The ramp is stepped
/// unconditionally — there is no early-out once the target is reached — so at rest the error is
/// zero, `minimum_step` pushes the accumulator up one LSB, the next update's error is -1 and pushes
/// it back. The accumulator alternates over a 1024-wide span that the gain's own `>> 13` swallows
/// for seven target words in eight. The oracle's own rest value creeps up about 1/16384 per 64
/// samples before a retarget truncates it back, which this does not reproduce exactly; it is 0.07%
/// of level and the trace cannot say what resets it.
class ControlRamp {
public:
    /// `voice_volume_apply`'s rate word for the part-volume path, hard-coded at the call site.
    static constexpr int volume_rate_word = 0xCC;

    /// The accumulator carries ten bits below the value it tracks.
    static constexpr int accumulator_shift = 10;

    /// The error is taken thirteen bits down before it meets the rate.
    static constexpr int error_shift = 13;

    /// The smallest move an update may make, in accumulator units.
    static constexpr int minimum_step = 0x400;

    /// Output samples one update covers, before the ZOH mask divides it further.
    ///
    /// `render_block` calls the ramp once per eight-sample sub-chunk and the value is held flat
    /// across them, so the smoother's clock is the sub-chunk and not the sample. **This is measured,
    /// not read off the tables**: the rate word's divider bits select mask zero, which taken alone
    /// would mean one update a sample and a glide eight times too fast. Against the oracle, a single
    /// CC#7 127->0 step decays with a time constant of 343 samples; one update a sample predicts 40.
    /// The eight-sample sub-chunk is also visible directly in the gain buffer, which holds eight
    /// copies of one value per call.
    static constexpr int samples_per_update = 8;

    /// The gain is read out of the accumulator thirteen bits down, in 1/16384ths.
    static constexpr int gain_shift = 13;
    static constexpr double gain_scale = 0x1p-14;

    /// What the engine emits for a zero value: not quite silence, and deliberately so.
    static constexpr float floor_gain = 1e-05F;

    /// The ramp's target for a volume word — `CONCAT44(0xcc, voice_volume_apply() << 2)`.
    ///
    /// The two low bits the shift adds are not decoration: the gain is read back out of the
    /// accumulator `>> 13` while the accumulator carries the value `<< 10`, so a target that
    /// skipped the shift would render every part exactly two stops down.
    [[nodiscard]] static constexpr int target_of(int volume_word) noexcept
    {
        return volume_word << 2;
    }

    /// The zero-order-hold mask a rate word selects, through the two tables that pick it.
    ///
    /// `g_ramp_flagword` turns the rate word's bits 12-13 into the flag bits 3-4 that then index
    /// `g_ramp_divider`. The volume path's `0xCC` lands on index zero, so the mask is zero and adds
    /// no hold of its own — the eight-sample cadence in `samples_per_update` is the whole of it.
    [[nodiscard]] static unsigned mask_of(int rate_word,
                                          std::span<const std::uint8_t> flagword,
                                          std::span<const std::uint8_t> divider) noexcept
    {
        const auto selector = static_cast<std::size_t>((rate_word >> 12) & 3);
        const unsigned flags = selector * 4 < flagword.size() ? flagword[selector * 4] : 0U;
        const auto index = static_cast<std::size_t>((flags >> 3) & 3);
        return index < divider.size() ? divider[index] : 0U;
    }

    /// Points the ramp at a new target — `voice_ramp_target_aux` @`18008a5e0`.
    ///
    /// Called once a control tick, not once a block. It reseeds the accumulator from the tracked
    /// value, which quantises away whatever sub-LSB residue the last tick's stepping left; that
    /// truncation is part of the behaviour and is why this is driven off the tick and not the block.
    void retarget(int target, int rate_word, unsigned mask) noexcept
    {
        rate_ = rate_word & 0xFFF;
        mask_ = mask;
        counter_ = 0;
        phase_ = 0;
        target_ = target;
        accumulator_ = current_ << accumulator_shift;
        held_ = decode(current_, accumulator_);
        active_ = current_ != target_;
    }

    /// Seeds the ramp at a value, with no glide — `tvf_env_prep` writes the same value to both the
    /// source and the target slot, so a voice starting mid-slide begins at the level already
    /// reached rather than sweeping up to it.
    void seed(int value, int rate_word, unsigned mask) noexcept
    {
        current_ = value;
        retarget(value, rate_word, mask);
    }

    /// Advances one sample and returns the gain it lands on.
    [[nodiscard]] float step() noexcept
    {
        // Arrived: the ramp deactivates and the last gain stands until something retargets it.
        //
        // Not cosmetic. `minimum_step` moves the accumulator whether or not there is any error
        // left, so a ramp that kept stepping at rest would walk straight past its target and out
        // the far side -- to a *negative* gain, one sample in two, phase-inverted at -84 dB. The
        // oracle's fade lands on 1e-05 and holds it flat for as long as the trace runs.
        if (!active_) {
            return held_;
        }

        // Held flat within a sub-chunk; the ramp's own clock only ticks at the boundary.
        if (++phase_ < samples_per_update) {
            return held_;
        }
        phase_ = 0;

        ++counter_;
        if ((static_cast<unsigned>(counter_) & mask_) != 0) {
            return held_;
        }

        const auto error = static_cast<std::int16_t>(
            ((target_ << accumulator_shift) - accumulator_) >> error_shift);
        int step = static_cast<int>(error) * rate_;
        if (step < 0) {
            step = std::min(step, -minimum_step);
        } else {
            step = std::max(step, minimum_step);
        }

        accumulator_ += step;
        current_ = accumulator_ >> accumulator_shift;
        active_ = current_ != target_;
        held_ = decode(current_, accumulator_);
        return held_;
    }

    /// Fills a block's worth of per-sample gains.
    void fill(std::span<double> gains) noexcept
    {
        for (double& gain : gains) {
            gain = static_cast<double>(step());
        }
    }

    /// The value the ramp currently tracks, in the target's own units.
    [[nodiscard]] int current() const noexcept { return current_; }

private:
    /// The engine reads the gain out of the *accumulator*, not the tracked value, and narrows to
    /// `int16` on the way — so the ten bits the accumulator carries below the value are not simply
    /// discarded, three of them survive into the gain.
    [[nodiscard]] static float decode(int current, int accumulator) noexcept
    {
        if (current == 0) {
            return floor_gain;
        }
        const auto narrowed = static_cast<std::int16_t>(accumulator >> gain_shift);
        return static_cast<float>(static_cast<double>(narrowed) * gain_scale);
    }

    int rate_ = 0;
    unsigned mask_ = 0;
    int phase_ = 0;
    int counter_ = 0;
    int current_ = 0;
    int target_ = 0;
    int accumulator_ = 0;
    float held_ = 0.0F;
    bool active_ = false;
};

/// The send matrix's coefficient smoother — the loop `fx_process_block` @`18008c2c0` opens with,
/// before it touches any audio.
///
/// Sixteen coefficients, each an `int16` current chasing an `int16` target, and the GS effect levels
/// are among them: `40 01 33` is coefficient 6 and `40 01 3a` is coefficient 12. Per 32-sample block
/// each is stepped sixteen times and the sixteen values written out for the block to use, so like
/// the part fader this is a per-sample gain rather than a scalar held across the block.
///
/// A third distinct shape, and worth not confusing with the other two. It is an exponential approach
/// like `ControlRamp`, but on a coefficient rather than a gain word, with its own rate and no
/// zero-order hold: each step closes `0x300/0x10000` — 1.172% — of what is left.
///
/// **Arrival comes free from the shift.** There is no `minimum_step` here. Both branches arrange for
/// the *negative* of the error to be shifted, and an arithmetic shift right rounds toward negative
/// infinity, so the step's magnitude is rounded up rather than down and can never truncate to zero.
/// Taking the obvious `(target - current) * rate >> shift` in both directions instead stalls 85
/// short of a full-scale target, which the oracle does not do.
///
/// Verified against the engine block for block: `40 01 33` driven 0 → 127 reproduces 5594, 10229,
/// 14069, 17248, … and settles on 32512 exactly, with no mismatch in 123 blocks.
class MatrixRamp {
public:
    /// Sub-steps the coefficient takes in one render block.
    static constexpr int steps_per_block = 16;

    /// The fraction of the remaining error a step closes, as a shift pair.
    static constexpr int rate = 0x300;
    static constexpr int rate_shift = 16;

    /// A 0-127 level's coefficient target.
    static constexpr int level_shift = 8;

    /// The coefficient decodes to a gain in 1/16384ths, so 0x40 — the power-on level — is unity.
    static constexpr double gain_scale = 0x1p-14;

    [[nodiscard]] static constexpr int target_of(int level) noexcept
    {
        return level << level_shift;
    }

    /// Fills a block's worth of per-sample gains, advancing the coefficient toward `level`.
    ///
    /// The first call lands on the target: a stream that never edits a level renders as though this
    /// did not exist, rather than gliding up from silence at the first block.
    void fill(int level, std::span<double> gains) noexcept
    {
        const int target = target_of(level);
        if (!seeded_) {
            current_ = target;
            seeded_ = true;
        }

        const std::size_t per = gains.size() / steps_per_block;
        std::size_t at = 0;
        for (int step_index = 0; step_index < steps_per_block; ++step_index) {
            // Whichever way it is going, the *negative* side is what gets shifted.
            const int step = target < current_
                                 ? ((target - current_) * rate) >> rate_shift
                                 : -(((current_ - target) * rate) >> rate_shift);
            current_ = static_cast<std::int16_t>(current_ + step);

            const double gain = static_cast<double>(current_) * gain_scale;
            for (std::size_t k = 0; k < per && at < gains.size(); ++k) {
                gains[at++] = gain;
            }
        }
        while (at < gains.size()) {
            gains[at++] = static_cast<double>(current_) * gain_scale;
        }
    }

    /// The coefficient in force, in its own units.
    [[nodiscard]] int current() const noexcept { return current_; }

private:
    int current_ = 0;
    bool seeded_ = false;
};

/// The engine's anti-zipper smoother for the filter's frequency coefficient — `voice_ctrl_ramp_c`
/// @`18005d8d0`.
///
/// The third kind of ramp in this file, and the third law. `ControlRamp` is exponential on a live
/// error; this one is **linear**: the step is computed once when the target is set and then held,
/// so the coefficient walks at a constant rate until it arrives. That is what the oracle trace
/// shows — stepping CC#74 under a held note, `g_svf_f_coef` climbs in *even* increments of
/// 0.127991 every 32 samples rather than in a decaying curve.
///
/// ```
/// step    = (target − current) * rate >> 13,  at retarget, then held
/// current += step, once per eight samples, clamped at the target
/// f       = (current >> 3) / 16384
/// ```
///
/// **The rate is one constant, not a per-partial word.** That mattered: an earlier port of these
/// ramps was reverted partly because the rate lived in a voice field with no path back to any
/// tone-table byte, so one guessed index had to stand for every partial. Read off the live ramp
/// slot in `g_voice_ramp_cutoff` across six tones spanning five programs and four bank LSBs, the
/// rate word is `0xCC` and the divider index zero every time. `0xCC` is the same 204 the part
/// fader uses, and the eight-sample cadence is that ramp's too — the two differ in their law, not
/// in their clock.
///
/// **Why it is audible.** Without it the coefficient jumps once per 320-sample control block. On a
/// gentle filter that is a mild zipper, which is why it went unnoticed; at high resonance it is a
/// different sound entirely, because each step relocates the pole of a ringing filter and
/// re-excites it. `MM6_-_MrX2010XG.mid` drives CC#71 to 100 on its two saw channels, pinning the
/// resonance byte on its floor of 4 — a Q of 16 — and then sweeps CC#74 3288 times per channel.
/// Stepped, that is heard as the *resonance* sweeping rather than the cutoff.
class CoefficientRamp {
public:
    /// The rate the engine closes the distance at, measured constant across tones.
    static constexpr int rate_word = 0xCC;

    /// The rate meets the distance thirteen bits down.
    static constexpr int rate_shift = 13;

    /// Samples between updates.
    static constexpr int samples_per_update = 8;

    /// The accumulator carries three bits below the coefficient's own.
    static constexpr int decode_shift = 3;

    /// The decoded coefficient is a 16-bit quantity over 2^14.
    static constexpr int decode_max = 0x7FFF;
    static constexpr double decode_scale = 1.0 / 16384.0;

    /// What a zero accumulator decodes to. The engine spells this exception out rather than letting
    /// it fall to 0.0, so a fully closed filter still has a coefficient the state matrix can use.
    static constexpr double floor_value = 1e-05;

    /// Turns an accumulator into the coefficient the filter runs on.
    [[nodiscard]] static constexpr double decode(int accumulator) noexcept
    {
        if (accumulator == 0) {
            return floor_value;
        }
        return static_cast<double>(std::min(decode_max, accumulator >> decode_shift)) * decode_scale;
    }

    /// Starts the ramp at an accumulator with no glide.
    ///
    /// A voice that begins part-way through a sweep starts where the sweep has got to, rather than
    /// sliding up to it from wherever the previous voice in the slot left off.
    void seed(int accumulator) noexcept
    {
        current_ = accumulator;
        target_ = accumulator;
        step_ = 0;
        phase_ = 0;
        active_ = false;
        seeded_ = true;
    }

    [[nodiscard]] bool is_seeded() const noexcept { return seeded_; }

    /// Points the ramp at a new accumulator and fixes the step it will walk at.
    void retarget(int accumulator) noexcept
    {
        if (!seeded_) {
            seed(accumulator);
            return;
        }
        if (accumulator == target_) {
            return;
        }

        target_ = accumulator;
        step_ = static_cast<int>(
            (static_cast<std::int64_t>(target_ - current_) * rate_word) >> rate_shift);

        // Only the climb needs a floor. Descending, the arithmetic shift of a negative product
        // already rounds away from zero, so the step can never stall at nothing; climbing, a small
        // enough distance truncates to zero and would stall forever. The engine forces the one and
        // not the other, and copying only half of that is what makes it exact.
        if (step_ == 0 && current_ < target_) {
            step_ = 1;
        }
        active_ = current_ != target_;
    }

    /// Advances one sample and returns the coefficient now in force.
    [[nodiscard]] double step() noexcept
    {
        if (!active_) {
            return decode(current_);
        }
        if (++phase_ < samples_per_update) {
            return decode(current_);
        }
        phase_ = 0;

        const int moved = current_ + step_;
        current_ = step_ < 1 ? std::max(moved, target_) : std::min(moved, target_);
        active_ = current_ != target_;
        return decode(current_);
    }

    /// The coefficient in force, without advancing.
    [[nodiscard]] double value() const noexcept { return decode(current_); }

    /// The accumulator in force. Diagnostic — the oracle traces compare against this.
    [[nodiscard]] int current() const noexcept { return current_; }

    /// Whether a glide is still running.
    [[nodiscard]] bool is_active() const noexcept { return active_; }

private:
    int current_ = 0;
    int target_ = 0;
    int step_ = 0;
    int phase_ = 0;
    bool active_ = false;
    bool seeded_ = false;
};

/// The engine's anti-zipper smoother for the filter's damping coefficient — `voice_ctrl_ramp_d`
/// @`18005dbf0`.
///
/// The fourth ramp, and it shares its *law* with `ControlRamp` rather than with its neighbour
/// `CoefficientRamp`: exponential on a live error, `>> 13`, a `0x400` minimum step either way, and
/// an accumulator ten bits below the value. Only the rate and the decode differ. So the filter's
/// two coefficients are smoothed by two different laws — `f` linearly, `q` exponentially — which is
/// why they are two classes and not one parameterised on a rate.
///
/// The rate is `0x100`, read off the live ramp slot the same way `CoefficientRamp`'s was. It
/// predicts the trace: `(1 − 256/8192)^4` is 0.8804, against a measured 0.880 per 32 samples,
/// confirming the same one-update-per-eight-samples clock the other three run on.
///
/// `q` only moves when the resonance byte does, so unlike `f` this is silent on most music — CC#71
/// is usually set once before a note and left. It is here because the engine runs it, and because
/// the stability ceiling `f` is clamped against is indexed by the `q` *actually reached* rather
/// than the one aimed at, which only a ramp can express.
class DampingRamp {
public:
    /// Measured on the live ramp slot, and the same for every tone tried.
    static constexpr int rate_word = 0x100;

    static constexpr int accumulator_shift = 10;
    static constexpr int error_shift = 13;
    static constexpr int minimum_step = 0x400;
    static constexpr int samples_per_update = 8;

    /// The value encoding is `CoefficientRamp`'s, so the two decode alike.
    [[nodiscard]] static constexpr double decode(int current) noexcept
    {
        return CoefficientRamp::decode(current);
    }

    /// Turns a coefficient into the integer the ramp carries.
    [[nodiscard]] static int encode(double value) noexcept
    {
        return static_cast<int>(value * 16384.0) << CoefficientRamp::decode_shift;
    }

    void seed(int current) noexcept
    {
        current_ = current;
        target_ = current;
        accumulator_ = current << accumulator_shift;
        phase_ = 0;
        active_ = false;
        seeded_ = true;
    }

    [[nodiscard]] bool is_seeded() const noexcept { return seeded_; }

    void retarget(int current) noexcept
    {
        if (!seeded_) {
            seed(current);
            return;
        }
        target_ = current;
        active_ = current_ != target_;
    }

    /// Advances one sample and returns the coefficient now in force.
    [[nodiscard]] double step() noexcept
    {
        if (!active_) {
            return decode(current_);
        }
        if (++phase_ < samples_per_update) {
            return decode(current_);
        }
        phase_ = 0;

        const auto error = static_cast<std::int16_t>(
            ((static_cast<std::int64_t>(target_) << accumulator_shift) - accumulator_)
            >> error_shift);
        int step = static_cast<int>(error) * rate_word;
        if (step < 0) {
            step = std::min(step, -minimum_step);
        } else {
            step = std::max(step, minimum_step);
        }

        accumulator_ += step;
        current_ = static_cast<int>(accumulator_ >> accumulator_shift);
        active_ = current_ != target_;
        return decode(current_);
    }

    [[nodiscard]] double value() const noexcept { return decode(current_); }
    [[nodiscard]] int current() const noexcept { return current_; }
    [[nodiscard]] bool is_active() const noexcept { return active_; }

private:
    int current_ = 0;
    int target_ = 0;
    std::int64_t accumulator_ = 0;
    int phase_ = 0;
    bool active_ = false;
    bool seeded_ = false;
};

} // namespace ts
