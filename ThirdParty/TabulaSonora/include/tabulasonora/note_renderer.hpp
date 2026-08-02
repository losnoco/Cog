#pragma once

#include "tabulasonora/drum_kit_table.hpp"
#include "tabulasonora/engine_noise.hpp"
#include "tabulasonora/interpolator.hpp"
#include "tabulasonora/lfo_engine.hpp"
#include "tabulasonora/patch_directory.hpp"
#include "tabulasonora/pitch_chain.hpp"
#include "tabulasonora/sampler.hpp"
#include "tabulasonora/table_set.hpp"
#include "tabulasonora/tva_chain.hpp"
#include "tabulasonora/tvf_chain.hpp"
#include "tabulasonora/wave_rom.hpp"

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ts {

/// One rendered note: stereo output plus the pre-pan mono the send buses are fed from.
struct RenderedNote {
    std::vector<float> left;
    std::vector<float> right;
    /// Pre-pan sum of the partials — the pan-independent send source.
    std::vector<float> mono;
    /// The tone's name.
    std::string name;
};

/// Renders one note by assembling the whole voice chain: sampler, pitch, filter, amplitude and pan.
///
/// Partials *sum*. Each is an independent voice dispatched into the same accumulation buffer, and
/// there is no divide-by-count anywhere in the mix path — averaging would silently halve every
/// two-partial patch relative to a one-partial one.
///
/// A note's timbre is latched at note-on, but volume and pitch keep tracking their controllers,
/// because the engine re-reads those each block. That is why they arrive here as optional
/// per-sample curves rather than scalars.
class NoteRenderer {
public:
    /// Samples per control tick.
    static constexpr int control_block = 320;

    /// Internal sample rate.
    static constexpr int sample_rate = 32000;

    /// Largest drum ring scale, so a caller can bound its buffers.
    static constexpr double max_drum_ring_scale = 16.0;

    /// Creates a note renderer over an open ROM image, which must outlive it.
    ///
    /// Tables are read once here, but wave data is read on demand, so releasing the image early
    /// fails at the first note rather than at construction.
    explicit NoteRenderer(const RomImage& rom);

    NoteRenderer(NoteRenderer&&) noexcept;
    NoteRenderer& operator=(NoteRenderer&&) noexcept;
    NoteRenderer(const NoteRenderer&) = delete;
    NoteRenderer& operator=(const NoteRenderer&) = delete;
    ~NoteRenderer();

    /// Optional per-note controller curves.
    ///
    /// A note's timbre is latched at note-on, but these keep tracking while it sounds.
    struct Controllers {
        /// Per-sample part volume; empty for unity.
        std::span<const double> volume;
        /// Per-sample bend and tune in milli-semitones; empty for none.
        std::span<const double> pitch_add;
        /// Mod-wheel depth per control tick in milli-semitones; empty when the wheel is untouched.
        /// This adds vibrato depth to LFO1 while the note sounds.
        std::span<const double> mod_wheel_per_tick;
    };

    /// The static tables this renderer loaded.
    [[nodiscard]] const TableSet& tables() const noexcept;

    /// The engine's shared pseudo-random source.
    ///
    /// One generator serves the whole engine — pitch start jitter, random pan, and the random LFO
    /// waveforms all draw from it — so it lives here rather than inside any one chain.
    [[nodiscard]] EngineNoise& noise() noexcept;

    /// The patch directory.
    [[nodiscard]] const PatchDirectory& directory() const noexcept;

    /// The drum kits.
    [[nodiscard]] const DrumKitTable& drums() const noexcept;

    /// The 4-tap resampler.
    ///
    /// The chains below are exposed so a host can assemble the voice itself — which is what the
    /// real-time block loop does, sharing one loaded set of tables and one wave cache with this
    /// renderer rather than duplicating either.
    [[nodiscard]] const Interpolator& interpolator() const noexcept;

    /// The wave decoder and player.
    [[nodiscard]] Sampler& sampler() noexcept;

    /// The shared segment-rate machine, which also decodes the envelope hold clock.
    [[nodiscard]] const EnvelopeMachine& envelopes() const noexcept;

    /// The amplitude chain.
    [[nodiscard]] const TvaChain& tva() const noexcept;

    /// The filter chain.
    [[nodiscard]] const TvfChain& tvf() const noexcept;

    /// The pitch chain.
    [[nodiscard]] const PitchChain& pitch() const noexcept;

    /// The two LFO engines.
    [[nodiscard]] const LfoEngine& lfo() const noexcept;

    /// The pan law.
    [[nodiscard]] const PanLaw& pan() const noexcept;

    /// Renders one melodic note.
    [[nodiscard]] RenderedNote render_note(int program,
                                           int note,
                                           int velocity,
                                           double hold_seconds,
                                           double tail_seconds,
                                           ToneMap map = ToneMap::sc8820,
                                           int bank = 0,
                                           int part_pan = 0x40,
                                           const Controllers& controllers = {});

    /// How much longer a hit needs than the nominal ring, given its coarse pitch.
    ///
    /// The ring exists because a drum ignores note-off, so the renderer has to decide for itself
    /// how long to run. A fixed window is wrong once a key is pitched down: the sample plays
    /// proportionally slower, so the hit outlasts it and is cut off mid-decay. Measured on the real
    /// DLL, the splash at NRPN 18h = 24 takes 4.68 s to fall 40 dB against 1.15 s untouched — four
    /// times the nominal 1.8 s ring. The envelope, not this window, is what should end the note.
    [[nodiscard]] static double drum_ring_scale(const DrumKey& key) noexcept;

    /// How much longer a hit needs than the nominal ring, before it is rendered.
    ///
    /// Lets a caller size its own buffers to the same window the render will use.
    [[nodiscard]] double drum_ring_scale(int note, int kit, int drum_pitch) const;

    /// Renders one drum hit.
    ///
    /// The note does not transpose the sample: the tone resolves at key 60 and the kit's
    /// coarse-pitch plane supplies an offset at half strength. The kit level is not part of the
    /// voice gain — it enters downstream, squared.
    [[nodiscard]] RenderedNote render_drum_note(int note,
                                                int velocity,
                                                double ring_seconds,
                                                double tail_seconds = 0.4,
                                                int kit = 0,
                                                std::span<const double> volume = {},
                                                int drum_pitch = 0,
                                                std::optional<int> drum_pan = std::nullopt);

    /// The key a drum hit's envelope rates are key-followed by.
    ///
    /// Not 60, and not the MIDI note. The engine keeps this at `voice+0x161`, filled by
    /// `voice_trigger_partials` as the note plus a transpose, and on a drum part the sum works out
    /// to the kit's own coarse-pitch plane. Read back off the DLL for the 61 sounding keys of the
    /// SC-55 standard kit, the plane equals the engine's index for all 61.
    ///
    /// The NRPN moves it a whole semitone per step, where the same step moves the *pitch* plane by
    /// two — so the offset is added here rather than reading the modified plane back.
    [[nodiscard]] static int envelope_rate_key(const DrumKey& key, int drum_pitch) noexcept;

    /// Holds control-rate values across their block, as the engine does.
    [[nodiscard]] static std::vector<double> expand(std::span<const double> ticks,
                                                    std::size_t sample_count);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ts
