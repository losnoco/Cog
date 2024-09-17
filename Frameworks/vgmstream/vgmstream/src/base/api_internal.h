#ifndef _API_INTERNAL_H_
#define _API_INTERNAL_H_
#include "../libvgmstream.h"
#include "../util/log.h"
#include "../vgmstream.h"
#include "plugins.h"

#if LIBVGMSTREAM_ENABLE

#define LIBVGMSTREAM_OK  0
#define LIBVGMSTREAM_ERROR_GENERIC  -1
#define LIBVGMSTREAM_ERROR_DONE  -2

#define INTERNAL_BUF_SAMPLES  1024

/* self-note: various API functions are just bridges to internal stuff.
 * Rather than changing the internal stuff to handle API structs/etc,
 * leave internals untouched for a while so external plugins/users may adapt.
 * (all the bridging around may be a tiiiiny bit slower but in this day and age potatos are pretty powerful) */

typedef struct {
    bool initialized;
    void* data;

    /* config (output values channels/size after mixing, though buf may be as big as input size) */
    int max_samples;
    int channels;       /* */
    int sample_size;    

    /* state */
    int samples;
    int bytes;
    int consumed;

} libvgmstream_priv_buf_t;

typedef struct {
    int64_t play_forever;
    int64_t play_samples;
    int64_t current;
} libvgmstream_priv_position_t;

/* vgmstream context/handle */
typedef struct {
    libvgmstream_format_t fmt;  // externally exposed
    libvgmstream_decoder_t dec; // externally exposed

    libvgmstream_config_t cfg;  // internal copy

    VGMSTREAM* vgmstream;

    libvgmstream_priv_buf_t buf;
    libvgmstream_priv_position_t pos;

    bool decode_done;
} libvgmstream_priv_t;


void libvgmstream_priv_reset(libvgmstream_priv_t* priv, bool reset_buf);
libvgmstream_sample_t api_get_output_sample_type(libvgmstream_priv_t* priv);
int api_get_sample_size(libvgmstream_sample_t sample_type);

STREAMFILE* open_api_streamfile(libstreamfile_t* libsf);

#endif
#endif
