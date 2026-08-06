#pragma once

#include "tabulasonora/rom_image.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace ts {

/// The two raw streams that make up one wave: a signed delta per sample, and a shift exponent per
/// 16-sample block packed two to a byte.
struct WaveStreams {
    /// One signed byte per sample, `sample_count + 1` long — the ping-pong sampler turns around
    /// *on* index `sample_count`, so that extra step is read.
    std::vector<std::uint8_t> delta;
    /// The deltas between the 32-sample exponent-block boundary and the data start —
    /// `scale_phase` of them, empty for an aligned wave.
    ///
    /// The engine's decoder does not begin at the data start: its predictor is zero at the block
    /// boundary *below* it, and these deltas are integrated in on the way. Their sum rides under
    /// every sample of the wave as a constant — measured live, `Crash Cym.1`'s eleven preamble
    /// deltas sum to exactly −0.041015625, and subtracting the module's predictor trace from this
    /// engine's decode of the same wave leaves precisely that constant at correlation 1.0. This
    /// is the in-signal DC the verification article documents on the crashes and the sine kick.
    std::vector<std::uint8_t> preamble_delta;
    /// Shift-exponent nibbles: byte `i >> 5` holds the exponents for samples `i`, with the low
    /// nibble used when `(i >> 4) & 1` is zero and the high nibble otherwise.
    std::vector<std::uint8_t> scale;
    /// Number of decodable samples.
    std::int32_t sample_count = 0;
    /// The descriptor's data start, exactly as given — not rounded to a block.
    std::int32_t data_start = 0;
    /// Where `data_start` falls inside its 32-sample exponent block, i.e. `loop & 0x1F`.
    ///
    /// The codec carries no absolute value per block, only differences, so a wave may legitimately
    /// begin partway into an exponent block and end partway through another. Decoding just has to
    /// index the exponents by the *absolute* sample position rather than by the offset into what was
    /// read, which is what this carries.
    std::int32_t scale_phase = 0;
};

/// The 24 MB wave ROM embedded in `SCCore.dll` — the literal Sound Canvas hardware mask ROM, two
/// banks addressed in 1 MB regions.
///
/// Bank A is the stacked SC-88/SC-88Pro ROM, bank B the SC-8820 ROM. Within a region the shift
/// exponents live at the bottom (one byte per 32 samples, so 32 KB covers the whole 1 MB) while the
/// deltas are addressed directly by sample index.
class WaveRom {
public:
    /// Size of one addressable ROM region, 1 MB.
    static constexpr std::int32_t region_size = 0x100000;

    /// Largest wave this class will attempt to read, as a sanity bound.
    static constexpr std::int32_t max_sample_count = 2'000'000;

    /// Regions bank A spans: **sixteen**, the full width of the region nibble.
    ///
    /// This read twelve for a long time, and descriptors reach past that — wave 4010 (TR-808 open
    /// hat) is region 12, wave 2092 (Bim Hit) region 14. That looked like a bug and is not: the two
    /// bank bases sit `0x1000030` apart, exactly 16 MB plus bank B's own 0x30 header, so regions
    /// 12–15 land at `0xc92700`–`0xf92700`, well inside bank A and nowhere near bank B. The mapping
    /// `bank_a_base + region * 1 MB` is right for every region a descriptor can name, and nothing
    /// here bounds or clamps it.
    ///
    /// Bank B is the one that genuinely stops early, at eight — see below.
    static constexpr int bank_a_region_count = 16;

    /// Regions of real ROM data in bank B — eight, not twelve.
    ///
    /// The manifest records both banks as 12 MB, but bank B's data ends at file offset `0x1892730`:
    /// regions 0–3 are the tail of the 1996 `rom_make` image and regions 4–7 are the 1999
    /// `8820_wv0` image. The declared 12 MB span runs past the end of the file altogether, so the
    /// nominal size cannot be used as a bound.
    static constexpr int bank_b_region_count = 8;

    /// Number of regions of real ROM data in a bank: 0 for bank A, 1 for bank B.
    [[nodiscard]] static constexpr int region_count(int bank) noexcept
    {
        return bank == 0 ? bank_a_region_count : bank_b_region_count;
    }

    /// Splits a descriptor's region byte into a bank and a bank-relative region index.
    struct SplitRegion {
        int bank = 0;
        int effective_region = 0;
    };

    [[nodiscard]] static constexpr SplitRegion split_region(int region) noexcept
    {
        const int bank = (region >> 4) & 1;
        return SplitRegion{bank, region - (16 * bank)};
    }

    /// Creates a wave-ROM view over an open image, which must outlive it.
    explicit WaveRom(const RomImage& rom);

    /// File offset of the first byte of a bank.
    [[nodiscard]] std::int64_t bank_base(int bank) const noexcept
    {
        return bank == 0 ? bank_a_base_ : bank_b_base_;
    }

    /// File offset of the base of a region, from a descriptor's region byte.
    [[nodiscard]] std::int64_t region_base(int region) const noexcept;

    /// Reads the delta and scale streams for one wave.
    ///
    /// Returns nothing when the descriptor describes no usable data — a non-positive or implausibly
    /// large length.
    ///
    /// The descriptor's field names are as recovered, and they are confusing: `loop` is the data
    /// start, `end` is the loop point, and `start` is the physical end.
    ///
    /// Decoding starts at `loop` exactly. An unaligned `loop` is not a malformed descriptor: the
    /// codec stores no absolute value per block, only differences, so a wave can begin partway into
    /// an exponent block and end partway through another, and the decoder simply has to index the
    /// exponents absolutely. This used to round the start down with `loop & ~0x1F`, which began the
    /// integration up to 31 samples early — and because the predictor has no leak and nothing
    /// downstream blocks DC, those extra deltas displaced the whole wave for its entire length
    /// rather than adding a moment of lead-in.
    [[nodiscard]] std::optional<WaveStreams> read_streams(int region, int loop, int start) const;

private:
    const RomImage* rom_;
    std::int64_t bank_a_base_ = 0;
    std::int64_t bank_b_base_ = 0;
};

} // namespace ts
