#pragma once

#include "tabulasonora/control_matrix.hpp"
#include "tabulasonora/lfo_engine.hpp"
#include "tabulasonora/part_modifiers.hpp"
#include "tabulasonora/pitch_chain.hpp"
#include "tabulasonora/sequence.hpp"
#include "tabulasonora/tva_chain.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>

namespace ts {

/// The GS receive switches, one per message class the module can be told to ignore.
///
/// The engine keeps these as a 16-bit mask at `part+0x3d6` and every recovered handler opens with a
/// test against it -- `cc11_expression` requires `0x810`, `cc64_hold_damper` `0x820`, and so on.
/// They are named booleans here rather than a mask because nothing needs the packed form: there is
/// no bulk dump to emit, and the SysEx that writes them (`40 1x 03`-`40 1x 12`, plus the bank pair
/// at `40 1x 23`/`24`) arrives one switch at a time.
///
/// Everything defaults on, which is the GS-reset state.
struct RxSwitches {
    bool pitch_bend = true;
    bool channel_pressure = true;
    bool program_change = true;
    /// The master gate every controller shares; the channel-mode messages (CC#120 up) bypass it.
    bool control_change = true;
    bool poly_pressure = true;
    bool notes = true;
    bool rpn = true;
    bool nrpn = true;
    bool modulation = true;
    bool volume = true;
    bool panpot = true;
    bool expression = true;
    bool hold = true;
    bool portamento = true;
    bool sostenuto = true;
    bool soft = true;
    /// Bank select, split MSB/LSB the way `40 1x 23`/`24` split it.
    bool bank_msb = true;
    bool bank_lsb = true;
};

/// One of the sixteen parts: the channel state a running engine keeps between events.
///
/// The engine has no MIDI output, so nothing can be read back from it — every piece of channel
/// state has to be tracked here. This is the live counterpart of `PartTimelines`, which records the
/// same values as breakpoints for the offline renderer.
///
/// Volume, expression and master are set through methods rather than left as public fields: each
/// one recomputes the combined scale, and a plain assignment that skipped that would leave the part
/// sounding at its previous level until the next one happened to move.
class Part {
public:
    Part() { recompute(); }

    /// Program in force, from the last program change.
    int program = 0;
    /// Bank select MSB, which carries the variation.
    int bank = 0;
    /// Bank select LSB, which on this module selects the tone map: 1-4 name a vintage and 0 keeps
    /// the configured default. The engine clamps the SysEx form to 1-4 (`sysex_part_bank_lsb`).
    int bank_lsb = 0;

    /// The MIDI channel this part listens on, 0-15, or 16 for off. GS SysEx `40 1x 02`.
    ///
    /// Owned by the tone generator rather than reset here, because the power-on value is the
    /// part's own channel and the part does not know its index.
    int rx_channel = 0;

    /// The GS receive switches, written by SysEx `40 1x 03`-`12` and `23`/`24`.
    RxSwitches rx;
    /// CC#10 pan.
    int pan = sequence_builder::default_pan;
    /// CC#1 modulation.
    int modulation = 0;
    /// CC#64 damper.
    int damper = 0;
    /// CC#91 reverb send.
    int reverb_send = sequence_builder::default_reverb_send;
    /// CC#93 chorus send.
    int chorus_send = sequence_builder::default_chorus_send;
    /// Part delay send, which has no Control Change and arrives only over SysEx.
    int delay_send = 0;
    /// CC#94 delay send. The Control Change alias of the SysEx part delay send: the engine routes
    /// both into the same byte (`caseD_5e` writes `part+0x44a`, as does `40 1x 2C`).
    /// Kept as the one `delay_send` field above.

    /// Pitch bend, as a 14-bit value with 8192 centred.
    int bend = 8192;
    /// Bend range in semitones, from RPN 00/00.
    int bend_range = 2;
    /// RPN 00/01 fine tune, as the full 14-bit value with 0x2000 centred; +-100 cents at the ends.
    int fine_tune = 0x2000;
    /// RPN 00/02 coarse tune, 0x40 centred, one semitone a step.
    int coarse_tune = 0x40;
    /// GS part key shift (`40 1x 16`), 0x40 centred; the engine clamps it to 0x28-0x58.
    int key_shift = 0x40;

    /// The controller assignment matrix (`40 2x`), which decides what each source modulates.
    ///
    /// Bend's pitch depth is the one route that lives elsewhere: it is `bend_range` above, because
    /// the engine stores it in the same byte RPN 00/00 writes. See `ControlMatrix`.
    ControlMatrix control;

    /// Channel aftertouch, which reaches pitch through the control matrix.
    int channel_pressure = 0;

    /// Which Control Change numbers this part's two assignable sources listen to (`40 1x 1F` and
    /// `20`), and what those controllers currently read.
    ///
    /// The numbers default to 16 and 17 — General Purpose 1 and 2, which is what makes those two
    /// controllers do anything on a GS module at all. Nothing else in the engine reads CC#16 or
    /// CC#17; they exist to be pointed at the matrix.
    ///
    /// An assignable source is not a controller with a fixed meaning, so it is tracked as a pair:
    /// the number decides which message feeds it, and the amount is what the matrix scales. Pointing
    /// CC1 at a controller that already means something — the mod wheel, say — does not take that
    /// meaning away; the message does both jobs.
    int cc1_number = 16;
    int cc2_number = 17;
    int cc1 = 0;
    int cc2 = 0;

    /// Bank select MSB while the engine is in XG mode, where it means something else entirely.
    ///
    /// XG inverts the pair: the *LSB* carries the variation and lands in `bank`, while the MSB
    /// chooses between melodic (below 0x7E), the SFX voice bank (0x40), the SFX kits (0x7E) and the
    /// drum kits (0x7F). It needs its own field because it decides melodic-versus-drum for the part
    /// and so has to survive until the next program change, which is when that decision is taken.
    ///
    /// **-1 means no bank select has been sent**, which is not the same as zero. A program change
    /// on a part whose bank was never written must not decide drum routing at all -- it leaves the
    /// part on its default, so channel 10 stays drums the way XG starts it. Reading an unwritten
    /// bank as 0 would silently make every default drum part melodic, which is exactly what a file
    /// that sends nothing but program changes would suffer.
    int xg_bank_msb = -1;

    /// Polyphonic aftertouch, per key.
    ///
    /// One byte a note rather than one a part, which is the whole of what makes it polyphonic: the
    /// engine's `poly_aftertouch_apply` takes the part's matrix depths and the pressure belonging
    /// to *that key*, so two notes held on one part can be modulated by different amounts.
    ///
    /// **Not cleared by note-on**, which is the opposite of the obvious guess and was measured
    /// rather than assumed. Press a key to full pressure, release it, strike it again and say
    /// nothing about pressure: the module sounds the new note *still bent*, an octave and a half up
    /// on a part whose `40 2x 30` depth is 0x58. The pressure belongs to the key, not to the note
    /// that happened to be sounding on it, and only another poly message or a reset moves it.
    std::array<std::uint8_t, 128> poly_pressure{};

    /// The pressure on one key, or zero for anything out of range.
    [[nodiscard]] int key_pressure(int note) const noexcept
    {
        return note >= 0 && note < 128 ? poly_pressure[static_cast<std::size_t>(note)] : 0;
    }

    /// The GS part modify offsets, 0x40 centred. Three writers share each one -- the sound
    /// controllers (CC#71-78), the NRPNs (`01 08`-`01 66`), and the part SysEx (`40 1x 30`-`37`)
    /// all land on the same per-part byte in the engine (`part+0x3e4`-`0x3eb`).
    ///
    /// All eight reach the synthesis chains through `modifiers()`.
    int vibrato_rate = 0x40;
    int vibrato_depth = 0x40;
    int vibrato_delay = 0x40;
    int tvf_cutoff = 0x40;
    int tvf_resonance = 0x40;
    int env_attack = 0x40;
    int env_decay = 0x40;
    int env_release = 0x40;

    /// The eight live modify offsets, in the form the synthesis chains take.
    [[nodiscard]] PartModifiers modifiers() const noexcept
    {
        return PartModifiers{vibrato_rate,
                             vibrato_depth,
                             vibrato_delay,
                             tvf_cutoff,
                             tvf_resonance,
                             env_attack,
                             env_decay,
                             env_release};
    }

    /// The velocity a note-on actually sounds at, after this part's velocity sense (`40 1x 1A`
    /// depth and `40 1x 1B` offset).
    ///
    /// Three details are load-bearing. A depth of zero collapses every velocity to **1**, not to
    /// silence. A depth of exactly 0x40 skips the multiply rather than multiplying by one, so a
    /// neutral part cannot lose a count to the shift's truncation. And the low clamp is to 1 as
    /// well: an offset that drives the sum negative still sounds, at the floor.
    [[nodiscard]] int effective_velocity(int velocity) const noexcept
    {
        int value = velocity;
        if (velocity_depth == 0) {
            value = 1;
        } else if (velocity_depth != 0x40) {
            value = (value * velocity_depth) >> 6;
        }

        value += (velocity_offset - 0x40) * 2;
        if (value < 0) {
            return 1;
        }
        return value > 0x7F ? 0x7F : value;
    }

    /// GS scale tuning (`40 1x 40`-`4B`), one entry per pitch class, 0x40 centred, a cent a step.
    std::array<int, 12> scale_tuning{
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};

    /// GS use-for-rhythm (`40 1x 15`): -1 follows the channel default, 0 forces melodic, 1 or 2
    /// route the part to the drum path on that drum map.
    int rhythm = -1;

    /// GS keyboard range (`40 1x 1D`/`1E`); note-ons outside it are ignored on melodic parts.
    int key_low = 0;
    int key_high = 0x7F;

    /// Whether this part runs through the four-band EQ (`40 4x 20`), which is `part+0x450`.
    ///
    /// **Off by default, which contradicts the SC-8820 manual.** The manual gives this parameter a
    /// default of `01 ON`; in this engine the part reset writes the byte to zero and *nothing
    /// anywhere sets it to one* except the SysEx handler, so a module that is never told to switch
    /// the EQ on never switches it on. That is a difference without a sound until a file also sends
    /// a non-flat `40 02`, since a flat EQ is exactly transparent either way — but on such a file
    /// it decides whether the EQ is heard at all, so it is worth an oracle check before it is
    /// trusted.
    bool eq_enabled = false;

    /// Whether this part feeds the insertion EFX block (`40 4x 22`), which is `part+0x452`.
    ///
    /// An EFX part's dry signal detours to the EFX input pair and **both of its sends are forced
    /// to the null bus** — the block's own `40 03 17`–`19` send levels replace them, which is the
    /// mechanism behind the manual's note that system-effect levels become common to all EFX
    /// parts.
    bool efx_enabled = false;

    /// The two biases on the envelope hold clock's rate index — the ninth tone-modify slot, which
    /// continues the `30`-`37` run past where Roland's published list ends.
    ///
    /// `envelope_delay` is the user offset (`40 1x 38`, `part+0x44b`) and survives a program
    /// change; `envelope_delay_tone` is the per-program one (`40 4x 38`, `part+0x45b`) that every
    /// program change forces back to centre, the same way the tone loader fills the eight slots
    /// before it. Both are `0x40` for no change, and they are summed.
    ///
    /// Above centre this does not merely lengthen an existing delay — it **arms** one on partials
    /// that have none, because the rate curve's first entry is zero. See
    /// `EnvelopeMachine::hold_samples`.
    int envelope_delay = 0x40;
    int envelope_delay_tone = 0x40;

    /// GS velocity sense depth and offset (`40 1x 1A`/`1B`), applied by `effective_velocity`.
    int velocity_depth = 0x40;
    int velocity_offset = 0x40;

    /// GS pitch offset fine (`40 1x 17`), stored raw: the unit is Hz on the module, which nothing
    /// downstream consumes yet.
    int pitch_offset_fine = 0x08;

    int rpn_msb = 0x7F;
    int rpn_lsb = 0x7F;
    int nrpn_msb = 0x7F;
    /// The selected NRPN's LSB, which for the drum parameters is the key number.
    int nrpn_lsb = 0x7F;

    /// Whether a CC#6 data entry commits to the selected NRPN rather than the selected RPN.
    ///
    /// The two share data entry, so the last selection made decides. Tracking this is what keeps a
    /// file's drum NRPNs out of the bend range.
    bool data_entry_is_nrpn = false;

    /// Per-drum-key overrides this part has taken from NRPN.
    DrumKeyOverrides drum_keys;

    /// Notes whose release is waiting for the damper to lift.
    ///
    /// Insertion-ordered rather than a set: notes are released oldest first when the pedal comes
    /// up, which is the order the offline renderer closes them in too.
    std::vector<int> sustained;

    /// CC#5 portamento time; indexes the glide-step table.
    int portamento_time = 0;
    /// Whether CC#65 portamento is on.
    bool portamento_on = false;
    /// Whether CC#126 mono mode is on, which flushes the part's voices at each note-on.
    bool mono = false;

    /// Whether the CC#67 soft pedal is down. Binary -- the engine reads only bit 6
    /// (`cc67_soft_pedal`). Latched but not yet consumed by the voice path.
    bool soft = false;

    /// CC#84 portamento control: the key the next note glides from, or -1 when unset.
    ///
    /// One-shot — the engine consumes it at the next note-on and resets the byte, so it glides
    /// exactly one note and does not latch a mode.
    int portamento_control_key = -1;

    /// The key the part last sounded, which portamento glides from, or -1.
    int last_key = -1;

    /// Whether the sostenuto pedal is down. CC#66 is binary — bit 6 only, as the engine reads it.
    bool sostenuto_down = false;

    /// Notes the sostenuto pedal captured — the ones sounding when it went down.
    std::vector<int> sostenuto_captured;

    /// Captured notes whose note-off arrived while the pedal held them.
    std::vector<int> sostenuto_released;

    /// Whether the damper is holding notes on.
    [[nodiscard]] bool damper_down() const noexcept { return damper >= 0x40; }

    /// CC#7 volume.
    [[nodiscard]] int volume() const noexcept { return volume_; }

    void set_volume(int value) noexcept
    {
        volume_ = value;
        recompute();
    }

    /// CC#11 expression.
    [[nodiscard]] int expression() const noexcept { return expression_; }

    void set_expression(int value) noexcept
    {
        expression_ = value;
        recompute();
    }

    /// Master volume, which is global but folds into the same law.
    [[nodiscard]] int master() const noexcept { return master_; }

    void set_master(int value) noexcept
    {
        master_ = value;
        recompute();
    }

    /// The chorus and delay sends in force, which chase `chorus_send` and `delay_send`.
    ///
    /// Per *part*, not per voice, because that is what the engine smooths: these two are taps in
    /// the 33-bus send matrix -- chorus at `DAT_181a6f310` tap 1, delay at `DAT_181a6e8c0` tap 1 --
    /// one coefficient each for the whole part. Only the reverb send is per voice.
    [[nodiscard]] double chorus_send_level() const noexcept { return chorus_send_level_; }
    [[nodiscard]] double delay_send_level() const noexcept { return delay_send_level_; }

    /// Controller units a send moves in one control tick.
    ///
    /// Measured on the matrix coefficient itself: CC#93 driven 0 -> 127 walks it in 64 steps of
    /// 16/1024, one every 640 samples, 1255 ms end to end. That is the same effective rate as the
    /// per-voice reverb send's 1260 ms -- one controller unit a tick -- reached in double steps at
    /// half the cadence.
    static constexpr double send_slew_per_tick = 1.0;

    /// Advances both toward their targets by one control tick's worth.
    void slew_sends(double per_tick) noexcept
    {
        chorus_send_level_ +=
            std::clamp(chorus_send - chorus_send_level_, -per_tick, per_tick);
        delay_send_level_ += std::clamp(delay_send - delay_send_level_, -per_tick, per_tick);
    }

    /// The combined volume multiplier, 1.0 with everything at 127.
    [[nodiscard]] double volume_scale() const noexcept { return volume_scale_; }

    /// The same combination as the integer `voice_volume_apply` yields, which is what the voices'
    /// anti-zipper ramps chase. The mixer reads the ramp rather than this directly.
    [[nodiscard]] int volume_word() const noexcept { return volume_word_; }

    /// Everything the control matrix modulates, each destination in its own unit.
    ///
    /// `part_mod_depth_recalc` keeps eleven running sums a part, one a destination, each the total
    /// of the sources' contributions, and clamps and scales each by its own law. This is that
    /// function, computed on demand rather than cached behind a dirty mask — the mask is a way of
    /// not recomputing, not a difference in what is computed.
    ///
    /// All six sources reach it: the mod wheel, bend, both aftertouches, and the two assignable
    /// controllers. Every one is summed raw before anything is clamped, which is why they are
    /// gathered here rather than scaled separately and added up — clamping each in turn would let
    /// the total escape the rail.
    ///
    /// `key_pressure` is the polyphonic aftertouch on the note being rendered, which is why this
    /// takes an argument at all: everything else here belongs to the part, and that one belongs to
    /// the key. The module applies it at exactly this point too, and for the same reason — it is
    /// the one source whose amount is not a property of the part, so it cannot be folded into a
    /// per-part cache. A caller with no note in hand passes nothing.
    [[nodiscard]] ControlMatrix::Modulation matrix(int key_pressure = 0) const noexcept
    {
        ControlMatrix::Modulation sums =
            control.applied_linear(ControlMatrix::Source::modulation, modulation);
        sums += control.applied_linear(ControlMatrix::Source::channel_pressure, channel_pressure);
        sums += control.applied_linear(ControlMatrix::Source::poly_pressure, key_pressure);
        sums += control.applied_linear(ControlMatrix::Source::cc1, cc1);
        sums += control.applied_linear(ControlMatrix::Source::cc2, cc2);
        sums += control.applied_bipolar(
            ControlMatrix::Source::bend, bend - 8192, bend_range + ControlMatrix::neutral);
        return ControlMatrix::scaled(sums);
    }

    /// The control matrix's contribution to pitch alone, in milli-semitones.
    ///
    /// The clamp behind this is not a round number by accident: 0xbe8 is 3048, which is 127 × 24,
    /// exactly the largest value `amount × (depth − 0x40)` can reach. The scale then works out to
    /// 24 semitones at the rail, which is what fixes the unit as milli-semitones.
    [[nodiscard]] double matrix_pitch_milli_semitones(int key_pressure = 0) const noexcept
    {
        return static_cast<double>(matrix(key_pressure).pitch);
    }

    /// The bend offset in milli-semitones.
    ///
    /// Retained for callers that want bend alone — the offline timelines still build a curve from
    /// it. The voice no longer uses it: bend reaches pitch through the control matrix, which is
    /// where the engine puts it and which differs from this by up to 0.8 cents at full deflection.
    [[nodiscard]] double bend_milli_semitones() const noexcept
    {
        return PitchChain::bend_offset_milli_semitones(bend, bend_range);
    }

    /// The part's static tune in milli-semitones: the net of RPN coarse and fine tune and the GS
    /// part key shift.
    ///
    /// One summed value, matching the engine, which folds all three into a single s16
    /// milli-semitone offset (`part+0x3ba`) added to every voice's pitch.
    [[nodiscard]] double tune_milli_semitones() const noexcept
    {
        return ((key_shift - 0x40) * 1000.0) + ((coarse_tune - 0x40) * 1000.0)
               + ((fine_tune - 0x2000) * 1000.0 / 0x2000);
    }

    /// The scale-tuning offset for a key, in milli-semitones: a cent a step, ten milli-semitones
    /// a cent.
    [[nodiscard]] double scale_offset_milli_semitones(int key) const noexcept
    {
        return (scale_tuning[static_cast<std::size_t>(((key % 12) + 12) % 12)] - 0x40) * 10.0;
    }

    /// Whether the RPN address currently selected is the null set (7F/7F), which parks data entry.
    [[nodiscard]] bool rpn_is_null() const noexcept { return rpn_msb == 0x7F && rpn_lsb == 0x7F; }

    /// Returns the part to its power-on state.
    void reset();

    /// Applies CC#121, which resets the controllers a reset message covers.
    ///
    /// The engine's `caseD_79`: modulation and pressure cleared, damper and soft and portamento
    /// lifted, expression back to full, the one-shot portamento key consumed, and the RPN/NRPN
    /// selection parked at null (`rpn_nrpn_decode` case 0x79). Volume, pan, the sends and the
    /// tuning stay -- a reset is not a power cycle.
    ///
    /// Releasing what the damper held is the caller's job, since it owns the voices.
    void reset_controllers()
    {
        set_expression(sequence_builder::default_expression);
        bend = 8192;
        damper = 0;
        soft = false;
        modulation = 0;
        channel_pressure = 0;
        poly_pressure.fill(0);
        portamento_on = false;
        portamento_control_key = -1;
        rpn_msb = 0x7F;
        rpn_lsb = 0x7F;
        nrpn_msb = 0x7F;
        nrpn_lsb = 0x7F;
        data_entry_is_nrpn = false;
    }

private:
    void recompute() noexcept
    {
        volume_scale_ = TvaChain::part_volume_scale(volume_, expression_, master_);
        volume_word_ = TvaChain::part_volume_word(volume_, expression_, master_);
    }

    int volume_ = sequence_builder::default_volume;
    int expression_ = sequence_builder::default_expression;
    int master_ = sequence_builder::default_master;
    double volume_scale_ = 1.0;
    int volume_word_ = TvaChain::part_volume_word();
    double chorus_send_level_ = 0.0;
    double delay_send_level_ = 0.0;
};

} // namespace ts
