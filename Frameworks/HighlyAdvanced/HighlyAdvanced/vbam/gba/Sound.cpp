#include <string.h>

#include "Sound.h"

#include "GBA.h"
#include "Globals.h"

#include "../common/Port.h"

#include "../apu/Gb_Apu.h"
#include "../apu/Multi_Buffer.h"

#define NR10 0x60
#define NR11 0x62
#define NR12 0x63
#define NR13 0x64
#define NR14 0x65
#define NR21 0x68
#define NR22 0x69
#define NR23 0x6c
#define NR24 0x6d
#define NR30 0x70
#define NR31 0x72
#define NR32 0x73
#define NR33 0x74
#define NR34 0x75
#define NR41 0x78
#define NR42 0x79
#define NR43 0x7c
#define NR44 0x7d
#define NR50 0x80
#define NR51 0x81
#define NR52 0x84

static inline GBA::blip_time_t blip_time(GBASystem *gba)
{
    return gba->SOUND_CLOCK_TICKS - gba->soundTicks;
}

void Gba_Pcm::init(GBASystem *gba)
{
    this->gba = gba;
	output    = 0;
	last_time = 0;
	last_amp  = 0;
	shift     = 0;
}

void Gba_Pcm::apply_control( int idx )
{
    shift = ~gba->ioMem [SGCNT0_H] >> (2 + idx) & 1;

	int ch = 0;
    if ( (gba->soundEnableFlag >> idx & 0x100) && (gba->ioMem [NR52] & 0x80) )
        ch = gba->ioMem [SGCNT0_H+1] >> (idx * 4) & 3;

    GBA::Blip_Buffer* out = 0;
	switch ( ch )
	{
    case 1: out = gba->stereo_buffer->right();  break;
    case 2: out = gba->stereo_buffer->left();   break;
    case 3: out = gba->stereo_buffer->center(); break;
	}

	if ( output != out )
	{
		if ( output )
		{
			output->set_modified();
            gba->pcm_synth [0].offset( blip_time(gba), -last_amp, output );
		}
		last_amp = 0;
		output = out;
	}
}

void Gba_Pcm::end_frame( GBA::blip_time_t time )
{
	last_time -= time;
	if ( last_time < -2048 )
		last_time = -2048;

	if ( output )
		output->set_modified();
}

void Gba_Pcm::update( int dac )
{
	if ( output )
	{
        GBA::blip_time_t time = blip_time(gba);

		dac = (s8) dac >> shift;
		int delta = dac - last_amp;
		if ( delta )
		{
			last_amp = dac;

			int filter = 0;
            if ( gba->soundInterpolation )
			{
				// base filtering on how long since last sample was output
				blip_long period = time - last_time;

				int idx = (unsigned) period / 512;
				if ( idx >= 3 )
					idx = 3;

				static int const filters [4] = { 0, 0, 1, 2 };
				filter = filters [idx];
			}

            gba->pcm_synth [filter].offset( time, delta, output );
		}
		last_time = time;
	}
}

void Gba_Pcm_Fifo::init(GBASystem *gba)
{
    this->gba = gba;
    pcm.init(gba);
}

void Gba_Pcm_Fifo::timer_overflowed( int which_timer )
{
	if ( which_timer == timer && enabled )
	{
        /* Mother 3 fix, refined to not break Metroid Fusion */
		if ( count == 16 || count == 0 )
		{
			// Need to fill FIFO
            int saved_count = count;
            CPUCheckDMA( gba, 3, which ? 4 : 2 );
            if ( saved_count == 0 && count == 16 )
                CPUCheckDMA( gba, 3, which ? 4 : 2 );
			if ( count == 0 )
			{
				// Not filled by DMA, so fill with 16 bytes of silence
				int reg = which ? FIFOB_L : FIFOA_L;
				for ( int n = 8; n--; )
				{
                    soundEvent(gba, reg  , (u16)0);
                    soundEvent(gba, reg+2, (u16)0);
				}
			}
		}

		// Read next sample from FIFO
		count--;
		dac = fifo [readIndex];
		readIndex = (readIndex + 1) & 31;
		pcm.update( dac );
	}
}

void Gba_Pcm_Fifo::write_control( int data )
{
	enabled = (data & 0x0300) ? true : false;
	timer   = (data & 0x0400) ? 1 : 0;

	if ( data & 0x0800 )
	{
		// Reset
		writeIndex = 0;
		readIndex  = 0;
		count      = 0;
		dac        = 0;
		memset( fifo, 0, sizeof fifo );
	}

	pcm.apply_control( which );
	pcm.update( dac );
}

void Gba_Pcm_Fifo::write_fifo( int data )
{
	fifo [writeIndex  ] = data & 0xFF;
	fifo [writeIndex+1] = data >> 8;
	count += 2;
	writeIndex = (writeIndex + 2) & 31;
}

static void apply_control(GBASystem *gba)
{
    gba->pcm [0].pcm.apply_control( 0 );
    gba->pcm [1].pcm.apply_control( 1 );
}

static int gba_to_gb_sound( int addr )
{
	static const int table [0x40] =
	{
		0xFF10,     0,0xFF11,0xFF12,0xFF13,0xFF14,     0,     0,
		0xFF16,0xFF17,     0,     0,0xFF18,0xFF19,     0,     0,
		0xFF1A,     0,0xFF1B,0xFF1C,0xFF1D,0xFF1E,     0,     0,
		0xFF20,0xFF21,     0,     0,0xFF22,0xFF23,     0,     0,
		0xFF24,0xFF25,     0,     0,0xFF26,     0,     0,     0,
		     0,     0,     0,     0,     0,     0,     0,     0,
		0xFF30,0xFF31,0xFF32,0xFF33,0xFF34,0xFF35,0xFF36,0xFF37,
		0xFF38,0xFF39,0xFF3A,0xFF3B,0xFF3C,0xFF3D,0xFF3E,0xFF3F,
	};
	if ( addr >= 0x60 && addr < 0xA0 )
		return table [addr - 0x60];
	return 0;
}

void soundEvent(GBASystem *gba, u32 address, u8 data)
{
	int gb_addr = gba_to_gb_sound( address );
	if ( gb_addr )
	{
        gba->ioMem[address] = data;
        gba->gb_apu->write_register( blip_time(gba), gb_addr, data );

		if ( address == NR52 )
            apply_control(gba);
	}

    gba->ioMem[NR52] = (gba->ioMem[NR52] & 0x80) | (gba->gb_apu->read_status() & 0x7f);

    // TODO: what about byte writes to SGCNT0_H etc.?
}

static void apply_volume( GBASystem *gba, bool apu_only = false )
{
	if ( !apu_only )
        gba->soundVolume_ = gba->soundVolume;

    if ( gba->gb_apu )
	{
		static float const apu_vols [4] = { 0.25, 0.5, 1, 0.25 };
        gba->gb_apu->volume( gba->soundVolume_ * apu_vols [gba->ioMem [SGCNT0_H] & 3] );
	}

	if ( !apu_only )
	{
		for ( int i = 0; i < 3; i++ )
            gba->pcm_synth [i].volume( 0.66 / 256 * gba->soundVolume_ );
	}
}

static void write_SGCNT0_H( GBASystem *gba, int data )
{
    WRITE16LE( &gba->ioMem [SGCNT0_H], data & 0x770F );
    gba->pcm [0].write_control( data      );
    gba->pcm [1].write_control( data >> 4 );
    apply_volume( gba, true );
}

void soundEvent(GBASystem *gba, u32 address, u16 data)
{
	switch ( address )
	{
	case SGCNT0_H:
        write_SGCNT0_H( gba, data );
		break;

	case FIFOA_L:
	case FIFOA_H:
        gba->pcm [0].write_fifo( data );
        WRITE16LE( &gba->ioMem[address], data );
		break;

	case FIFOB_L:
	case FIFOB_H:
        gba->pcm [1].write_fifo( data );
        WRITE16LE( &gba->ioMem[address], data );
		break;

	case 0x88:
		data &= 0xC3FF;
        WRITE16LE( &gba->ioMem[address], data );
		break;

	default:
        soundEvent( gba, address & ~1, (u8) (data     ) ); // even
        soundEvent( gba, address |  1, (u8) (data >> 8) ); // odd
		break;
	}
}

void soundTimerOverflow(GBASystem *gba, int timer)
{
    gba->pcm [0].timer_overflowed( timer );
    gba->pcm [1].timer_overflowed( timer );
}

static void end_frame( GBASystem *gba, GBA::blip_time_t time )
{
    gba->pcm [0].pcm.end_frame( time );
    gba->pcm [1].pcm.end_frame( time );

    gba->gb_apu       ->end_frame( time );
    gba->stereo_buffer->end_frame( time );
}

void flush_samples(GBASystem *gba, GBA::Multi_Buffer * buffer)
{
	// We want to write the data frame by frame to support legacy audio drivers
	// that don't use the length parameter of the write method.
	// TODO: Update the Win32 audio drivers (DS, OAL, XA2), and flush all the
	// samples at once to help reducing the audio delay on all platforms.
    blip_long soundBufferLen = ( gba->soundSampleRate / 60 ) * 4;

	// soundBufferLen should have a whole number of sample pairs
    assert( soundBufferLen % (2 * sizeof *gba->soundFinalWave) == 0 );

	// number of samples in output buffer
    blip_long const out_buf_size = soundBufferLen / sizeof *gba->soundFinalWave;

    while ( buffer->samples_avail() )
	{
        long samples_read = buffer->read_samples( (GBA::blip_sample_t*) gba->soundFinalWave, out_buf_size );
        if(gba->soundPaused)
            soundResume(gba);

        gba->output->write(gba->soundFinalWave, samples_read * sizeof *gba->soundFinalWave);
	}
}

static void apply_filtering(GBASystem *gba)
{
    gba->soundFiltering_ = gba->soundFiltering;

    int const base_freq = (int) (32768 - gba->soundFiltering_ * 16384);
    blip_long const nyquist = gba->stereo_buffer->sample_rate() / 2;

	for ( int i = 0; i < 3; i++ )
	{
		blip_long cutoff = base_freq >> i;
		if ( cutoff > nyquist )
			cutoff = nyquist;
        gba->pcm_synth [i].treble_eq( GBA::blip_eq_t( 0, 0, gba->stereo_buffer->sample_rate(), cutoff ) );
	}
}

void psoundTickfn(GBASystem *gba)
{
    if ( gba->gb_apu && gba->stereo_buffer )
	{
		// Run sound hardware to present
        end_frame( gba, gba->SOUND_CLOCK_TICKS );

        flush_samples( gba, gba->stereo_buffer );

        if ( gba->soundFiltering_ != gba->soundFiltering )
            apply_filtering(gba);

        if ( gba->soundVolume_ != gba->soundVolume )
            apply_volume(gba);
	}
}

static void apply_muting(GBASystem *gba)
{
    if ( !gba->stereo_buffer || !gba->ioMem )
		return;

	// PCM
    apply_control(gba);

    if ( gba->gb_apu )
	{
		// APU
		for ( int i = 0; i < 4; i++ )
		{
            if ( gba->soundEnableFlag >> i & 1 )
                gba->gb_apu->set_output( gba->stereo_buffer->center(),
                        gba->stereo_buffer->left(), gba->stereo_buffer->right(), i );
			else
                gba->gb_apu->set_output( 0, 0, 0, i );
		}
	}
}

static void reset_apu(GBASystem *gba)
{
    if (gba->gb_apu)
	{
        gba->gb_apu->reduce_clicks(gba->soundDeclicking);
        gba->gb_apu->reset( gba->gb_apu->mode_agb, true );
	}

    if ( gba->stereo_buffer )
        gba->stereo_buffer->clear();

    gba->soundTicks = gba->SOUND_CLOCK_TICKS;
}

static void remake_stereo_buffer(GBASystem *gba)
{
    if ( !gba->ioMem )
		return;

	// Clears pointers kept to old stereo_buffer
    gba->pcm [0].init(gba);
    gba->pcm [1].init(gba);

	// Stereo_Buffer
    if (gba->stereo_buffer)
	{
        delete gba->stereo_buffer;
        gba->stereo_buffer = 0;
	}

    gba->stereo_buffer = new GBA::Stereo_Buffer; // TODO: handle out of memory
    gba->stereo_buffer->set_sample_rate( gba->soundSampleRate ); // TODO: handle out of memory
    gba->stereo_buffer->clock_rate( gba->gb_apu->clock_rate );

	// PCM
    gba->pcm [0].which = 0;
    gba->pcm [1].which = 1;
    apply_filtering(gba);

	// APU
    if ( !gba->gb_apu )
	{
        gba->gb_apu = new GBA::Gb_Apu; // TODO: handle out of memory
        reset_apu(gba);
	}

    apply_muting(gba);
    apply_volume(gba);
}

void soundShutdown(GBASystem *gba)
{
	// APU
    if ( !gba->gb_apu )
	{
        delete gba->gb_apu;
        gba->gb_apu = 0;
	}

	// Stereo_Buffer
    if (gba->stereo_buffer)
	{
        delete gba->stereo_buffer;
        gba->stereo_buffer = 0;
	}
}

void soundPause(GBASystem *gba)
{
    gba->soundPaused = true;
}

void soundResume(GBASystem *gba)
{
    gba->soundPaused = false;
}

void soundSetVolume( GBASystem *gba, float volume )
{
    gba->soundVolume = volume;
}

float soundGetVolume(GBASystem *gba)
{
    return gba->soundVolume;
}

void soundSetEnable(GBASystem *gba, int channels)
{
    gba->soundEnableFlag = channels;
    apply_muting(gba);
}

int soundGetEnable(GBASystem *gba)
{
    return (gba->soundEnableFlag & 0x30f);
}

void soundReset(GBASystem *gba)
{
    remake_stereo_buffer(gba);
    reset_apu(gba);

    gba->soundPaused = true;
    gba->SOUND_CLOCK_TICKS = GBASystem::SOUND_CLOCK_TICKS_;
    gba->soundTicks        = GBASystem::SOUND_CLOCK_TICKS_;

    soundEvent( gba, NR52, (u8) 0x80 );
}

bool soundInit(GBASystem *gba, GBASoundOut *out)
{
    gba->soundPaused = true;
    gba->output = out;
	return true;
}

long soundGetSampleRate(GBASystem *gba)
{
    return gba->soundSampleRate;
}

void soundSetSampleRate(GBASystem *gba, long sampleRate)
{
    if ( gba->soundSampleRate != sampleRate )
	{
		{
            gba->soundSampleRate      = sampleRate;
		}

        remake_stereo_buffer(gba);
	}
}
