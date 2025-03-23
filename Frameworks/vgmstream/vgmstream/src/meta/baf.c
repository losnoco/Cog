#include "meta.h"
#include "../coding/coding.h"
#include "../util/endianness.h"


/* .BAF - Bizarre Creations bank file [Blur (PS3), Project Gotham Racing 4 (X360), Geometry Wars (PC)] */
VGMSTREAM* init_vgmstream_baf(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    char bank_name[0x22+1], stream_name[0x20+1], file_name[STREAM_NAME_SIZE];
    off_t start_offset, header_offset, name_offset;
    size_t stream_size;
    uint32_t channel_count, sample_rate, num_samples, version, codec, tracks;
    int big_endian, loop_flag, total_subsongs, target_subsong = sf->stream_index;
    read_u32_t read_u32;

    /* checks */
    if (!is_id32be(0x00, sf, "BANK"))
        return NULL;

    if (!check_extensions(sf, "baf"))
        return NULL;

    /* use BANK size to check endianness */
    big_endian = guess_endian32(0x04, sf);
    read_u32 = big_endian ? read_u32be : read_u32le;

    /* 0x04: bank size */
    version = read_u32(0x08,sf);
    if (version != 0x03 && version != 0x04 && version != 0x05)
        goto fail;
    total_subsongs = read_u32(0x0c,sf);
    if (target_subsong == 0) target_subsong = 1;
    if (target_subsong < 0 || target_subsong > total_subsongs || total_subsongs < 1) goto fail;
    /* - in v3 */
    /* 0x10: 0? */
    /* 0x11: bank name */
    /* - in v4/5 */
    /* 0x10: 1? */
    /* 0x11: padding flag? */
    /* 0x12: bank name */

    /* find target WAVE chunk */
    {
        int i;
        off_t offset = read_u32(0x04, sf);

        for (i = 0; i < total_subsongs; i++) {
            if (i+1 == target_subsong)
                break;
            offset += read_u32(offset+0x04, sf); /* WAVE size, variable per codec */

            /* skip companion "CUE " (found in 007: Blood Stone, contains segment cues) */
            if (is_id32be(offset+0x00, sf, "CUE ")) {
                offset += read_u32(offset+0x04, sf); /* CUE size */
            }
        }
        header_offset = offset;
    }

    /* parse header */
    if (!is_id32be(header_offset+0x00, sf, "WAVE"))
        goto fail;
    codec        = read_u32(header_offset+0x08, sf);
    name_offset  = header_offset + 0x0c;
    start_offset = read_u32(header_offset+0x2c, sf);
    stream_size  = read_u32(header_offset+0x30, sf);
    tracks = 0;

    switch(codec) {
        case 0x03: /* PCM16 */
            switch(version) {
                case 0x03: /* Geometry Wars (PC) */
                    sample_rate     = read_u32(header_offset+0x38, sf);
                    channel_count   = read_u32(header_offset+0x40, sf);
                    /* no actual flag, just loop +15sec songs */
                    loop_flag = (pcm_bytes_to_samples(stream_size, channel_count, 16) > 15*sample_rate);
                    break;

                case 0x04: /* Project Gotham Racing 4 (X360) */
                    sample_rate     = read_u32(header_offset+0x3c, sf);
                    channel_count   = read_u32(header_offset+0x44, sf);
                    loop_flag       =  read_u8(header_offset+0x4b, sf);
                    break;

                case 0x05: /* Blur 2 (X360) */
                    sample_rate     = read_u32(header_offset+0x40, sf);
                    channel_count   = read_u32(header_offset+0x48, sf);
                    loop_flag       =  read_u8(header_offset+0x50, sf) != 0;
                    break;

                default:
                    goto fail;
            }

            num_samples = pcm_bytes_to_samples(stream_size, channel_count, 16);
            break;

        case 0x07: /* PSX ADPCM (0x21 frame size) */
            if (version == 0x04 && read_u32(header_offset + 0x3c, sf) != 0) {
                /* The Club (PS3), Blur (Prototype) (PS3) */
                sample_rate     = read_u32(header_offset+0x3c, sf);
                channel_count   = read_u32(header_offset+0x44, sf);
                loop_flag       =  read_u8(header_offset+0x4b, sf);

                /* mini-header at the start of the stream */
                num_samples     = read_u32le(start_offset+0x04, sf) / channel_count;
                start_offset   += read_u32le(start_offset+0x00, sf); /* 0x08 */
                break;
            }

            switch (version) {
                case 0x04: /* Blur (PS3) */
                case 0x05: /* James Bond 007: Blood Stone (X360) */
                    sample_rate     = read_u32(header_offset+0x40, sf);
                    num_samples     = read_u32(header_offset+0x44, sf);
                    loop_flag       =  read_u8(header_offset+0x48, sf);
                    tracks          =  read_u8(header_offset+0x49, sf);
                    channel_count   =  read_u8(header_offset+0x4b, sf);

                    if (tracks) {
                        channel_count = channel_count * tracks;
                    }
                    break;

                default:
                    goto fail;
            }
            break;

        case 0x08: /* XMA1 */
        case 0x09: /* XMA2 */
            switch(version) {
                case 0x04: /* Project Gotham Racing 4 (X360) */
                    sample_rate     = read_u32(header_offset+0x3c, sf);
                    channel_count   = read_u32(header_offset+0x44, sf);
                    loop_flag       =  read_u8(header_offset+0x54, sf) != 0;
                    break;

                case 0x05: /* James Bond 007: Blood Stone (X360) */
                    sample_rate     = read_u32(header_offset+0x40, sf);
                    channel_count   = read_u32(header_offset+0x48, sf);
                    loop_flag       =  read_u8(header_offset+0x58, sf) != 0;
                    break;

                default:
                    goto fail;
            }
            break;

        default:
            goto fail;
    }
    /* others: pan/vol? fixed values? (0x19, 0x10) */

    /* after WAVEs there may be padding then DATAs chunks, but offsets point after DATA size */


    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->meta_type = meta_BAF;
    vgmstream->sample_rate = sample_rate;
    vgmstream->num_streams = total_subsongs;
    vgmstream->stream_size = stream_size;

    switch(codec) {
        case 0x03:
            vgmstream->coding_type = big_endian ? coding_PCM16BE : coding_PCM16LE;
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x02;

            vgmstream->num_samples = num_samples;
            vgmstream->loop_start_sample = 0;
            vgmstream->loop_end_sample = num_samples;
            break;

        case 0x07:
            vgmstream->coding_type = coding_PSX_cfg;
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x21;

            vgmstream->num_samples = num_samples;
            vgmstream->loop_start_sample = 0;
            vgmstream->loop_end_sample = num_samples;
            break;

#ifdef VGM_USE_FFMPEG
        case 0x08:
        case 0x09: {
            int is_xma1 = (codec == 0x08);
            int block_size = 0x10000;

            /* need to manually find sample offsets, it was a thing with XMA1 */
            {
                ms_sample_data msd = {0};

                msd.xma_version  = is_xma1 ? 1 : 2;
                msd.channels     = channel_count;
                msd.data_offset  = start_offset;
                msd.data_size    = stream_size;
                msd.loop_flag    = loop_flag;

                switch(version) {
                    case 0x04:
                        msd.loop_start_b = read_u32(header_offset+0x4c, sf);
                        msd.loop_end_b   = read_u32(header_offset+0x50, sf);
                        msd.loop_start_subframe  = (read_u8(header_offset+0x55, sf) >> 0) & 0x0f;
                        msd.loop_end_subframe    = (read_u8(header_offset+0x55, sf) >> 4) & 0x0f;
                        break;

                    case 0x05:
                        msd.loop_start_b = read_u32(header_offset+0x50, sf);
                        msd.loop_end_b   = read_u32(header_offset+0x54, sf);
                        msd.loop_start_subframe  = (read_u8(header_offset+0x59, sf) >> 0) & 0x0f;
                        msd.loop_end_subframe    = (read_u8(header_offset+0x59, sf) >> 4) & 0x0f;
                        break;

                    default:
                        goto fail;
                }
                xma_get_samples(&msd, sf);

                vgmstream->coding_type = coding_FFmpeg;
                vgmstream->layout_type = layout_none;

                vgmstream->num_samples = msd.num_samples; /* also at 0x58(v4)/0x5C(v5) for XMA1, but unreliable? */
                vgmstream->loop_start_sample = msd.loop_start_sample;
                vgmstream->loop_end_sample = msd.loop_end_sample;

                vgmstream->codec_data = is_xma1
                    ? init_ffmpeg_xma1_raw(sf, start_offset, stream_size, vgmstream->channels, vgmstream->sample_rate, 0)
                    : init_ffmpeg_xma2_raw(sf, start_offset, stream_size, vgmstream->num_samples, vgmstream->channels, vgmstream->sample_rate, block_size, 0);
                if (!vgmstream->codec_data) goto fail;
            }

            xma_fix_raw_samples_ch(vgmstream, sf, start_offset, stream_size, channel_count, 1,1);
            break;
        }
#endif

        default:
            VGM_LOG("BAF: unknown codec %x\n", codec);
            goto fail;
    }

    read_string(bank_name, sizeof(bank_name), (version == 0x03) ? 0x11 : 0x12, sf);
    read_string(stream_name, sizeof(stream_name), name_offset, sf);
    get_streamfile_basename(sf, file_name, STREAM_NAME_SIZE);

    if (bank_name[0] && strcmp(file_name, bank_name) != 0)
        snprintf(vgmstream->stream_name, STREAM_NAME_SIZE, "%s/%s", bank_name, stream_name);
    else
        snprintf(vgmstream->stream_name, STREAM_NAME_SIZE, "%s", stream_name);


    if (!vgmstream_open_stream(vgmstream, sf, start_offset))
        goto fail;
    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}
