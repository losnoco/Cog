#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ts {

/// One allpass stage: a read tap, a write tap and the two coefficients around them.
struct AllpassStage {
    int write_tap = 0;
    int read_tap = 0;
    double coef_a = 0.0;
    double coef_b = 0.0;
};

/// One reverb tank's eight ring offsets, resolved from their names once.
///
/// `presets.json` carries these as a name-keyed object because that is the shape the harvester
/// dumps. Reading them that way is not: indexing by name inside the sample loop cost sixteen string
/// hashes per sample — four per tank plus eight in the output taps — which measured 44.8 ms per ten
/// seconds of audio against the chorus's 1.1 ms for comparable arithmetic. They are flattened here,
/// once, at load.
struct TankTaps {
    int tap10 = 0;
    int tap14 = 0;
    int tap18 = 0;
    int tap1c = 0;
    int tap20 = 0;
    int tap24 = 0;
    int tap28 = 0;
    int tap2c = 0;
};

/// One reverb tank.
struct ReverbTank {
    TankTaps taps;
    double coef_a = 0.0;
    double coef_b = 0.0;
};

/// The two nested allpasses inside each tank.
struct TankAllpasses {
    AllpassStage a0;
    AllpassStage a1;
    AllpassStage b0;
    AllpassStage b1;
};

/// Coefficients for one GS reverb type.
struct ReverbPreset {
    std::array<AllpassStage, 4> diffusers;
    ReverbTank tank_a;
    ReverbTank tank_b;
    TankAllpasses tank_allpasses;
    int injection_tap = 0;
    double damp_feedback = 0.0;
    double damp_input = 0.0;
    double gain_input = 0.0;
    double gain_injection = 0.0;
    double gain_feedback = 0.0;
    double gain_output = 0.0;
};

/// Coefficients for one GS chorus type.
struct ChorusPreset {
    int lfo_increment = 0;
    double lpf_a = 0.0;
    double lpf_b = 0.0;
    int tap1_depth = 0;
    int tap1_base = 0;
    int tap2_depth = 0;
    int tap2_base = 0;
    double feedback = 0.0;
    double gain_write = 0.0;
    double gain_tap = 0.0;
};

/// The eight GS reverb types plus the power-on default.
struct ReverbPresets {
    /// Send-bus gain at controller 127.
    static constexpr double send_at_full_scale = 1.02;

    std::vector<std::string> type_names;
    ReverbPreset defaults;
    std::vector<ReverbPreset> types;
};

/// The eight GS chorus types plus the power-on default.
struct ChorusPresets {
    /// Send-bus gain at controller 127.
    static constexpr double send_at_full_scale = 0.3428;

    std::vector<std::string> type_names;
    ChorusPreset defaults;
    std::vector<ChorusPreset> types;
};

/// The ten GS delay types and the two published conversion tables.
struct DelayPresets {
    /// Send-bus gain at full scale.
    static constexpr double send_at_full_scale = 0.356;

    /// Fixed input pre-delay ahead of the feedback line, in samples — 60 ms at 32 kHz.
    ///
    /// Measured from first arrival; it is not in the preset table.
    static constexpr int pre_delay_samples = 1920;

    /// Pre-lowpass coefficient ladder; index 0 bypasses, which every preset uses.
    static constexpr std::array<double, 8> pre_low_pass_coefficients{
        0.0, 0.10, 0.18, 0.28, 0.40, 0.55, 0.70, 0.84};

    std::vector<std::string> type_names;
    std::vector<double> time_milliseconds;
    std::vector<double> ratio_percent;
    std::vector<std::array<int, 10>> raw_presets;
};

class RomImage;

/// The coefficient sets for the three GS send effects.
///
/// Computed from the DLL rather than fitted to audio: `EffectProgrammer` decodes the reverb and
/// chorus coefficients out of the engine's own preset tables, and the delay presets come straight
/// from its preset table. Within each effect the topology is identical across all types — only
/// these numbers change, which is why one implementation runs every type unchanged.
class EffectPresets {
public:
    /// Environment variable that overrides where presets are read from.
    static constexpr std::string_view path_variable = "TABULASONORA_PRESETS";

    /// The presets in use.
    ///
    /// Throws `std::runtime_error` if none have been supplied and none could be found. Ordinarily
    /// nothing needs to be done: opening a `NoteRenderer` computes them from its ROM.
    [[nodiscard]] static const EffectPresets& defaults();

    /// Computes the presets from the DLL, unless a set is already in force.
    ///
    /// An explicit `use`, an environment override and a file beside the executable all take
    /// precedence — they exist so a host can pin a harvested set, and a computed set must not
    /// silently replace one. In the ordinary case none of those is present and this fills in the
    /// computed presets, which match a live harvest bit for bit — see `EffectProgrammer`.
    static void ensure_from(const RomImage& rom);

    /// Supplies presets explicitly, for a host that manages its own data files.
    static void use(EffectPresets presets);

    /// Whether presets are available, without throwing.
    [[nodiscard]] static bool available();

    /// Assembles a preset set from computed parts.
    [[nodiscard]] static EffectPresets
    from_parts(ReverbPresets reverb, ChorusPresets chorus, DelayPresets delay);

    /// Parses presets from a JSON document.
    ///
    /// Throws `std::runtime_error` if the document is not a well-formed preset set.
    [[nodiscard]] static EffectPresets parse(std::string_view json);

    [[nodiscard]] const ReverbPresets& reverb() const noexcept { return reverb_; }

    [[nodiscard]] const ChorusPresets& chorus() const noexcept { return chorus_; }

    [[nodiscard]] const DelayPresets& delay() const noexcept { return delay_; }

private:
    ReverbPresets reverb_;
    ChorusPresets chorus_;
    DelayPresets delay_;
};

} // namespace ts
