#pragma once

#include "tabulasonora/note_renderer.hpp"
#include "tabulasonora/part.hpp"
#include "tabulasonora/sequence_renderer.hpp"
#include "tabulasonora/smf_reader.hpp"
#include "tabulasonora/voice_pool.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace ts {

/// How a running engine should behave.
struct ToneGeneratorOptions {
    /// Which vintage's tone map program changes resolve against.
    ToneMap map = ToneMap::sc8820;

    /// MIDI channel routed to the drum path.
    int drum_channel = 9;

    bool reverb = true;
    bool chorus = true;
    bool delay = true;

    /// Force an effect type instead of taking it from the stream.
    std::optional<int> reverb_type;
    std::optional<int> chorus_type;
    std::optional<int> delay_type;

    /// How long a drum hit rings before its release is spliced in.
    ///
    /// A drum ignores note-off, so this is not the note's length: the tone's own envelope does the
    /// decay and this only bounds how long the voice occupies a slot.
    double drum_ring_seconds = 1.8;

    /// Linear gain applied to the audio handed to the host.
    double output_gain = 1.0;

    /// Per-channel mute and solo, read live so a mixer can change it while sound is running.
    const ChannelMask* channels = nullptr;
};

/// The real-time engine: MIDI in, audio out, rendered a block at a time.
///
/// This is the block-based voice loop the hardware runs. Events are applied at the render-block
/// boundary — the grid the engine itself quantises them to — voices are allocated from a fixed pool
/// of 64 and stolen when it runs out, and each block is summed into a dry pair and three send buses
/// that the effects then process. Nothing about a note has to be known in advance, so a note can be
/// held indefinitely and released whenever.
///
/// It shares its DSP with `SequenceRenderer` rather than reimplementing it: the same envelopes, the
/// same sampler, the same tables. The two differ in what they can express, not in how they sound —
/// the offline path renders each note whole and never runs out of polyphony, while this one
/// enforces the engine's own limit and can be driven live.
///
/// Not thread-safe. Events and rendering must come from the same thread, or be serialised by the
/// caller.
class ToneGenerator {
public:
    /// Internal sample rate.
    static constexpr int sample_rate = NoteRenderer::sample_rate;

    /// Samples per render block — the grid events are applied on.
    static constexpr int block_size = smf::block_grid;

    /// Samples per control tick, at 100 Hz.
    static constexpr int control_block = NoteRenderer::control_block;

    /// How many MIDI ports the engine accepts input on.
    ///
    /// Two, because the module has thirty-two parts and addresses them as `port * 16 + channel`. It
    /// allocates all thirty-two unconditionally — the part count global is initialised to `0x20`
    /// and the second part array sits exactly sixteen strides on from the first — but
    /// `midi_drain_ready_to_ports` masks the port field out of every incoming packet with
    /// `and r8b,0Fh`, so nothing but port A can be reached. Widening that mask to `0x1f` admits the
    /// second port and no more, which is what this engine implements.
    static constexpr int port_count = 2;

    /// How many parts the engine has: sixteen per port.
    static constexpr int part_count = port_count * Sequence::channel_count;

    /// Creates an engine over a note renderer's loaded tables, which must outlive it.
    explicit ToneGenerator(NoteRenderer& notes, const ToneGeneratorOptions& options = {});

    ToneGenerator(ToneGenerator&&) noexcept;
    ToneGenerator& operator=(ToneGenerator&&) noexcept;
    ToneGenerator(const ToneGenerator&) = delete;
    ToneGenerator& operator=(const ToneGenerator&) = delete;
    ~ToneGenerator();

    /// Linear gain applied to the audio handed to the host.
    ///
    /// A trim on the way out, applied where the block is copied to the caller rather than inside
    /// the block loop, so no voice, effect or feedback path sees it. `reset` leaves it alone.
    [[nodiscard]] double output_gain() const noexcept;
    void set_output_gain(double gain) noexcept;

    /// How many samples have been rendered since the last reset.
    [[nodiscard]] std::int64_t position() const noexcept;

    /// How many notes have sounded since the last reset.
    ///
    /// A note that resolves to nothing — an unassigned program, or a velocity outside every
    /// partial's window — is not counted, since no voice starts.
    [[nodiscard]] int note_count() const noexcept;

    /// How many voices are currently sounding, including those fading after being stolen.
    [[nodiscard]] int active_voices() const noexcept;

    /// The thirty-two parts, indexed by `port * 16 + channel`.
    ///
    /// The first sixteen are port A and are what a host that never names a port drives, so an
    /// engine sent only port-A traffic behaves exactly as a sixteen-part one.
    [[nodiscard]] const Part& part(int index) const noexcept;

    /// The voice allocator.
    [[nodiscard]] const VoicePool& voices() const noexcept;

    /// The drum kit in force, as the last program change on the drum part resolved it.
    ///
    /// Worth reading rather than recomputing: a program the map does not define leaves the kit as
    /// it was, so `kit_for_program` over the part's current program does not always answer what is
    /// actually loaded.
    [[nodiscard]] int drum_kit() const noexcept;

    /// The drum kit in force on one port, 0 or 1.
    ///
    /// Each port has its own drum part, so each carries its own kit. `drum_kit()` is port A's.
    [[nodiscard]] int drum_kit_for(int port) const noexcept;

    /// Which drum map row a program change on the drum part resolves against.
    ///
    /// The module derives this from the part's *internal* bank code, and that translation is not
    /// reversed — so nothing in a MIDI file reaches it. Until it is, the row is set by the host,
    /// which is the only way the second map's kits can be sounded at all.
    ///
    /// Deliberately **not** cleared by `reset`. The kit is, because a program change selects it and
    /// `reset` undoes what MIDI did; the row is configuration, like the tone map.
    [[nodiscard]] std::optional<int> drum_map_row() const noexcept;
    void set_drum_map_row(std::optional<int> row) noexcept;

    /// The drum map row this engine actually resolves against.
    [[nodiscard]] int effective_drum_map_row() const noexcept;

    /// Silences everything and returns every part to its power-on state.
    void reset();

    /// Applies one MIDI event on port A; its position is ignored, since it applies now.
    void send(const MidiEvent& message);

    /// Applies one MIDI event on a port, 0 or 1. Anything wider folds onto those two.
    void send(int port, const MidiEvent& message);

    /// Applies one channel voice message on port A.
    ///
    /// The equivalent of the module's `TG_ShortMidiIn`, which builds a packet with the port field
    /// hardwired to zero and so can only ever reach port A.
    void send_channel(int status, int data1, int data2);

    /// Applies one channel voice message on a port, 0 or 1. Anything wider folds onto those two.
    ///
    /// The port travels with the message rather than being selected beforehand, which is how the
    /// module works: it dispatches on the port field of each packet as that packet is drained, and
    /// nothing carries the field over from one message to the next.
    void send_channel(int port, int status, int data1, int data2);

    /// Applies one system-exclusive message on port A, including the leading `F0`.
    void send_sysex(std::span<const std::uint8_t> bytes);

    /// Applies one system-exclusive message on a port, 0 or 1.
    ///
    /// GS part addressing is port-relative: a `40 1n` block address names a part on whichever port
    /// the message arrived on. The module does this by latching the arriving packet's port field
    /// into the high nibble of its current-channel global and selecting the part array from it, so
    /// the same address means a different part on each port.
    void send_sysex(int port, std::span<const std::uint8_t> bytes);

    /// Applies one USB-MIDI Event Packet: `(port << 4) | class` in the low byte, then the MIDI
    /// message in the three above it, least-significant first.
    ///
    /// The equivalent of the module's `TG_PMidiIn`, which is the only one of its exports that can
    /// name a port. The message length is taken from the status byte rather than the class nibble,
    /// so a caller that leaves the class at zero still gets the right message.
    ///
    /// The port field is masked with `0x1f` — the class nibble plus the low bit of the port — so
    /// ports 0 and 1 pass through and anything wider folds onto them by its low bit. That is the
    /// module's own mask widened by one bit: it ships as `0x0f`, which discards the port outright
    /// and is why the stock DLL reaches only sixteen of its thirty-two parts.
    void send_packet(std::uint32_t packet);

    /// Renders audio into two equal-length channels.
    ///
    /// Any length is accepted. Blocks are still rendered whole and the remainder carried, because a
    /// voice counts its control tick in blocks. A caller that wants events to land exactly where
    /// the engine would put them should render in multiples of `block_size` and send between calls.
    ///
    /// Throws `std::invalid_argument` if the two channels differ in length.
    void render(std::span<float> left, std::span<float> right);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ts
