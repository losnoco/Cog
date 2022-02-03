#include <assert.h>

#include "MIDIPlayer.h"

MIDIPlayer::MIDIPlayer()
{
	uSamplesRemaining = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;
    initialized = false;
}

void MIDIPlayer::setSampleRate(unsigned long rate)
{
	if (mStream.size())
	{
		for (unsigned long i = 0; i < mStream.size(); i++)
		{
			mStream[i].m_timestamp = (unsigned long)((uint64_t)mStream[i].m_timestamp * rate / uSampleRate);
		}
	}

	if (uTimeCurrent)
	{
		uTimeCurrent = (unsigned long)((uint64_t)uTimeCurrent * rate / uSampleRate);
	}

	if (uTimeEnd)
	{
		uTimeEnd = (unsigned long)((uint64_t)uTimeEnd * rate / uSampleRate);
	}

	if (uTimeLoopStart)
	{
		uTimeLoopStart = (unsigned long)((uint64_t)uTimeLoopStart * rate / uSampleRate);
	}

	uSampleRate = rate;

	shutdown();
}

bool MIDIPlayer::Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags)
{
	assert(!mStream.size());

	midi_file.serialize_as_stream( subsong, mStream, mSysexMap, uStreamLoopStart, uStreamEnd, clean_flags );

	if (mStream.size())
	{
		uStreamPosition = 0;
		uTimeCurrent = 0;

		uLoopMode = loop_mode;

		uTimeEnd = midi_file.get_timestamp_end( subsong, true ) + 1000;

		if (uLoopMode & loop_mode_enable)
		{
			uTimeLoopStart = midi_file.get_timestamp_loop_start( subsong, true );
			unsigned long uTimeLoopEnd = midi_file.get_timestamp_loop_end( subsong, true );

			if ( uTimeLoopStart != ~0UL || uTimeLoopEnd != ~0UL )
			{
				uLoopMode |= loop_mode_force;
			}

			if ( uTimeLoopStart == ~0UL )
				uTimeLoopStart = 0;
			if ( uTimeLoopEnd == ~0UL )
				uTimeLoopEnd = uTimeEnd - 1000;

			if ((uLoopMode & loop_mode_force))
			{
				unsigned long i;
				unsigned char note_on[128 * 16];
				memset( note_on, 0, sizeof( note_on ) );
				for (i = 0; i < mStream.size() && i < uStreamEnd; i++)
				{
					uint32_t ev = mStream[ i ].m_event & 0x800000F0;
					if ( ev == 0x90 || ev == 0x80 )
					{
						unsigned long port = ( mStream[ i ].m_event & 0x7F000000 ) >> 24;
						unsigned long ch = mStream[ i ].m_event & 0x0F;
						unsigned long note = ( mStream[ i ].m_event >> 8 ) & 0x7F;
						unsigned long on = ( ev == 0x90 ) && ( mStream[ i ].m_event & 0xFF0000 );
						unsigned long bit = 1 << port;
						note_on [ ch * 128 + note ] = ( note_on [ ch * 128 + note ] & ~bit ) | ( bit * on );
					}
				}
				mStream.resize( i );
				uTimeEnd = uTimeLoopEnd - 1;
				if ( uTimeEnd < mStream[ i - 1 ].m_timestamp )
					uTimeEnd = mStream[ i - 1 ].m_timestamp;
				for ( unsigned long j = 0; j < 128 * 16; j++ )
				{
					if ( note_on[ j ] )
					{
						for ( unsigned long k = 0; k < 8; k++ )
						{
							if ( note_on[ j ] & ( 1 << k ) )
							{
								mStream.push_back( midi_stream_event( uTimeEnd, (uint32_t)(( k << 24 ) + ( j >> 7 ) + ( j & 0x7F ) * 0x100 + 0x90 )) );
							}
						}
					}
				}
				uTimeEnd = uTimeLoopEnd;
			}
		}

		if (uSampleRate != 1000)
		{
			unsigned long rate = uSampleRate;
			uSampleRate = 1000;
			setSampleRate(rate);
		}

		return true;
	}

	return false;
}

unsigned long MIDIPlayer::Play(float * out, unsigned long count)
{
	assert(mStream.size());

	if ( !startup() ) return 0;

	unsigned long done = 0;
    
    unsigned int needs_block_size = send_event_needs_time();
    unsigned int into_block = 0;
    
    // This should be a multiple of block size, and have leftover

	while ( uSamplesRemaining )
	{
		unsigned long todo = uSamplesRemaining;
		if (todo > done - count) todo = done - count;
        if (needs_block_size && todo > needs_block_size)
            todo = needs_block_size;
        if (todo < needs_block_size)
        {
            uSamplesRemaining = 0;
            into_block = todo;
            break;
        }
		render( out + done * 2, todo );
		uSamplesRemaining -= todo;
		done += todo;
        uTimeCurrent += todo;
	}

	while (done < count)
	{
		unsigned long todo = uTimeEnd - uTimeCurrent;
		if (todo > count - done) todo = count - done;

		unsigned long time_target = todo + uTimeCurrent;
		unsigned long stream_end = uStreamPosition;

		while (stream_end < mStream.size() && mStream[stream_end].m_timestamp < time_target) stream_end++;

		if (stream_end > uStreamPosition)
		{
			for (; uStreamPosition < stream_end; uStreamPosition++)
			{
				midi_stream_event * me = &mStream[uStreamPosition];
				
				unsigned long samples_todo = me->m_timestamp - uTimeCurrent - into_block;
				if ( samples_todo )
				{
					if ( samples_todo > count - done )
					{
						uSamplesRemaining = samples_todo - ( count - done );
						samples_todo = count - done;
					}
                    if (!needs_block_size && samples_todo)
                    {
                        render( out + done * 2, samples_todo );
                        done += samples_todo;
                        uTimeCurrent += samples_todo;
                    }

					if ( uSamplesRemaining )
					{
                        uSamplesRemaining += into_block;
						return done;
					}
				}


                if (needs_block_size)
                {
                    into_block += samples_todo;
                    while (into_block >= needs_block_size)
                    {
                        render( out + done * 2, needs_block_size );
                        done += needs_block_size;
                        into_block -= needs_block_size;
                        uTimeCurrent += needs_block_size;
                    }
                    send_event_time_filtered( me->m_event, into_block );
                }
                else
                    send_event_filtered( me->m_event );
			}
		}

        if ( done < count )
        {
            unsigned long samples_todo;
            if ( uStreamPosition < mStream.size() ) samples_todo = mStream[uStreamPosition].m_timestamp;
            else samples_todo = uTimeEnd;
            samples_todo -= uTimeCurrent;
            if ( needs_block_size )
                into_block = samples_todo;
            if ( samples_todo > count - done )
                samples_todo = count - done;
            if ( needs_block_size && samples_todo > needs_block_size )
                samples_todo = needs_block_size;
            if ( samples_todo >= needs_block_size )
            {
                render( out + done * 2, samples_todo );
                done += samples_todo;
                uTimeCurrent += samples_todo;
                if (needs_block_size)
                    into_block -= samples_todo;
            }
        }
        
        if (!needs_block_size)
            uTimeCurrent = time_target;

        if (time_target >= uTimeEnd)
		{
			if ( uStreamPosition < mStream.size() )
			{
				for (; uStreamPosition < mStream.size(); uStreamPosition++)
				{
                    if ( needs_block_size )
                        send_event_time_filtered( mStream[ uStreamPosition ].m_event, into_block );
                    else
                        send_event_filtered( mStream[ uStreamPosition ].m_event );
				}
			}

			if ((uLoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
			{
				if (uStreamLoopStart == ~0)
				{
					uStreamPosition = 0;
					uTimeCurrent = 0;
				}
				else
				{
					uStreamPosition = uStreamLoopStart;
					uTimeCurrent = uTimeLoopStart;
				}
			}
			else break;
		}
    }

    uSamplesRemaining = into_block;

	return done;
}

void MIDIPlayer::Seek(unsigned long sample)
{
	if (sample >= uTimeEnd)
	{
		if ((uLoopMode & (loop_mode_enable | loop_mode_force)) == (loop_mode_enable | loop_mode_force))
		{
			while (sample >= uTimeEnd) sample -= uTimeEnd - uTimeLoopStart;
		}
		else
		{
			sample = uTimeEnd;
		}
	}

	if (uTimeCurrent > sample)
	{
		// hokkai, let's kill any hanging notes
		uStreamPosition = 0;

		shutdown();
	}

	if (!startup()) return;

	uTimeCurrent = sample;

	std::vector<midi_stream_event> filler;

	unsigned long stream_start = uStreamPosition;

	for (; uStreamPosition < mStream.size() && mStream[uStreamPosition].m_timestamp < uTimeCurrent; uStreamPosition++);

	uSamplesRemaining = mStream[uStreamPosition].m_timestamp - uTimeCurrent;

	if (uStreamPosition > stream_start)
	{
		filler.resize( uStreamPosition - stream_start );
		filler.assign( &mStream[stream_start], &mStream[uStreamPosition] );

		unsigned long i, j;
		midi_stream_event * me = &filler[0];

		for (i = 0, stream_start = uStreamPosition - stream_start; i < stream_start; i++)
		{
			midi_stream_event & e = me[i];
			if ((e.m_event & 0x800000F0) == 0x90 && (e.m_event & 0xFF0000)) // note on
			{
				if ((e.m_event & 0x0F) == 9) // hax
				{
					e.m_event = 0;
					continue;
				}
				uint32_t m = (e.m_event & 0x7F00FF0F) | 0x80; // note off
				uint32_t m2 = e.m_event & 0x7F00FFFF; // also note off
				for (j = i + 1; j < stream_start; j++)
				{
					midi_stream_event & e2 = me[j];
					if ((e2.m_event & 0xFF00FFFF) == m || e2.m_event == m2)
					{
						// kill 'em
						e.m_event = 0;
						e2.m_event = 0;
						break;
					}
				}
			}
		}
        
        float * temp;
        unsigned int needs_time = send_event_needs_time();

        if (needs_time)
        {
            temp = (float *) malloc(needs_time * 2 * sizeof(float));
            if (temp)
            {
                unsigned int render_junk = 0;
                for (i = 0; i < stream_start; i++)
                {
                    if (me[i].m_event)
                    {
                        send_event_time_filtered(me[i].m_event, render_junk);
                        render_junk += 16;
                        if (render_junk >= needs_time)
                        {
                            render(temp, needs_time);
                            render_junk -= needs_time;
                        }
                    }
                }
                uSamplesRemaining = render_junk;
                free(temp);
            }
        }
        else
        {
            temp = (float *) malloc(16 * 2 * sizeof(float));
            if (temp)
            {
                for (i = 0; i < stream_start; i++)
                {
                    if (me[i].m_event)
                    {
                        send_event_filtered(me[i].m_event);
                    }
                }
                free(temp);
            }
        }
	}
}

void MIDIPlayer::setLoopMode(unsigned int mode)
{
    if (uLoopMode != mode)
    {
        if (mode & loop_mode_enable)
            uTimeEnd -= uSampleRate;
        else
            uTimeEnd += uSampleRate;
    }
    uLoopMode = mode;
}

void MIDIPlayer::send_event_filtered(uint32_t b)
{
    if (!(b & 0x80000000u))
    {
        send_event(b);
    }
    else
    {
        unsigned int p_index = b & 0xffffff;
        const uint8_t * p_data;
        size_t p_size, p_port;
        mSysexMap.get_entry(p_index, p_data, p_size, p_port);
        send_sysex_filtered(p_data, p_size, p_port);
    }
}

void MIDIPlayer::send_event_time_filtered(uint32_t b, unsigned int time)
{
    if (!(b & 0x80000000u))
    {
        if (reverb_chorus_disabled)
        {
            uint32_t _b = b & 0x7FF0;
            if (_b == 0x5BB0 || _b == 0x5DB0)
                return;
        }
        send_event_time(b, time);
    }
    else
    {
        unsigned int p_index = b & 0xffffff;
        const uint8_t * p_data;
        size_t p_size, p_port;
        mSysexMap.get_entry(p_index, p_data, p_size, p_port);
        send_sysex_time_filtered(p_data, p_size, p_port, time);
    }
}

void MIDIPlayer::setFilterMode(filter_mode m, bool disable_reverb_chorus)
{
    mode = m;
    reverb_chorus_disabled = disable_reverb_chorus;
    if (initialized)
    {
        sysex_reset(0, 0);
        sysex_reset(1, 0);
        sysex_reset(2, 0);
    }
}

static const uint8_t syx_reset_gm[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static const uint8_t syx_reset_gm2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };
static const uint8_t syx_reset_gs[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7 };
static const uint8_t syx_reset_xg[] = { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

static const uint8_t syx_gs_limit_bank_lsb[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x41, 0x00, 0x03, 0x00, 0xF7 };

static bool syx_equal(const uint8_t * a, const uint8_t * b)
{
    while (*a != 0xF7 && *b != 0xF7 && *a == *b)
    {
        a++; b++;
    }
    
    return *a == *b;
}

static bool syx_is_reset(const uint8_t * data)
{
    return syx_equal(data, syx_reset_gm) || syx_equal(data, syx_reset_gm2) || syx_equal(data, syx_reset_gs) || syx_equal(data, syx_reset_xg);
}

void MIDIPlayer::sysex_send_gs(size_t port, uint8_t * data, size_t size, unsigned int time)
{
    unsigned long i;
    unsigned char checksum = 0;
    for (i = 5; i + 1 < size && data[i+1] != 0xF7; ++i)
        checksum += data[i];
    checksum = (128 - checksum) & 127;
    data[i] = checksum;
    if (time)
        send_sysex_time(data, size, port, time);
    else
        send_sysex(data, size, port);
}

void MIDIPlayer::sysex_reset_sc(uint32_t port, unsigned int time)
{
    unsigned int i;
    uint8_t message[11];
    
    memcpy(message, syx_gs_limit_bank_lsb, 11);

    message[7] = 1;

    switch (mode)
    {
        default: break;
            
        case filter_sc55:
            message[8] = 1;
            break;
            
        case filter_sc88:
            message[8] = 2;
            break;
            
        case filter_sc88pro:
            message[8] = 3;
            break;
            
        case filter_sc8850:
        case filter_default:
            message[8] = 4;
            break;
    }
    
    for (i = 0x41; i <= 0x49; ++i)
    {
        message[6] = i;
        sysex_send_gs(port, message, sizeof(message), time);
    }
    message[6] = 0x40;
    sysex_send_gs(port, message, sizeof(message), time);
    for (i = 0x4A; i <= 0x4F; ++i)
    {
        message[6] = i;
        sysex_send_gs(port, message, sizeof(message), time);
    }
}

void MIDIPlayer::sysex_reset(size_t port, unsigned int time)
{
    if (initialized)
    {
        if (time)
        {
            send_sysex_time(syx_reset_xg, sizeof(syx_reset_xg), port, time);
            send_sysex_time(syx_reset_gm2, sizeof(syx_reset_gm2), port, time);
            send_sysex_time(syx_reset_gm, sizeof(syx_reset_gm), port, time);
        }
        else
        {
            send_sysex(syx_reset_xg, sizeof(syx_reset_xg), port);
            send_sysex(syx_reset_gm2, sizeof(syx_reset_gm2), port);
            send_sysex(syx_reset_gm, sizeof(syx_reset_gm), port);
        }
        
        switch (mode)
        {
            case filter_gm:
                /*
                if (time)
                    send_sysex_time(syx_reset_gm, sizeof(syx_reset_gm), port, time);
                else
                    send_sysex(syx_reset_gm, sizeof(syx_reset_gm), port);
                 */
                break;
                
            case filter_gm2:
                if (time)
                    send_sysex_time(syx_reset_gm2, sizeof(syx_reset_gm2), port, time);
                else
                    send_sysex(syx_reset_gm2, sizeof(syx_reset_gm2), port);
                break;
                
            case filter_sc55:
            case filter_sc88:
            case filter_sc88pro:
            case filter_sc8850:
            case filter_default:
                if (time)
                    send_sysex_time(syx_reset_gs, sizeof(syx_reset_gs), port, time);
                else
                    send_sysex(syx_reset_gs, sizeof(syx_reset_gs), port);
                sysex_reset_sc(port, time);
                break;
                
            case filter_xg:
                if (time)
                    send_sysex_time(syx_reset_xg, sizeof(syx_reset_xg), port, time);
                else
                    send_sysex(syx_reset_xg, sizeof(syx_reset_xg), port);
                break;
        }
        
        {
            unsigned int i;
            for (i = 0; i < 16; ++i)
            {
                if (time)
                {
                    send_event_time(0x78B0 + i + (port << 24), time);
                    send_event_time(0x79B0 + i + (port << 24), time);
                    if (mode != filter_xg || i != 9)
                    {
                        send_event_time(0x20B0 + i + (port << 24), time);
                        send_event_time(0x00B0 + i + (port << 24), time);
                        send_event_time(0xC0 + i + (port << 24), time);
                    }
                }
                else
                {
                    send_event(0x78B0 + i + (port << 24));
                    send_event(0x79B0 + i + (port << 24));
                    if (mode != filter_xg || i != 9)
                    {
                        send_event(0x20B0 + i + (port << 24));
                        send_event(0x00B0 + i + (port << 24));
                        send_event(0xC0 + i + (port << 24));
                    }
                }
            }
        }
        
        if (mode == filter_xg)
        {
            if (time)
            {
                send_event_time(0x20B9 + (port << 24), time);
                send_event_time(0x7F00B9 + (port << 24), time);
                send_event_time(0xC9 + (port << 24), time);
            }
            else
            {
                send_event(0x20B9 + (port << 24));
                send_event(0x7F00B9 + (port << 24));
                send_event(0xC9 + (port << 24));
            }
        }

        if (reverb_chorus_disabled)
        {
            unsigned int i;
            if (time)
            {
                for (i = 0; i < 16; ++i)
                {
                    send_event_time(0x5BB0 + i + (port << 24), time);
                    send_event_time(0x5DB0 + i + (port << 24), time);
                }
            }
            else
            {
                for (i = 0; i < 16; ++i)
                {
                    send_event(0x5BB0 + i + (port << 24));
                    send_event(0x5DB0 + i + (port << 24));
                }
            }
        }
    }
}

void MIDIPlayer::send_sysex_filtered(const uint8_t *data, size_t size, size_t port)
{
    send_sysex(data, size, port);
    if (syx_is_reset(data) && mode != filter_default)
    {
        sysex_reset(port, 0);
    }
}

void MIDIPlayer::send_sysex_time_filtered(const uint8_t *data, size_t size, size_t port, unsigned int time)
{
    send_sysex_time(data, size, port, time);
    if (syx_is_reset(data) && mode != filter_default)
    {
        sysex_reset(port, time);
    }
}

bool MIDIPlayer::GetLastError(std::string& p_out)
{
    return get_last_error(p_out);
}
