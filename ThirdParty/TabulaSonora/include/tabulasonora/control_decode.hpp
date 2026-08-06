#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace ts {

/// Every part parameter a MIDI message can set to a value.
///
/// This is the vocabulary the two front ends share. It deliberately covers only the messages whose
/// whole effect is "store this number against this part" — the ones where both paths were writing
/// the same knowledge into different files. Messages that *do* something (note on and off, the
/// pedals' release behaviour, the channel-mode messages, program change and its bank resolution)
/// stay with the front end that has the machinery to act on them, because there is nothing to share
/// there but a switch label.
enum class ControlTarget {
    volume,
    expression,
    pan,
    modulation,
    damper,
    reverb_send,
    chorus_send,
    delay_send,
    bank,
    bend_range,

    // The part modify offsets, all eight of them.
    vibrato_rate,
    vibrato_depth,
    vibrato_delay,
    tvf_cutoff,
    tvf_resonance,
    env_attack,
    env_decay,
    env_release,

    velocity_depth,
    velocity_offset,

    channel_pressure,

    /// The control matrix's pitch routes, one per source.
    matrix_modulation_pitch,
    matrix_pressure_pitch,

    /// The system EQ block, which is global rather than per part.
    eq_low_frequency,
    eq_low_gain,
    eq_high_frequency,
    eq_high_gain,

    /// Per-part EQ enable (`40 4x 20`).
    eq_enabled,
};

/// One parameter set by one message.
struct ControlUpdate {
    ControlTarget target = ControlTarget::volume;
    /// The part's channel, or -1 for the global targets.
    int channel = -1;
    int value = 0;

    [[nodiscard]] constexpr bool is_global() const noexcept { return channel < 0; }
};

/// Which parameter a Control Change writes, if it writes one.
///
/// CC#10's zero is folded to one here rather than at each call site: the wheel cannot reach the
/// random pan position, and only the GS SysEx panpot can write a true zero.
[[nodiscard]] std::optional<ControlUpdate>
decode_control_change(int channel, int controller, int value) noexcept;

/// Which parameter a Roland GS DT1 message writes, if it writes one.
///
/// Takes the whole message including `F0` and the checksum. Returns nothing for a message that is
/// malformed, fails its checksum, or addresses something outside this vocabulary — a caller that
/// needs those cases handles them itself, and both do.
[[nodiscard]] std::optional<ControlUpdate> decode_gs_sysex(std::span<const std::uint8_t> bytes,
                                                           int port = 0) noexcept;

/// Whether a GS DT1 message is well formed and its checksum folds to zero.
///
/// Exposed because both front ends drop a bad message and neither should be spelling the fold out
/// for itself.
[[nodiscard]] bool gs_checksum_ok(std::span<const std::uint8_t> bytes) noexcept;

/// What an XG message is, for a caller deciding whether it has the machinery to act.
///
/// XG is a second SysEx dialect, not an extension of GS: the module keeps a mode flag and swaps its
/// whole parameter parser when XG System On arrives. These are the blocks that reach anything.
enum class XgMessage {
    /// Not XG, or an XG address this engine does nothing with.
    none,
    /// `00 00 7E` — switch every part onto the XG map and reset.
    system_on,
    /// `00 00 7F` — All Parameter Reset.
    all_parameter_reset,
    /// `00 00 00`-`03` master tune, `04` master volume, `06` transpose.
    system_parameter,
    /// `02 01 pp` — reverb and chorus type, time and return.
    effect1,
    /// `08 nn pp` — a Multi Part parameter.
    multi_part,
    /// `3n rr pp` — a Drum Setup parameter.
    drum_setup,
};

/// An XG message split into the parts a front end needs to act on it.
struct XgAddress {
    XgMessage kind = XgMessage::none;
    /// Address bytes, as sent.
    int high = 0;
    int mid = 0;
    int low = 0;
    /// First data byte, or 0 for a message that carries none.
    int value = 0;
    /// For `multi_part`, the part the message addresses, already remapped from XG's part numbering
    /// to this engine's. -1 for every other kind.
    int part = -1;
};

/// Classifies an XG message and remaps its part number, without deciding what to do about it.
///
/// Takes the whole message including `F0` and `F7`. XG carries no checksum — the length and the
/// terminator are the whole of its validity — so this checks the frame and nothing else.
///
/// `part` may exceed any given configuration's part count — XG addresses `nn` up to 0x3F, and the
/// caller range-checks it against the parts it actually has. Out of range must be *ignored*: the
/// module wraps it into a heap overflow, and folding it onto a part that does exist would be
/// audible and wrong.
[[nodiscard]] XgAddress decode_xg_sysex(std::span<const std::uint8_t> bytes) noexcept;

/// Which parameter an XG Multi Part message writes, if it writes one this vocabulary covers.
///
/// Only the parameters whose whole effect is "store this number against this part" appear. Bank and
/// program, part mode, receive channel, note shift, detune, the receive switches and the scale
/// tuning all *do* something, and stay with the front end that can do it.
[[nodiscard]] std::optional<ControlUpdate> decode_xg_multi_part(const XgAddress& address) noexcept;

} // namespace ts
