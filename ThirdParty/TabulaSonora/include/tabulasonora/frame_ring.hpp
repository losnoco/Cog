#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace ts {

/// A lock-free single-producer, single-consumer ring of interleaved stereo frames.
///
/// Not part of the engine, and not in the reference build: this is the piece a *host* needs. The
/// engine renders whole blocks on whatever thread drives it, and every real-time host -- the
/// terminal player here, a plugin later -- has to get those blocks to a callback it does not own.
/// It lives in the library rather than beside the player because that problem is the host's, not
/// the player's, and because here it is covered by the sanitiser suite.
///
/// The producing thread renders; the consuming thread is the audio callback. Nothing here
/// allocates, locks or blocks after construction, which is the whole point: the callback runs on a
/// real-time thread where a page fault or a contended mutex is an audible glitch.
///
/// The producer owns `write_` and the consumer owns `read_`; each only ever increases its own
/// counter, so no compare-and-swap is needed. Counters are 64-bit frame totals rather than wrapped
/// indices, so `write_ - read_` is the fill level directly and there is no ambiguity between full
/// and empty.
class FrameRing {
public:
    /// Creates a ring holding at least this many frames, rounded up to a power of two.
    explicit FrameRing(std::size_t frames);

    /// Frames the ring can hold.
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

    /// Frames currently queued, as the producer sees it.
    [[nodiscard]] std::size_t queued() const noexcept
    {
        return static_cast<std::size_t>(write_.load(std::memory_order_relaxed)
                                        - read_.load(std::memory_order_acquire));
    }

    /// Frames the producer can write right now.
    [[nodiscard]] std::size_t writable() const noexcept { return capacity_ - queued(); }

    /// Writes interleaved stereo frames. The caller must not exceed `writable`.
    void write(std::span<const float> interleaved) noexcept;

    /// Asks the consumer to drop everything queued.
    ///
    /// The producer cannot rewind `write_` itself without racing the consumer's reads, so the drop
    /// is delegated: this bumps a counter, and the consumer -- which owns `read_` -- performs it.
    /// The producer must then stop writing until `flush_pending` goes false, or the frames it wrote
    /// in the meantime would be dropped along with the stale ones.
    void request_flush() noexcept { flush_.fetch_add(1, std::memory_order_release); }

    /// Whether a requested flush has yet to be performed by the consumer.
    [[nodiscard]] bool flush_pending() const noexcept
    {
        return flush_.load(std::memory_order_relaxed) != ack_.load(std::memory_order_acquire);
    }

    /// Whether the consumer should treat an empty ring as intended rather than as an underrun.
    void set_starvation_expected(bool expected) noexcept
    {
        expect_starvation_.store(expected, std::memory_order_relaxed);
    }

    /// Fills interleaved stereo frames, zero-padding whatever the ring could not supply.
    ///
    /// Called from the audio callback. Returns the frames actually supplied.
    std::size_t read(std::span<float> interleaved) noexcept;

    /// How many times the consumer has come up short.
    ///
    /// Underruns are the one thing a player should never hide: a glitch the listener hears but the
    /// display does not mention looks like a fault in the engine.
    [[nodiscard]] std::int64_t underruns() const noexcept
    {
        return underruns_.load(std::memory_order_relaxed);
    }

    /// Peak absolute level of the last block the consumer handed to the device.
    [[nodiscard]] std::pair<float, float> peak() const noexcept
    {
        return {peak_left_.load(std::memory_order_relaxed),
                peak_right_.load(std::memory_order_relaxed)};
    }

private:
    std::vector<float> samples_;
    std::size_t capacity_;
    std::size_t mask_;

    alignas(64) std::atomic<std::uint64_t> write_{0};
    alignas(64) std::atomic<std::uint64_t> read_{0};

    std::atomic<std::uint32_t> flush_{0};
    std::atomic<std::uint32_t> ack_{0};
    std::atomic<bool> expect_starvation_{false};

    std::atomic<std::int64_t> underruns_{0};
    std::atomic<float> peak_left_{0.0F};
    std::atomic<float> peak_right_{0.0F};
};

} // namespace ts
