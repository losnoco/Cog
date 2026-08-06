#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace ts::sf2 {

/// The SF2 generators this exporter emits.
///
/// Numbered as the specification numbers them. Only the ones actually written are listed; the value
/// is what goes in the `sfGenOper` field, so this is not an internal enumeration to be renumbered.
enum class Gen : std::uint16_t {
    start_addrs_offset = 0,
    end_addrs_offset = 1,
    startloop_addrs_offset = 2,
    endloop_addrs_offset = 3,
    mod_lfo_to_pitch = 5,
    vib_lfo_to_pitch = 6,
    mod_env_to_pitch = 7,
    initial_filter_fc = 8,
    initial_filter_q = 9,
    mod_lfo_to_filter_fc = 10,
    mod_env_to_filter_fc = 11,
    mod_lfo_to_volume = 13,
    chorus_effects_send = 15,
    reverb_effects_send = 16,
    pan = 17,
    delay_mod_lfo = 21,
    freq_mod_lfo = 22,
    delay_vib_lfo = 23,
    freq_vib_lfo = 24,
    delay_mod_env = 25,
    attack_mod_env = 26,
    hold_mod_env = 27,
    decay_mod_env = 28,
    sustain_mod_env = 29,
    release_mod_env = 30,
    delay_vol_env = 33,
    attack_vol_env = 34,
    hold_vol_env = 35,
    decay_vol_env = 36,
    sustain_vol_env = 37,
    release_vol_env = 38,
    keynum_to_vol_env_hold = 39,
    keynum_to_vol_env_decay = 40,
    instrument = 41,
    key_range = 43,
    vel_range = 44,
    initial_attenuation = 48,
    coarse_tune = 51,
    fine_tune = 52,
    sample_id = 53,
    sample_modes = 54,
    scale_tuning = 56,
    exclusive_class = 57,
    overriding_root_key = 58,

    /// spessasynth's extended generators, past the specification's 60.
    ///
    /// They give the vibrato LFO the amplitude and filter destinations standard SF2 denies it,
    /// which is what lets the tone-common LFO1 be represented at all. A conforming reader stops at
    /// 60 and ignores these, so nothing that depends on them may be load-bearing.
    vib_lfo_rate = 62,
    vib_lfo_amplitude_depth = 63,
    vib_lfo_to_filter_fc = 64,
    mod_lfo_rate = 65,
    mod_lfo_amplitude_depth = 66,
};

/// `sampleModes` values. SF2 has no ping-pong and no reverse; both are baked into the sample data.
enum class LoopMode : std::int16_t {
    no_loop = 0,
    loop = 1,
    loop_until_release = 3,
};

/// One generator setting inside a zone.
struct Generator {
    Gen oper = Gen::instrument;
    std::int16_t amount = 0;

    /// A ranged generator packs its bounds into the two bytes of the amount word.
    [[nodiscard]] static Generator range(Gen oper, int low, int high) noexcept
    {
        return Generator{oper, static_cast<std::int16_t>((high << 8) | (low & 0xFF))};
    }

    [[nodiscard]] static Generator value(Gen oper, int amount) noexcept
    {
        return Generator{oper, static_cast<std::int16_t>(amount)};
    }
};

/// One modulator record, written verbatim into `pmod`/`imod`.
struct Modulator {
    std::uint16_t source = 0;
    Gen destination = Gen::initial_attenuation;
    std::int16_t amount = 0;
    std::uint16_t amount_source = 0;
    std::uint16_t transform = 0;
};

/// A zone: a set of generators and modulators, terminated by `instrument` or `sample_id`.
///
/// A zone carrying neither is the global zone, and must come first in its bag.
struct Zone {
    std::vector<Generator> generators;
    std::vector<Modulator> modulators;
};

/// How the sample data is stored.
enum class Codec {
    /// Plain 16-bit PCM in `smpl`. Every reader handles it; nothing else is exact.
    pcm,
    /// Per-sample FLAC streams, SF3-style. Lossless, and **spessasynth only** — canonical SF3 is
    /// Vorbis, so no other reader recognises a `fLaC` stream here.
    flac,
    /// Per-sample Ogg Vorbis streams. Lossy, and what SF3 actually specifies, so it travels.
    vorbis,
};

/// Whether this build can write a codec. The compressed ones are optional dependencies.
[[nodiscard]] bool codec_available(Codec codec) noexcept;

/// Encodes one mono sample run. Returns empty when the encoder is unavailable or fails.
[[nodiscard]] std::vector<std::uint8_t> encode_flac(std::span<const float> samples,
                                                    int sample_rate);
[[nodiscard]] std::vector<std::uint8_t>
encode_vorbis(std::span<const float> samples, int sample_rate, float quality = 0.5F);

/// One sample header. Offsets are in samples into the pooled PCM.
struct Sample {
    std::string name;
    std::uint32_t start = 0;
    std::uint32_t end = 0;
    std::uint32_t loop_start = 0;
    std::uint32_t loop_end = 0;
    std::uint32_t sample_rate = 32000;
    std::uint8_t original_key = 60;
    std::int8_t correction = 0;
    std::uint16_t link = 0;
    std::uint16_t type = 1; // mono
};

struct Instrument {
    std::string name;
    std::vector<Zone> zones;
};

struct Preset {
    std::string name;
    std::uint16_t program = 0;
    /// The packed `wBank` word: MSB in the low 7 bits, LSB in the high byte, bit 7 the drum flag.
    std::uint16_t bank = 0;
    std::vector<Zone> zones;
};

/// A complete bank, ready to be written.
struct Bank {
    std::string name = "Sound Canvas";
    std::string engine = "EMU8000";
    std::string software;
    std::string comment;

    /// The pooled PCM every sample indexes, at `Sample::sample_rate`.
    std::vector<float> pool;

    std::vector<Sample> samples;
    std::vector<Instrument> instruments;
    std::vector<Preset> presets;

    /// Replaces the reader's built-in default modulator set when non-empty (the `DMOD` chunk).
    std::vector<Modulator> default_modulators;
};

/// What a written file needed, so a caller can report whether it stayed inside SF2's limits.
struct WriteReport {
    std::size_t sample_count = 0;
    std::size_t instrument_count = 0;
    std::size_t preset_count = 0;
    std::size_t ibag_count = 0;
    std::size_t pbag_count = 0;
    std::size_t igen_count = 0;
    std::size_t pgen_count = 0;
    std::int64_t pcm_bytes = 0;
    std::int64_t file_bytes = 0;
    Codec codec = Codec::pcm;

    /// Whether any index exceeded 16 bits, i.e. whether `xdta` was load-bearing rather than
    /// merely present.
    [[nodiscard]] bool needed_xdta() const noexcept
    {
        return igen_count > 0xFFFF || pgen_count > 0xFFFF || ibag_count > 0xFFFF
               || pbag_count > 0xFFFF;
    }
};

/// Writes a bank as an SF2 file, always including the `xdta` extension chunk.
///
/// `xdta` is not conditional on the counts. The Sound Canvas set needs it — `igen` alone runs to
/// several hundred thousand records against SF2's 16-bit bag indices — and a writer that emits it
/// only when it overflows is a writer whose extension path is exercised by nothing but the largest
/// input. It is written for every bank so the common case tests it.
///
/// Throws `std::runtime_error` if the file cannot be written.
WriteReport write(const std::filesystem::path& path, const Bank& bank,
                  Codec codec = Codec::pcm);

} // namespace ts::sf2
