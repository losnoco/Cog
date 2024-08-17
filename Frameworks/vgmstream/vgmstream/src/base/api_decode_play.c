#include "api_internal.h"
#if LIBVGMSTREAM_ENABLE

#define INTERNAL_BUF_SAMPLES  1024
//TODO: use internal 

static void reset_buf(libvgmstream_priv_t* priv) {
    /* state reset */
    priv->buf.samples = 0;
    priv->buf.bytes = 0;
    priv->buf.consumed = 0;

    if (priv->buf.initialized)
        return;
    int output_channels = priv->vgmstream->channels;
    int input_channels = priv->vgmstream->channels;
    vgmstream_mixing_enable(priv->vgmstream, 0, &input_channels, &output_channels); //query

    /* static config */
    priv->buf.channels = input_channels;
    if (priv->buf.channels < output_channels)
        priv->buf.channels = output_channels;

    priv->buf.sample_size = sizeof(sample_t);
    priv->buf.max_samples = INTERNAL_BUF_SAMPLES;
    priv->buf.max_bytes = priv->buf.max_samples * priv->buf.sample_size * priv->buf.channels;
    priv->buf.data = malloc(priv->buf.max_bytes);

    priv->buf.initialized = true;
}

static void update_buf(libvgmstream_priv_t* priv, int samples_done) {
    priv->buf.samples = samples_done;
    priv->buf.bytes = samples_done * priv->buf.sample_size * priv->buf.channels;
    //priv->buf.consumed = 0; //external

    if (!priv->pos.play_forever) {
        priv->decode_done = (priv->pos.current >= priv->pos.play_samples);
        priv->pos.current += samples_done;
    }
}


// update decoder info based on last render, though at the moment it's all fixed
static void update_decoder_info(libvgmstream_priv_t* priv, int samples_done) {

    // output copy
    priv->dec.buf = priv->buf.data;
    priv->dec.buf_bytes = priv->buf.bytes;
    priv->dec.buf_samples = priv->buf.samples;
    priv->dec.done = priv->decode_done;
}

LIBVGMSTREAM_API int libvgmstream_play(libvgmstream_t* lib) {
    if (!lib || !lib->priv)
        return LIBVGMSTREAM_ERROR_GENERIC;

    libvgmstream_priv_t* priv = lib->priv;
    if (priv->decode_done)
        return LIBVGMSTREAM_ERROR_GENERIC;

    reset_buf(priv);
    if (!priv->buf.data)
        return LIBVGMSTREAM_ERROR_GENERIC;

    int to_get = priv->buf.max_samples;
    if (!priv->pos.play_forever && to_get + priv->pos.current > priv->pos.play_samples)
        to_get = priv->pos.play_samples - priv->pos.current;

    int decoded = render_vgmstream(priv->buf.data, to_get, priv->vgmstream);
    update_buf(priv, decoded);
    update_decoder_info(priv, decoded);

    return LIBVGMSTREAM_OK;
}


/* _play decodes a single frame, while this copies partially that frame until frame is over */
LIBVGMSTREAM_API int libvgmstream_fill(libvgmstream_t* lib, void* buf, int buf_samples) {
    if (!lib || !lib->priv || !buf || !buf_samples)
        return LIBVGMSTREAM_ERROR_GENERIC;

    libvgmstream_priv_t* priv = lib->priv;
    if (priv->decode_done)
        return LIBVGMSTREAM_ERROR_GENERIC;

    if (priv->buf.consumed >= priv->buf.samples) {
        int err = libvgmstream_play(lib);
        if (err < 0) return err;
    }

    int copy_samples = priv->buf.samples;
    if (copy_samples > buf_samples)
        copy_samples = buf_samples;
    int copy_bytes = priv->buf.sample_size * priv->buf.channels * copy_samples;
    int skip_bytes = priv->buf.sample_size * priv->buf.channels * priv->buf.consumed;

    memcpy(buf, ((uint8_t*)priv->buf.data) + skip_bytes, copy_bytes);
    priv->buf.consumed += copy_samples;

    return copy_samples;
}


LIBVGMSTREAM_API int64_t libvgmstream_get_play_position(libvgmstream_t* lib) {
    if (!lib || !lib->priv)
        return LIBVGMSTREAM_ERROR_GENERIC;

    libvgmstream_priv_t* priv = lib->priv;
    if (!priv->vgmstream)
        return LIBVGMSTREAM_ERROR_GENERIC;

    return priv->vgmstream->pstate.play_position;
}


LIBVGMSTREAM_API void libvgmstream_seek(libvgmstream_t* lib, int64_t sample) {
    if (!lib || !lib->priv)
        return;

    libvgmstream_priv_t* priv = lib->priv;
    if (!priv->vgmstream)
        return;

    seek_vgmstream(priv->vgmstream, sample);

    priv->pos.current = priv->vgmstream->pstate.play_position;
}


LIBVGMSTREAM_API void libvgmstream_reset(libvgmstream_t* lib) {
    if (!lib || !lib->priv)
        return;

    libvgmstream_priv_t* priv = lib->priv;
    if (priv->vgmstream) {
        reset_vgmstream(priv->vgmstream);
    }
    libvgmstream_priv_reset(priv, false);
}

#endif
