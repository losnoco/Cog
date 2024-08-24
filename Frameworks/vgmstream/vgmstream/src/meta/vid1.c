#include "meta.h"
#include "../coding/coding.h"
#include "../layout/layout.h"
#include "../util/endianness.h"
#include "../util/layout_utils.h"


/* VID1 - Factor 5/DivX format GC/Xbox games [Gun (GC), Tony Hawk's American Wasteland (GC), Enter The Matrix (Xbox)]*/
VGMSTREAM* init_vgmstream_vid1(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    uint32_t start_offset, header_offset;
    int loop_flag = 0, channels, sample_rate;
    uint32_t codec;
    bool big_endian;
    read_u32_t read_u32;


    /* checks */
    if (is_id32be(0x00, sf, "VID1")) { /* BE (GC) */
        big_endian = true;
    }
    else if (is_id32le(0x00,sf, "VID1")) { /* LE (Xbox) */
        big_endian = false;
    }
    else {
        return NULL;
    }

    /* .vid: video + (often) audio
     * .ogg: audio only [Gun (GC)], .logg: for plugins */
    if (!check_extensions(sf,"vid,ogg,logg"))
        return NULL;

    read_u32 = big_endian ? read_u32be : read_u32le;

    /* chunked/blocked format containing video or audio frames */

    /* find actual header start/size in the chunks (id + size + null) */
    {
        off_t offset = read_u32(0x04, sf);

        if (read_u32(offset + 0x00, sf) != 0x48454144) /* "HEAD" */
            goto fail;
        start_offset = offset + read_u32(offset + 0x04, sf);
        offset += 0x0c;

        /* videos have VIDH before AUDH */
        if (read_u32(offset + 0x00, sf) == 0x56494448) { /* "VIDH" */
            offset += read_u32(offset + 0x04, sf);
        }

        if (read_u32(offset, sf) != 0x41554448) /* "AUDH" */
            goto fail;
        offset += 0x0c;

        header_offset = offset;
        codec       = read_u32(offset + 0x00, sf);
        sample_rate = read_u32(offset + 0x04, sf);
        channels    = read_u8 (offset + 0x08, sf);
        /* 0x09: flag? 0/1 */
        /* 0x0a+: varies per codec (PCM/X-IMA: nothing, DSP: coefs, Vorbis: bitrate/frame info?) + padding */
    }


    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channels, loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->meta_type = meta_VID1;
    vgmstream->sample_rate = sample_rate;
    vgmstream->codec_endian = big_endian;

    switch(codec) {
        case 0x50433136: /* "PC16" [Enter the Matrix (Xbox)] */
            vgmstream->coding_type = coding_PCM16_int;
            vgmstream->layout_type = layout_blocked_vid1;
            break;

        case 0x5841504D: /* "XAPM" [Enter the Matrix (Xbox)] */
            vgmstream->coding_type = coding_XBOX_IMA;
            vgmstream->layout_type = layout_blocked_vid1;
            break;

        case 0x4150434D: /* "APCM" [Enter the Matrix (GC)] */
            vgmstream->coding_type = coding_NGC_DSP;
            vgmstream->layout_type = layout_blocked_vid1;

            dsp_read_coefs_be(vgmstream, sf, header_offset + 0x0a, 0x20);
            break;

#ifdef VGM_USE_VORBIS
        case 0x56415544: { /* "VAUD" [Gun (GC)] */
            vorbis_custom_config cfg = {0};

            vgmstream->num_samples = read_u32(header_offset + 0x20, sf);

            vgmstream->codec_data = init_vorbis_custom(sf, header_offset + 0x24, VORBIS_VID1, &cfg);
            if (!vgmstream->codec_data) goto fail;
            vgmstream->coding_type = coding_VORBIS_custom;
            vgmstream->layout_type = layout_none;
            break;
        }
#endif

        default:
            goto fail;
    }

    if (!vgmstream_open_stream(vgmstream, sf, start_offset))
        goto fail;

    /* calc num_samples as playable data size varies between files/blocks */
    if (vgmstream->layout_type == layout_blocked_vid1) {
        blocked_counter_t cfg = {0};
        cfg.offset = start_offset;

        blocked_count_samples(vgmstream, sf, &cfg);
    }

    return vgmstream;
fail:
    close_vgmstream(vgmstream);
    return NULL;
}
