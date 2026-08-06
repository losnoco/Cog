#pragma once

// The render vocabulary -- channel mask, options and result -- that outlived the whole-note
// renderer these types were declared in. See the verification article for why that path was
// retired.

#include "tabulasonora/note_renderer.hpp"
#include "tabulasonora/sequence.hpp"

#include <atomic>
#include <filesystem>
#include <optional>
#include <vector>

namespace ts {

/// Per-channel mute and solo.
///
/// Read live so a mixer can change it while sound is running, and read where the engine *mixes*
/// rather than where it starts notes: a part goes quiet on the next block and comes back on the
/// one after, and a muted part goes on consuming polyphony exactly as an audible one does.
///
/// The flags are atomic because this is the one place in the library where two threads meet: a UI
/// thread sets them while the render thread reads them on every note-on. Relaxed ordering is enough
/// -- each flag stands alone, nothing is published through it, and the only requirement is that a
/// change is eventually seen and never seen half-written. That is what the reference build's
/// `Volatile` bought too.
class ChannelMask {
public:
    /// Parts the mask covers: sixty-four, `port * 16 + channel`.
    ///
    /// Sixteen for as long as the engine had one port. A four-port file addresses sixty-four
    /// parts and every one of them has to be mutable, or the mixer would silently refuse to touch
    /// three quarters of a score. Files that use one port only ever index the first sixteen.
    static constexpr int channel_count = 64;

    /// Whether a channel should sound.
    ///
    /// Solo wins: once anything is soloed, only soloed channels are audible regardless of mutes.
    [[nodiscard]] bool is_audible(int channel) const noexcept;

    [[nodiscard]] bool is_muted(int channel) const noexcept;
    [[nodiscard]] bool is_soloed(int channel) const noexcept;
    [[nodiscard]] bool any_soloed() const noexcept;

    void set_muted(int channel, bool muted) noexcept;
    void set_soloed(int channel, bool soloed) noexcept;
    void reset() noexcept;

    /// Whether nothing is muted or soloed, so every channel sounds.
    [[nodiscard]] bool is_default() const noexcept;

private:
    std::array<std::atomic<bool>, channel_count> muted_{};
    std::array<std::atomic<bool>, channel_count> soloed_{};
};

/// How a sequence should be rendered.
struct RenderOptions {
    /// Which vintage's tone map program changes resolve against.
    ToneMap map = ToneMap::sc8820;

    /// Time to render past the last note.
    double tail_seconds = 2.2;

    /// Linear gain applied to the finished mix.
    double output_gain = 1.0;

    /// MIDI channel routed to the drum path.
    int drum_channel = 9;

    /// Drum map row, or nothing to take the row the vintage selects.
    ///
    /// Nothing in a MIDI file can reach this: the module picks it from the part's internal bank
    /// code, and that translation is not reversed. Both renderers take it, and they must, or one
    /// file would render as two different arrangements.
    std::optional<int> drum_map_row;

    /// The row this actually resolves to.
    [[nodiscard]] int effective_drum_map_row() const noexcept
    {
        if (drum_map_row) {
            return *drum_map_row;
        }
        return DrumKitTable::row_for_map(map).value_or(0);
    }

    bool reverb = true;
    bool chorus = true;
    bool delay = true;
    bool efx = true;

    /// Force an effect type instead of taking it from the stream.
    std::optional<int> reverb_type;
    std::optional<int> chorus_type;
    std::optional<int> delay_type;

    /// Stop rendering at this many seconds, if given.
    std::optional<double> end_seconds;

    /// Per-channel mute and solo, or nothing for everything audible.
    const ChannelMask* channels = nullptr;

};

/// A finished render.
struct RenderResult {
    std::vector<float> left;
    std::vector<float> right;
    int sample_rate = NoteRenderer::sample_rate;
    /// How many notes actually rendered.
    int note_count = 0;
    /// Peak absolute sample across both channels, after the output gain.
    float peak = 0.0F;
};

} // namespace ts
