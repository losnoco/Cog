#pragma once

#include <cstdint>
#include <span>

namespace ts {

/// One entry of the wave-descriptor table: where a sample lives in the wave ROM, how it is tuned,
/// and how it loops.
///
/// The field names are as recovered and they are genuinely confusing: `loop` is the data start,
/// `end` is the loop point, and `start` is the physical end. They are kept rather than renamed so
/// the code lines up with the reverse-engineering notes and the trace dumps.
struct WaveDescriptor {
    /// Bytes per descriptor record.
    static constexpr int stride = 0x16;

    /// ROM region, 0–127. Bit 4 selects the bank.
    int region = 0;
    /// Start of the sample data — delta index zero.
    int loop = 0;
    /// The loop point.
    int end = 0;
    /// The physical end of the data.
    int start = 0;
    /// MIDI note at which the sample plays back untransposed.
    int root_key = 0;
    /// Fine-tune word; native pitch is `root_key + (1024 - fine_tune) / 1000`.
    int fine_tune = 0;
    /// Second fine-tune word, at descriptor `0x0E`, neutral at 1024 like the first.
    ///
    /// `partial_compute_pitch @ 18005fc20` computes two native pitches —
    /// `voice+0x1fc = root*1000 - fine + 0x400`, then
    /// `voice+0x200 = voice+0x1fc - desc[0x0e] + 0x400` — and the exponent is taken against
    /// `voice+0x1fc`. That reads as though the second one goes nowhere, and every text search for
    /// `voice+0x200` agrees, which is how it was written off twice.
    ///
    /// It is read. `voices_control_update @ 1800849a0` walks the voices with its pointer at
    /// **`voice + 4`**, so the pair at `180084c13` —
    ///
    /// ```asm
    /// MOV EAX,dword ptr [RDI + 0x1fc]   ; voice+0x200
    /// MOV dword ptr [RDI + 0x1f8],EAX   ; voice+0x1fc
    /// LEA RCX,[RDI + -0x4]              ; the voice itself, for voice_block_process
    /// ```
    ///
    /// — is `voice+0x1fc = voice+0x200`. The four-byte skew is why no search for the literal offset
    /// ever found it.
    ///
    /// It is **not** run every tick, and it is **not** run at note-on. The condition, read off
    /// `voices_control_update` with its pointer at `voice + 4`:
    ///
    /// ```c
    /// if (voice+0x16c == 1) {                  // the ordinary sounding state
    ///     if (voice+4 != 0) {                  // a one-shot flag, cleared here
    ///         voice+4 = 0;
    ///         if (voice+0x1b0 != 0) { …retrigger…; voice+0x16c = 2; goto next; }
    ///         voice+0x1fc = voice+0x200;       // ← the term is adopted here, and only here
    ///     }
    ///     voice_block_process(voice);
    /// }
    /// ```
    ///
    /// So the term is adopted once, on a control tick where something else has already set
    /// `voice+4`, and never on the retrigger path. A fresh note-on does not set that flag, which is
    /// why `native_milli_semitones` leaves the term out.
    ///
    /// **What sets `voice+4` is the wave reaching its loop.** `voice_report_finished` @`18008aec0`
    /// sweeps the run-flag array, and for any voice carrying bit `0x10` it clears the bit, posts an
    /// event, and writes `voice+4 = 1`:
    ///
    /// ```c
    /// if ((*flags & 0x10) != 0) { *flags &= 0xef; …; *(voice_base(i) + 4) = 1; }
    /// ```
    ///
    /// So a note plays its attack at `native_milli_semitones` and its loop at
    /// `adopted_milli_semitones`, one control tick after it crosses the loop point. Measured with
    /// `scdec pitchword`, which reports whether the two words have converged: Atmosphere's first
    /// partial adopts between ticks 40 and 50 at note 48, and the loop at sample 6480 played at
    /// ratio 0.49 arrives at tick 41.3. The crossing scales with the rate, halving per octave —
    /// 60–90 ticks at note 36, 40–60 at note 48, 20–40 at note 60.
    ///
    /// That also explains the old measurement's inconsistency, quoted below. Whether a reading
    /// caught the term depended on how far into the note it looked and how fast that wave loops,
    /// which is exactly why "the same wave does both on different notes".
    ///
    /// **The adoption does retune the running voice.** Traced on the pitch ramp's own slot
    /// (`g_voice_ramp_pitch` @`181a1cbf0`, stride 0x18: +8 current, +0xc target, +0x14 increment),
    /// Atmosphere's first partial moves its *target* at sample 13440 — `cur=212614 tgt=212684
    /// step=34`, glides over about two samples, and its increment settles from 32163 to 32258, a
    /// rise of 5.11 cents against the 52 milli-semitones the term is worth. The exponent unit is
    /// 1/16384 of an octave, so the 70-unit move is exactly the term.
    ///
    /// Reading the increment rather than the target reports this late, because the ramp glides;
    /// reading the wrong voice hides it entirely, because a partial whose `second_fine_tune` is
    /// neutral never moves. An earlier note here claimed the copy only staged a value and did not
    /// retune. That was both of those mistakes at once, and it was wrong.
    ///
    /// It is nonetheless **not modelled**, and the reason is worth stating precisely rather than
    /// dressing up. Wiring it — the offline path walking its ratios to find the crossing, the
    /// realtime path testing the reader at the tick — takes the 239-case oracle note gate from
    /// clean to five failing assertions on programs 4, 38 and 66. The magnitudes say the fault is
    /// in the wiring, not the mechanism: program 38's term is **three** milli-semitones, 0.3 of a
    /// cent, and no 0.3-cent retune moves an envelope window by 1.2 dB. What it does do is assign
    /// straight to the voice's root underneath `PitchRamp` and the pitch envelope, so the change
    /// arrives as a discontinuity rather than as a retarget the ramp glides into — which is
    /// precisely what the engine does *not* do.
    ///
    /// Doing it properly means routing the new root through the pitch ramp as a target, the way a
    /// bend or a TVP move goes, and finding the crossing as an event rather than predicting a tick
    /// — a glide, a real-time pitch change and the pitch envelope all move the rate the crossing
    /// is reached at, so no precomputed tick survives them.
    ///
    /// **This behaviour is more fragile than anything else in this header, and the fragility is in
    /// the timing rather than the law.** The law is one subtraction. What decides whether a render
    /// matches is *which sample* the crossing is noticed on, and that is a chain of three: the
    /// decoder crossing `loop_start`, `voice_report_finished` turning that into `voice+4` on its
    /// next sweep, and `voices_control_update` retargeting the pitch ramp on the tick after that.
    /// The engine posts the middle step as an event — `(voice << 8) | 5` — rather than polling.
    ///
    /// Position is not a clock. It advances with the *waveform*, so TVP, the pitch ramp, a glide
    /// and any real-time pitch change all move when the crossing arrives; two renders that differ
    /// by a cent reach it at different samples. Anything that computes the crossing from a nominal
    /// rate is wrong before it starts, and anything that polls it on a grid can be a whole tick out
    /// depending on where the grid falls.
    ///
    /// So the shape to copy is the engine's: the decoder raises an event exactly on the address
    /// transition, and the reaction is delayed only by however long this engine's own event ticking
    /// takes to pick it up — no more, no less. Until the two pipelines line up sample for sample,
    /// a test of this is a test of the alignment and not of the law, which is why the switch that
    /// turns it off is worth having beside it and why the gate is green with it absent.
    ///
    /// Confirmed on the engine rather than inferred: `scdec pitchword` reads both words off live
    /// voices, and for a struck note they stay *different* — Atmosphere's first partial sits at
    /// `0x1fc = 60243` against `0x200 = 60191`, and Synth Drum at `56316` against `56356`. Equal
    /// would mean adopted; they are not.
    ///
    /// An earlier note here recorded "84 of 254 non-neutral cases carry the term and 37 plainly do
    /// not" and concluded that applying it unconditionally was the closest available. That was
    /// measured against the module's pitch word across cases that include voices well past their
    /// note-on, which is exactly where the flag does get set — so it counted the adopted state
    /// without separating it from the struck one. Against rendered audio it is the wrong call: the
    /// 239-case oracle note gate goes from nine failing assertions to none by leaving it out.
    int second_fine_tune = 1024;
    /// Raw flag byte. Bit 0 is bidirectional, bit 2 is reverse; bit 1 takes no part in the
    /// dispatch.
    int flags = 0;

    /// ROM bank: 0 for bank A, 1 for bank B.
    [[nodiscard]] constexpr int bank() const noexcept { return (region >> 4) & 1; }

    /// True when the sampler plays this wave backwards.
    [[nodiscard]] constexpr bool reverse() const noexcept { return ((flags >> 2) & 1) != 0; }

    /// True when the sampler plays this wave bidirectionally (ping-pong).
    [[nodiscard]] constexpr bool ping_pong() const noexcept { return (flags & 1) != 0; }

    /// Native pitch in semitones — the effective root, including both fine tunes.
    [[nodiscard]] constexpr double native_pitch() const noexcept
    {
        return native_milli_semitones() / 1000.0;
    }

    /// Native pitch in milli-semitones — what the sampler's ratio is taken against.
    ///
    /// The module's `voice+0x1fc` *after* the control tick overwrites it with `voice+0x200` — both
    /// fine tunes, which is what a sounding note is tuned to. See `second_fine_tune`. Every ratio
    /// in this engine divides by this, so the sites must not drift apart.
    /// The native pitch a note-on plays at — `voice+0x1fc`, **without** `second_fine_tune`.
    ///
    /// The second term is not part of this. `partial_compute_pitch` writes it into a *separate*
    /// word, `voice+0x200`, and `voices_control_update` copies that over `voice+0x1fc` only under
    /// the condition recovered below — which a fresh note-on does not meet.
    [[nodiscard]] constexpr double native_milli_semitones() const noexcept
    {
        return (root_key * 1000.0) + 1024.0 - fine_tune;
    }

    /// The same, with `second_fine_tune` folded in — `voice+0x200`.
    ///
    /// Kept because the word is real and the engine does adopt it, just not when a note starts.
    [[nodiscard]] constexpr double adopted_milli_semitones() const noexcept
    {
        return native_milli_semitones() - (second_fine_tune - 1024.0);
    }

    /// Parses a descriptor from its `stride` raw bytes.
    [[nodiscard]] static constexpr WaveDescriptor
    parse(std::span<const std::uint8_t> record) noexcept
    {
        // The three position fields are 20-bit, stored most-significant nibble first.
        const int loop = ((record[0x01] & 0x0F) << 16) | (record[0x02] << 8) | record[0x03];
        const int end = ((record[0x07] & 0x0F) << 16) | (record[0x08] << 8) | record[0x09];
        const int start = ((record[0x0B] & 0x0F) << 16) | (record[0x0C] << 8) | record[0x0D];

        return WaveDescriptor{
            .region = record[0x00] & 0x7F,
            .loop = loop,
            .end = end,
            .start = start,
            .root_key = record[0x06],
            .fine_tune = record[0x04] | (record[0x05] << 8),
            .second_fine_tune = record[0x0E] | (record[0x0F] << 8),
            .flags = record[0x0A],
        };
    }

    [[nodiscard]] friend constexpr bool operator==(const WaveDescriptor&,
                                                   const WaveDescriptor&) noexcept = default;
};

/// A key range within a multisample and the wave it selects.
struct MultisampleZone {
    int key_low = 0;
    int key_high = 0;
    int wave = 0;
};

} // namespace ts
