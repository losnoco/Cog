/*
 * fuzz.c
 * ------
 * Purpose: Tiny libopenmpt user to be used by fuzzing tools
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <unistd.h>

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_stream_callbacks_file.h>

#define BUFFERSIZE 450  // shouldn't match OpenMPT's internal mix buffer size (512)
#define SAMPLERATE 22050

static int16_t buffer[BUFFERSIZE];

static int ErrFunc (int error, void *)
{
	switch (error)
	{
		case OPENMPT_ERROR_INVALID_ARGUMENT:
		case OPENMPT_ERROR_OUT_OF_RANGE:
		case OPENMPT_ERROR_LENGTH:
		case OPENMPT_ERROR_DOMAIN:
		case OPENMPT_ERROR_LOGIC:
		case OPENMPT_ERROR_UNDERFLOW:
		case OPENMPT_ERROR_OVERFLOW:
		case OPENMPT_ERROR_RANGE:
		case OPENMPT_ERROR_RUNTIME:
		case OPENMPT_ERROR_EXCEPTION:
			abort();
		default:
			return OPENMPT_ERROR_FUNC_RESULT_NONE;
	}
}

int main( int argc, char * argv[] ) {
	static FILE * file = NULL;
	static openmpt_module * mod = NULL;
	static size_t count = 0;
	static int i = 0;
	(void)argc;
#ifdef __AFL_HAVE_MANUAL_CONTROL
	__AFL_INIT();
#endif
	file = fopen( argv[1], "rb" );
	mod = openmpt_module_create2( openmpt_stream_get_file_callbacks(), file, NULL, NULL, ErrFunc, NULL, NULL, NULL, NULL );
	fclose( file );
	if ( mod == NULL )
		return 1;

	// verify API contract : If the file can be loaded, header probing must be successful too.
	if ( openmpt_probe_file_header_from_stream( OPENMPT_PROBE_FILE_HEADER_FLAGS_DEFAULT, openmpt_stream_get_file_callbacks(), file, NULL, NULL, ErrFunc, NULL, NULL, NULL ) == OPENMPT_PROBE_FILE_HEADER_RESULT_FAILURE )
		abort();

	openmpt_module_ctl_set( mod, "render.resampler.emulate_amiga", (openmpt_module_get_num_orders( mod ) & 1) ? "0" : "1" );
	// render about a second of the module for fuzzing the actual mix routines
	for(; i < 50; i++) {
		count = openmpt_module_read_mono( mod, SAMPLERATE, BUFFERSIZE, buffer );
		if ( count == 0 ) {
			break;
		}
	}
	openmpt_module_set_position_seconds( mod, 1.0 );
	openmpt_module_read_mono( mod, SAMPLERATE, BUFFERSIZE, buffer );
	openmpt_module_set_position_order_row( mod, 3, 16 );
	openmpt_module_read_mono( mod, SAMPLERATE, BUFFERSIZE, buffer );

	// fuzz string-related stuff
	openmpt_free_string ( openmpt_module_get_metadata( mod, "date" ) );
	openmpt_free_string ( openmpt_module_get_metadata( mod, "message" ) );
	openmpt_module_destroy( mod );
	return 0;
}
