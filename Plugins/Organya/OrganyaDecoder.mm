//
//  OrganyaDecoder.m
//  Organya
//
//  Created by Christopher Snowhill on 12/4/22.
//

#import <Cocoa/Cocoa.h>

#import "OrganyaDecoder.h"

#import "AudioChunk.h"
#import "PlaylistController.h"

#import <cstdio>
#import <cstring>
#import <cstdlib>
#import <vector>
#import <cmath>
#import <map>

/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* NX-Engine source code was used as reference.       */
/* Cave Story and its music were written by Pixel ( 天谷 大輔 ) */

namespace Organya {
	
	//========= PART 0 : INPUT/OUTPUT AND UTILITY ========//
	using std::fgetc;
	int fgetw(FILE* fp) { int a = fgetc(fp), b = fgetc(fp); return (b<<8) + a; }
	int fgetd(FILE* fp) { int a = fgetw(fp), b = fgetw(fp); return (b<<16) + a; }
	double fgetv(FILE* fp) // Load a numeric value from text file; one per line.
	{
		char Buf[4096], *p=Buf; Buf[4095]='\0';
		if(!std::fgets(Buf, sizeof(Buf)-1, fp)) return 0.0;
		// Ignore empty lines. If the line was empty, try next line.
		if(!Buf[0] || Buf[0]=='\r' || Buf[0]=='\n') return fgetv(fp);
		while(*p && *p++ != ':') {} // Skip until a colon character.
		return std::strtod(p, 0);   // Parse the value and return it.
	}
	
	int coggetc(id<CogSource> fp) {
		uint8_t value;
		if([fp read:&value amount:sizeof(value)] != sizeof(value)) {
			return -1;
		}
		return value;
	}
	int coggetw(id<CogSource> fp) { int a = coggetc(fp); int b = coggetc(fp); return (b<<8) + a; }
	int coggetd(id<CogSource> fp) { int a = coggetw(fp); int b = coggetw(fp); return (b<<16) + a; }
	
	//========= PART 1 : SOUND EFFECT PLAYER (PXT) ========//
	
	static signed char Waveforms[6][256];
	static void GenerateWaveforms(void) {
		/* Six simple waveforms are used as basis for the signal generators in PXT: */
		for(unsigned seed=0, i=0; i<256; ++i) {
			/* These waveforms are bit-exact with PixTone v1.0.3. */
			seed = (seed * 214013) + 2531011; // Linear congruential generator
			Waveforms[0][i] = 0x40 * std::sin(i * 3.1416 / 0x80); // Sine
			Waveforms[1][i] = ((0x40+i) & 0x80) ? 0x80-i : i;     // Triangle
			Waveforms[2][i] = -0x40 + i/2;                        // Sawtooth up
			Waveforms[3][i] =  0x40 - i/2;                        // Sawtooth down
			Waveforms[4][i] =  0x40 - (i & 0x80);                 // Square
			Waveforms[5][i] = (signed char)(seed >> 16) / 2;      // Pseudorandom
		}
	}
	
	struct Pxt {
		struct Channel {
			bool enabled;
			int nsamples;
			
			// Waveform generator
			struct Wave {
				const signed char* wave;
				double pitch;
				int level, offset;
			};
			Wave carrier;   // The main signal to be generated.
			Wave frequency; // Modulator to the main signal.
			Wave amplitude; // Modulator to the main signal.
			
			// Envelope generator (controls the overall amplitude)
			struct Env {
				int initial;                    // initial value (0-63)
				struct { int time, val; } p[3]; // time offset & value, three of them
				int Evaluate(int i) const { // Linearly interpolate between the key points:
					int prevval = initial, prevtime=0;
					int nextval = 0,       nexttime=256;
					for(int j=2; j>=0; --j) if(i < p[j].time) { nexttime=p[j].time; nextval=p[j].val; }
					for(int j=0; j<=2; ++j) if(i >=p[j].time) { prevtime=p[j].time; prevval=p[j].val; }
					if(nexttime <= prevtime) return prevval;
					return (i-prevtime) * (nextval-prevval) / (nexttime-prevtime) + prevval;
				}
			} envelope;
			
			// Synthesize the sound effect.
			std::vector<int> Synth() {
				if(!enabled) return {};
				std::vector<int> result(nsamples);
				
				auto& c = carrier, &f = frequency, &a = amplitude;
				double mainpos = c.offset, maindelta = 256*c.pitch/nsamples;
				for(size_t i=0; i<result.size(); ++i) {
					auto s = [this,i](double p=1) { return 256*p*i/nsamples; };
					// Take sample from each of the three signal generators:
					int freqval = f.wave[0xFF & int(f.offset + s(f.pitch))] * f.level;
					int ampval  = a.wave[0xFF & int(a.offset + s(a.pitch))] * a.level;
					int mainval = c.wave[0xFF & int(mainpos)              ] * c.level;
					// Apply amplitude & envelope to the main signal level:
					result[i] = mainval * (ampval+4096) / 4096 * envelope.Evaluate(s()) / 4096;
					// Apply frequency modulation to maindelta:
					mainpos += maindelta * (1 + (freqval / (freqval<0 ? 8192. : 2048.)));
				}
				return result;
			}
		} channels[4]; /* Four parallel FM-AM modulators with envelope generators. */
		
		void Load(FILE* fp) { // Load PXT file from disk and initialize synthesizer.
			/* C++11 simplifies things by a great deal.              */
			/* This function would be a lot more complex without it. */
			auto f = [fp](){ return (int) fgetv(fp); };
			for(auto&c: channels)
				c = { f() != 0, f(), // enabled, length
					{ Waveforms[f()%6], fgetv(fp), f(), f() }, // carrier wave
					{ Waveforms[f()%6], fgetv(fp), f(), f() }, // frequency wave
					{ Waveforms[f()%6], fgetv(fp), f(), f() }, // amplitude wave
					{ f(), { {f(),f()}, {f(),f()}, {f(),f()} } } // envelope
				};
		}
	};
	
	//========= PART 2 : SONG PLAYER (ORG) ========//
	/* Note: Requires PXT synthesis for percussion (drums). */
	
	static short WaveTable[100*256];
	static std::vector<short> DrumSamples[12];
	
	void LoadWaveTable(void) {
		NSURL *url = [[NSBundle bundleWithIdentifier:@"co.losno.Organya"] URLForResource:@"wavetable" withExtension:@"dat"];
		if(!url) return;
		NSString *path = [url path];
		FILE* fp = std::fopen([path UTF8String], "rb");
		if(!fp) return;
		for(size_t a=0; a<100*256; ++a)
			WaveTable[a] = (signed char) fgetc(fp);
		std::fclose(fp);
	}

	void LoadDrums(void) {
		GenerateWaveforms();
		/* List of PXT files containing these percussion instruments: */
		static const int patch[] = {0x96,0,0x97,0, 0x9a,0x98,0x99,0, 0x9b,0,0,0};
		for(unsigned drumno=0; drumno<12; ++drumno)
		{
			if(!patch[drumno]) continue; // Leave that non-existed drum file unloaded
			// Load the drum parameters
			char Buf[64] = {};
			std::snprintf(Buf, sizeof(Buf)-1, "fx%02x", patch[drumno]);
			NSURL *url = [[NSBundle bundleWithIdentifier:@"co.losno.Organya"] URLForResource:[NSString stringWithUTF8String:Buf] withExtension:@"pxt"];
			if(!url) continue;
			NSString *path = [url path];
			FILE* fp = std::fopen([path UTF8String], "rb");
			if(!fp) continue;
			Pxt d;
			d.Load(fp);
			std::fclose(fp);
			// Synthesize and mix the drum's component channels
			auto& sample = DrumSamples[drumno];
			for(auto& c: d.channels)
			{
				auto buf = c.Synth();
				if(buf.size() > sample.size()) sample.resize(buf.size());
				for(size_t a=0; a<buf.size(); ++a)
					sample[a] += buf[a];
			}
		}
	}
	
	struct Song {
		int ms_per_beat, samples_per_beat, loop_start, loop_end;
		int cur_beat, total_beats;
		int loop_count;
		struct Ins {
			int tuning, wave;
			bool pi; // true=all notes play for exactly 1024 samples.
			std::size_t n_events;

			struct Event { int note, length, volume, panning; };
			std::map<int/*beat*/, Event> events;

			// Volatile data, used & changed during playback:
			double phaseacc, phaseinc, cur_vol;
			int    cur_pan, cur_length, cur_wavesize;
			const short* cur_wave;
		} ins[16];

		BOOL Load(id<CogSource> fp) {
			[fp seek:0 whence:SEEK_SET];
			char Signature[6];
			if([fp read:Signature amount:sizeof(Signature)] != sizeof(Signature))
				return NO;
			if(memcmp(Signature, "Org-02", 6) != 0)
				return NO;
			// Load song parameters
			ms_per_beat     = coggetw(fp);
			/*steps_per_bar =*/coggetc(fp); // irrelevant
			/*beats_per_step=*/coggetc(fp); // irrelevant
			loop_start      = coggetd(fp);
			loop_end        = coggetd(fp);
			// Load each instrument parameters (and initialize them)
			for(auto& i: ins)
				i = { coggetw(fp), coggetc(fp), coggetc(fp)!=0, (unsigned)coggetw(fp),
					  {}, 0,0,0,0,0,0,0 };
			// Load events for each instrument
			for(auto& i: ins)
			{
				std::vector<std::pair<int,Ins::Event>> events( i.n_events );
				for(auto& n: events) n.first          = coggetd(fp);
				for(auto& n: events) n.second.note    = coggetc(fp);
				for(auto& n: events) n.second.length  = coggetc(fp);
				for(auto& n: events) n.second.volume  = coggetc(fp);
				for(auto& n: events) n.second.panning = coggetc(fp);
				i.events.insert(events.begin(), events.end());
			}
			
			return YES;
		}
		
		void Reset(void) {
			cur_beat = 0;
			total_beats = 0;
			loop_count = 0;
		}

		std::vector<float> Synth(double sampling_rate)
		{
			// Determine playback settings:
			double samples_per_millisecond = sampling_rate * 1e-3, master_volume = 4e-6;
			int    samples_per_beat = ms_per_beat * samples_per_millisecond; // rounded.
			// Begin synthesis
			{
				if(cur_beat == loop_end) {
					cur_beat = loop_start;
					loop_count++;
				}
				// Synthesize this beat in stereo sound (two channels).
				std::vector<float> result( samples_per_beat * 2, 0.f );
				for(auto &i: ins)
				{
					// Check if there is an event for this beat
					auto j = i.events.find(cur_beat);
					if(j != i.events.end())
					{
						auto& event = j->second;
						if(event.volume  != 255) i.cur_vol = event.volume * master_volume;
						if(event.panning != 255) i.cur_pan = event.panning;
						if(event.note    != 255)
						{
							// Calculate the note's wave data sampling frequency (equal temperament)
							double freq = std::pow(2.0, (event.note + i.tuning/1000.0 + 155.376) / 12);
							// Note: 155.376 comes from:
							//         12*log(256*440)/log(2) - (4*12-3-1) So that note 4*12-3 plays at 440 Hz.
							// Note: Optimizes into
							//         pow(2, (note+155.376 + tuning/1000.0) / 12.0)
							//         2^(155.376/12) * exp( (note + tuning/1000.0)*log(2)/12 )
							// i.e.    7901.988*exp(0.057762265*(note + tuning*1e-3))
							i.phaseinc     = freq / sampling_rate;
							i.phaseacc     = 0;
							// And determine the actual wave data to play
							i.cur_wave     = &WaveTable[256 * (i.wave % 100)];
							i.cur_wavesize = 256;
							i.cur_length   = i.pi ? 1024/i.phaseinc : (event.length * samples_per_beat);

							if(&i >= &ins[8]) // Percussion is different
							{
								const auto& d = DrumSamples[i.wave % 12];
								i.phaseinc = event.note * (22050/32.5) / sampling_rate; // Linear frequency
								i.cur_wave     = &d[0];
								i.cur_wavesize = (int) d.size();
								i.cur_length   = d.size() / i.phaseinc;
							}
							// Ignore missing drum samples
							if(i.cur_wavesize <= 0) i.cur_length = 0;
						}
					}

					// Generate wave data. Calculate left & right volumes...
					auto left  = (i.cur_pan > 6 ? 12 - i.cur_pan : 6) * i.cur_vol;
					auto right = (i.cur_pan < 6 ?      i.cur_pan : 6) * i.cur_vol;
					int n = samples_per_beat > i.cur_length ? i.cur_length : samples_per_beat;
					for(int p=0; p<n; ++p)
					{
						double pos = i.phaseacc;
						// Take a sample from the wave data.
						/* We could do simply this: */
						//int sample = i.cur_wave[ unsigned(pos) % i.cur_wavesize ];
						/* But since we have plenty of time, use neat Lanczos filtering. */
						/* This improves especially the low rumble noises substantially. */
						enum { radius = 2 };
						auto lanczos = [](double d) -> double
						{
							if(d == 0.) return 1.;
							if(std::fabs(d) > radius) return 0.;
							double dr = (d *= 3.14159265) / radius;
							return std::sin(d) * std::sin(dr) / (d*dr);
						};
						double scale = 1/i.phaseinc > 1 ? 1 : 1/i.phaseinc, density = 0, sample = 0;
						int min = -radius/scale + pos - 0.5;
						int max =  radius/scale + pos + 0.5;
						for(int m=min; m<max; ++m) // Collect a weighted average.
						{
							double factor = lanczos( (m-pos+0.5) * scale );
							density += factor;
							sample += i.cur_wave[m<0 ? 0 : m%i.cur_wavesize] * factor;
						}
						if(density > 0.) sample /= density; // Normalize
						// Save audio in float32 format:
						result[p*2 + 0] += sample * left;
						result[p*2 + 1] += sample * right;
						i.phaseacc += i.phaseinc;
					}
					i.cur_length -= n;
				}
				
				cur_beat++;

				return result;
			}
		}
	};
}

@implementation OrganyaDecoder

// Need this static initializer to create the static global tables that sidplayfp doesn't really lock access to
+ (void)initialize {
	Organya::LoadWaveTable();
	Organya::LoadDrums();
}

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	m_song = new Organya::Song;
	if(!m_song->Load(s)) {
		return NO;
	}
	
	long loopCount = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultLoopCount"] intValue];
	double fadeTime = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
	if(fadeTime < 0.0) {
		fadeTime = 0.0;
	}

	long beatsToEnd = m_song->loop_start + (m_song->loop_end - m_song->loop_start) * loopCount;
	double lengthOfSong = ((double)m_song->ms_per_beat * 1e-3) * (double)beatsToEnd;
	length = (int)ceil(lengthOfSong * sampleRate);
	lengthWithFade = (int)ceil((lengthOfSong + fadeTime) * sampleRate);

	renderedTotal = 0.0;
	fadeTotal = fadeRemain = (int)ceil(sampleRate * fadeTime);
	
	samplesDiscard = 0;
	
	m_song->Reset();

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
			  @"sampleRate": @(sampleRate),
			  @"totalFrames": @(lengthWithFade),
			  @"bitsPerSample": @(32),
			  @"floatingPoint": @(YES),
			  @"channels": @(2),
			  @"seekable": @(YES),
			  @"endian": @"host",
			  @"encoding": @"synthesized" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk *)readAudio {
	double streamTimestamp = (double)(m_song->cur_beat) * (double)(m_song->ms_per_beat) * 0.001;

	std::vector<float> samples = m_song->Synth(sampleRate);

	int rendered = (int)(samples.size() / 2);
	
	renderedTotal += rendered;

	if(!IsRepeatOneSet() && renderedTotal >= length) {
		float *sampleBuf = &samples[0];
		long fadeEnd = fadeRemain - rendered;
		if(fadeEnd < 0)
			fadeEnd = 0;
		float fadePosf = (float)fadeRemain / (float)fadeTotal;
		const float fadeStep = 1.0f / (float)fadeTotal;
		for(long fadePos = fadeRemain; fadePos > fadeEnd; --fadePos, fadePosf -= fadeStep) {
			long offset = (fadeRemain - fadePos) * 2;
			sampleBuf[offset + 0] *= fadePosf;
			sampleBuf[offset + 1] *= fadePosf;
		}
		rendered = (int)(fadeRemain - fadeEnd);
		fadeRemain = fadeEnd;
	}

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	[chunk setStreamTimestamp:streamTimestamp];

	if(samplesDiscard) {
		[chunk assignSamples:&samples[samplesDiscard * 2] frameCount:rendered - samplesDiscard];
		samplesDiscard = 0;
	} else {
		[chunk assignSamples:&samples[0] frameCount:rendered];
	}

	return chunk;
}

- (long)seek:(long)frame {
	long originalFrame = frame;
	
	if(frame < renderedTotal) {
		m_song->Reset();
		renderedTotal = 0;
		fadeRemain = fadeTotal;
	}
	
	long msPerLoop = (m_song->loop_end - m_song->loop_start) * m_song->ms_per_beat;
	long msIntro = m_song->loop_start * m_song->ms_per_beat;
	
	long samplesPerBeat = (long)ceil(m_song->ms_per_beat * 1e-3 * sampleRate);
	long samplesPerLoop = (long)ceil(msPerLoop * 1e-3 * sampleRate);
	long samplesIntro = (long)ceil(msIntro * 1e-3 * sampleRate);
	
	if(samplesPerLoop) {
		while (frame >= (samplesIntro + samplesPerLoop)) {
			frame -= samplesPerLoop;
			m_song->loop_count++;
		}
	}
	
	long beatTarget = frame / samplesPerBeat;
	samplesDiscard = frame % samplesPerBeat;

	m_song->cur_beat = (int) beatTarget;

	return originalFrame;
}

- (void)cleanUp {
	if(m_song) {
		delete m_song;
		m_song = NULL;
	}

	source = nil;
}

- (void)close {
	[self cleanUp];
}

- (void)dealloc {
	[self close];
}

- (void)setSource:(id<CogSource>)s {
	source = s;
}

- (id<CogSource>)source {
	return source;
}

+ (NSArray *)fileTypes {
	return @[@"org"];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Organya File", @"vg.icns", @"org"],
	];
}

@end
