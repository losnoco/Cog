#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>

namespace ts {

/// The GS controller assignment matrix (`40 2x`): six sources, each with eleven depths.
///
/// This is the module's modulation routing — how far the mod wheel bends the pitch, how much
/// aftertouch opens the filter, how deep CC1 drives LFO2, and so on. The engine keeps one
/// eleven-byte block per source starting at `part+0x3fc` and hands the block to
/// `modmatrix_apply_linear` or `modmatrix_apply_bipolar` with the controller's current amount; the
/// result is eleven signed modulation values the voice adds to its own.
///
/// The depths are 0x40-centred, so a source assigned nothing anywhere leaves the voice alone.
struct ControlMatrix {
    /// Where a modulation comes from, in the order `40 2x` addresses them.
    enum class Source {
        modulation = 0,   ///< CC#1, `40 2x 00`-`0A`
        bend = 1,         ///< pitch bend, `40 2x 10`-`1A`
        channel_pressure, ///< channel aftertouch, `40 2x 20`-`2A`
        poly_pressure,    ///< polyphonic aftertouch, `40 2x 30`-`3A`
        cc1,              ///< the part's CC1, `40 2x 40`-`4A`
        cc2,              ///< the part's CC2, `40 2x 50`-`5A`
    };

    /// What a modulation reaches. The index is the low nibble of the SysEx address.
    enum class Destination {
        pitch = 0,  ///< -24…+24 semitones
        tvf_cutoff, ///< -9600…+9600 cents
        amplitude,  ///< -100…+100 %
        lfo1_rate,  ///< -10…+10 Hz
        lfo1_pitch, ///< 0…600 cents
        lfo1_tvf,   ///< 0…2400 cents
        lfo1_tva,   ///< 0…100 %
        lfo2_rate,  ///< -10…+10 Hz
        lfo2_pitch, ///< 0…600 cents
        lfo2_tvf,   ///< 0…2400 cents
        lfo2_tva,   ///< 0…100 %
    };

    static constexpr int source_count = 6;
    static constexpr int destination_count = 11;

    /// The value every depth centres on, and the value of an unassigned route.
    static constexpr int neutral = 0x40;

    /// The mod wheel's LFO1 pitch depth at power-on — the one destination that is not zero or
    /// centred, and the reason a GM file's mod wheel produces vibrato without being told to.
    static constexpr int default_modulation_lfo1_pitch = 0x0A;

    /// `[source][destination]`.
    std::array<std::array<int, destination_count>, source_count> depth{};

    ControlMatrix() noexcept { reset(); }

    /// Returns every route to power-on.
    void reset() noexcept
    {
        for (auto& source : depth) {
            // The three continuous destinations and the two LFO rates are bipolar and centre at
            // 0x40; the six LFO depths are amounts and start at zero.
            source = {neutral, neutral, neutral, neutral, 0, 0, 0, neutral, 0, 0, 0};
        }
        at(Source::modulation, Destination::lfo1_pitch) = default_modulation_lfo1_pitch;

        // Bend's pitch depth is deliberately absent here — see `bend_pitch_lives_in_bend_range`.
    }

    [[nodiscard]] int& at(Source source, Destination destination) noexcept
    {
        return depth[static_cast<std::size_t>(source)][static_cast<std::size_t>(destination)];
    }

    [[nodiscard]] int at(Source source, Destination destination) const noexcept
    {
        return depth[static_cast<std::size_t>(source)][static_cast<std::size_t>(destination)];
    }

    /// Stores one depth the way `sysex_part_control_matrix` does, which is not a plain assignment.
    ///
    /// Ten of the eleven destinations take the byte as it arrives. **Pitch does not**: it is
    /// clamped to 0x28-0x58, which is ±24 semitones, and that range is the reason the pitch law's
    /// 0xbe8 clamp is the top of the scale rather than a rail a real stream can reach — 24 × 127 is
    /// exactly 3048. Storing the byte raw instead lets a depth of 0x7f reach that clamp at a third
    /// of the wheel's travel, so the pitch ramps steeply and then stops dead half way up. That is
    /// what it did here until the module was asked: against `40 21 00` at 0x7f the module ramps
    /// evenly to 24 semitones across the whole wheel, and this engine had already railed by 64.
    ///
    /// Bend's pitch depth never reaches this function — see `bend_pitch_lives_in_bend_range`. The
    /// engine clamps that one to 0x40-0x58 instead, one-sided, which is the 0-24 range
    /// `Part::bend_range` already holds it to.
    void store(Source source, Destination destination, int value) noexcept
    {
        at(source, destination) =
            destination == Destination::pitch ? std::clamp(value, 0x28, 0x58) : value;
    }

    /// What one source contributes, once its depths have been scaled by its current amount.
    ///
    /// Named fields rather than an array, deliberately. The engine's own output is eleven shorts in
    /// an order that is neither the SysEx order nor the storage order — within each LFO group it
    /// runs TVA, TVF, pitch, the reverse of how the messages arrive. Handing consumers an index
    /// would hand them that permutation to get wrong; handing them names retires it here.
    struct Modulation {
        int pitch = 0;      ///< added to the voice's pitch
        int tvf_cutoff = 0; ///< added to the cutoff sum
        int amplitude = 0;  ///< scales the voice's level
        int lfo1_rate = 0;
        int lfo1_pitch = 0;
        int lfo1_tvf = 0;
        int lfo1_tva = 0;
        int lfo2_rate = 0;
        int lfo2_pitch = 0;
        int lfo2_tvf = 0;
        int lfo2_tva = 0;

        /// One field, chosen by destination.
        ///
        /// Keyed by the enum rather than by an integer, which is the whole of what makes it safe:
        /// the permutation the fields exist to retire is a numeric one, and a caller who cannot
        /// supply a number cannot reintroduce it. Summing eleven destinations in a loop needs this;
        /// reaching for a single one should still use the field.
        [[nodiscard]] constexpr int at(Destination destination) const noexcept
        {
            switch (destination) {
            case Destination::pitch:
                return pitch;
            case Destination::tvf_cutoff:
                return tvf_cutoff;
            case Destination::amplitude:
                return amplitude;
            case Destination::lfo1_rate:
                return lfo1_rate;
            case Destination::lfo1_pitch:
                return lfo1_pitch;
            case Destination::lfo1_tvf:
                return lfo1_tvf;
            case Destination::lfo1_tva:
                return lfo1_tva;
            case Destination::lfo2_rate:
                return lfo2_rate;
            case Destination::lfo2_pitch:
                return lfo2_pitch;
            case Destination::lfo2_tvf:
                return lfo2_tvf;
            case Destination::lfo2_tva:
                break;
            }
            return lfo2_tva;
        }

        /// Accumulates another source's contribution.
        ///
        /// The engine keeps one running sum a destination and every source adds into it before
        /// anything is clamped, so the sources have to be summed raw — adding scaled results
        /// instead would clamp each source separately and let the total escape the rail.
        constexpr Modulation& operator+=(const Modulation& other) noexcept
        {
            pitch += other.pitch;
            tvf_cutoff += other.tvf_cutoff;
            amplitude += other.amplitude;
            lfo1_rate += other.lfo1_rate;
            lfo1_pitch += other.lfo1_pitch;
            lfo1_tvf += other.lfo1_tvf;
            lfo1_tva += other.lfo1_tva;
            lfo2_rate += other.lfo2_rate;
            lfo2_pitch += other.lfo2_pitch;
            lfo2_tvf += other.lfo2_tvf;
            lfo2_tva += other.lfo2_tva;
            return *this;
        }
    };

    /// Scales this source's depths by a controller amount — `modmatrix_apply_linear`.
    ///
    /// Used by every source except bend, which has its own law (`modmatrix_apply_bipolar`, a
    /// per-destination 16-bit scale rather than a shift) and is not this function.
    ///
    /// This reads more simply than the engine's version, and the reason is worth knowing rather
    /// than trusting. The engine permutes **twice**: `sysex_part_control_matrix` writes SysEx
    /// destination 3 to block byte 4 and destination 10 to block byte 11, skipping byte 3
    /// altogether; then `modmatrix_apply_linear` reads block byte 5 into output 6 and byte 7 into
    /// output 4, running each LFO group backwards. Composed, the two cancel — storing by SysEx
    /// index and naming the outputs makes both disappear. The check that this is a real
    /// cancellation and not a coincidence is that the laws then line up with the published ranges:
    /// every destination that comes out bipolar is one the manual documents as ±something, and
    /// every quartered one is documented as a 0-upward amount.
    ///
    /// Three scalings, not one. Pitch takes the product whole; the two continuous destinations and
    /// the LFO rates take it halved; the six LFO depths take it quartered *and* are unipolar — they
    /// are amounts rather than offsets, so they are not measured from 0x40. The halving is written
    /// as a magnitude shift with the sign reapplied, because an arithmetic shift of a negative
    /// rounds toward minus infinity and the engine's does not.
    [[nodiscard]] Modulation applied_linear(Source source, int amount) const noexcept
    {
        const auto& row = depth[static_cast<std::size_t>(source)];

        // A bipolar destination, halved. `(|d| * amount) >> 1` with the sign put back.
        const auto halved = [amount](int value) {
            const int offset = value - neutral;
            const int magnitude = (std::abs(offset) * amount) >> 1;
            return offset < 0 ? -magnitude : magnitude;
        };

        // A unipolar depth, quartered. No 0x40 offset: zero means none.
        const auto quartered = [amount](int value) { return (value * amount) >> 2; };

        return Modulation{
            .pitch = (row[0] - neutral) * amount,
            .tvf_cutoff = halved(row[1]),
            .amplitude = halved(row[2]),
            .lfo1_rate = halved(row[3]),
            .lfo1_pitch = quartered(row[4]),
            .lfo1_tvf = quartered(row[5]),
            .lfo1_tva = quartered(row[6]),
            .lfo2_rate = halved(row[7]),
            .lfo2_pitch = quartered(row[8]),
            .lfo2_tvf = quartered(row[9]),
            .lfo2_tva = quartered(row[10]),
        };
    }

    /// Scales this source's depths by a signed amount — `modmatrix_apply_bipolar`.
    ///
    /// Bend's law, and only bend's. It is not a variant of the linear apply: where that one shifts,
    /// this one multiplies by a per-destination 16-bit constant and takes the high word, and its
    /// amount is a signed 14-bit deflection rather than a 7-bit controller.
    ///
    /// The three constants turn out to be the same three-way split the linear apply makes. Against
    /// the `<< 2` the magnitude already carries, `0xfe16` is ×1, `0x7f00` is ×½ and `0x3f81` is ×¼
    /// — whole for pitch, halved for the continuous destinations and the LFO rates, quartered for
    /// the six LFO depths. That the two functions agree on which destination gets which treatment,
    /// by completely different arithmetic, is the check that neither has been misread.
    ///
    /// The sign is the product of two: the depth's side of centre and the amount's. A downward bend
    /// through a negative depth pushes pitch *up*, which is the point of an inverted assignment.
    /// `pitch_depth` supplies the cell this matrix does not store — bend's, which lives in
    /// `Part::bend_range`. Passing it in keeps that one store rather than mirroring it here.
    [[nodiscard]] Modulation
    applied_bipolar(Source source, int amount, int pitch_depth = neutral) const noexcept
    {
        const auto& row = depth[static_cast<std::size_t>(source)];
        const int magnitude = (amount < 0 ? -amount : amount) << 2;
        const bool amount_negative = amount < 0;

        // A destination measured from centre, whose sign combines with the amount's.
        const auto centred = [magnitude, amount_negative](int value, int constant) {
            const int offset = value - neutral;
            if (offset == 0) {
                return 0;
            }
            const int scaled =
                static_cast<int>((static_cast<unsigned>(std::abs(offset) * magnitude) >> 8)
                                     * static_cast<unsigned>(constant)
                                 >> 16);
            return (offset < 0) != amount_negative ? -scaled : scaled;
        };

        // A unipolar depth: zero means none, so only the amount carries a sign.
        const auto amountwise = [magnitude, amount_negative](int value, int constant) {
            if (value == 0) {
                return 0;
            }
            const int scaled = static_cast<int>((static_cast<unsigned>(value * magnitude) >> 8)
                                                    * static_cast<unsigned>(constant)
                                                >> 16);
            return amount_negative ? -scaled : scaled;
        };

        constexpr int whole = 0xFE16;
        constexpr int half = 0x7F00;
        constexpr int quarter = 0x3F81;

        return Modulation{
            .pitch = centred(pitch_depth, whole),
            .tvf_cutoff = centred(row[1], half),
            .amplitude = centred(row[2], half),
            .lfo1_rate = centred(row[3], half),
            .lfo1_pitch = amountwise(row[4], quarter),
            .lfo1_tvf = amountwise(row[5], quarter),
            .lfo1_tva = amountwise(row[6], quarter),
            .lfo2_rate = centred(row[7], half),
            .lfo2_pitch = amountwise(row[8], quarter),
            .lfo2_tvf = amountwise(row[9], quarter),
            .lfo2_tva = amountwise(row[10], quarter),
        };
    }

    /// What turns a summed destination into its own unit — `part_mod_depth_recalc`, once per
    /// destination.
    ///
    /// The engine keeps eleven running sums per part, one a destination, each the total of the five
    /// part-wide sources. A sum is a raw `depth x amount` product in no unit at all; this is the
    /// step that gives it one. Every destination does the same three things — clamp the magnitude,
    /// shift it, take the high word of a 16-bit multiply — and differs only in the three constants.
    struct Law {
        /// The magnitude ceiling, applied before the scale.
        int clamp;
        /// The left shift the magnitude takes first.
        int shift;
        /// The 16.16 multiplier whose high word is the result.
        int multiplier;
    };

    /// The three constants for one destination.
    ///
    /// Two things are worth reading off this table rather than trusting. The clamps are exactly the
    /// largest sum each destination can reach: 0xbe8 is 127 x 24 for a whole product, 4000 is
    /// 127 x 63 / 2 for a halved one, and 0xfc0 is 127 x 127 / 4 for a quartered one — so the clamp
    /// is not a safety rail that a real stream might hit, it is the top of the scale. And each
    /// `clamp << shift` lands just inside 16 bits (64512 is the largest, for the LFO amplitude
    /// depths), which is what keeps the engine's unsigned-short intermediate from wrapping. Both
    /// facts fall out of the constants; neither was put there by hand.
    ///
    /// The full-scale results are the published ranges, exactly: +-24000 milli-semitones of pitch,
    /// +-24576 cutoff units (9600 cents at 2.56 a cent), +-32512 of 0x7f00 amplitude, +-6553 of LFO
    /// increment (10 Hz, since 65536 increments a tick at 100 Hz is 100 Hz), and for the LFO depths
    /// 32512, 6144 and 6000 — 100 %, 2400 cents and 600 cents. Eleven constants reproducing seven
    /// documented figures is the check that the table has been read off correctly.
    [[nodiscard]] static constexpr Law law(Destination destination) noexcept
    {
        switch (destination) {
        case Destination::pitch:
            return Law{0xBE8, 3, 0xFBF8};
        case Destination::tvf_cutoff:
            return Law{4000, 3, 0xC49C};
        case Destination::amplitude:
            return Law{4000, 4, 0x820D};
        case Destination::lfo1_rate:
        case Destination::lfo2_rate:
            return Law{4000, 1, 0xD1B8};
        case Destination::lfo1_pitch:
        case Destination::lfo2_pitch:
            return Law{0xFC0, 1, 0xBE7A};
        case Destination::lfo1_tvf:
        case Destination::lfo2_tvf:
            return Law{0xFC0, 1, 0xC30D};
        case Destination::lfo1_tva:
        case Destination::lfo2_tva:
            break;
        }
        return Law{0xFC0, 4, 0x8105};
    }

    /// Clamps and scales a destination's summed sources into that destination's unit.
    ///
    /// Sign-magnitude, like everything else here: the clamp and the multiply see the magnitude and
    /// the sign is reapplied, which is not the same as clamping a signed value and shifting it.
    [[nodiscard]] static constexpr int scaled(Destination destination, int sum) noexcept
    {
        if (sum == 0) {
            return 0;
        }

        const Law rule = law(destination);
        const unsigned magnitude =
            static_cast<unsigned>(std::min(std::abs(sum), rule.clamp) << rule.shift);
        const int value =
            static_cast<int>((magnitude * static_cast<unsigned>(rule.multiplier)) >> 16);
        return sum < 0 ? -value : value;
    }

    /// Every destination's summed sources, each scaled by its own law.
    [[nodiscard]] static constexpr Modulation scaled(const Modulation& sums) noexcept
    {
        return Modulation{
            .pitch = scaled(Destination::pitch, sums.pitch),
            .tvf_cutoff = scaled(Destination::tvf_cutoff, sums.tvf_cutoff),
            .amplitude = scaled(Destination::amplitude, sums.amplitude),
            .lfo1_rate = scaled(Destination::lfo1_rate, sums.lfo1_rate),
            .lfo1_pitch = scaled(Destination::lfo1_pitch, sums.lfo1_pitch),
            .lfo1_tvf = scaled(Destination::lfo1_tvf, sums.lfo1_tvf),
            .lfo1_tva = scaled(Destination::lfo1_tva, sums.lfo1_tva),
            .lfo2_rate = scaled(Destination::lfo2_rate, sums.lfo2_rate),
            .lfo2_pitch = scaled(Destination::lfo2_pitch, sums.lfo2_pitch),
            .lfo2_tvf = scaled(Destination::lfo2_tvf, sums.lfo2_tvf),
            .lfo2_tva = scaled(Destination::lfo2_tva, sums.lfo2_tva),
        };
    }

    /// Bend's pitch depth is **not** stored here, and this says so out loud.
    ///
    /// `40 2x 10` (BEND PITCH CONTROL) and RPN 00/00 (pitch bend sensitivity) are not two
    /// parameters that happen to agree — they are one byte, `part+0x408`, written by both handlers
    /// with the same 0–24 semitone clamp. The engine's bend range *is* this matrix cell. Keeping a
    /// second copy here would be a second source of truth for one value, so `Part::bend_range`
    /// owns it and both messages write there.
    static constexpr bool bend_pitch_lives_in_bend_range = true;
};

} // namespace ts
