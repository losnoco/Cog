/*
 * libopenmpt_test.cpp
 * -------------------
 * Purpose: libopenmpt test suite driver
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "BuildSettings.h"

#include "libopenmpt_internal.h"

#include "test/test.h"

#include <iostream>
#include <locale>

#include <clocale>
#include <cstdlib>

using namespace OpenMPT;

#if defined( LIBOPENMPT_BUILD_TEST )

#if (defined(_WIN32) || defined(WIN32)) && (defined(_UNICODE) || defined(UNICODE))
#if defined(__GNUC__)
// mingw-w64 g++ does only default to special C linkage for "main", but not for "wmain" (see <https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/>).
extern "C"
#endif
int wmain( int /*argc*/ , wchar_t * /*argv*/ [] ) {
#else
int main( int /*argc*/ , char * /*argv*/ [] ) {
#endif
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

#endif // LIBOPENMPT_BUILD_TEST
