#pragma once

#include "tabulasonora/effect_presets.hpp"
#include "tabulasonora/rom_image.hpp"

namespace ts {

/// Computes the reverb, chorus and delay coefficient sets from the DLL's own preset tables — the
/// managed re-implementation of the engine's start-up register program.
///
/// The coefficients were long believed to exist nowhere in the file, because searching for the
/// harvested values (tap 9118, coefficient 0.484375) finds nothing. They are all there — encoded.
/// The engine's effect units are programmed through a register file (`param_set_msb` /
/// `param_set_lsb` at `0x18008f340` / `0x18008f4e0`), and the programs are data: per-character rows
/// of ring taps stored *relative to a base the loader adds* (`fx_load_dsp_preset` at `0x18007f840`,
/// base `0x2000` for every GS macro), and coefficient rows in a signed-14-bit floating encoding
/// (`fixed14_to_float` at `0x180085170`). Add the base and decode the fixed point, and every
/// harvested number falls out — verified bit for bit against a live-engine harvest across all eight
/// reverb macros, all eight chorus macros and both GM defaults, with no residual.
///
/// The register program itself (`fx_apply_reverb_chorus_type` at `0x18007f490`,
/// `chorus_apply_params` at `0x180004090`) contributes the handful of values that are computed
/// rather than stored: the injection tap from the pre-delay byte, the damping pair from the pre-LPF
/// ladder, the tank feedback gain from the reverb time, and the chorus tap geometry from the
/// macro's delay, rate and depth bytes. Those laws are transcribed here with their `float`
/// arithmetic intact, so the results carry the same rounding the engine's do.
///
/// Every offset is pinned to the SOUND Canvas VA 1.1.6 build, like the rest of the ROM readers.
class EffectProgrammer {
public:
    /// Number of GS delay macros.
    static constexpr int delay_type_count = 10;

    /// Bytes per delay preset row.
    static constexpr int delay_preset_stride = 10;

    /// Computes the complete preset set from an open ROM image.
    ///
    /// Throws `std::runtime_error` if a table does not look like the expected data.
    [[nodiscard]] static EffectPresets compute(const RomImage& rom);

    /// Reads the ten GS delay presets straight out of the DLL's own preset table.
    [[nodiscard]] static std::vector<std::array<int, delay_preset_stride>>
    read_delay_presets(const RomImage& rom);
};

} // namespace ts
