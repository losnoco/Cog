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

    /// Pre-lowpass coefficient ladder; index 0 bypasses, which every preset uses.
    static constexpr std::array<double, 8> pre_low_pass_coefficients{
        0.0, 0.10, 0.18, 0.28, 0.40, 0.55, 0.70, 0.84};

    std::vector<std::string> type_names;
    std::vector<double> time_milliseconds;
    std::vector<double> ratio_percent;
    std::vector<std::array<int, 10>> raw_presets;
};

/// One shelving band: `H(z) = (b0 + b1·z⁻¹) / (1 − a1·z⁻¹)`.
///
/// A one-pole shelf, not a biquad. At 0 dB the stored row is exactly `{1, −a, a}`, which makes the
/// numerator and denominator identical and the response algebraically unity — the flat setting is
/// exact rather than merely close, and that identity is the check that the three coefficients have
/// been read in the right order.
struct EqBand {
    double b0 = 1.0;
    double b1 = 0.0;
    double a1 = 0.0;
};

/// The four-band EQ block's coefficient tables — two bands, two corner frequencies each.
///
/// The engine computes nothing here: every gain setting is a stored row, and the SysEx values index
/// straight into them. The corner is not a parameter but a choice of table, which is why `40 02 00`
/// and `40 02 02` take 0 or 1 and nothing else.
struct EqPresets {
    /// Gain settings per band: `40 02 01` and `40 02 03` accept 0x34–0x4C, which is −12…+12 dB.
    static constexpr int gain_count = 25;

    /// The lowest gain byte, subtracted to index the tables.
    static constexpr int gain_base = 0x34;

    /// Corner frequencies each band can be switched between, in Hz, for reporting only — the
    /// coefficients carry the real thing.
    static constexpr std::array<int, 2> low_frequencies{200, 400};
    static constexpr std::array<int, 2> high_frequencies{3000, 6000};

    /// `[frequency][gain byte − 0x34]`.
    std::array<std::array<EqBand, gain_count>, 2> low{};
    std::array<std::array<EqBand, gain_count>, 2> high{};

    /// The band for a SysEx frequency and gain pair, clamped the way the engine clamps them.
    [[nodiscard]] const EqBand& low_band(int frequency, int gain) const noexcept;
    [[nodiscard]] const EqBand& high_band(int frequency, int gain) const noexcept;
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
    from_parts(ReverbPresets reverb, ChorusPresets chorus, DelayPresets delay, EqPresets eq = {});

    /// Parses presets from a JSON document.
    ///
    /// Throws `std::runtime_error` if the document is not a well-formed preset set.
    [[nodiscard]] static EffectPresets parse(std::string_view json);

    [[nodiscard]] const ReverbPresets& reverb() const noexcept { return reverb_; }

    [[nodiscard]] const ChorusPresets& chorus() const noexcept { return chorus_; }

    [[nodiscard]] const DelayPresets& delay() const noexcept { return delay_; }

    /// The EQ coefficient tables.
    ///
    /// These are read from the DLL and are not part of the JSON round trip, unlike the other three:
    /// there is nothing to harvest, because the engine stores every setting rather than deriving
    /// it. A set parsed from a pinned `presets.json` therefore carries no EQ, and `is_present`
    /// says so — the EQ stage passes audio through untouched rather than pretending to be flat.
    [[nodiscard]] const EqPresets& eq() const noexcept { return eq_; }

    /// Whether EQ coefficients are available.
    [[nodiscard]] bool has_eq() const noexcept { return has_eq_; }

private:
    ReverbPresets reverb_;
    ChorusPresets chorus_;
    DelayPresets delay_;
    EqPresets eq_;
    bool has_eq_ = false;
};

} // namespace ts
