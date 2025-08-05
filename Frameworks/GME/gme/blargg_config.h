// Library configuration. Modify this file as necessary.

#ifndef BLARGG_CONFIG_H
#define BLARGG_CONFIG_H

// Uncomment to use zlib for transparent decompression of gzipped files
//#define HAVE_ZLIB_H

// Uncomment and edit list to support only the listed game music types,
// so that the others don't get linked in at all.
/*
#define GME_TYPE_LIST \
	gme_ay_type,\
	gme_gbs_type,\
	gme_gym_type,\
	gme_hes_type,\
	gme_kss_type,\
	gme_nsf_type,\
	gme_nsfe_type,\
	gme_sap_type,\
	gme_spc_type,\
	gme_vgm_type,\
	gme_vgz_type
*/

// Uncomment to enable platform-specific optimizations
//#define BLARGG_NONPORTABLE 1

// Uncomment to use faster, lower quality sound synthesis
//#define BLIP_BUFFER_FAST 1

// Uncomment one of the following two if automatic byte-order determination doesn't work
//#define BLARGG_BIG_ENDIAN 1
//#define BLARGG_LITTLE_ENDIAN 1

#define debug_printf(a, ...)

// Use standard config.h if present
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#endif
