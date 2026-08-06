#pragma once

namespace ts {

/// The GS part modify offsets, as the synthesis chains consume them.
///
/// Every one of these is a 0x40-centred byte that three writers share — the sound controllers
/// (CC#71-78), the NRPNs (`01 08`-`01 66`) and the part SysEx (`40 1x 30`-`37`) all land on the
/// same byte in the engine, at `part+0x3e4` through `part+0x3eb`.
///
/// They are carried as a value rather than read off the part because the chains that consume them
/// build voices, not parts: `TvaChain` and `TvfChain` are pure functions of a partial and a note,
/// and the offline renderer drives them with no `Part` in sight. A default-constructed set is
/// neutral and must leave every result bit-identical to having no modifiers at all, which is what
/// makes it safe to pass one everywhere.
///
/// The engine pairs each of these with a second byte at `part+0x453`-`0x45a`, filled from the tone
/// table only when the part is on a user bank (MSB 0x40 or 0x41) and reset to 0x40 otherwise. A
/// standard MIDI file never programs a user tone, so that sibling is neutral throughout and the
/// combined law reduces to the offsets here. Modelling it belongs with user tones, not here.
struct PartModifiers {
    /// CC#76 / NRPN `01 08` / `40 1x 30` — `part+0x3e4`.
    int vibrato_rate = neutral;
    /// CC#77 / NRPN `01 09` / `40 1x 31` — `part+0x3e5`.
    int vibrato_depth = neutral;
    /// CC#78 / NRPN `01 0A` / `40 1x 37` — `part+0x3eb`.
    int vibrato_delay = neutral;
    /// CC#74 / NRPN `01 20` / `40 1x 32` — `part+0x3e6`.
    int tvf_cutoff = neutral;
    /// CC#71 / NRPN `01 21` / `40 1x 33` / XG `08 nn 19` — `part+0x3e7`.
    ///
    /// This one was recorded as unread for a long time, and the reason is worth keeping: Ghidra
    /// prints `0x3e7` and `0x3e8` in *decimal*, alone among the eight, so a search for the hex
    /// spelling finds the four writes and misses all eleven reads. Two of those reads are
    /// synthesis — voice setup seeds `voice+0xee` from it, and stage A (`FUN_1800845c0`) recomputes
    /// it every control tick, both through the voice's heap part pointer at `voice+0x128`.
    ///
    /// It runs the *opposite* way to the rest: the law is `(0x80 - sibling - this) * 2`, so a
    /// larger controller value makes a **smaller** resonance byte, and the byte is reciprocal-Q.
    /// CC#71 above about 0x50 pins it on its floor of 4, which is Q = 16.
    int tvf_resonance = neutral;
    /// CC#73 / NRPN `01 63` / `40 1x 34` — `part+0x3e8`.
    int env_attack = neutral;
    /// CC#75 / NRPN `01 64` / `40 1x 35` — `part+0x3e9`.
    int env_decay = neutral;
    /// CC#72 / NRPN `01 66` / `40 1x 36` — `part+0x3ea`.
    int env_release = neutral;

    /// The neutral value every one of these centres on.
    static constexpr int neutral = 0x40;

    /// True when nothing here would change a result.
    [[nodiscard]] constexpr bool is_neutral() const noexcept
    {
        return vibrato_rate == neutral && vibrato_depth == neutral && vibrato_delay == neutral
               && tvf_cutoff == neutral && tvf_resonance == neutral && env_attack == neutral
               && env_decay == neutral && env_release == neutral;
    }

    /// The envelope rate bias for segments 0 and 1, in rate-curve index steps.
    ///
    /// The engine sums the modify byte with its user-tone sibling and doubles the pair's distance
    /// from centre — `(0x457 + 0x3e8) * 2 - 0x100` — so one step of the controller moves the rate
    /// curve two entries. It biases the *index*, not the duration, which is why a segment can be
    /// driven clean off either end of the curve.
    [[nodiscard]] constexpr int attack_bias() const noexcept { return (env_attack - neutral) * 2; }

    /// The envelope rate bias for segments 2 and 3.
    [[nodiscard]] constexpr int decay_bias() const noexcept { return (env_decay - neutral) * 2; }

    /// The envelope rate bias for the release segment.
    [[nodiscard]] constexpr int release_bias() const noexcept
    {
        return (env_release - neutral) * 2;
    }

    /// The cutoff offset, in the 15-bit cutoff units the filter chain sums.
    ///
    /// A controller step is a whole 0x100 here, which is why cutoff sweeps over CC#74 are coarse
    /// on this module: 128 steps span the entire cutoff range.
    [[nodiscard]] constexpr int cutoff_offset() const noexcept
    {
        return (tvf_cutoff - neutral) * 0x100;
    }
};

} // namespace ts
