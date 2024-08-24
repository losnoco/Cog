#include "api_internal.h"
#if LIBVGMSTREAM_ENABLE


static int get_internal_log_level(libvgmstream_loglevel_t level) {
    switch(level) {
        case LIBVGMSTREAM_LOG_LEVEL_ALL: return VGM_LOG_LEVEL_ALL;
        case LIBVGMSTREAM_LOG_LEVEL_DEBUG: return VGM_LOG_LEVEL_DEBUG;
        case LIBVGMSTREAM_LOG_LEVEL_INFO: return VGM_LOG_LEVEL_INFO;
        case LIBVGMSTREAM_LOG_LEVEL_NONE:
        default:
            return 0;
    }
}

LIBVGMSTREAM_API void libvgmstream_set_log(libvgmstream_log_t* cfg) {
    if (!cfg)
        return;

    int ilevel = get_internal_log_level(cfg->level);
    if (cfg->stdout_callback) {
        //vgmstream_set_log_stdout(ilevel);
        vgm_log_set_callback(NULL, ilevel, 1, NULL);
    }
    else {
        //vgmstream_set_log_callback(ilevel, cfg->callback);
        vgm_log_set_callback(NULL, ilevel, 0, cfg->callback);
    }
}


LIBVGMSTREAM_API const char** libvgmstream_get_extensions(size_t* size) {
    return vgmstream_get_formats(size);
}

LIBVGMSTREAM_API const char** libvgmstream_get_common_extensions(size_t* size) {
    return vgmstream_get_common_formats(size);
}


LIBVGMSTREAM_API int libvgmstream_format_describe(libvgmstream_t* lib, char* dst, int dst_size) {
    if (!lib || !lib->priv)
        return LIBVGMSTREAM_ERROR_GENERIC;

    libvgmstream_priv_t* priv = lib->priv;
    if (!priv->vgmstream)
        return LIBVGMSTREAM_ERROR_GENERIC;

    describe_vgmstream(priv->vgmstream, dst, dst_size);
    return LIBVGMSTREAM_OK; //TODO return truncated chars
}


LIBVGMSTREAM_API bool libvgmstream_is_valid(const char* filename, libvgmstream_valid_t* cfg) {
    if (!filename)
        return false;

    if (!cfg)
        return vgmstream_ctx_is_valid(filename, NULL);

    vgmstream_ctx_valid_cfg icfg = {
        .is_extension = cfg->is_extension,
        .skip_standard = cfg->skip_default,
        .reject_extensionless = cfg->reject_extensionless,
        .accept_unknown = cfg->accept_unknown,
        .accept_common = cfg->accept_common
    };
    return vgmstream_ctx_is_valid(filename, &icfg);
}


LIBVGMSTREAM_API int libvgmstream_get_title(libvgmstream_t* lib, libvgmstream_title_t* cfg, char* buf, int buf_len) {
    if (!buf || !buf_len || !cfg)
        return LIBVGMSTREAM_ERROR_GENERIC;

    buf[0] = '\0';
    if (!lib || !lib->priv)
        return LIBVGMSTREAM_ERROR_GENERIC;

    libvgmstream_priv_t* priv = lib->priv;
    vgmstream_title_t icfg = {
        .force_title = cfg->force_title,
        .subsong_range = cfg->subsong_range,
        .remove_extension = cfg->remove_extension,
        .remove_archive = cfg->remove_archive,
    };
    vgmstream_get_title(buf, buf_len, cfg->filename, priv->vgmstream, &icfg);
    return LIBVGMSTREAM_OK;
}

LIBVGMSTREAM_API bool libvgmstream_is_virtual_filename(const char* filename) {
    return vgmstream_is_virtual_filename(filename);
}

#endif
