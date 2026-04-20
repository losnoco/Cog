#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

#include "MIDIPlayer.h"

const uint8_t syx_reset_gm[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
const uint8_t syx_reset_gm2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
const uint8_t syx_reset_gs[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
const uint8_t syx_reset_xg[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t syx_gs_limit_bank_lsb[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

bool syx_equal(const uint8_t *a, const uint8_t *b) {
	while(*a != 0xF7 && *b != 0xF7 && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

bool syx_is_reset(const uint8_t *data) {
	return syx_equal(data, &syx_reset_gm[0]) || syx_equal(data, &syx_reset_gm2[0]) ||
	       syx_equal(data, &syx_reset_gs[0]) || syx_equal(data, &syx_reset_xg[0]);
}

bool syx_is_gs(const uint8_t *data, size_t size) {
	if(data[0] != 0xF0 || data[size - 1] != 0xF7) return false;
	if(data[1] != 0x41 || data[3] != 0x42 || data[4] != 0x12) return false;
	unsigned long i;
	unsigned char checksum = 0;
	for(i = 5; i + 1 < size && data[i + 1] != 0xF7; ++i)
		checksum += data[i];
	checksum = (128 - checksum) & 127;
	if(data[i] != checksum) return false;
	return true;
}

/* ── MIDIPlayer ──────────────────────────────────────────────────────────── */

MIDIPlayer::MIDIPlayer() {
	dSampleRate = 44100;
	port_mask = 1;
	mode = filter_default;
	reverb_chorus_disabled = false;
	initialized = false;
	master_volume = 1.0f;
	midi_file = nullptr;
	sequencer = nullptr;
	subsong_start_seconds = 0.0;
	subsong_end_seconds = 0.0;
	duration_seconds = 0.0;
	loop_mode_flags = 0;
	samples_rendered = 0;
	samples_total = 0;
}

MIDIPlayer::~MIDIPlayer() {
	teardownSequencer();
}

void MIDIPlayer::setSampleRate(double rate) {
	dSampleRate = rate;
	teardownSequencer();
	shutdown();
}

void MIDIPlayer::setLoopMode(unsigned int mode) {
	loop_mode_flags = mode;
	if(sequencer) {
		int loop_count = (mode & loop_mode_enable) ? -1 : 1;
		ss_sequencer_set_loop_count(sequencer, loop_count);
	}
}

void MIDIPlayer::setFilterMode(filter_mode m, bool disable_rc) {
	mode = m;
	reverb_chorus_disabled = disable_rc;
	if(initialized) {
		/* Apply filter reset immediately at t=0 equivalent (sample offset 0). */
		for(unsigned p = 0; p < 4; ++p) {
			if(port_mask & (1u << p))
				dispatchFilterReset(p, 0);
		}
	}
}

/* ── port mask derivation ────────────────────────────────────────────────── */

static unsigned derive_port_mask(const SS_MIDIFile *midi) {
	unsigned mask = 1u;
	if(!midi) return mask;
	for(size_t ti = 0; ti < midi->track_count; ++ti) {
		int p = midi->tracks[ti].port;
		if(p >= 0 && p < 32)
			mask |= (1u << p);
	}
	return mask;
}

/* ── Subsong window helpers ──────────────────────────────────────────────── */

static double subsong_start_ticks(const SS_MIDIFile *midi, size_t subsong) {
	if(!midi || midi->format != 2 || subsong == 0) return 0.0;
	if(subsong >= midi->track_count) return 0.0;
	const SS_MIDITrack *prev = &midi->tracks[subsong - 1];
	if(prev->event_count == 0) return 0.0;
	return (double)prev->events[prev->event_count - 1].ticks;
}

static double subsong_end_ticks(const SS_MIDIFile *midi, size_t subsong) {
	if(!midi || midi->format != 2) return (double)midi->last_voice_event_tick;
	if(subsong >= midi->track_count) return (double)midi->last_voice_event_tick;
	const SS_MIDITrack *tr = &midi->tracks[subsong];
	if(tr->event_count == 0) return subsong_start_ticks(midi, subsong);
	return (double)tr->events[tr->event_count - 1].ticks;
}

/* ── Load / sequencer lifecycle ──────────────────────────────────────────── */

bool MIDIPlayer::Load(SS_MIDIFile *in_midi, unsigned subsong, unsigned loop_mode, double fade_seconds) {
	teardownSequencer();

	if(!in_midi) return false;
	midi_file = in_midi;
	loop_mode_flags = loop_mode;
	port_mask = derive_port_mask(midi_file);

	/* Compute subsong time window (format-2 only; single-song otherwise). */
	double start_ticks = subsong_start_ticks(midi_file, subsong);
	double end_ticks = subsong_end_ticks(midi_file, subsong);
	subsong_start_seconds = ss_midi_ticks_to_seconds(midi_file, (size_t)start_ticks);
	subsong_end_seconds = ss_midi_ticks_to_seconds(midi_file, (size_t)end_ticks);
	duration_seconds = subsong_end_seconds - subsong_start_seconds;
	if(duration_seconds < 0.0) duration_seconds = 0.0;
	this->fade_seconds = fade_seconds;

	samples_rendered = 0;
	samples_total = (long)std::llround(duration_seconds * dSampleRate);

	return true;
}

bool MIDIPlayer::buildSequencer() {
	if(sequencer) return true;
	if(!midi_file) return false;

	if(!startup()) return false;

	SS_Processor *proc = getProcessor();
	if(proc) {
		sequencer = ss_sequencer_create(proc);
	} else {
		SS_SequencerCallbacks cb;
		memset(&cb, 0, sizeof(cb));
		cb.sample_rate = (uint32_t)std::lround(dSampleRate);
		cb.midi_command = &MIDIPlayer::midi_command_cb;
		cb.set_master_volume = &MIDIPlayer::master_volume_cb;
		cb.context = this;
		sequencer = ss_sequencer_create_callbacks(&cb);
	}
	if(!sequencer) return false;

	if(!ss_sequencer_load_midi(sequencer, midi_file)) {
		ss_sequencer_free(sequencer);
		sequencer = nullptr;
		return false;
	}

	int loop_count = (loop_mode_flags & loop_mode_enable) ? -1 : 1;
	ss_sequencer_set_loop_count(sequencer, loop_count);
	ss_sequencer_set_fade_seconds(sequencer, fade_seconds);

	/* Initial filter reset before any MIDI events. */
	for(unsigned p = 0; p < 4; ++p) {
		if(port_mask & (1u << p))
			dispatchFilterReset(p, 0);
	}

	/* Seek to subsong start time, if any (format-2). */
	if(subsong_start_seconds > 0.0)
		ss_sequencer_set_time(sequencer, subsong_start_seconds);

	ss_sequencer_play(sequencer);
	master_volume = 1.0f;
	return true;
}

void MIDIPlayer::teardownSequencer() {
	if(sequencer) {
		/* Clear the processor if necessary */
		ss_sequencer_set_synthesizer(sequencer, getProcessor());
		ss_sequencer_free(sequencer);
		sequencer = nullptr;
	}
}

/* ── Callback trampolines ────────────────────────────────────────────────── */

void MIDIPlayer::midi_command_cb(void *ctx, const uint8_t *data, size_t length, double timestamp) {
	static_cast<MIDIPlayer *>(ctx)->queueMidi(data, length, timestamp);
}

void MIDIPlayer::master_volume_cb(void *ctx, float value) {
	static_cast<MIDIPlayer *>(ctx)->handleMasterVolume(value);
}

void MIDIPlayer::queueMidi(const uint8_t *data, size_t length, double timestamp) {
	if(!data || length == 0) return;
	PendingEvent ev;
	ev.data.assign(data, data + length);
	ev.timestamp = timestamp;
	pending_events.push_back(std::move(ev));
}

void MIDIPlayer::inject(std::vector<uint8_t> bytes, double timestamp) {
	PendingEvent ev;
	ev.data = std::move(bytes);
	ev.timestamp = timestamp;
	pending_events.push_back(std::move(ev));
}

/* ── Filter reset injection ──────────────────────────────────────────────── */

static inline std::vector<uint8_t> make_port_select(unsigned port) {
	std::vector<uint8_t> v = { 0xF5, (uint8_t)((port & 0x0F) + 1) };
	return v;
}

static inline std::vector<uint8_t> make_sysex(const uint8_t *raw, size_t len) {
	return std::vector<uint8_t>(raw, raw + len);
}

static inline std::vector<uint8_t> make_cc(unsigned channel, unsigned cc, unsigned value) {
	std::vector<uint8_t> v = { (uint8_t)(0xB0 | (channel & 0x0F)), (uint8_t)cc, (uint8_t)value };
	return v;
}

static inline std::vector<uint8_t> make_pc(unsigned channel, unsigned program) {
	std::vector<uint8_t> v = { (uint8_t)(0xC0 | (channel & 0x0F)), (uint8_t)program };
	return v;
}

static std::vector<uint8_t> gs_bank_lsb_sysex(unsigned part, unsigned map_id) {
	std::vector<uint8_t> v(syx_gs_limit_bank_lsb,
	                       syx_gs_limit_bank_lsb + sizeof(syx_gs_limit_bank_lsb));
	v[6] = (uint8_t)part;
	v[8] = (uint8_t)map_id;
	unsigned checksum = 0;
	for(size_t i = 5; i + 1 < v.size() && v[i + 1] != 0xF7; ++i)
		checksum += v[i];
	checksum = (128 - checksum) & 127;
	for(size_t i = 5; i + 1 < v.size(); ++i) {
		if(v[i + 1] == 0xF7) {
			v[i] = (uint8_t)checksum;
			break;
		}
	}
	return v;
}

void MIDIPlayer::dispatchFilterReset(size_t port, uint32_t sample_offset) {
	if(mode == filter_default) return;

	/* Time at the start of the current block; events landing at this time
	 * will be dispatched with sample offset 0 (or whatever we pass). */
	double base_time = sequencer ? ss_sequencer_get_time(sequencer) : 0.0;
	(void)sample_offset;

	/* All filter mode resets begin with the general resets. */
	inject(make_port_select((unsigned)port), base_time);
	inject(make_sysex(syx_reset_xg, sizeof(syx_reset_xg)), base_time);
	inject(make_sysex(syx_reset_gm2, sizeof(syx_reset_gm2)), base_time);
	inject(make_sysex(syx_reset_gm, sizeof(syx_reset_gm)), base_time);

	unsigned map_id = 0;
	switch(mode) {
		case filter_gm:
			/* GM-only: no follow-up reset */
			break;
		case filter_gm2:
			inject(make_sysex(syx_reset_gm2, sizeof(syx_reset_gm2)), base_time);
			break;
		case filter_sc55:
			map_id = 1; goto gs_path;
		case filter_sc88:
			map_id = 2; goto gs_path;
		case filter_sc88pro:
			map_id = 3; goto gs_path;
		case filter_sc8850:
		case filter_default:
			map_id = 4;
		gs_path:
			inject(make_sysex(syx_reset_gs, sizeof(syx_reset_gs)), base_time);
			for(unsigned i = 0x41; i <= 0x49; ++i)
				inject(gs_bank_lsb_sysex(i, map_id), base_time);
			inject(gs_bank_lsb_sysex(0x40, map_id), base_time);
			for(unsigned i = 0x4A; i <= 0x4F; ++i)
				inject(gs_bank_lsb_sysex(i, map_id), base_time);
			break;
		case filter_xg:
			inject(make_sysex(syx_reset_xg, sizeof(syx_reset_xg)), base_time);
			break;
	}

	for(unsigned ch = 0; ch < 16; ++ch) {
		inject(make_port_select((unsigned)port), base_time);
		inject(make_cc(ch, 0x78, 0), base_time); /* all sound off */
		inject(make_cc(ch, 0x79, 0), base_time); /* reset all controllers */
		if(mode != filter_xg || ch != 9) {
			inject(make_cc(ch, 0x20, 0), base_time); /* bank LSB */
			inject(make_cc(ch, 0x00, 0), base_time); /* bank MSB */
			inject(make_pc(ch, 0), base_time); /* program 0 */
		}
	}

	if(mode == filter_xg) {
		inject(make_port_select((unsigned)port), base_time);
		inject(make_cc(9, 0x20, 0), base_time);
		inject(make_cc(9, 0x00, 0x7F), base_time);
		inject(make_pc(9, 0), base_time);
	}

	if(reverb_chorus_disabled) {
		for(unsigned ch = 0; ch < 16; ++ch) {
			inject(make_port_select((unsigned)port), base_time);
			inject(make_cc(ch, 0x5B, 0), base_time);
			inject(make_cc(ch, 0x5D, 0), base_time);
		}
	}
}

void MIDIPlayer::sysex_reset(size_t port, uint32_t sample_offset) {
	dispatchFilterReset(port, sample_offset);
}

/* ── Play ────────────────────────────────────────────────────────────────── */

unsigned long MIDIPlayer::Play(float *out, unsigned long count) {
	if(!midi_file) return 0;
	if(!sequencer && !buildSequencer()) return 0;
	if(ss_sequencer_is_finished(sequencer)) return 0;

	unsigned long done = 0;
	const uint32_t chunk_max = std::max<uint32_t>(1, getChunkSize());
	const bool has_processor = getProcessor() != nullptr;
	const bool is_looping = (loop_mode_flags & loop_mode_enable) != 0;

	while(done < count) {
		uint32_t chunk = chunk_max;
		if(chunk > (uint32_t)(count - done)) chunk = (uint32_t)(count - done);
		if(!is_looping && samples_total > 0 &&
		   samples_rendered + (long)chunk > samples_total) {
			long remaining = samples_total - samples_rendered;
			if(remaining <= 0) break;
			chunk = (uint32_t)remaining;
		}
		if(chunk == 0) break;

		pending_events.clear();

		double block_start = ss_sequencer_get_time(sequencer);
		ss_sequencer_tick(sequencer, chunk);

		/* For callback-mode, dispatch queued events to backend with sample
		 * offsets within this chunk.  Track the current port from 0xF5. */
		if(!has_processor) {
			unsigned current_port = 0;
			for(auto &e : pending_events) {
				if(e.data.empty()) continue;
				double offset_seconds = e.timestamp - block_start;
				if(offset_seconds < 0.0) offset_seconds = 0.0;
				long offset = std::lround(offset_seconds * dSampleRate);
				if(offset < 0) offset = 0;
				if(offset >= (long)chunk) offset = (long)chunk - 1;

				if(e.data[0] == 0xF5 && e.data.size() >= 2) {
					current_port = e.data[1] ? (unsigned)(e.data[1] - 1) : 0u;
					continue;
				}
				dispatchMidi(e.data.data(), e.data.size(), (uint32_t)offset, current_port);
			}
		}

		renderChunk(out + done * 2, chunk);

		/* Apply master_volume externally when the backend can't do it for us
		 * (callback-mode synths).  Processor-mode players no-op this via
		 * their handleMasterVolume override. */
		if(!has_processor && master_volume != 1.0f) {
			float *p = out + done * 2;
			float scale = master_volume;
			for(uint32_t i = 0; i < chunk; ++i) {
				p[0] *= scale;
				p[1] *= scale;
				p += 2;
			}
		}

		done += chunk;
		samples_rendered += chunk;

		if(ss_sequencer_is_finished(sequencer))
			break;
	}

	return done;
}

void MIDIPlayer::Seek(unsigned long sample) {
	if(!midi_file) return;
	if(!sequencer && !buildSequencer()) return;

	double target_seconds = subsong_start_seconds + (double)sample / dSampleRate;

	/* For callback-mode backends, reset the synthesizer state before
	 * replaying filler events, so we don't leave stuck notes.  Processor-
	 * mode Seek is handled internally by ss_sequencer_set_time. */
	if(!getProcessor()) {
		shutdown();
		if(!startup()) return;
		for(unsigned p = 0; p < 4; ++p) {
			if(port_mask & (1u << p))
				dispatchFilterReset(p, 0);
		}
		/* Flush any synth-bound reset events before seek-replay. */
		pending_events.clear();
	}

	ss_sequencer_set_time(sequencer, target_seconds);

	/* For callback mode, set_time dispatched non-note filler events via
	 * callback.  Replay them to the backend now so state is correct. */
	if(!getProcessor() && !pending_events.empty()) {
		unsigned current_port = 0;
		for(auto &e : pending_events) {
			if(e.data.empty()) continue;
			if(e.data[0] == 0xF5 && e.data.size() >= 2) {
				current_port = e.data[1] ? (unsigned)(e.data[1] - 1) : 0u;
				continue;
			}
			dispatchMidi(e.data.data(), e.data.size(), 0u, current_port);
		}
		pending_events.clear();
	}

	samples_rendered = (long)sample;
}

unsigned long MIDIPlayer::Tell() const {
	if(!sequencer) return 0;
	double t = ss_sequencer_get_time(sequencer) - subsong_start_seconds;
	if(t < 0.0) t = 0.0;
	return (unsigned long)std::lround(t * dSampleRate);
}

bool MIDIPlayer::GetLastError(std::string &p_out) {
	return get_last_error(p_out);
}
