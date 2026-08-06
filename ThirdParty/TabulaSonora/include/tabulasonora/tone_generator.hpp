#pragma once

#include "tabulasonora/note_renderer.hpp"
#include "tabulasonora/output_filter.hpp"
#include "tabulasonora/part.hpp"
#include "tabulasonora/render_options.hpp"
#include "tabulasonora/smf_reader.hpp"
#include "tabulasonora/voice_pool.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace ts {

/// How a running engine should behave.
struct ToneGeneratorOptions {
    /// Asks for polyphony that grows on demand instead of stealing.
    static constexpr int unlimited_polyphony = 0;

    /// Maximum simultaneous voices, or `unlimited_polyphony`.
    ///
    /// A note costs one voice per sounding partial, so a two-partial patch halves the note count.
    /// The default is the hardware's 64, which is what makes a stream sound like the module —
    /// including the stealing, which is audible and part of the character. Raising it is a
    /// deliberate departure.
    ///
    /// `unlimited_polyphony` makes the pool **grow** rather than steal: when it runs out it
    /// allocates another chunk of slots, so every note in the file sounds. That is what an offline
    /// render wants and what an audio thread must not have — growing allocates, and allocating
    /// inside the block loop is the thing a real-time thread cannot do. It also has no upper bound
    /// on memory or CPU beyond what the file asks for, which is acceptable when the only
    /// requirement is to be right.
    int polyphony = 64;

    /// MIDI ports to accept input on: 1, 2 or 4, giving 16, 32 or 64 parts.
    ///
    /// Two is the hardware and the default. Four is a deliberate extension past what the module can
    /// do -- see `ToneGenerator::max_port_count` -- and wants a raised `polyphony` beside it, since
    /// sixty-four parts sharing sixty-four voices would steal without pause. Values that are not
    /// 1, 2 or 4 are rejected: the part index is formed by masking, which needs a power of two.
    int ports = 2;

    /// Internal 32-sample chunks a MIDI message waits before it reaches a part.
    ///
    /// **Anything compared against the oracle wants these three on.** `event_delay_blocks = 4`,
    /// `bypass_output_filter = false`, and real sample offsets on the sends. The module has no
    /// switch for any of them: it always stages, always runs its output stage, and always stamps a
    /// message to the millisecond. A comparison made with them off is measuring this engine against
    /// a differently-timed one, and every latency it turns up has to be explained away by hand --
    /// which is how the loop-crossing retune was misread twice.
    ///
    /// They default off because the existing suite is written against renders that begin where they
    /// always have, and because being able to take a stage out of the picture is what makes a
    /// failure attributable. That is a convenience for tests of *laws*; it is the wrong setting for
    /// a test of a *render*.
    ///
    /// The module never applies a message on arrival. `TG_ShortMidiIn` only puts it in a ring;
    /// `TG_Process` walks it from there into a staging ring, raises a task bit, and dispatches that
    /// bit on a later chunk. The note-on latency measured against the oracle is 128 samples, which
    /// is four of these chunks.
    ///
    /// The chunk is the unit rather than the host call, because `TG_Process` over-renders: it
    /// serves whatever the last call left buffered before rendering anything new, so a host asking
    /// for a length that is not a whole number of chunks leaves a remainder behind and the grid
    /// drifts against the host's boundaries. A deferral counted in host calls drifts with it.
    ///
    /// **Zero, the default, applies on arrival** -- what this engine has always done, and what its
    /// tests are written against, since many send a message and inspect the part on the next line.
    /// Four models the module. It is an option rather than a constant because the behaviours it
    /// governs are only testable when the two pipelines line up sample for sample, so being able to
    /// take it out of the picture is what makes a failure attributable.
    ///
    /// Not everything in that 128 is necessarily staging: `tg_output_filter` runs even when the
    /// host rate already matches the engine's, and any group delay it carries would sit in the same
    /// measurement. Pinning the split is what the next measurement is for.
    int event_delay_blocks = 0;

    /// Whether to skip the module's output stage.
    ///
    /// `tg_output_filter` runs on every chunk `TG_Process` emits, at every host rate including the
    /// engine's own -- see `OutputFilter`. At 1:1 it reduces to a single sample of delay, which is
    /// a sample of the latency between this engine and the module and nothing else audible.
    ///
    /// Bypassing it is the default for the same reason the event pipeline is off by default: the
    /// suite is written against renders that begin where they always have, and a stage whose whole
    /// effect at this rate is one sample of delay is not worth shifting every pinned fixture for.
    /// Turn it off -- that is, run the filter -- when lining a render up against the oracle.
    bool bypass_output_filter = true;

    /// The rate a message's sample offset is read against, when one is given.
    ///
    /// `TG_ShortMidiIn` does not keep the offset it is handed. It converts it straight to
    /// milliseconds — `offset * 1000 / g_host_sample_rate` — and stamps the message with that, and
    /// `TG_Process` then releases a message once its stamp is reached. The comparison works because
    /// the counter it is measured against ticks once per 32-sample chunk, and 32 samples at the
    /// engine's 32 kHz is exactly one millisecond.
    ///
    /// So the resolution of event placement inside a buffer is a millisecond, and the rate here is
    /// the *host's* — the offset is in the host's samples, not the engine's. Defaults to the
    /// engine's own rate, which is what this engine renders at.
    int host_sample_rate = OutputFilter::engine_rate;

    /// Which tone map program changes resolve against.
    ///
    /// `ToneMap::xg` is the switch that starts the engine in XG mode, rather than a separate flag,
    /// because on the module the two are one thing: XG System On is precisely what moves every part
    /// onto the XG map. Setting it here is the same state a file would have reached by sending it.
    ///
    /// It is a *starting* state, not a lock. A file is still free to change mode -- any Roland
    /// SysEx leaves XG parsing, exactly as it would have -- and this remains the default map that a
    /// part with no map selected falls back to, the same way `sc8820` does. `ToneGenerator::reset`
    /// returns to it.
    ///
    /// There is deliberately no detection: nothing here infers XG from the shape of a file's bank
    /// selects. A bank LSB of 18 is a legitimate if unusual GS map selector, and guessing would
    /// break the files that mean it. A file that wants XG says so, or the host does.
    ToneMap map = ToneMap::sc8820;

    /// MIDI channel routed to the drum path.
    int drum_channel = 9;

    bool reverb = true;
    bool chorus = true;
    bool delay = true;

    /// The insertion EFX block, `40 03` and the per-part `40 4x 22` switch.
    ///
    /// Only a first tranche of the 65 algorithms is transcribed; a type outside it passes the
    /// signal through unchanged, with routing and send levels still honoured.
    bool efx = true;

    /// Force an effect type instead of taking it from the stream.
    std::optional<int> reverb_type;
    std::optional<int> chorus_type;
    std::optional<int> delay_type;

    /// Linear gain applied to the audio handed to the host.
    double output_gain = 1.0;

    /// Per-channel mute and solo, read live so a mixer can change it while sound is running.
    const ChannelMask* channels = nullptr;
};

/// The real-time engine: MIDI in, audio out, rendered a block at a time.
///
/// This is the block-based voice loop the hardware runs. Events are applied at the render-block
/// boundary — the grid the engine itself quantises them to — voices are allocated from a pool that
/// holds the hardware's 64 unless `ToneGeneratorOptions` says otherwise, and each block is summed
/// into a dry pair and three send buses that the effects then process. Nothing about a note has to be known in advance, so a note can be
/// held indefinitely and released whenever.
///
/// It shares its DSP with the note renderer rather than reimplementing it: the same envelopes, the
/// same sampler, the same tables. `NoteRenderer` renders one note in isolation, for analysis and
/// for the gates; this is the only path a *song* takes, offline or live, which is why an exported
/// render and a live performance of the same file cannot drift apart.
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

    /// How many MIDI ports the *hardware* accepts input on.
    ///
    /// Two, because the module has thirty-two parts and addresses them as `port * 16 + channel`. It
    /// allocates all thirty-two unconditionally — the part count global is initialised to `0x20`
    /// and the second part array sits exactly sixteen strides on from the first — but
    /// `midi_drain_ready_to_ports` masks the port field out of every incoming packet with
    /// `and r8b,0Fh`, so nothing but port A can be reached. Widening that mask to `0x1f` admits the
    /// second port and no more, which is what this engine implements.
    static constexpr int port_count = 2;

    /// How many parts the hardware has: sixteen per port.
    static constexpr int part_count = port_count * Sequence::channel_count;

    /// The widest this engine can be asked to run, past what the module can do.
    ///
    /// `ToneGeneratorOptions::ports` accepts 1, 2 or 4, giving 16, 32 or 64 parts, and the default
    /// is the hardware's 2. Four is an **extension, not a fidelity feature**: the module allocates
    /// thirty-two parts and no more, so nothing above `port_count` reproduces anything it does. It
    /// exists for the same reason `unlimited_polyphony` does -- a host driving more than
    /// thirty-two channels of material through one engine, where being able to play the file
    /// matters more than matching a module that could not have played it either.
    ///
    /// Pair it with the polyphony. Thirty-two parts share the hardware's 64 voices already; sixty-
    /// four parts asking for the same 64 will steal continuously and sound nothing like the extra
    /// parts were worth having. Something like 256 is the sensible companion, and
    /// `unlimited_polyphony` for offline work.
    static constexpr int max_port_count = 4;

    /// Parts at `max_port_count`.
    static constexpr int max_part_count = max_port_count * Sequence::channel_count;

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

    /// How many ports this engine was created with; parts are `ports() * 16`.
    [[nodiscard]] int ports() const noexcept;

    /// How many parts this engine was created with.
    [[nodiscard]] int parts() const noexcept;

    /// How many samples have been rendered since the last reset.
    [[nodiscard]] std::int64_t position() const noexcept;

    /// How many notes have sounded since the last reset.
    ///
    /// A note that resolves to nothing — an unassigned program, or a velocity outside every
    /// partial's window — is not counted, since no voice starts.
    [[nodiscard]] int note_count() const noexcept;

    /// How many voices are currently sounding, including those fading after being stolen.
    [[nodiscard]] int active_voices() const noexcept;

    /// Whether this render ever ran out of voices.
    ///
    /// False means the polyphony setting made no difference to the output: nothing was stolen and
    /// nothing had to grow, so every larger limit would have produced the same audio. A caller
    /// comparing digests across polyphony settings can use this to say whether a match is
    /// meaningful or merely a file that never got busy.
    [[nodiscard]] bool polyphony_limit_reached() const noexcept;

    /// How many sounding voices were taken to make room.
    [[nodiscard]] int stolen_voices() const noexcept;

    /// The number of voice slots that exist, after any growth.
    ///
    /// Not the peak *usage*: nothing counts how many voices sounded at once, only how many slots
    /// were allocated. For a growing pool the two converge, because it only grows when it runs out;
    /// for a fixed one this is just the limit.
    [[nodiscard]] int voice_slots() const noexcept;

    /// The parts, indexed by `port * 16 + channel`; `parts()` of them.
    ///
    /// The first sixteen are port A and are what a host that never names a port drives, so an
    /// engine sent only port-A traffic behaves exactly as a sixteen-part one.
    ///
    /// An index past `parts()` is **clamped**, not undefined — but it is still a caller's bug, and
    /// `ChannelMask::channel_count` is not the bound to walk: the mask is sixty-four wide whatever
    /// the engine is.
    [[nodiscard]] const Part& part(int index) const noexcept;

    /// The voice allocator.
    [[nodiscard]] const VoicePool& voices() const noexcept;

    /// The drum kit in force, as the last program change on the drum part resolved it.
    ///
    /// Worth reading rather than recomputing: a program the map does not define leaves the kit as
    /// it was, so `kit_for_program` over the part's current program does not always answer what is
    /// actually loaded.
    [[nodiscard]] int drum_kit() const noexcept;

    /// The drum kit in force on one port.
    ///
    /// Each port has its own drum part, so each carries its own kit — per (port, map), the
    /// module's eight kit buffers; this reports the port's MAP1 kit, the default rhythm part's.
    /// `drum_kit()` is port A's.
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

    /// Whether the engine is in XG mode, having seen XG System On and no Roland or GM reset since.
    [[nodiscard]] bool xg_mode() const noexcept;

    /// Which tone map a part's program change currently resolves against.
    ///
    /// Not `ToneGeneratorOptions::map`: a bank select LSB names a vintage, and XG mode overrides
    /// both and puts every part on the XG map. A display that names programs has to ask per part
    /// and per moment, because this changes while the music plays.
    [[nodiscard]] ToneMap part_tone_map(int index) const noexcept;

    /// The bank a part's melodic lookup is given.
    ///
    /// Usually `part(i).bank`, but not under XG: the bank pair is inverted there, and bank MSB 64
    /// substitutes the SFX voice column rather than any variation the part stored.
    [[nodiscard]] int part_lookup_bank(int index) const noexcept;

    /// Whether a part is currently sounding drums.
    ///
    /// The channel number does not answer this. GS routes a part to the drum path with
    /// use-for-rhythm SysEx, and XG does it from bank select alone, so under XG any part can be
    /// drums and the drum part can be melodic.
    [[nodiscard]] bool part_is_drum(int index) const noexcept;

    /// The drum kit a part would sound, or -1 if it is not a drum part.
    [[nodiscard]] int part_drum_kit(int index) const noexcept;

    /// Silences everything and returns every part to its power-on state.
    void reset();

    /// Where in the buffer a message lands, in the host's samples.
    ///
    /// The module stamps every message with `offset * 1000 / host_sample_rate` milliseconds and
    /// releases it when the chunk counter reaches that, so a buffer's worth of messages arrive
    /// spread across it rather than all at its start. Zero — the default — is "at the top of the
    /// buffer", which is what a caller that has always sent its events immediately before rendering
    /// is really saying.
    static constexpr int immediately = 0;

    /// Applies one MIDI event on port A; its position is ignored, since it applies now.
    void send(const MidiEvent& message);

    /// Applies one MIDI event on a port. Anything past `ports()` folds onto the ports that exist.
    void send(int port, const MidiEvent& message);

    /// Applies one MIDI event at a position inside the buffer about to be rendered.
    ///
    /// The form a sequencer wants: it knows where each event falls, and handing that over lets the
    /// engine place it the way the module does rather than quantising it to a render boundary
    /// first. Offset leads, as it does on the other stamped sends.
    void send_at(int sample_offset, int port, const MidiEvent& message);

    /// Applies one channel voice message on port A.
    ///
    /// The equivalent of the module's `TG_ShortMidiIn`, which builds a packet with the port field
    /// hardwired to zero and so can only ever reach port A.
    void send_channel(int status, int data1, int data2);

    /// Applies one channel voice message on a port. Anything past `ports()` folds onto those that
    /// exist.
    ///
    /// The port travels with the message rather than being selected beforehand, which is how the
    /// module works: it dispatches on the port field of each packet as that packet is drained, and
    /// nothing carries the field over from one message to the next.
    void send_channel(int port, int status, int data1, int data2);

    /// Applies one MIDI event at a position inside the buffer about to be rendered.
    ///
    /// The offset leads rather than trails so that this cannot be confused with `send_channel`:
    /// with a trailing offset, a four-argument call would match both "port, status, data1, data2"
    /// and "status, data1, data2, offset", and every existing caller would quietly rebind.
    ///
    /// `sample_offset` is in the *host's* samples. See `ToneGeneratorOptions::host_sample_rate`.
    void send_channel_at(int sample_offset, int status, int data1, int data2);

    /// The same, on a given port.
    void send_channel_at(int sample_offset, int port, int status, int data1, int data2);

    /// Applies one system-exclusive message on port A, including the leading `F0`.
    void send_sysex(std::span<const std::uint8_t> bytes);

    /// Applies a SysEx message at a position inside the buffer about to be rendered.
    ///
    /// Offset first, for the same reason `send_channel_at` puts it there.
    void send_sysex_at(int sample_offset, std::span<const std::uint8_t> bytes);

    /// Applies one system-exclusive message on a port.
    ///
    /// GS part addressing is port-relative: a `40 1n` block address names a part on whichever port
    /// the message arrived on. The module does this by latching the arriving packet's port field
    /// into the high nibble of its current-channel global and selecting the part array from it, so
    /// the same address means a different part on each port.
    void send_sysex(int port, std::span<const std::uint8_t> bytes);

    /// The same, on a given port.
    void send_sysex_at(int sample_offset, int port, std::span<const std::uint8_t> bytes);

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
