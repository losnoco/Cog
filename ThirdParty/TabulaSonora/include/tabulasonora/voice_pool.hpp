#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace ts {

/// What a voice slot is currently doing.
enum class VoiceState {
    /// Unused and available.
    free,
    /// Sounding with the key still down.
    held,
    /// Sounding through its release after note-off.
    releasing,
};

class VoicePool;

/// A handle to one voice slot: an index into the pool's parallel arrays plus the control-rate
/// operations that act on it.
///
/// Deliberately a handle rather than an object holding the render state. The per-voice DSP state
/// lives in struct-of-arrays form on the pool, which is the shape the engine itself uses — it
/// renders voices in groups of four. Objects appear at the control-rate seams, where a virtual call
/// costs nothing; the per-sample loop underneath stays flat.
struct Voice {
    const VoicePool* pool = nullptr;
    int index = 0;

    [[nodiscard]] VoiceState state() const noexcept;
    [[nodiscard]] int channel() const noexcept;
    [[nodiscard]] int note() const noexcept;
    [[nodiscard]] int velocity() const noexcept;
    /// The note group this voice belongs to; partials of one note share it.
    [[nodiscard]] int note_group() const noexcept;

    [[nodiscard]] bool is_active() const noexcept { return state() != VoiceState::free; }
};

/// The 64-voice allocator.
///
/// Polyphony is 64, and a note consumes one voice per sounding partial — so a two-partial patch
/// halves the effective note count. Partials of one note share a *note group* so that stealing
/// removes a whole note rather than leaving a half-sounding remnant, which is what the engine's
/// pair-aware stealing achieves.
///
/// The stealing **policy** here — prefer a free slot, then the oldest releasing note, then the
/// oldest held note — is a documented approximation. The engine's own allocator
/// (`note_group_alloc`, `voice_steal_run`, `voice_steal_oldest`) was located and named during the
/// reverse engineering but its selection rules were never traced, so this is not claimed to be
/// faithful. It is isolated here so it can be replaced without touching any DSP.
class VoicePool {
public:
    /// Total voice slots.
    static constexpr int max_voices = 64;

    /// Voices are rendered in groups of this size; the engine's layout is SIMD-shaped.
    static constexpr int group_size = 4;

    /// Called with the index of each slot taken from a sounding note, before it is reassigned.
    ///
    /// A host that renders voices needs this: the slot is about to be overwritten, so whatever was
    /// sounding in it has to be moved somewhere it can fade out. Cutting it dead instead clicks.
    std::function<void(int)> stealing;

    /// Number of slots currently sounding.
    [[nodiscard]] int active_count() const noexcept;

    /// Starts a new note group, to be shared by that note's partials.
    [[nodiscard]] int begin_note_group() noexcept { return ++next_note_group_; }

    /// Allocates one voice, stealing if necessary.
    [[nodiscard]] Voice allocate(int channel, int note, int velocity, int note_group);

    /// Moves every held voice for a note into its release, and reports how many.
    int release(int channel, int note) noexcept;

    /// Frees a voice whose release has finished.
    void free_slot(int index) noexcept
    {
        state_[static_cast<std::size_t>(index)] = VoiceState::free;
    }

    /// Frees every voice immediately.
    void reset() noexcept;

    /// The sounding voices.
    [[nodiscard]] std::vector<Voice> active() const;

    [[nodiscard]] VoiceState state_of(int index) const noexcept
    {
        return state_[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] int channel_of(int index) const noexcept
    {
        return channel_[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] int note_of(int index) const noexcept
    {
        return note_[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] int velocity_of(int index) const noexcept
    {
        return velocity_[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] int note_group_of(int index) const noexcept
    {
        return note_group_[static_cast<std::size_t>(index)];
    }

private:
    [[nodiscard]] int find_free() const noexcept;
    [[nodiscard]] int find_oldest(VoiceState state) const noexcept;
    void steal_group(int note_group, int except);

    std::array<VoiceState, max_voices> state_{};
    std::array<int, max_voices> channel_{};
    std::array<int, max_voices> note_{};
    std::array<int, max_voices> velocity_{};
    std::array<int, max_voices> note_group_{};
    std::array<std::int64_t, max_voices> sequence_{};

    std::int64_t counter_ = 0;
    int next_note_group_ = 0;
};

inline VoiceState Voice::state() const noexcept
{
    return pool->state_of(index);
}

inline int Voice::channel() const noexcept
{
    return pool->channel_of(index);
}

inline int Voice::note() const noexcept
{
    return pool->note_of(index);
}

inline int Voice::velocity() const noexcept
{
    return pool->velocity_of(index);
}

inline int Voice::note_group() const noexcept
{
    return pool->note_group_of(index);
}

} // namespace ts
