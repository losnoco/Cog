/*
 * fuzz.cpp
 * --------
 * Purpose: Tiny libopenmpt user to be used by fuzzing tools
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include <memory>
#include <cstdint>
#include <cstdlib>

#include <cerrno>
#include <unistd.h>

#include <libopenmpt/libopenmpt.h>

#include "../../common/mptRandom.h"

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
			std::abort();
		default:
			return OPENMPT_ERROR_FUNC_RESULT_NONE;
	}
}

__AFL_FUZZ_INIT();

int main( int argc, char * argv[] ) {
	(void)argc;
	(void)argv;
	openmpt_module_create_from_memory2( buffer, BUFFERSIZE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr );
#ifdef __AFL_HAVE_MANUAL_CONTROL
	__AFL_INIT();
#endif

	unsigned char *fileBuffer = __AFL_FUZZ_TESTCASE_BUF;  // must be after __AFL_INIT and before __AFL_LOOP!

	while (__AFL_LOOP(10000)) {
		int fileSize = __AFL_FUZZ_TESTCASE_LEN;
		OpenMPT::mpt::reinit_global_random();
		openmpt_module * mod = openmpt_module_create_from_memory2( fileBuffer, fileSize, nullptr, nullptr, ErrFunc, nullptr, nullptr, nullptr, nullptr);
		if ( mod == NULL )
			break;

		// verify API contract: If the file can be loaded, header probing must be successful too.
		if ( openmpt_probe_file_header( OPENMPT_PROBE_FILE_HEADER_FLAGS_DEFAULT, fileBuffer, fileSize, fileSize, nullptr, nullptr, ErrFunc, nullptr, nullptr, nullptr ) == OPENMPT_PROBE_FILE_HEADER_RESULT_FAILURE )
			std::abort();

		openmpt_module_ctl_set( mod, "render.resampler.emulate_amiga", (openmpt_module_get_num_orders( mod ) & 1) ? "0" : "1" );
		// render about a second of the module for fuzzing the actual mix routines
		for(int i = 0; i < 50; i++) {
			size_t count = openmpt_module_read_mono( mod, SAMPLERATE, BUFFERSIZE, buffer );
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
	}
	return 0;
}
