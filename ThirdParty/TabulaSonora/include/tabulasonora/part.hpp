#pragma once

#include "tabulasonora/lfo_engine.hpp"
#include "tabulasonora/pitch_chain.hpp"
#include "tabulasonora/sequence.hpp"
#include "tabulasonora/tva_chain.hpp"

#include <array>
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

    /// Channel aftertouch, stored but not yet routed: the engine feeds it to the modulation
    /// matrix (`channel_pressure_apply`), which this port does not model beyond the mod wheel.
    int channel_pressure = 0;

    /// The GS part modify offsets, 0x40 centred. Three writers share each one -- the sound
    /// controllers (CC#71-78), the NRPNs (`01 08`-`01 66`), and the part SysEx (`40 1x 30`-`37`)
    /// all land on the same per-part byte in the engine (`part+0x3e4`-`0x3eb`).
    ///
    /// Latched but not yet consumed: routing them into the LFO, TVF and envelope setup is the
    /// remaining synthesis work, tracked alongside the insertion-EFX block.
    int vibrato_rate = 0x40;
    int vibrato_depth = 0x40;
    int vibrato_delay = 0x40;
    int tvf_cutoff = 0x40;
    int tvf_resonance = 0x40;
    int env_attack = 0x40;
    int env_decay = 0x40;
    int env_release = 0x40;

    /// GS scale tuning (`40 1x 40`-`4B`), one entry per pitch class, 0x40 centred, a cent a step.
    std::array<int, 12> scale_tuning{0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
                                     0x40, 0x40, 0x40, 0x40, 0x40, 0x40};

    /// GS use-for-rhythm (`40 1x 15`): -1 follows the channel default, 0 forces melodic, 1 or 2
    /// route the part to the drum path on that drum map.
    int rhythm = -1;

    /// GS keyboard range (`40 1x 1D`/`1E`); note-ons outside it are ignored on melodic parts.
    int key_low = 0;
    int key_high = 0x7F;

    /// GS velocity sense depth and offset (`40 1x 1A`/`1B`), stored but not yet applied to the
    /// velocity curve.
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

    /// The combined volume multiplier, 1.0 with everything at 127.
    [[nodiscard]] double volume_scale() const noexcept { return volume_scale_; }

    /// The mod wheel's contribution to LFO1 pitch depth, in milli-semitones.
    [[nodiscard]] double mod_wheel_depth() const noexcept
    {
        return LfoEngine::mod_wheel_depth(modulation);
    }

    /// The bend offset in milli-semitones.
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
    }

    int volume_ = sequence_builder::default_volume;
    int expression_ = sequence_builder::default_expression;
    int master_ = sequence_builder::default_master;
    double volume_scale_ = 1.0;
};

} // namespace ts
