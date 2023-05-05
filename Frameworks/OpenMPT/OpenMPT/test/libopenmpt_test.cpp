/*
 * libopenmpt_test.cpp
 * -------------------
 * Purpose: libopenmpt test suite driver
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/macros.hpp"

#if defined( LIBOPENMPT_BUILD_TEST )

#if defined(__MINGW32__) && !defined(__MINGW64__)
#include <sys/types.h>
#endif

#include "../libopenmpt/libopenmpt_internal.h"

#include "test.h"

#include <iostream>
#include <locale>

#include <clocale>
#include <cstdlib>

#if defined( __DJGPP__ )
#include <crt0.h>
#endif /* __DJGPP__ */

using namespace OpenMPT;

#if defined( __DJGPP__ )
/* Work-around <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45977> */
/* clang-format off */
extern "C" {
	int _crt0_startup_flags = 0
		| _CRT0_FLAG_NONMOVE_SBRK          /* force interrupt compatible allocation */
		| _CRT0_DISABLE_SBRK_ADDRESS_WRAP  /* force NT compatible allocation */
		| _CRT0_FLAG_LOCK_MEMORY           /* lock all code and data at program startup */
		| 0;
}
/* clang-format on */
#endif /* __DJGPP__ */
#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
#if defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))
// mingw-w64 g++ does only default to special C linkage for "main", but not for "wmain" (see <https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/>).
extern "C" int wmain( int /*argc*/ , wchar_t * /*argv*/ [] );
extern "C"
#endif
int wmain( int /*argc*/ , wchar_t * /*argv*/ [] ) {
#else
int main( int /*argc*/ , char * /*argv*/ [] ) {
#endif
#if defined( __DJGPP__ )
	_crt0_startup_flags &= ~_CRT0_FLAG_LOCK_MEMORY;  /* disable automatic locking for all further memory allocations */
#endif /* __DJGPP__ */

	try {
	
		// run test with "C" / classic() locale
		Test::DoTests();

		// try setting the C locale to the user locale
		setlocale( LC_ALL, "" );
		
		// run all tests again with a set C locale
		Test::DoTests();
		
		// try to set the C and C++ locales to the user locale
		try {
			std::locale old = std::locale::global( std::locale( "" ) );
			(void)old;
		} catch ( ... ) {
			// Setting c++ global locale does not work.
			// This is no problem for libopenmpt, just continue.
		}
		
		// and now, run all tests once again
		Test::DoTests();

	} catch ( const std::exception & e ) {
		std::cerr << "TEST ERROR: exception: " << ( e.what() ? e.what() : "" ) << std::endl;
		return -1;
	} catch ( ... ) {
		std::cerr << "TEST ERROR: unknown exception" << std::endl;
		return -1;
	}
	return 0;
}

#else // !LIBOPENMPT_BUILD_TEST

unsigned char libopenmpt_test_dummy = 0;

#endif // LIBOPENMPT_BUILD_TEST
