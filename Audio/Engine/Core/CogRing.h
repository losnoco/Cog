//
//  CogRing.h
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

#ifndef CogRing_h
#define CogRing_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma clang assume_nonnull begin

/// A single-producer, single-consumer ring of interleaved float frames.
///
/// Exactly one thread writes and exactly one thread reads. Nothing after
/// `cog_ring_create` allocates or blocks, so either side may be a real-time
/// thread. Positions are 64-bit frame counters that never wrap: the frames
/// written and read since creation (or the last `cog_ring_reset`).
///
/// Flushes are requested by the producer and carried out by the consumer. A
/// request records the producer's current write position; when the consumer
/// honours it, it discards everything before that position. Frames the
/// producer writes after requesting are kept, so it need not wait for the
/// acknowledgement before writing post-seek audio.
typedef struct CogRing CogRing;

/// Creates a ring holding at least `minimumFrames` frames of `channels`
/// channels; the capacity is rounded up to a power of two. Returns NULL if
/// either argument is zero or the allocation fails.
CogRing *_Nullable cog_ring_create(size_t minimumFrames, uint32_t channels);
void cog_ring_destroy(CogRing *_Nullable ring);

size_t cog_ring_capacity(const CogRing *ring);
uint32_t cog_ring_channels(const CogRing *ring);

/// Empties the ring and zeroes its counters. Only safe while neither the
/// producer nor the consumer is running.
void cog_ring_reset(CogRing *ring);

// MARK: Producer

/// Frames the producer can write without overwriting unread frames.
size_t cog_ring_writable(const CogRing *ring);

/// Writes up to `count` frames and returns how many were written.
size_t cog_ring_write(CogRing *ring, const float *frames, size_t count);

/// Frames written since creation.
uint64_t cog_ring_write_position(const CogRing *ring);

/// Asks the consumer to discard everything written so far. Returns the
/// request's epoch, which `cog_ring_flush_acknowledged` reaches once the
/// consumer has honoured it (or a later request).
uint64_t cog_ring_request_flush(CogRing *ring);

/// The epoch of the last flush the consumer has honoured.
uint64_t cog_ring_flush_acknowledged(const CogRing *ring);

// MARK: Consumer

/// Frames the consumer can read.
size_t cog_ring_readable(const CogRing *ring);

/// Reads up to `count` frames into `frames` and returns how many were read.
/// Passing NULL discards them instead.
size_t cog_ring_read(CogRing *ring, float *_Nullable frames, size_t count);

/// Frames read (or discarded) since creation.
uint64_t cog_ring_read_position(const CogRing *ring);

/// Carries out a pending flush request, if any, and returns the number of
/// frames discarded. Call before reading.
size_t cog_ring_honour_flush(CogRing *ring);

/// As `cog_ring_honour_flush`, first copying up to `maxKept` of the
/// discarded frames (the ones that would have been read next) into `kept`
/// and setting `keptCount` to how many.
size_t cog_ring_honour_flush_keeping(CogRing *ring, float *_Nullable kept, size_t maxKept, size_t *_Nullable keptCount);

#pragma clang assume_nonnull end

#ifdef __cplusplus
}
#endif

#endif /* CogRing_h */
