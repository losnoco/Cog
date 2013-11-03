#include <assert.h>

#include "MIDIPlayer.h"

MIDIPlayer::MIDIPlayer()
{
	uSamplesRemaining = 0;
	uSampleRate = 1000;
	uTimeCurrent = 0;
	uTimeEnd = 0;
	uTimeLoopStart = 0;
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

	if ( uSamplesRemaining )
	{
		unsigned long todo = uSamplesRemaining;
		if (todo > count) todo = count;
		render( out, todo );
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
				
				unsigned long samples_todo = me->m_timestamp - uTimeCurrent;
				if ( samples_todo )
				{
					if ( samples_todo > count - done )
					{
						uSamplesRemaining = samples_todo - ( count - done );
						samples_todo = count - done;
					}
					render( out + done * 2, samples_todo );
					done += samples_todo;

					if ( uSamplesRemaining )
					{
						uTimeCurrent = me->m_timestamp;
						return done;
					}
				}

				send_event( me->m_event );

				uTimeCurrent = me->m_timestamp;
			}
		}

		if ( done < count )
		{
			unsigned long samples_todo;
			if ( uStreamPosition < mStream.size() ) samples_todo = mStream[uStreamPosition].m_timestamp;
			else samples_todo = uTimeEnd;
			samples_todo -= uTimeCurrent;
			if ( samples_todo > count - done ) samples_todo = count - done;
			render( out + done * 2, samples_todo );
			done += samples_todo;
		}

		uTimeCurrent = time_target;

		if (uTimeCurrent >= uTimeEnd)
		{
			if ( uStreamPosition < mStream.size() )
			{
				for (; uStreamPosition < mStream.size(); uStreamPosition++)
				{
					send_event( mStream[ uStreamPosition ].m_event );
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

		for (i = 0; i < stream_start; i++)
		{
			if (me[i].m_event)
				send_event(me[i].m_event);
		}
	}
}

