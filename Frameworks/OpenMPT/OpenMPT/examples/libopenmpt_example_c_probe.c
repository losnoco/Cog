/*
 * libopenmpt_example_c_probe.c
 * ----------------------------
 * Purpose: libopenmpt C API probing example
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

/*
 * Usage: libopenmpt_example_c_probe SOMEMODULE ...
 * Returns 0 on successful probing for all files.
 * Returns 1 on failed probing for 1 or more files.
 * Returns 2 on error.
 */

#define LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY 1
#define LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT  2

#define LIBOPENMPT_EXAMPLE_PROBE_RESULT LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
	(void)userdata;

	if ( message ) {
		fprintf( stderr, "%s\n", message );
	}
}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
static int probe_file( const wchar_t * filename ) {
#else
static int probe_file( const char * filename ) {
#endif

	int result = 0;
	int mod_err = OPENMPT_ERROR_OK;
	FILE * file = NULL;

#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )
	int result_binary = 0;
	int probe_file_header_result = OPENMPT_PROBE_FILE_HEADER_RESULT_FAILURE;
	const char * probe_file_header_result_str = NULL;
#endif
#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT )
	double probability = 0.0;
#endif

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	if ( wcslen( filename ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_probe SOMEMODULE'." );
		goto fail;
	}
#else
	if ( strlen( filename ) == 0 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_probe SOMEMODULE'." );
		goto fail;
	}
#endif

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
	file = _wfopen( filename, L"rb" );
#else
	file = fopen( filename, "rb" );
#endif
	if ( !file ) {
		fprintf( stderr, "Error: %s\n", "fopen() failed." );
		goto fail;
	}

	#if ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_BINARY )
		probe_file_header_result = openmpt_probe_file_header_from_stream( OPENMPT_PROBE_FILE_HEADER_FLAGS_DEFAULT, openmpt_stream_get_file_callbacks(), file, &libopenmpt_example_logfunc, NULL, &openmpt_error_func_default, NULL, &mod_err, NULL );
		probe_file_header_result_str = NULL;
		result_binary = 0;
		switch ( probe_file_header_result ) {
			case OPENMPT_PROBE_FILE_HEADER_RESULT_SUCCESS:
				probe_file_header_result_str = "Success     ";
				result_binary = 1;
				break;
			case OPENMPT_PROBE_FILE_HEADER_RESULT_FAILURE:
				probe_file_header_result_str = "Failure     ";
				result_binary = 0;
				break;
			case OPENMPT_PROBE_FILE_HEADER_RESULT_WANTMOREDATA:
				probe_file_header_result_str = "WantMoreData";
				result_binary = 0;
				break;
			case OPENMPT_PROBE_FILE_HEADER_RESULT_ERROR:
				result_binary = 0;
				fprintf( stderr, "Error: %s\n", "openmpt_probe_file_header() failed." );
				goto fail;
				break;
			default:
				result_binary = 0;
				fprintf( stderr, "Error: %s\n", "openmpt_probe_file_header() failed." );
				goto fail;
				break;
		}
#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
		fprintf( stdout, "%s - %ls\n", probe_file_header_result_str, filename );
#else
		fprintf( stdout, "%s - %s\n", probe_file_header_result_str, filename );
#endif
		if ( result_binary ) {
			result = 0;
		} else {
			result = 1;
		}
	#elif ( LIBOPENMPT_EXAMPLE_PROBE_RESULT == LIBOPENMPT_EXAMPLE_PROBE_RESULT_FLOAT )
		probability = openmpt_could_open_probability2( openmpt_stream_get_file_callbacks(), file, 0.25, &libopenmpt_example_logfunc, NULL, &openmpt_error_func_default, NULL, &mod_err, NULL );
#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
		fprintf( stdout, "%s: %f - %ls\n", "Result", probability, filename );
#else
		fprintf( stdout, "%s: %f - %s\n", "Result", probability, filename );
#endif
		if ( probability >= 0.5 ) {
			result = 0;
		} else {
			result = 1;
		}
	#else
		#error "LIBOPENMPT_EXAMPLE_PROBE_RESULT is wrong"
	#endif

	goto cleanup;

fail:

	result = 2;

cleanup:

	if ( file ) {
		fclose( file );
		file = 0;
	}

	return result;
}

#if ( defined( _WIN32 ) || defined( WIN32 ) ) && ( defined( _UNICODE ) || defined( UNICODE ) )
int wmain( int argc, wchar_t * argv[] ) {
#else
int main( int argc, char * argv[] ) {
#endif

	int global_result = 0;

	if ( argc <= 1 ) {
		fprintf( stderr, "Error: %s\n", "Wrong invocation. Use 'libopenmpt_example_c_probe SOMEMODULE ...'." );
		goto fail;
	}

	for ( int i = 1; i < argc; ++i ) {
		int result = probe_file( argv[i] );
		if ( result > global_result ) {
			global_result = result;
		}
	}

	goto cleanup;

fail:

	global_result = 2;

cleanup:

	return global_result;

}

