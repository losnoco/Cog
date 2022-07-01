/*
 * libopenmpt_bass.c
 * -----------------
 * Purpose: Example of how to use libopenmpt with the BASS audio library.
 * Notes  : BASS from un4seen (http://www.un4seen.com/bass.html) is used for audio output.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_bass SOMEMODULE
 */

#include <stdio.h>
#include <string.h>
#if ( defined( _WIN32 ) || defined( WIN32 ) )
#include <Windows.h>
#define sleep(n) Sleep(n)
#else
#include <unistd.h>
#endif

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>
#include <bass.h>

#define SAMPLERATE 48000

DWORD CALLBACK StreamProc(HSTREAM handle, void *buffer, DWORD length, void *user)
{
	// length is in bytes, but libopenmpt wants samples => divide by number of channels (2) and size of a sample (float = 4)
	// same for return value.
	size_t count = openmpt_module_read_interleaved_float_stereo( (openmpt_module *)user, SAMPLERATE, length / (2 * sizeof(float)), (float *)buffer );
	count *= (2 * sizeof(float));
	// Reached end of stream?
	if(count < length) count |= BASS_STREAMPROC_END;
	return (DWORD)count;
}


#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif

	FILE * file = 0;
	openmpt_module * mod = 0;
	HSTREAM stream = 0;
	int result = 0;

	if ( argc != 2 ) {
		fprintf( stderr, "Usage: %s SOMEMODULE\n", argv[0] );
		return 1;
	}

	if ( HIWORD( BASS_GetVersion() ) !=BASSVERSION ) {
		fprintf( stderr, "Error: Wrong BASS version\n" );
		return 1;
	}

	if ( !BASS_Init( -1, SAMPLERATE, 0, NULL, NULL ) ) {
		fprintf( stderr, "Error: Cannot initialize BASS (error %d)\n", BASS_ErrorGetCode() );
		return 1;
	}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Usage: %s SOMEMODULE\n", argv[0] );
		result = 1;
		goto fail;
	}
	file = _wfopen( argv[1], L"rb" );
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Usage: %s SOMEMODULE\n", argv[0] );
		result = 1;
		goto fail;
	}
	file = fopen( argv[1], "rb" );
#endif
	if ( !file ) {
		fprintf( stderr, "Error: %s\n", "fopen() failed." );
		result = 1;
		goto fail;
	}

	mod = openmpt_module_create( openmpt_stream_get_file_callbacks(), file, NULL, NULL, NULL );
	if ( !mod ) {
		fprintf( stderr, "Error: %s\n", "openmpt_module_create() failed." );
		goto fail;
	}
	
	// Create a "pull" channel. BASS will call StreamProc whenever the channel needs new data to be decoded.
	stream = BASS_StreamCreate(SAMPLERATE, 2, BASS_SAMPLE_FLOAT, StreamProc, mod);
	BASS_ChannelPlay(stream, FALSE);

	while ( BASS_ChannelIsActive( stream ) ) {
		// Do something useful here
		sleep(1);
	}

	BASS_StreamFree( stream );
	openmpt_module_destroy( mod );

fail:
	BASS_Free();
	return result;
}
