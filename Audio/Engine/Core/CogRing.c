//
//  CogRing.c
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

#include "CogRing.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// Apple silicon has 128-byte cache lines; keeping each side's counter on its
// own line stops the producer and consumer from bouncing one line between
// cores on every write and read.
#define COG_CACHE_LINE 128

struct CogRing {
	float *storage;
	size_t capacity; // frames, a power of two
	size_t mask;
	uint32_t channels;

	_Alignas(COG_CACHE_LINE) _Atomic uint64_t writePosition;
	_Atomic uint64_t flushTarget;
	_Atomic uint64_t flushRequested;

	_Alignas(COG_CACHE_LINE) _Atomic uint64_t readPosition;
	_Atomic uint64_t flushAcknowledged;
};

static size_t next_power_of_two(size_t value) {
	size_t result = 1;
	while(result < value) {
		if(result > SIZE_MAX / 2) return 0;
		result <<= 1;
	}
	return result;
}

CogRing *cog_ring_create(size_t minimumFrames, uint32_t channels) {
	if(!minimumFrames || !channels) return NULL;

	const size_t capacity = next_power_of_two(minimumFrames);
	if(!capacity || capacity > SIZE_MAX / channels / sizeof(float)) return NULL;

	CogRing *ring = aligned_alloc(COG_CACHE_LINE, (sizeof(CogRing) + COG_CACHE_LINE - 1) & ~(size_t)(COG_CACHE_LINE - 1));
	if(!ring) return NULL;

	ring->storage = calloc(capacity * channels, sizeof(float));
	if(!ring->storage) {
		free(ring);
		return NULL;
	}
	ring->capacity = capacity;
	ring->mask = capacity - 1;
	ring->channels = channels;
	atomic_init(&ring->writePosition, 0);
	atomic_init(&ring->flushTarget, 0);
	atomic_init(&ring->flushRequested, 0);
	atomic_init(&ring->readPosition, 0);
	atomic_init(&ring->flushAcknowledged, 0);
	return ring;
}

void cog_ring_destroy(CogRing *ring) {
	if(!ring) return;
	free(ring->storage);
	free(ring);
}

size_t cog_ring_capacity(const CogRing *ring) {
	return ring->capacity;
}

uint32_t cog_ring_channels(const CogRing *ring) {
	return ring->channels;
}

void cog_ring_reset(CogRing *ring) {
	atomic_store_explicit(&ring->writePosition, 0, memory_order_relaxed);
	atomic_store_explicit(&ring->flushTarget, 0, memory_order_relaxed);
	atomic_store_explicit(&ring->flushRequested, 0, memory_order_relaxed);
	atomic_store_explicit(&ring->readPosition, 0, memory_order_relaxed);
	atomic_store_explicit(&ring->flushAcknowledged, 0, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
}

// MARK: - Producer

size_t cog_ring_writable(const CogRing *ring) {
	const uint64_t write = atomic_load_explicit(&ring->writePosition, memory_order_relaxed);
	const uint64_t read = atomic_load_explicit(&ring->readPosition, memory_order_acquire);
	return ring->capacity - (size_t)(write - read);
}

size_t cog_ring_write(CogRing *ring, const float *frames, size_t count) {
	const uint64_t write = atomic_load_explicit(&ring->writePosition, memory_order_relaxed);
	const uint64_t read = atomic_load_explicit(&ring->readPosition, memory_order_acquire);
	const size_t space = ring->capacity - (size_t)(write - read);
	if(count > space) count = space;
	if(!count) return 0;

	const size_t start = (size_t)write & ring->mask;
	const size_t first = (count < ring->capacity - start) ? count : ring->capacity - start;
	const size_t channels = ring->channels;
	memcpy(ring->storage + start * channels, frames, first * channels * sizeof(float));
	if(count > first) {
		memcpy(ring->storage, frames + first * channels, (count - first) * channels * sizeof(float));
	}

	atomic_store_explicit(&ring->writePosition, write + count, memory_order_release);
	return count;
}

uint64_t cog_ring_write_position(const CogRing *ring) {
	return atomic_load_explicit(&ring->writePosition, memory_order_acquire);
}

uint64_t cog_ring_request_flush(CogRing *ring) {
	// The target is published before the request count, so a consumer that
	// sees the new request also sees this target (or a later one).
	const uint64_t write = atomic_load_explicit(&ring->writePosition, memory_order_relaxed);
	atomic_store_explicit(&ring->flushTarget, write, memory_order_relaxed);
	return atomic_fetch_add_explicit(&ring->flushRequested, 1, memory_order_release) + 1;
}

uint64_t cog_ring_flush_acknowledged(const CogRing *ring) {
	return atomic_load_explicit(&ring->flushAcknowledged, memory_order_acquire);
}

// MARK: - Consumer

size_t cog_ring_readable(const CogRing *ring) {
	const uint64_t write = atomic_load_explicit(&ring->writePosition, memory_order_acquire);
	const uint64_t read = atomic_load_explicit(&ring->readPosition, memory_order_relaxed);
	return (size_t)(write - read);
}

size_t cog_ring_read(CogRing *ring, float *frames, size_t count) {
	const uint64_t write = atomic_load_explicit(&ring->writePosition, memory_order_acquire);
	const uint64_t read = atomic_load_explicit(&ring->readPosition, memory_order_relaxed);
	const size_t available = (size_t)(write - read);
	if(count > available) count = available;
	if(!count) return 0;

	if(frames) {
		const size_t start = (size_t)read & ring->mask;
		const size_t first = (count < ring->capacity - start) ? count : ring->capacity - start;
		const size_t channels = ring->channels;
		memcpy(frames, ring->storage + start * channels, first * channels * sizeof(float));
		if(count > first) {
			memcpy(frames + first * channels, ring->storage, (count - first) * channels * sizeof(float));
		}
	}

	atomic_store_explicit(&ring->readPosition, read + count, memory_order_release);
	return count;
}

uint64_t cog_ring_read_position(const CogRing *ring) {
	return atomic_load_explicit(&ring->readPosition, memory_order_acquire);
}

size_t cog_ring_honour_flush(CogRing *ring) {
	const uint64_t requested = atomic_load_explicit(&ring->flushRequested, memory_order_acquire);
	if(requested == atomic_load_explicit(&ring->flushAcknowledged, memory_order_relaxed)) {
		return 0;
	}

	const uint64_t target = atomic_load_explicit(&ring->flushTarget, memory_order_relaxed);
	const uint64_t read = atomic_load_explicit(&ring->readPosition, memory_order_relaxed);
	size_t discarded = 0;
	if(target > read) {
		discarded = (size_t)(target - read);
		atomic_store_explicit(&ring->readPosition, target, memory_order_release);
	}
	atomic_store_explicit(&ring->flushAcknowledged, requested, memory_order_release);
	return discarded;
}
