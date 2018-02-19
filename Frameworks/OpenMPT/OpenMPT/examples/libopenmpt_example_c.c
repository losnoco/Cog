/*
 * libopenmpt_example_c.c
 * ----------------------
 * Purpose: libopenmpt C API example
 * Notes  : PortAudio is used for sound output.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_c SOMEMODULE
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

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
	(void)userdata;
	if ( message ) {
		fprintf( stderr, "openmpt: %s\n", message );
	}
}

static int libopenmpt_example_errfunc( int error, void * userdata ) {
	(void)userdata;
	(void)error;
	return OPENMPT_ERROR_FUNC_RESULT_DEFAULT & ~OPENMPT_ERROR_FUNC_RESULT_LOG;
}

static void libopenmpt_example_print_error( const char * func_name, int mod_err, const char * mod_err_str ) {
	if ( !func_name ) {
		func_name = "unknown function";
	}
	if ( mod_err == OPENMPT_ERROR_OUT_OF_MEMORY ) {
		mod_err_str = openmpt_error_string( mod_err );
		if ( !mod_err_str ) {
			fprintf( stderr, "Error: %s\n", "OPENMPT_ERROR_OUT_OF_MEMORY" );
		} else {
			fprintf( stderr, "Error: %s\n", mod_err_str );
			openmpt_free_string( mod_err_str );
			mod_err_str = NULL;
		}
	} else {
		if ( !mod_err_str ) {
			mod_err_str = openmpt_error_string( mod_err );
			if ( !mod_err_str ) {
				fprintf( stderr, "Error: %s failed.\n", func_name );
			} else {
				fprintf( stderr, "Error: %s failed: %s\n", func_name, mod_err_str );
			}
			openmpt_free_string( mod_err_str );
			mod_err_str = NULL;
		}
		fprintf( stderr, "Error: %s failed: %s\n", func_name, mod_err_str );
	}
}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif

	int result = 0;
	FILE * file = 0;
	openmpt_module * mod = 0;
	int mod_err = OPENMPT_ERROR_OK;
	const char * mod_err_str = NULL;
	size_t count = 0;
	PaError pa_error = paNoError;
	int pa_initialized = 0;
	PaStream * stream = 0;

	if ( argc != 2 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c SOMEMODULE'." );
		goto fail;
	}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c SOMEMODULE'." );
		goto fail;
	}
	file = _wfopen( argv[1], L"rb" );
#else
	if ( strlen( argv[1] ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c SOMEMODULE'." );
		goto fail;
	}
	file = fopen( argv[1], "rb" );
#endif
	if ( !file ) {
		fprintf( stderr, "Error: %s\n", "fopen() failed." );
		goto fail;
	}

	mod = openmpt_module_create2( openmpt_stream_get_file_callbacks(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
	if ( !mod ) {
		libopenmpt_example_print_error( "openmpt_module_create2()", mod_err, mod_err_str );
		openmpt_free_string( mod_err_str );
		mod_err_str = NULL;
		goto fail;
	}
	openmpt_module_set_error_func( mod, NULL, NULL );

	pa_error = Pa_Initialize();
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_Initialize() failed." );
		goto fail;
	}
	pa_initialized = 1;

	pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16 | paNonInterleaved, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
		goto fail;
	}
	if ( !stream ) {
		fprintf( stderr, "Error: %s\n", "Pa_OpenStream() failed." );
		goto fail;
	}

	pa_error = Pa_StartStream( stream );
	if ( pa_error != paNoError ) {
		fprintf( stderr, "Error: %s\n", "Pa_StartStream() failed." );
		goto fail;
	}

	while ( 1 ) {

		openmpt_module_error_clear( mod );
		count = openmpt_module_read_stereo( mod, SAMPLERATE, BUFFERSIZE, left, right );
		mod_err = openmpt_module_error_get_last( mod );
		mod_err_str = openmpt_module_error_get_last_message( mod );
		if ( mod_err != OPENMPT_ERROR_OK ) {
			libopenmpt_example_print_error( "openmpt_module_read_stereo()", mod_err, mod_err_str );
			openmpt_free_string( mod_err_str );
			mod_err_str = NULL;
		}
		if ( count == 0 ) {
			break;
		}

		pa_error = Pa_WriteStream( stream, buffers, (unsigned long)count );
		if ( pa_error == paOutputUnderflowed ) {
			pa_error = paNoError;
		}
		if ( pa_error != paNoError ) {
			fprintf( stderr, "Error: %s\n", "Pa_WriteStream() failed." );
			goto fail;
		}
	}

	result = 0;

	goto cleanup;

fail:

	result = 1;

cleanup:

	if ( stream ) {
		if ( Pa_IsStreamActive( stream ) == 1 ) {
			Pa_StopStream( stream );
		}
		Pa_CloseStream( stream );
		stream = 0;
	}

	if ( pa_initialized ) {
		Pa_Terminate();
		pa_initialized = 0;
	}

	if ( mod ) {
		openmpt_module_destroy( mod );
		mod = 0;
	}

	if ( file ) {
		fclose( file );
		file = 0;
	}

	return result;
}
