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

#if defined( unix ) || defined( __unix__ ) || defined( __unix )
#include <unistd.h>
#if defined( _POSIX_VERSION )
#if ( _POSIX_VERSION > 0 )
#ifndef POSIX
#define POSIX
#endif
#endif
#endif
#endif

#if defined( __MINGW32__ ) && !defined( __MINGW64__ )
#include <sys/types.h>
#endif

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>
#if OPENMPT_API_VERSION_AT_LEAST( 0, 7, 0 )
#if defined( LIBOPENMPT_STREAM_CALLBACKS_FILE_MINGW ) && defined( __MINGW32__ )
#include <libopenmpt/libopenmpt_stream_callbacks_file_mingw.h>
#elif defined( LIBOPENMPT_STREAM_CALLBACKS_FILE_MSVCRT ) && ( defined( _MSC_VER ) || ( defined( __clang__ ) && defined( _WIN32 ) ) )
#include <libopenmpt/libopenmpt_stream_callbacks_file_msvcrt.h>
#elif defined( LIBOPENMPT_STREAM_CALLBACKS_FILE_POSIX ) && defined( POSIX ) && defined( _POSIX_C_SOURCE )
#if ( _POSIX_C_SOURCE > 200112L )
#include <libopenmpt/libopenmpt_stream_callbacks_file_posix.h>
#else
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>
#endif
#else
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>
#endif
#else
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>
#endif

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#elif defined( __GNUC__ ) && !defined( __clang__ ) && !defined( _MSC_VER )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif
#include <portaudio.h>
#if defined( __clang__ )
#pragma clang diagnostic pop
#elif defined( __GNUC__ ) && !defined( __clang__ ) && !defined( _MSC_VER )
#pragma GCC diagnostic pop
#endif

#if defined( __DJGPP__ )
#include <crt0.h>
#endif /* __DJGPP__ */

#define BUFFERSIZE 480
#define SAMPLERATE 48000

static int16_t left[BUFFERSIZE];
static int16_t right[BUFFERSIZE];
static int16_t * const buffers[2] = { left, right };
static int16_t interleaved_buffer[BUFFERSIZE * 2];
static int is_interleaved = 0;

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

#if defined( __DJGPP__ )
/* clang-format off */
int _crt0_startup_flags = 0
	| _CRT0_FLAG_NONMOVE_SBRK          /* force interrupt compatible allocation */
	| _CRT0_DISABLE_SBRK_ADDRESS_WRAP  /* force NT compatible allocation */
	| _CRT0_FLAG_LOCK_MEMORY           /* lock all code and data at program startup */
	| 0;
/* clang-format on */
#endif /* __DJGPP__ */
#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
#if defined( __clang__ ) && !defined( _MSC_VER )
int wmain( int argc, wchar_t * argv[] );
#endif
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif
#if defined( __DJGPP__ )
	/* clang-format off */
	_crt0_startup_flags &= ~_CRT0_FLAG_LOCK_MEMORY;  /* disable automatic locking for all further memory allocations */
	/* clang-format on */
#endif /* __DJGPP__ */

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

#if OPENMPT_API_VERSION_AT_LEAST( 0, 7, 0 )
#if defined( LIBOPENMPT_STREAM_CALLBACKS_FILE_MINGW ) && defined( __MINGW32__ )
	mod = openmpt_module_create2( openmpt_stream_get_file_mingw_callbacks(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
#elif defined( LIBOPENMPT_STREAM_CALLBACKS_FILE_MSVCRT ) && ( defined( _MSC_VER ) || ( defined( __clang__ ) && defined( _WIN32 ) ) )
	mod = openmpt_module_create2( openmpt_stream_get_file_msvcrt_callbacks(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
#elif defined( LIBOPENMPT_STREAM_CALLBACKS_FILE_POSIX ) && defined( POSIX ) && defined( _POSIX_C_SOURCE )
#if ( _POSIX_C_SOURCE > 200112L )
	mod = openmpt_module_create2( openmpt_stream_get_file_posix_callbacks(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
#else
	mod = openmpt_module_create2( openmpt_stream_get_file_callbacks2(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
#endif
#else
	mod = openmpt_module_create2( openmpt_stream_get_file_callbacks2(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
#endif
#else
	mod = openmpt_module_create2( openmpt_stream_get_file_callbacks(), file, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
#endif

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
	if ( pa_error == paSampleFormatNotSupported ) {
		is_interleaved = 1;
		pa_error = Pa_OpenDefaultStream( &stream, 0, 2, paInt16, SAMPLERATE, paFramesPerBufferUnspecified, NULL, NULL );
	}
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
		count = is_interleaved ? openmpt_module_read_interleaved_stereo( mod, SAMPLERATE, BUFFERSIZE, interleaved_buffer ) : openmpt_module_read_stereo( mod, SAMPLERATE, BUFFERSIZE, left, right );
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

		pa_error = is_interleaved ? Pa_WriteStream( stream, interleaved_buffer, (unsigned long)count ) : Pa_WriteStream( stream, buffers, (unsigned long)count );
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
		(void)pa_initialized;
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
