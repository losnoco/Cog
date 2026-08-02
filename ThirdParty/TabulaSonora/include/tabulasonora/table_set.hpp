#pragma once

#include "tabulasonora/rom_image.hpp"
#include "tabulasonora/table_manifest.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ts {

/// The static tables the engine needs, loaded from `SCCore.dll` or from a directory of extracted
/// `.bin` slices.
///
/// Accessor names follow the project's symbol labels (`g_amp_curve_hi` becomes `amp_curve_hi`) so
/// the code greps cleanly against the reverse-engineering notes. Those labels are behavioural
/// hypotheses, not Roland's own names.
///
/// Element counts are derived from each table's byte length, not from the manifest's `shape` string
/// -- several shapes record a byte count where an element count would be expected
/// (`curve_level_2a00.bin` is 256 bytes holding 128 `uint16_t` entries, yet its shape reads `256`).
///
/// The set owns its bytes. Byte-typed tables are spans into that storage and typed tables are
/// converted copies, so every view here stays valid for the lifetime of the set and none of them
/// depends on the `RomImage` outliving it.
class TableSet {
public:
    /// Cache file names for every table in the manifest.
    struct names {
        static constexpr std::string_view amp_curve_hi = "curve_amp_hi_2ba0.bin";
        static constexpr std::string_view amp_curve_lo = "curve_amp_lo_2da0.bin";
        static constexpr std::string_view tvf_env_depth = "curve_filtenvdepth_2f00.bin";
        static constexpr std::string_view filter_type_coef = "curve_filttype_987b00.bin";
        static constexpr std::string_view level_curve = "curve_level_2a00.bin";
        static constexpr std::string_view pitch_bias = "curve_pitchbias_2890.bin";
        static constexpr std::string_view pitch_depth_vs = "curve_pitchdepthvs_28d0.bin";
        static constexpr std::string_view pitch_env = "curve_pitchenv_2578.bin";
        static constexpr std::string_view env_rate_out = "curve_rateout_3060.bin";
        static constexpr std::string_view reso_curve = "curve_reso_2b88.bin";
        static constexpr std::string_view env_scale_curve = "curve_scale_28e8.bin";
        static constexpr std::string_view rate_curve = "curve_segrate_2900.bin";
        static constexpr std::string_view vel_curve = "curve_vel_2b00.bin";
        static constexpr std::string_view vel_sens = "curve_velsens_01520.bin";
        static constexpr std::string_view vel_xfade = "curve_velxfade_01020.bin";
        static constexpr std::string_view env_shape = "env_shape_7a90.bin";
        static constexpr std::string_view env_start_phase = "env_startphase_7a30.bin";
        static constexpr std::string_view interp_coef = "interp_coef_a0f210.bin";
        static constexpr std::string_view kf_pitch = "kf_pitch_01b20.bin";
        static constexpr std::string_view kf_pitch_rate0 = "kf_pitchrate0_01f20.bin";
        static constexpr std::string_view kf_pitch_rate1 = "kf_pitchrate1_01aa0.bin";
        static constexpr std::string_view kf_tva_level = "kf_tvalevel_026a0.bin";
        static constexpr std::string_view kf_tva_level2 = "kf_tvalevel2_01fa0.bin";
        static constexpr std::string_view kf_tva_rate0 = "kf_tvarate0_023a0.bin";
        static constexpr std::string_view kf_tva_rate1 = "kf_tvarate1_020a0.bin";
        static constexpr std::string_view kf_tvf_env = "kf_tvfenv_02a20.bin";
        static constexpr std::string_view kf_tvf_rate = "kf_tvfrate_03920.bin";
        static constexpr std::string_view layered = "layered_1896690.bin";
        static constexpr std::string_view lfo_cents = "lfo_cents_2690.bin";
        static constexpr std::string_view lfo_delay = "lfo_delay_a2590.bin";
        static constexpr std::string_view lfo_rate = "lfo_rate_2790.bin";
        static constexpr std::string_view portamento_step = "porta_step_7800.bin";
        static constexpr std::string_view lfo_wave = "lfo_wave_1740.bin";
        static constexpr std::string_view lfo_wave_bank = "lfo_wavebank_a17f0.bin";
        static constexpr std::string_view lfo_wave_map = "lfo_wavemap_87ae0.bin";
        static constexpr std::string_view dir_lut1 = "lut1_2e30.bin";
        static constexpr std::string_view dir_lut2 = "lut2_28b0.bin";
        static constexpr std::string_view dir_lut3 = "lut3_32b0.bin";
        static constexpr std::string_view multisample = "multisample_a.bin";
        static constexpr std::string_view pan = "pan_a2fa1.bin";
        static constexpr std::string_view ramp_divider = "ramp_divider_a03e00.bin";
        static constexpr std::string_view ramp_exp = "ramp_exp_986420.bin";
        static constexpr std::string_view ramp_flagword = "ramp_flagword_a84d8.bin";
        static constexpr std::string_view tone = "tone_a.bin";
        static constexpr std::string_view tvf_cutoff_ceil = "tvf_ceil_a7ed0.bin";
        static constexpr std::string_view tvf_q_lp = "tvf_q_lp_a7cd0.bin";
        static constexpr std::string_view tvf_q_t6 = "tvf_q_t6_a7fd0.bin";
        static constexpr std::string_view tvf_cutoff_warp = "tvf_warp_a83d0.bin";
        static constexpr std::string_view wavedesc = "wavedesc_a.bin";
    };

    /// Loads every table by slicing the DLL at its manifest offset.
    [[nodiscard]] static TableSet from_rom(const RomImage& rom);

    /// Loads every table from a directory of extracted `.bin` slices.
    ///
    /// Throws `std::runtime_error` if a cache file is missing or has the wrong length. A length
    /// mismatch means the cache was extracted against an older manifest and must be regenerated;
    /// reading it anyway would silently misalign every table after it.
    [[nodiscard]] static TableSet from_cache_directory(const std::filesystem::path& directory,
                                                       const TableManifest* manifest = nullptr);

    TableSet(TableSet&&) noexcept;
    TableSet& operator=(TableSet&&) noexcept;
    TableSet(const TableSet&) = delete;
    TableSet& operator=(const TableSet&) = delete;
    ~TableSet();

    /// The offset map these tables were loaded through.
    [[nodiscard]] const TableManifest& manifest() const noexcept { return *manifest_; }

    /// Returns the untouched bytes of one table; see `names`.
    ///
    /// Throws `std::out_of_range` if no such table was loaded.
    [[nodiscard]] std::span<const std::uint8_t> raw(std::string_view name) const;

    /// `g_amp_curve_hi` — 256 entries, high half of the log-to-linear amplitude lookup.
    [[nodiscard]] std::span<const std::uint16_t> amp_curve_hi() const noexcept
    {
        return amp_curve_hi_;
    }

    /// `g_amp_curve_lo` — 256 entries, low half of the log-to-linear amplitude lookup.
    [[nodiscard]] std::span<const std::uint16_t> amp_curve_lo() const noexcept
    {
        return amp_curve_lo_;
    }

    /// `g_tvf_env_depth` — TVF env-depth conversion region, addressed by VA-relative offset.
    [[nodiscard]] std::span<const std::uint8_t> tvf_env_depth() const noexcept
    {
        return tvf_env_depth_;
    }

    /// `g_filter_type_coef` — 16 entries; filter TYPE byte to tap/coefficient word.
    [[nodiscard]] std::span<const std::uint32_t> filter_type_coef() const noexcept
    {
        return filter_type_coef_;
    }

    /// `g_level_curve` — 128 entries; envelope stage byte to 16-bit log attenuation.
    [[nodiscard]] std::span<const std::uint16_t> level_curve() const noexcept
    {
        return level_curve_;
    }

    /// `g_pitch_bias` — 128 entries; pitch-envelope bias sub-table.
    [[nodiscard]] std::span<const std::uint8_t> pitch_bias() const noexcept { return pitch_bias_; }

    /// `g_pitch_depth_vs` — 64 entries; pitch-envelope depth versus velocity.
    [[nodiscard]] std::span<const std::uint16_t> pitch_depth_vs() const noexcept
    {
        return pitch_depth_vs_;
    }

    /// `g_pitch_env` — pitch-envelope conversion region, addressed by VA-relative offset.
    [[nodiscard]] std::span<const std::uint8_t> pitch_env() const noexcept { return pitch_env_; }

    /// `g_env_rate_out` — 512 entries; the rate-scale output curve, exactly 2^((i−0x80)/32) in 8.8.
    [[nodiscard]] std::span<const std::uint16_t> env_rate_out() const noexcept
    {
        return env_rate_out_;
    }

    /// `g_reso_curve` — 64 entries; resonance curve for partial block byte 0x4a.
    [[nodiscard]] std::span<const std::int16_t> reso_curve() const noexcept { return reso_curve_; }

    /// `g_env_scale_curve` — 256 entries; envelope rate/level scale, 0x40-neutral.
    [[nodiscard]] std::span<const std::uint8_t> env_scale_curve() const noexcept
    {
        return env_scale_curve_;
    }

    /// `g_rate_curve` — 128 entries; rate byte to segment time.
    [[nodiscard]] std::span<const std::uint16_t> rate_curve() const noexcept { return rate_curve_; }

    /// `g_vel_curve` — 256 entries; velocity response curve.
    [[nodiscard]] std::span<const std::uint8_t> vel_curve() const noexcept { return vel_curve_; }

    /// `g_vel_sens` — the velocity response curves, 16 rows of 0x80 *bytes*. Row 0 is the identity;
    /// the rest bend velocity progressively toward the top of the range. Like <see
    /// cref="VelXfade"/> the manifest records this as `u16`, but the engine indexes it byte-wise as
    /// `[(selector & 0xf) * 0x80 + velocity]`.
    [[nodiscard]] std::span<const std::uint8_t> vel_sens() const noexcept { return vel_sens_; }

    /// `g_vel_xfade` — the multisample velocity-crossfade curve, 16 rows of 0x80 *bytes*. The
    /// manifest records this as `u16`, but the engine indexes it byte-wise as `[(selector & 0xf) *
    /// 0x80 + position]`.
    [[nodiscard]] std::span<const std::uint8_t> vel_xfade() const noexcept { return vel_xfade_; }

    /// `g_env_shape` — the fast-approach segment shape. Only the first 256 entries (0x200 bytes)
    /// are used; the cache over-reads slightly. Must be interpolated, not looked up bare.
    [[nodiscard]] std::span<const std::uint16_t> env_shape() const noexcept { return env_shape_; }

    /// `g_env_startphase` — 128 entries; per-segment initial phase seed.
    [[nodiscard]] std::span<const std::uint16_t> env_start_phase() const noexcept
    {
        return env_start_phase_;
    }

    /// `g_interp_coef_table` — the 4-tap FIR resample kernel as 128 phases of 4 taps, flattened to
    /// 512 floats. Index a phase as `phase * 4 + tap`. Every phase sums to 1.0.
    [[nodiscard]] std::span<const float> interp_coef() const noexcept { return interp_coef_; }

    /// `g_kf_pitch` — 8 rows of 128; pitch key-follow, indexed `row * 0x80 + key`.
    [[nodiscard]] std::span<const std::int16_t> kf_pitch() const noexcept { return kf_pitch_; }

    /// `g_kf_pitchrate0` — 128×128; pitch-envelope rate key-follow for the segments.
    [[nodiscard]] std::span<const std::uint8_t> kf_pitch_rate0() const noexcept
    {
        return kf_pitch_rate0_;
    }

    /// `g_kf_pitchrate1` — 128×128; pitch-envelope rate key-follow for the release.
    [[nodiscard]] std::span<const std::uint8_t> kf_pitch_rate1() const noexcept
    {
        return kf_pitch_rate1_;
    }

    /// `g_kf_tvalevel` — 128×128; TVA base-level key-follow.
    [[nodiscard]] std::span<const std::uint8_t> kf_tva_level() const noexcept
    {
        return kf_tva_level_;
    }

    /// `g_kf_tvalevel2` — 128×128; TVA level key-follow, second term.
    [[nodiscard]] std::span<const std::uint8_t> kf_tva_level2() const noexcept
    {
        return kf_tva_level2_;
    }

    /// `g_kf_tvarate0` — 128×128; TVA envelope rate key-follow for the segments.
    [[nodiscard]] std::span<const std::uint8_t> kf_tva_rate0() const noexcept
    {
        return kf_tva_rate0_;
    }

    /// `g_kf_tvarate1` — 128×128; TVA envelope rate key-follow for the release.
    [[nodiscard]] std::span<const std::uint8_t> kf_tva_rate1() const noexcept
    {
        return kf_tva_rate1_;
    }

    /// `g_kf_tvfenv` — TVF env-depth key-follow. Only rows 0–15 are used; the cache is an over-read
    /// whose used prefix matches the DLL.
    [[nodiscard]] std::span<const std::int16_t> kf_tvf_env() const noexcept { return kf_tvf_env_; }

    /// `g_kf_tvfrate` — 128×128; TVF envelope rate key-follow. Row 0 doubles as `g_kf_tvfrate2`,
    /// the release table, which covers 98% of partials.
    [[nodiscard]] std::span<const std::uint8_t> kf_tvf_rate() const noexcept
    {
        return kf_tvf_rate_;
    }

    /// `g_layered_table` — 50 alternate-articulation records of 0x18 bytes.
    [[nodiscard]] std::span<const std::uint8_t> layered() const noexcept { return layered_; }

    /// `g_lfo_cents_tbl` — 128 entries; LFO pitch-depth byte to milli-semitones.
    [[nodiscard]] std::span<const std::uint16_t> lfo_cents() const noexcept { return lfo_cents_; }

    /// `g_lfo_delay_tbl` — 128 entries; LFO1 delay byte to delay ticks.
    [[nodiscard]] std::span<const std::uint16_t> lfo_delay() const noexcept { return lfo_delay_; }

    /// `g_lfo_rate_tbl` — 128 entries; LFO1 rate byte to 16-bit phase increment.
    [[nodiscard]] std::span<const std::uint16_t> lfo_rate() const noexcept { return lfo_rate_; }

    /// `g_porta_step` — 128 entries; portamento glide in milli-semitones per control tick, indexed
    /// by the CC#5 time byte. <remarks> The glide is linear in pitch, not in frequency: 0 is
    /// instant, 64 crosses an octave in about half a second, and 127 takes two minutes over the
    /// same octave.
    /// </remarks>
    [[nodiscard]] std::span<const std::uint16_t> portamento_step() const noexcept
    {
        return portamento_step_;
    }

    /// `g_lfo_wave` — the half-sine LFO waveform used by selector 0.
    [[nodiscard]] std::span<const std::uint8_t> lfo_wave() const noexcept { return lfo_wave_; }

    /// `g_lfo_wavebank` — LFO wavetables 8..0x1f, 24 rows of 0x81 entries.
    [[nodiscard]] std::span<const std::int16_t> lfo_wave_bank() const noexcept
    {
        return lfo_wave_bank_;
    }

    /// `g_lfo_wavemap` — 32 entries; waveform selector nibble to table index.
    [[nodiscard]] std::span<const std::uint8_t> lfo_wave_map() const noexcept
    {
        return lfo_wave_map_;
    }

    /// `g_dir_lut1` — 128 entries; tone-map to LUT2 row.
    [[nodiscard]] std::span<const std::uint8_t> dir_lut1() const noexcept { return dir_lut1_; }

    /// `g_dir_lut2` — bank to LUT3 row.
    [[nodiscard]] std::span<const std::uint8_t> dir_lut2() const noexcept { return dir_lut2_; }

    /// `g_dir_lut3` — 32768 entries; program to tone number.
    [[nodiscard]] std::span<const std::uint16_t> dir_lut3() const noexcept { return dir_lut3_; }

    /// `g_multisample_table` — key/velocity zone to wave number, stride 0x8c.
    [[nodiscard]] std::span<const std::uint8_t> multisample() const noexcept
    {
        return multisample_;
    }

    /// `g_pan_tbl` — 128 entries. Left gain is `T[127 − p] / 127`, right is `T[p − 1] / 127`;
    /// centre is 75/127, neither constant-power nor linear.
    [[nodiscard]] std::span<const std::uint8_t> pan() const noexcept { return pan_; }

    /// `g_ramp_divider` — the anti-zipper zero-order-hold masks `[0, 7, 31, 127]`.
    [[nodiscard]] std::span<const std::uint8_t> ramp_divider() const noexcept
    {
        return ramp_divider_;
    }

    /// `g_ramp_exp_tbl` — 257 entries of 2^17 · 2^(i/256), the SVF f-coefficient decode.
    [[nodiscard]] std::span<const std::int32_t> ramp_exp() const noexcept { return ramp_exp_; }

    /// `g_ramp_flagword` — per-destination ramp-divider selector bits.
    [[nodiscard]] std::span<const std::uint8_t> ramp_flagword() const noexcept
    {
        return ramp_flagword_;
    }

    /// `g_tone_table` — tone records of stride 0x100: 0x24 header plus partial blocks of 0x6e.
    [[nodiscard]] std::span<const std::uint8_t> tone() const noexcept { return tone_; }

    /// `g_tvf_cutoff_ceil` — 128 entries; cutoff ceiling by resonance byte.
    [[nodiscard]] std::span<const std::uint16_t> tvf_cutoff_ceil() const noexcept
    {
        return tvf_cutoff_ceil_;
    }

    /// `g_tvf_q_lp` — 256 entries; Q trim for filter type 0 at neutral resonance.
    [[nodiscard]] std::span<const std::uint16_t> tvf_q_lp() const noexcept { return tvf_q_lp_; }

    /// `g_tvf_q_t6` — 256 entries; Q trim for filter type 6.
    [[nodiscard]] std::span<const std::uint16_t> tvf_q_t6() const noexcept { return tvf_q_t6_; }

    /// `g_tvf_cutoff_warp` — 129 entries; the cutoff warp applied at stage C.
    [[nodiscard]] std::span<const std::uint16_t> tvf_cutoff_warp() const noexcept
    {
        return tvf_cutoff_warp_;
    }

    /// `g_wavedesc_table` — wave descriptors of stride 0x16: ROM coordinates, root key, loop.
    [[nodiscard]] std::span<const std::uint8_t> wavedesc() const noexcept { return wavedesc_; }

private:
    TableSet(std::unordered_map<std::string, std::vector<std::uint8_t>> raw,
             const TableManifest& manifest);

    void bind();

    /// Reinterprets a table's bytes as a wider element type.
    ///
    /// Copies rather than casting in place. The upstream C# reinterprets the array directly, which
    /// it can because a managed array is suitably aligned; doing the same here would be both an
    /// alignment and a strict-aliasing violation, and the copy costs about a megabyte once at
    /// start-up. The element count comes from the byte length, so a table whose cache over-reads
    /// simply yields a few extra entries, exactly as upstream.
    template<typename T>
    [[nodiscard]] static std::vector<T> reinterpret_as(std::span<const std::uint8_t> bytes);

    std::unordered_map<std::string, std::vector<std::uint8_t>> raw_;
    const TableManifest* manifest_;

    std::vector<std::uint16_t> amp_curve_hi_;
    std::vector<std::uint16_t> amp_curve_lo_;
    std::span<const std::uint8_t> tvf_env_depth_;
    std::vector<std::uint32_t> filter_type_coef_;
    std::vector<std::uint16_t> level_curve_;
    std::span<const std::uint8_t> pitch_bias_;
    std::vector<std::uint16_t> pitch_depth_vs_;
    std::span<const std::uint8_t> pitch_env_;
    std::vector<std::uint16_t> env_rate_out_;
    std::vector<std::int16_t> reso_curve_;
    std::span<const std::uint8_t> env_scale_curve_;
    std::vector<std::uint16_t> rate_curve_;
    std::span<const std::uint8_t> vel_curve_;
    std::span<const std::uint8_t> vel_sens_;
    std::span<const std::uint8_t> vel_xfade_;
    std::vector<std::uint16_t> env_shape_;
    std::vector<std::uint16_t> env_start_phase_;
    std::vector<float> interp_coef_;
    std::vector<std::int16_t> kf_pitch_;
    std::span<const std::uint8_t> kf_pitch_rate0_;
    std::span<const std::uint8_t> kf_pitch_rate1_;
    std::span<const std::uint8_t> kf_tva_level_;
    std::span<const std::uint8_t> kf_tva_level2_;
    std::span<const std::uint8_t> kf_tva_rate0_;
    std::span<const std::uint8_t> kf_tva_rate1_;
    std::vector<std::int16_t> kf_tvf_env_;
    std::span<const std::uint8_t> kf_tvf_rate_;
    std::span<const std::uint8_t> layered_;
    std::vector<std::uint16_t> lfo_cents_;
    std::vector<std::uint16_t> lfo_delay_;
    std::vector<std::uint16_t> lfo_rate_;
    std::vector<std::uint16_t> portamento_step_;
    std::span<const std::uint8_t> lfo_wave_;
    std::vector<std::int16_t> lfo_wave_bank_;
    std::span<const std::uint8_t> lfo_wave_map_;
    std::span<const std::uint8_t> dir_lut1_;
    std::span<const std::uint8_t> dir_lut2_;
    std::vector<std::uint16_t> dir_lut3_;
    std::span<const std::uint8_t> multisample_;
    std::span<const std::uint8_t> pan_;
    std::span<const std::uint8_t> ramp_divider_;
    std::vector<std::int32_t> ramp_exp_;
    std::span<const std::uint8_t> ramp_flagword_;
    std::span<const std::uint8_t> tone_;
    std::vector<std::uint16_t> tvf_cutoff_ceil_;
    std::vector<std::uint16_t> tvf_q_lp_;
    std::vector<std::uint16_t> tvf_q_t6_;
    std::vector<std::uint16_t> tvf_cutoff_warp_;
    std::span<const std::uint8_t> wavedesc_;
};

} // namespace ts
