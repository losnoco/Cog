#pragma once

#include "tabulasonora/rom_image.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace ts {

/// One record of the DLL's own insertion-effect directory.
///
/// The engine names its 65 EFX types itself: a 0x28-byte record carries the display name, the GS
/// type key `40 03 00`/`01` selects, the index into the algorithm dispatch table, and the default
/// parameter block the type loads on selection. The record layout — and the fact that the Ghidra
/// symbol lands 12 bytes into it, on the type key — is documented in the spec's FINDINGS; the
/// directory sits at `0x181895660`, 66 records, the 66th being the blank `0xFFFF` "no effect
/// assigned" state.
struct EfxRecord {
    /// Display name, exactly as the DLL spells it (`Equalizer`, `Lo-Fi`, `OD / OD2`, …).
    std::string name;

    /// GS type key, `(MSB << 8) | LSB`.
    std::uint16_t type_key = 0xFFFF;

    /// Index into the algorithm dispatch table. A scramble of the type order — Spectrum is type
    /// `01 01` but algorithm 6 — so it is read from the record, never inferred.
    std::uint16_t dispatch = 1;

    /// The 0x1C default bytes the type loads: the 20 GS parameters (`40 03 03`–`16`), the three
    /// send levels (`17`–`19`), and the internal routing bytes behind them.
    std::array<std::uint8_t, 0x1C> defaults{};
};

/// The GS insertion EFX block: one stereo effect ahead of the send network.
///
/// This is the subsystem the spec's scope note leaves to the downstream engine. The block-level
/// machinery is transcribed from the engine and is the same for all 65 types: a register file of
/// byte parameters converted to float coefficients (`fx_reg_write` @ `0x1800898d0`, with the
/// per-register width map and the bit-15 scale select), a per-type packed preset that fills
/// registers `0x8A`–`0x1EF` on selection (`fx_set_algo_index` @ `0x180062410`), and two delay
/// lines whose base decrements one sample per sample, so an algorithm is a program of taps at
/// fixed offsets (`fx_delayline_wrap` @ `0x180089830`).
///
/// The per-algorithm processors are transcribed individually, and only a first tranche exists:
/// Thru, Overdrive and Distortion (one dataflow — the decompiled functions differ only in
/// presets). A type whose processor has not been transcribed yet passes the signal through
/// unchanged and reports itself via `implemented()`, so a host can say which effect it is not
/// rendering rather than rendering it wrong.
///
/// Roland-derived data — the directory, the presets, the parameter curves — is decoded from the
/// user's own DLL at runtime, like every other table in this engine. Offsets are pinned to SOUND
/// Canvas VA 1.1.6.
class InsertionEffect {
public:
    /// Registers in the file: engine numbers `0x80`–`0x1FF`, stored zero-based.
    static constexpr int register_count = 0x180;

    /// Bytes in the live parameter block, addressed by `40 03 03`–`1E` plus internal state.
    static constexpr int parameter_bytes = 0x20;

    explicit InsertionEffect(const RomImage& rom);
    ~InsertionEffect();

    InsertionEffect(InsertionEffect&&) noexcept;
    InsertionEffect& operator=(InsertionEffect&&) noexcept;

    /// The directory, in file order: 65 named types plus the blank "no effect" record.
    [[nodiscard]] const std::vector<EfxRecord>& directory() const;

    /// Selects a type by its GS key and loads its defaults, exactly as `40 03 00`/`01` does.
    ///
    /// A key that matches no record leaves the block on algorithm 0 (Thru), which is the engine's
    /// own behaviour — the algorithm index is only ever written from a successful map scan.
    void select_type(int msb, int lsb);

    /// Writes one byte of the `40 03` block, addresses `0x03`–`0x1E`.
    void set_parameter(int address, int value);

    /// The record currently selected, or the "no effect" record before any selection.
    [[nodiscard]] const EfxRecord& current() const;

    /// Whether the selected type's processor has been transcribed.
    ///
    /// False means the block is passing the signal through unchanged: routing, sends and defaults
    /// are all live, but the DSP itself is not this effect's.
    [[nodiscard]] bool implemented() const;

    /// The block's common send levels, `40 03 17`–`19`, as linear gains on the block's output.
    /// EFX parts have their own sends forced off; these are what replaces them.
    [[nodiscard]] double reverb_send() const noexcept;
    [[nodiscard]] double chorus_send() const noexcept;
    [[nodiscard]] double delay_send() const noexcept;

    /// Processes one block in place: EFX-part dry in, effect out.
    ///
    /// The engine doubles the input and halves the output around each algorithm
    /// (`fx_process_block`), and that pairing is preserved here.
    void process(std::span<const float> in_left,
                 std::span<const float> in_right,
                 std::span<float> out_left,
                 std::span<float> out_right);

    /// Clears delay lines and scratch, keeping type and parameters.
    void reset();

    /// The programmed state, for oracle harnesses: the float coefficient file and the reverb tap
    /// program, in the engine's own layout. A diff against a live `scdec efxdump` names the exact
    /// register a wrong curve, width kind or preset slice landed in.
    [[nodiscard]] std::span<const float> coefficients() const;
    [[nodiscard]] std::span<const std::int32_t> tap_program() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ts
