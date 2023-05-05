/*
 * libopenmpt_example_c_pipe.c
 * ---------------------------
 * Purpose: libopenmpt C API simple pipe example
 * Notes  : This example writes raw 48000Hz / stereo / 16bit native endian PCM data to stdout.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: cat SOMEMODULE | libopenmpt_example_c_pipe | aplay --file-type raw --format=dat
 */

#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_fd.h>

#if defined( __DJGPP__ )
#include <crt0.h>
#endif /* __DJGPP__ */

#define BUFFERSIZE 480
#define SAMPLERATE 48000

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

static ssize_t xwrite( int fd, const void * buffer, size_t size ) {
	size_t written = 0;
	ssize_t retval = 0;
	while ( written < size ) {
		retval = write( fd, (const char *)buffer + written, size - written );
		if ( retval < 0 ) {
			if ( errno != EINTR ) {
				break;
			}
			retval = 0;
		}
		written += retval;
	}
	return written;
}

static int16_t buffer[BUFFERSIZE * 2];

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
	openmpt_module * mod = 0;
	int mod_err = OPENMPT_ERROR_OK;
	const char * mod_err_str = NULL;
	size_t count = 0;
	size_t written = 0;
	(void)argv;

	if ( argc != 1 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_pipe' and connect stdin and stdout." );
		goto fail;
	}

	mod = openmpt_module_create2( openmpt_stream_get_fd_callbacks(), (void *)(uintptr_t)STDIN_FILENO, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
	if ( !mod ) {
		libopenmpt_example_print_error( "openmpt_module_create2()", mod_err, mod_err_str );
		openmpt_free_string( mod_err_str );
		mod_err_str = NULL;
		goto fail;
	}

	while ( 1 ) {

		openmpt_module_error_clear( mod );
		count = openmpt_module_read_interleaved_stereo( mod, SAMPLERATE, BUFFERSIZE, buffer );
		mod_err = openmpt_module_error_get_last( mod );
		mod_err_str = openmpt_module_error_get_last_message( mod );
		if ( mod_err != OPENMPT_ERROR_OK ) {
			libopenmpt_example_print_error( "openmpt_module_read_interleaved_stereo()", mod_err, mod_err_str );
			openmpt_free_string( mod_err_str );
			mod_err_str = NULL;
		}
		if ( count == 0 ) {
			break;
		}

		written = xwrite( STDOUT_FILENO, buffer, count * 2 * sizeof( int16_t ) );
		if ( written == 0 ) {
			fprintf( stderr, "Error: %s\n", "write() failed." );
			goto fail;
		}
	}

	result = 0;

	goto cleanup;

fail:

	result = 1;

cleanup:

	if ( mod ) {
		openmpt_module_destroy( mod );
		mod = 0;
	}

	return result;
}
