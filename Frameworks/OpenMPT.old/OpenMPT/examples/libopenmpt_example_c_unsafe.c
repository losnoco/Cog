/*
 * libopenmpt_example_c_unsafe.c
 * -----------------------------
 * Purpose: libopenmpt C API simplified example
 * Notes  : PortAudio is used for sound output.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_c_unsafe SOMEMODULE
 * CAUTION: This simple example does no error cheking at all.
 */

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>

#include <portaudio.h>

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static int16_t left[BUFFERSIZE];
static int16_t right[BUFFERSIZE];
static int16_t * const buffers[2] = { left, right };

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif
	FILE * file = 0;
	openmpt_module * mod = 0;
	size_t count = 0;
	PaStream * stream = 0;
	(void)argc;
#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	file = _wfopen( argv[1], L"rb" );
#else
	file = fopen( argv[1], "rb" );
#endif
	mod = openmpt_module_create2( openmpt_stream_get_file_callbacks(), file, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
	fclose( file );
	Pa_Initialize();
	Pa_OpenDefaultStream( &stream, 0, 2, paInt16 | paNonInterleaved, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
	Pa_StartStream( stream );
	while ( 1 ) {
		count = openmpt_module_read_stereo( mod, SAMPLERATE, BUFFERSIZE, left, right );
		if ( count == 0 ) {
			break;
		}
		Pa_WriteStream( stream, buffers, (unsigned long)count );
	}
	Pa_StopStream( stream );
	Pa_CloseStream( stream );
	Pa_Terminate();
	openmpt_module_destroy( mod );
	return 0;
}
