#include "midi_processor.h"

#include <string.h>

const uint8_t midi_processor::lds_default_tempo[5] = { 0xFF, 0x51, 0x07, 0xA1, 0x20 };

#define ENABLE_WHEEL
//#define ENABLE_VIB
//#define ENABLE_ARP
//#define ENABLE_TREM

#ifdef ENABLE_WHEEL
#define WHEEL_RANGE_HIGH 12
#define WHEEL_RANGE_LOW 0
#define WHEEL_SCALE(x) ((x) * 512 / WHEEL_RANGE_HIGH)
#define WHEEL_SCALE_LOW(x) (WHEEL_SCALE(x) & 127)
#define WHEEL_SCALE_HIGH(x) (((WHEEL_SCALE(x) >> 7) + 64) & 127)
#endif

#ifdef ENABLE_VIB
// Vibrato (sine) table
static const unsigned char vibtab[] = {
  0, 13, 25, 37, 50, 62, 74, 86, 98, 109, 120, 131, 142, 152, 162,
  171, 180, 189, 197, 205, 212, 219, 225, 231, 236, 240, 244, 247,
  250, 252, 254, 255, 255, 255, 254, 252, 250, 247, 244, 240, 236,
  231, 225, 219, 212, 205, 197, 189, 180, 171, 162, 152, 142, 131,
  120, 109, 98, 86, 74, 62, 50, 37, 25, 13
};
#endif

#ifdef ENABLE_TREM
// Tremolo (sine * sine) table
static const unsigned char tremtab[] = {
  0, 0, 1, 1, 2, 4, 5, 7, 10, 12, 15, 18, 21, 25, 29, 33, 37, 42, 47,
  52, 57, 62, 67, 73, 79, 85, 90, 97, 103, 109, 115, 121, 128, 134,
  140, 146, 152, 158, 165, 170, 176, 182, 188, 193, 198, 203, 208,
  213, 218, 222, 226, 230, 234, 237, 240, 243, 245, 248, 250, 251,
  253, 254, 254, 255, 255, 255, 254, 254, 253, 251, 250, 248, 245,
  243, 240, 237, 234, 230, 226, 222, 218, 213, 208, 203, 198, 193,
  188, 182, 176, 170, 165, 158, 152, 146, 140, 134, 127, 121, 115,
  109, 103, 97, 90, 85, 79, 73, 67, 62, 57, 52, 47, 42, 37, 33, 29,
  25, 21, 18, 15, 12, 10, 7, 5, 4, 2, 1, 1, 0
};
#endif

bool midi_processor::is_lds( std::vector<uint8_t> const& p_file, const char * p_extension )
{
    if ( strcasecmp( p_extension, "LDS" ) ) return false;
    if ( p_file.size() < 1 ) return false;
    if ( p_file[ 0 ] > 2 ) return false;
    return true;
}

struct sound_patch
{
	// skip 11 bytes worth of Adlib crap
    uint8_t keyoff;
#ifdef ENABLE_WHEEL
    uint8_t portamento;
    int8_t glide;
#endif
	// skip 1 byte
#ifdef ENABLE_VIB
    uint8_t vibrato;
    uint8_t vibrato_delay;
#endif
#ifdef ENABLE_TREM
    uint8_t modulator_tremolo;
    uint8_t carrier_tremolo;
    uint8_t tremolo_delay;
#endif
#ifdef ENABLE_ARP
    uint8_t arpeggio;
    int8_t arpeggio_table[12];
#endif
	// skip 4 bytes worth of digital instrument crap
	// skip 3 more bytes worth of Adlib crap that isn't even used
    uint8_t midi_instrument;
    uint8_t midi_velocity;
    uint8_t midi_key;
    int8_t midi_transpose;
	// skip 2 bytes worth of MIDI dummy fields or whatever
};

struct channel_state {
#ifdef ENABLE_WHEEL
    int16_t gototune, lasttune;
#endif
    uint16_t packpos;
    int8_t finetune;
#ifdef ENABLE_WHEEL
    uint8_t glideto, portspeed;
#endif
    uint8_t nextvol, volmod, volcar,
		keycount, packwait;
#ifdef ENABLE_VIB
    uint8_t vibwait, vibspeed, vibrate, vibcount;
#endif
#ifdef ENABLE_TREM
    uint8_t trmstay, trmwait, trmspeed, trmrate, trmcount,
		trcwait, trcspeed, trcrate, trccount;
#endif
#ifdef ENABLE_ARP
    uint8_t arp_count, arp_size, arp_speed, arp_pos;
    int8_t arp_tab[12];
#endif

	struct {
        uint8_t chandelay, sound;
        uint16_t high;
	} chancheat;
};

void playsound( uint8_t current_instrument[], std::vector<sound_patch> const& patches, uint8_t last_note[], uint8_t last_channel[], uint8_t last_instrument[], uint8_t last_volume[], uint8_t last_sent_volume[],
#ifdef ENABLE_WHEEL
    int16_t last_pitch_wheel[],
#endif
    channel_state * c, uint8_t allvolume, unsigned current_timestamp, unsigned sound, unsigned chan, unsigned high, midi_track & track )
{
    uint8_t buffer[ 2 ];
	current_instrument[ chan ] = sound;
    if ( sound >= patches.size() ) return;
	const sound_patch & patch = patches[ current_instrument[ chan ] ];
	unsigned channel = ( patch.midi_instrument >= 0x80 ) ? 9 : ( chan == 9 ) ? 10 : chan;
	unsigned saved_last_note = last_note[ chan ];
	unsigned note;
	
	if ( channel != 9 )
	{
		// set fine tune
		high += c->finetune;

		// arpeggio handling
#ifdef ENABLE_ARP
		if(patch.arpeggio)
		{
			short arpcalc = patch.arpeggio_table[0] << 4;

			high += arpcalc;
		}
#endif

		// and MIDI transpose
		high = (int)high + ( patch.midi_transpose << 4 );

		note = high
#ifdef ENABLE_WHEEL
			- c->lasttune
#endif
			;

		// glide handling
#ifdef ENABLE_WHEEL
		if(c->glideto != 0)
		{
			c->gototune = note - ( last_note[ chan ] << 4 ) + c->lasttune;
			c->portspeed = c->glideto;
			c->glideto = c->finetune = 0;
			return;
		}
#endif

		if ( patch.midi_instrument != last_instrument[ chan ] )
		{
			buffer[ 0 ] = patch.midi_instrument;
			track.add_event( midi_event( current_timestamp, midi_event::program_change, channel, buffer, 1 ) );
			last_instrument[ chan ] = patch.midi_instrument;
		}
	}
	else
	{
		note = ( patch.midi_instrument & 0x7F ) << 4;
	}

	unsigned volume = 127;

	if ( c->nextvol )
	{
		volume = ( c->nextvol & 0x3F ) * 127 / 63;
		last_volume[ chan ] = volume;
	}

	if ( allvolume )
	{
		volume = volume * allvolume / 255;
	}

	if ( volume != last_sent_volume[ channel ] )
	{
		buffer[ 0 ] = 7;
		buffer[ 1 ] = volume;
		track.add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ chan ], buffer, 2 ) );
		last_sent_volume[ channel ] = volume;
	}

	if ( saved_last_note != 0xFF )
	{
		buffer[ 0 ] = saved_last_note;
		buffer[ 1 ] = 127;
		track.add_event( midi_event( current_timestamp, midi_event::note_off, last_channel[ chan ], buffer, 2 ) );
		last_note[ chan ] = 0xFF;
#ifdef ENABLE_WHEEL
		if ( channel != 9 )
		{
			note += c->lasttune;
			c->lasttune = 0;
			if ( last_pitch_wheel[ channel ] != 0 )
			{
				buffer[ 0 ] = 0;
				buffer[ 1 ] = 64;
				track.add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
				last_pitch_wheel[ channel ] = 0;
			}
		}
#endif
	}
#ifdef ENABLE_WHEEL
	if ( c->lasttune != last_pitch_wheel[ channel ] )
	{
		buffer[ 0 ] = WHEEL_SCALE_LOW( c->lasttune );
		buffer[ 1 ] = WHEEL_SCALE_HIGH( c->lasttune );
		track.add_event( midi_event( current_timestamp, midi_event::pitch_wheel, channel, buffer, 2 ) );
		last_pitch_wheel[ channel ] = c->lasttune;
	}
	if( !patch.glide || last_note[ chan ] == 0xFF )
#endif
	{
#ifdef ENABLE_WHEEL
		if( !patch.portamento || last_note[ chan ] == 0xFF )
#endif
		{
			buffer[ 0 ] = note >> 4;
			buffer[ 1 ] = patch.midi_velocity;
			track.add_event( midi_event( current_timestamp, midi_event::note_on, channel, buffer, 2 ) );
			last_note[ chan ] = note >> 4;
			last_channel[ chan ] = channel;
#ifdef ENABLE_WHEEL
			c->gototune = c->lasttune;
#endif
		}
#ifdef ENABLE_WHEEL
		else
		{
            c->gototune = note - ( last_note[ chan ] << 4 ) + c->lasttune;
			c->portspeed = patch.portamento;
			buffer[ 0 ] = last_note[ chan ] = saved_last_note;
			buffer[ 1 ] = patch.midi_velocity;
			track.add_event( midi_event( current_timestamp, midi_event::note_on, channel, buffer, 2 ) );
		}
#endif
	}
#ifdef ENABLE_WHEEL
	else
	{
		buffer[ 0 ] = note >> 4;
		buffer[ 1 ] = patch.midi_velocity;
		track.add_event( midi_event( current_timestamp, midi_event::note_on, channel, buffer, 2 ) );
		last_note[ chan ] = note >> 4;
		last_channel[ chan ] = channel;
		c->gototune = patch.glide;
		c->portspeed = patch.portamento;
	}
#endif

#ifdef ENABLE_VIB
	if(!patch.vibrato)
	{
		c->vibwait = c->vibspeed = c->vibrate = 0;
	}
	else
	{
		c->vibwait = patch.vibrato_delay;
		// PASCAL:    c->vibspeed = ((i->vibrato >> 4) & 15) + 1;
		c->vibspeed = (patch.vibrato >> 4) + 2;
		c->vibrate = (patch.vibrato & 15) + 1;
	}
#endif

#ifdef ENABLE_TREM
	if(!(c->trmstay & 0xf0))
	{
		c->trmwait = (patch.tremolo_delay & 0xf0) >> 3;
		// PASCAL:    c->trmspeed = (i->mod_trem >> 4) & 15;
		c->trmspeed = patch.modulator_tremolo >> 4;
		c->trmrate = patch.modulator_tremolo & 15;
		c->trmcount = 0;
	}

	if(!(c->trmstay & 0x0f))
	{
		c->trcwait = (patch.tremolo_delay & 15) << 1;
		// PASCAL:    c->trcspeed = (i->car_trem >> 4) & 15;
		c->trcspeed = patch.carrier_tremolo >> 4;
		c->trcrate = patch.carrier_tremolo & 15;
		c->trccount = 0;
	}
#endif

#ifdef ENABLE_ARP
	c->arp_size = patch.arpeggio & 15;
	c->arp_speed = patch.arpeggio >> 4;
	memcpy(c->arp_tab, patch.arpeggio_table, 12);
	c->arp_pos = c->arp_count = 0;
#endif
#ifdef ENABLE_VIB
	c->vibcount = 0;
#endif
#ifdef ENABLE_WHEEL
	c->glideto = 0;
#endif
	c->keycount = patch.keyoff;
	c->nextvol = c->finetune = 0;
}

bool midi_processor::process_lds( std::vector<uint8_t> const& p_file, midi_container & p_out )
{
	struct position_data
	{
        uint16_t pattern_number;
        uint8_t transpose;
	};

    uint8_t mode;
    /*uint16_t speed;*/
    uint8_t tempo;
    uint8_t pattern_length;
    uint8_t channel_delay[ 9 ];
    /*uint8_t register_bd;*/
    uint16_t patch_count;
    std::vector<sound_patch> patches;
    uint16_t position_count;
    std::vector<position_data> positions;
    std::size_t pattern_count;
    std::vector<uint16_t> patterns;

    std::vector<uint8_t>::const_iterator it = p_file.begin();
	std::vector<uint8_t>::const_iterator end = p_file.end();

	if ( end == it ) return false;
    mode = *it++;
    if ( mode > 2 ) return false; /*throw exception_io_data( "Invalid LDS mode" );*/
    /*speed = it[ 0 ] | ( it[ 1 ] << 8 );*/
	if ( end - it < 4 ) return false;
    tempo = it[ 2 ];
    pattern_length = it[ 3 ];
    it += 4;
	if ( end - it < 9 ) return false;
	for ( unsigned i = 0; i < 9; ++i )
        channel_delay[ i ] = *it++;
    /*register_bd = *it++;*/ it++;

	if ( end - it < 2 ) return false;
    patch_count = it[ 0 ] | ( it[ 1 ] << 8 );
	if ( !patch_count ) return false;
    it += 2;
    patches.resize( patch_count );
	if ( end - it < 46 * patch_count ) return false;
	for ( unsigned i = 0; i < patch_count; ++i )
	{
		sound_patch & patch = patches[ i ];
        it += 11;
        patch.keyoff = *it++;
#ifdef ENABLE_WHEEL
        patch.portamento = *it++;
        patch.glide = *it++;
        it++;
#else
        it += 3;
#endif
#ifdef ENABLE_VIB
        patch.vibrato = *it++;
        patch.vibrato_delay = *it++;
#else
        it += 2;
#endif
#ifdef ENABLE_TREM
        patch.modulator_tremolo = *it++;
        patch.carrier_tremolo = *it++;
        patch.tremolo_delay = *it++;
#else
        it += 3;
#endif
#ifdef ENABLE_ARP
        patch.arpeggio = *it++;
		for ( unsigned j = 0; j < 12; ++j )
            patch.arpeggio_table[ j ] = *it++;
        it += 7;
#else
        it += 20;
#endif
        patch.midi_instrument = *it++;
        patch.midi_velocity = *it++;
        patch.midi_key = *it++;
        patch.midi_transpose = *it++;
        it += 2;

#ifdef ENABLE_WHEEL
		// hax
		if ( patch.midi_instrument >= 0x80 )
		{
			patch.glide = 0;
		}
#endif
	}

	if ( end - it < 2 ) return false;
    position_count = it[ 0 ] | ( it[ 1 ] << 8 );
	if ( !position_count ) return false;
    it += 2;
    positions.resize( 9 * position_count );
	if ( end - it < 3 * position_count ) return false;
	for ( unsigned i = 0; i < position_count; ++i )
	{
		for ( unsigned j = 0; j < 9; ++j )
		{
			position_data & position = positions[ i * 9 + j ];
            position.pattern_number = it[ 0 ] | ( it[ 1 ] << 8 );
            if ( position.pattern_number & 1 ) return false; /*throw exception_io_data( "Odd LDS pattern number" );*/
			position.pattern_number >>= 1;
            position.transpose = it[ 2 ];
            it += 3;
		}
	}

	if ( end - it < 2 ) return false;
    it += 2;

    pattern_count = ( end - it ) / 2;
    patterns.resize( pattern_count );
	for ( unsigned i = 0; i < pattern_count; ++i )
	{
        patterns[ i ] = it[ 0 ] | ( it[ 1 ] << 8 );
        it += 2;
	}

    uint8_t /*jumping,*/ fadeonoff, allvolume, hardfade, tempo_now, pattplay;
    uint16_t posplay, jumppos;
    uint32_t mainvolume;
    std::vector<channel_state> channel;
    channel.resize( 9 );
    std::vector<unsigned> position_timestamps;
    position_timestamps.resize( position_count, ~0u );

    uint8_t current_instrument[9] = { 0 };

    uint8_t last_channel[9];
    uint8_t last_instrument[9];
    uint8_t last_note[9];
    uint8_t last_volume[9];
    uint8_t last_sent_volume[11];
#ifdef ENABLE_WHEEL
    int16_t last_pitch_wheel[11];
#endif
    uint8_t ticks_without_notes[11];

	memset( last_channel, 0, sizeof( last_channel ) );
	memset( last_instrument, 0xFF, sizeof( last_instrument ) );
	memset( last_note, 0xFF, sizeof( last_note ) );
	memset( last_volume, 127, sizeof( last_volume ) );
	memset( last_sent_volume, 127, sizeof( last_sent_volume ) );
#ifdef ENABLE_WHEEL
	memset( last_pitch_wheel, 0, sizeof( last_pitch_wheel ) );
#endif
	memset( ticks_without_notes, 0, sizeof( ticks_without_notes ) );

	unsigned current_timestamp = 0;

    uint8_t buffer[ 2 ];

	p_out.initialize( 1, 35 );

	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, lds_default_tempo, _countof( lds_default_tempo ) ) );
		for ( unsigned i = 0; i < 11; ++i )
		{
			buffer[ 0 ] = 120;
			buffer[ 1 ] = 0;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 121;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
#ifdef ENABLE_WHEEL
			buffer[ 0 ] = 0x65;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0x64;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0x06;
			buffer[ 1 ] = WHEEL_RANGE_HIGH;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0x26;
			buffer[ 1 ] = WHEEL_RANGE_LOW;
			track.add_event( midi_event( 0, midi_event::control_change, i, buffer, 2 ) );
			buffer[ 0 ] = 0;
			buffer[ 1 ] = 64;
			track.add_event( midi_event( 0, midi_event::pitch_wheel, i, buffer, 2 ) );
#endif
		}
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
		p_out.add_track( track );
	}

    std::vector<midi_track> tracks;
	{
		midi_track track;
		track.add_event( midi_event( 0, midi_event::extended, 0, end_of_track, _countof( end_of_track ) ) );
        tracks.resize( 10, track );
	}

	tempo_now = 3;
    /*jumping = 0;*/
	fadeonoff = 0;
	allvolume = 0;
	hardfade = 0;
	pattplay = 0;
	posplay = 0;
	jumppos = 0;
	mainvolume = 0;
    memset( &channel[0], 0, sizeof( channel_state ) * 9 );

    const uint16_t maxsound = 0x3F;
    const uint16_t maxpos = 0xFF;

	bool playing = true;
	while ( playing )
	{
        uint16_t        chan;
#ifdef ENABLE_VIB
        uint16_t        wibc;
#endif
#if defined(ENABLE_VIB) || defined(ENABLE_ARP)
        int16_t         tune;
#endif
#ifdef ENABLE_ARP
        int16_t         arpreg;
#endif
#ifdef ENABLE_TREM
        uint16_t        tremc;
#endif
		bool            vbreak;
		unsigned        i;
		channel_state * c;

		if(fadeonoff)
		{
			if(fadeonoff <= 128)
			{
				if(allvolume > fadeonoff || allvolume == 0)
				{
					allvolume -= fadeonoff;
				}
				else
				{
					allvolume = 1;
					fadeonoff = 0;
					if(hardfade != 0)
					{
						playing = false;
						hardfade = 0;
						for(i = 0; i < 9; i++)
							channel[i].keycount = 1;
					}
				}
			}
			else if((unsigned)((allvolume + (0x100 - fadeonoff)) & 0xff) <= mainvolume)
			{
				allvolume += 0x100 - fadeonoff;
			}
			else
			{
				allvolume = mainvolume;
				fadeonoff = 0;
			}
		}

		// handle channel delay
		for(chan = 0; chan < 9; ++chan)
		{
			channel_state * _c = &channel[chan];
			if(_c->chancheat.chandelay)
			{
				if(!(--_c->chancheat.chandelay))
				{
					playsound( current_instrument, patches, last_note, last_channel, last_instrument, last_volume, last_sent_volume,
#ifdef ENABLE_WHEEL
						last_pitch_wheel,
#endif
						_c, allvolume, current_timestamp, _c->chancheat.sound, chan, _c->chancheat.high, tracks[ chan ] );
					ticks_without_notes[ last_channel[ chan ] ] = 0;
				}
			}
		}

		// handle notes
		if(!tempo_now)
		{
            if ( pattplay == 0 && position_timestamps[ posplay ] == ~0u )
			{
				position_timestamps[ posplay ] = current_timestamp;
			}

			vbreak = false;
			for(unsigned _chan = 0; _chan < 9; _chan++)
			{
				channel_state * _c = &channel[_chan];
				if(!_c->packwait)
				{
					unsigned short	patnum = positions[posplay * 9 + _chan].pattern_number;
					unsigned char	transpose = positions[posplay * 9 + _chan].transpose;

                    if ( (unsigned long)(patnum + _c->packpos) >= patterns.size() ) return false; /*throw exception_io_data( "Invalid LDS pattern number" );*/

					unsigned comword = patterns[patnum + _c->packpos];
					unsigned comhi = comword >> 8;
					unsigned comlo = comword & 0xff;
					if(comword)
					{
						if(comhi == 0x80)
						{
							_c->packwait = comlo;
						}
						else if(comhi >= 0x80)
						{
							switch(comhi) {
							case 0xff:
								{
									unsigned volume = ( comlo & 0x3F ) * 127 / 63;
									last_volume[ _chan ] = volume;
									if ( volume != last_sent_volume[ last_channel[ _chan ] ] )
									{
										buffer[ 0 ] = 7;
										buffer[ 1 ] = volume;
										tracks[ _chan ].add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ _chan ], buffer, 2 ) );
										last_sent_volume[ last_channel [ _chan ] ] = volume;
									}
								}
								break;
							case 0xfe:
								tempo = comword & 0x3f;
								break;
							case 0xfd:
								_c->nextvol = comlo;
								break;
							case 0xfc:
								playing = false;
								// in real player there's also full keyoff here, but we don't need it
								break;
							case 0xfb:
								_c->keycount = 1;
								break;
							case 0xfa:
								vbreak = true;
								jumppos = (posplay + 1) & maxpos;
								break;
							case 0xf9:
								vbreak = true;
								jumppos = comlo & maxpos;
                                /*jumping = 1;*/
								if(jumppos <= posplay)
								{
									p_out.add_track_event( 0, midi_event( position_timestamps[ jumppos ], midi_event::extended, 0, loop_start, _countof( loop_start ) ) );
									p_out.add_track_event( 0, midi_event( current_timestamp + tempo - 1, midi_event::extended, 0, loop_end, _countof( loop_end ) ) );
									playing = false;
								}
								break;
							case 0xf8:
#ifdef ENABLE_WHEEL
								_c->lasttune = 0;
#endif
								break;
							case 0xf7:
#ifdef ENABLE_VIB
								_c->vibwait = 0;
								// PASCAL: _c->vibspeed = ((comlo >> 4) & 15) + 2;
								_c->vibspeed = (comlo >> 4) + 2;
								_c->vibrate = (comlo & 15) + 1;
#endif
								break;
							case 0xf6:
#ifdef ENABLE_WHEEL
								_c->glideto = comlo;
#endif
								break;
							case 0xf5:
								_c->finetune = comlo;
								break;
							case 0xf4:
								if(!hardfade) {
									allvolume = mainvolume = comlo;
									fadeonoff = 0;
								}
								break;
							case 0xf3:
								if(!hardfade) fadeonoff = comlo;
								break;
							case 0xf2:
#ifdef ENABLE_TREM
								_c->trmstay = comlo;
#endif
								break;
							case 0xf1:
								buffer[ 0 ] = 10;
								buffer[ 1 ] = ( comlo & 0x3F ) * 127 / 63;
								tracks[ _chan ].add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ _chan ], buffer, 2 ) );
								break;
							case 0xf0:
								buffer[ 0 ] = comlo & 0x7F;
								tracks[ _chan ].add_event( midi_event( current_timestamp, midi_event::program_change, last_channel[ _chan ], buffer, 1 ) );
								break;
							default:
#ifdef ENABLE_WHEEL
								if(comhi < 0xa0)
									_c->glideto = comhi & 0x1f;
#endif
								break;
							}
						}
						else
						{
							unsigned char	sound;
							unsigned short	high;
							signed char	transp = transpose << 1;
							transp >>= 1;

							if(transpose & 128) {
								sound = (comlo + transp) & maxsound;
								high = comhi << 4;
							} else {
								sound = comlo & maxsound;
								high = (comhi + transp) << 4;
							}

							/*
							PASCAL:
							sound = comlo & maxsound;
							high = (comhi + (((transpose + 0x24) & 0xff) - 0x24)) << 4;
							*/

							if( !channel_delay[ _chan ] )
							{
								playsound( current_instrument, patches, last_note, last_channel, last_instrument, last_volume, last_sent_volume,
#ifdef ENABLE_WHEEL
									last_pitch_wheel,
#endif
									_c, allvolume, current_timestamp, sound, _chan, high, tracks[ _chan ] );
								ticks_without_notes[ last_channel[ _chan ] ] = 0;
							}
							else
							{
								_c->chancheat.chandelay = channel_delay[_chan];
								_c->chancheat.sound = sound;
								_c->chancheat.high = high;
							}
						}
					}
					_c->packpos++;
				}
				else
				{
					_c->packwait--;
				}
			}

			tempo_now = tempo;
			/*
			The continue table is updated here, but this is only used in the
			original player, which can be paused in the middle of a song and then
			unpaused. Since AdPlug does all this for us automatically, we don't
			have a continue table here. The continue table update code is noted
			here for reference only.

			if(!pattplay) {
			conttab[speed & maxcont].position = posplay & 0xff;
			conttab[speed & maxcont].tempo = tempo;
			}
			*/
			pattplay++;
			if(vbreak)
			{
				pattplay = 0;
				for(i = 0; i < 9; i++) channel[i].packpos = channel[i].packwait = 0;
				posplay = jumppos;
                if ( posplay >= position_count ) return false; /*throw exception_io_data( "Invalid LDS position jump" );*/
			}
			else if(pattplay >= pattern_length)
			{
				pattplay = 0;
				for(i = 0; i < 9; i++) channel[i].packpos = channel[i].packwait = 0;
				posplay = (posplay + 1) & maxpos;
				if ( posplay >= position_count ) playing = false; //throw exception_io_data( "LDS reached the end without a loop or end command" );
			}
		}
		else
		{
			tempo_now--;
		}

		// make effects
		for(chan = 0; chan < 9; ++chan)
		{
			c = &channel[chan];
			if(c->keycount > 0)
			{
				if( c->keycount == 1 && last_note[ chan ] != 0xFF )
				{
					buffer[ 0 ] = last_note[ chan ];
					buffer[ 1 ] = 127;
					tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::note_off, last_channel[ chan ], buffer, 2 ) );
					last_note[ chan ] = 0xFF;
#ifdef ENABLE_WHEEL
					if ( 0 != last_pitch_wheel[ last_channel[ chan ] ] )
					{
						buffer[ 0 ] = 0;
						buffer[ 1 ] = 64;
						tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
						last_pitch_wheel[ last_channel[ chan ] ] = 0;
						c->lasttune = 0;
						c->gototune = 0;
					}
#endif
				}
				c->keycount--;
			}

#ifdef ENABLE_ARP
			// arpeggio
			if(c->arp_size == 0)
			{
				arpreg = 0;
			}
			else
			{
				arpreg = c->arp_tab[c->arp_pos] << 4;
				if(arpreg == -0x800)
				{
					if(c->arp_pos > 0) c->arp_tab[0] = c->arp_tab[c->arp_pos - 1];
					c->arp_size = 1; c->arp_pos = 0;
					arpreg = c->arp_tab[0] << 4;
				}

				if(c->arp_count == c->arp_speed) {
					c->arp_pos++;
					if(c->arp_pos >= c->arp_size) c->arp_pos = 0;
					c->arp_count = 0;
				}
				else
				{
					c->arp_count++;
				}
			}
#endif

#ifdef ENABLE_WHEEL
			// glide & portamento
			if(c->lasttune != c->gototune)
			{
				if(c->lasttune > c->gototune)
				{
					if(c->lasttune - c->gototune < c->portspeed)
					{
						c->lasttune = c->gototune;
					}
					else
					{
						c->lasttune -= c->portspeed;
					}
				}
				else
				{
					if(c->gototune - c->lasttune < c->portspeed)
					{
						c->lasttune = c->gototune;
					}
					else
					{
						c->lasttune += c->portspeed;
					}
				}

#ifdef ENABLE_ARP
				arpreg +=
#else
                int16_t arpreg =
#endif
					c->lasttune;

				if ( arpreg != last_pitch_wheel[ last_channel[ chan ] ] )
				{
					buffer[ 0 ] = WHEEL_SCALE_LOW( arpreg );
					buffer[ 1 ] = WHEEL_SCALE_HIGH( arpreg );
					tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
					last_pitch_wheel[ last_channel[ chan ] ] = arpreg;
				}
			} else
			{
#ifdef ENABLE_VIB
				// vibrato
				if(!c->vibwait)
				{
					if(c->vibrate)
					{
						wibc = vibtab[c->vibcount & 0x3f] * c->vibrate;

						if((c->vibcount & 0x40) == 0)
							tune = c->lasttune + (wibc >> 8);
						else
							tune = c->lasttune - (wibc >> 8);

#ifdef ENABLE_ARP
						tune += arpreg;
#endif

						if ( tune != last_pitch_wheel[ last_channel[ chan ] ] )
						{
							buffer[ 0 ] = WHEEL_SCALE_LOW( tune );
							buffer[ 1 ] = WHEEL_SCALE_HIGH( tune );
							tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
							last_pitch_wheel[ last_channel[ chan ] ] = tune;
						}

						c->vibcount += c->vibspeed;
					}
#ifdef ENABLE_ARP
					else if(c->arp_size != 0)
					{	// no vibrato, just arpeggio
						tune = c->lasttune + arpreg;

						if ( tune != last_pitch_wheel[ last_channel[ chan ] ] )
						{
							buffer[ 0 ] = WHEEL_SCALE_LOW( tune );
							buffer[ 1 ] = WHEEL_SCALE_HIGH( tune );
							tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
							last_pitch_wheel[ last_channel[ chan ] ] = tune;
						}
					}
#endif
				}
#ifdef ENABLE_ARP
				else
#endif
#endif
#ifdef ENABLE_ARP
				{	// no vibrato, just arpeggio
#ifdef ENABLE_VIB
					c->vibwait--;
#endif

					if(c->arp_size != 0)
					{
						tune = c->lasttune + arpreg;

						if ( tune != last_pitch_wheel[ last_channel[ chan ] ] )
						{
							buffer[ 0 ] = WHEEL_SCALE_LOW( tune );
							buffer[ 1 ] = WHEEL_SCALE_HIGH( tune );
							tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::pitch_wheel, last_channel[ chan ], buffer, 2 ) );
							last_pitch_wheel[ last_channel[ chan ] ] = tune;
						}
					}
				}
#endif
			}
#endif

#ifdef ENABLE_TREM
			unsigned volume = last_volume[ chan ];

			// tremolo (modulator)
			if(!c->trmwait)
			{
				if(c->trmrate)
				{
					tremc = tremtab[c->trmcount & 0x7f] * c->trmrate;
					if((tremc >> 7) <= volume)
						volume = volume - (tremc >> 7);
					else
						volume = 0;

					c->trmcount += c->trmspeed;
				}
			}
			else
			{
				c->trmwait--;
			}

			// tremolo (carrier)
			if(!c->trcwait)
			{
				if(c->trcrate)
				{
					tremc = tremtab[c->trccount & 0x7f] * c->trcrate;
					if((tremc >> 7) <= volume)
						volume = volume - (tremc >> 8);
					else
						volume = 0;
				}
			}
			else
			{
				c->trcwait--;
			}

			if ( allvolume )
			{
				volume = volume * allvolume / 255;
			}

			if ( volume != last_sent_volume[ last_channel[ chan ] ] )
			{
				buffer[ 0 ] = 7;
				buffer[ 1 ] = volume;
				tracks[ chan ].add_event( midi_event( current_timestamp, midi_event::control_change, last_channel[ chan ], buffer, 2 ) );
				last_sent_volume[ last_channel[ chan ] ] = volume;
			}
#endif

		}

		++current_timestamp;
	}

	--current_timestamp;

	for ( unsigned i = 0; i < 9; ++i )
	{
		midi_track & track = tracks[ i ];
		unsigned long count = track.get_count();
		if ( count > 1 )
		{
			if ( last_note[ i ] != 0xFF )
			{
				buffer[ 0 ] = last_note[ i ];
				buffer[ 1 ] = 127;
				track.add_event( midi_event( current_timestamp + channel[ i ].keycount, midi_event::note_off, last_channel[ i ], buffer, 2 ) );
#ifdef ENABLE_WHEEL
				if ( last_pitch_wheel[ last_channel[ i ] ] != 0 )
				{
					buffer[ 0 ] = 0;
					buffer[ 1 ] = 0x40;
					track.add_event( midi_event( current_timestamp + channel[ i ].keycount, midi_event::pitch_wheel, last_channel[ i ], buffer, 2 ) );
				}
#endif
			}
			p_out.add_track( track );
		}
	}

    return true;
}
