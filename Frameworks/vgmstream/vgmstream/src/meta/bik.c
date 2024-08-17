#include "meta.h"
#include "../coding/coding.h"
#include "../util.h"

static bool bink_get_info(STREAMFILE* sf, int target_subsong, int* p_total_subsongs, size_t* p_stream_size, int* p_channels, int* p_sample_rate, int* p_num_samples);

/* BINK 1/2 - RAD Game Tools movies (audio/video format) */
VGMSTREAM* init_vgmstream_bik(STREAMFILE* sf) {
    VGMSTREAM * vgmstream = NULL;
    int channels = 0, loop_flag = 0, sample_rate = 0, num_samples = 0;
    int total_subsongs = 0, target_subsong = sf->stream_index;
    size_t stream_size;

    /* checks */
    /* bink1/2 header, followed by version-char (audio is the same) */
    if ((read_u32be(0x00,sf) & 0xffffff00) != get_id32be("BIK\0") &&
        (read_u32be(0x00,sf) & 0xffffff00) != get_id32be("KB2\0"))
        goto fail;

    /* .bik/bk2: standard
    *  .bik2: older?
     * .xmv: Reflections games [Driver: Parallel Lines (Wii), Emergency Heroes (Wii)]
     * .bik.ps3: Neversoft games [Guitar Hero: Warriors of Rock (PS3)]
     * .bik.xen: Neversoft games [various Guitar Hero (PC/PS3/X360)]
     * .vid: Etrange Libellules games [Alice in Wonderland (PC)] 
     * .bika: fake extension for demuxed audio */
    if (!check_extensions(sf,"bik,bk2,bik2,ps3,xmv,xen,vid,bika"))
        goto fail;

    /* find target stream info and samples */
    if (!bink_get_info(sf, target_subsong, &total_subsongs, &stream_size, &channels, &sample_rate, &num_samples))
        goto fail;

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channels, loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->layout_type = layout_none;
    vgmstream->sample_rate = sample_rate;
    vgmstream->num_samples = num_samples;
    vgmstream->num_streams = total_subsongs;
    vgmstream->stream_size = stream_size;
    vgmstream->meta_type = meta_BINK;

#ifdef VGM_USE_FFMPEG
    {
        /* target_subsong should be passed manually */
        vgmstream->codec_data = init_ffmpeg_header_offset_subsong(sf, NULL,0, 0x0,0, target_subsong);
        if (!vgmstream->codec_data) goto fail;
        vgmstream->coding_type = coding_FFmpeg;
    }
#else
    goto fail;
#endif

    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}

/* official values */
#define BINK_MAX_FRAMES 1000000
#define BINK_MAX_TRACKS 256

/**
 * Gets stream info, and number of samples in a BINK file by reading all frames' headers (as it's VBR),
 * as they are not in the main header. The header for BINK1 and 2 is the same.
 * (a ~3 min movie needs ~6000-7000 frames = fseeks, should be fast enough)
 * see: https://wiki.multimedia.cx/index.php?title=Bink_Container */
static bool bink_get_info(STREAMFILE* sf, int target_subsong, int* p_total_subsongs, size_t* p_stream_size, int* p_channels, int* p_sample_rate, int* p_num_samples) {
    uint32_t* offsets = NULL; //TODO rename
    uint32_t num_samples_b = 0;
    off_t cur_offset;
    size_t stream_size = 0;

    /* known revisions:
     * bik1: b,d,f,g,h,i,k [no "j"]
     * bik2: a,d,f,g,h,i,j,k,m,n [no "l"]
     * (current public binkplay.exe allows 1=f~k and 2=f~n) */
    uint32_t head_id    = read_u32be(0x00,sf);
    uint32_t file_size  = read_u32le(0x04,sf) + 0x08;
    uint32_t num_frames = read_u32le(0x08,sf);
    /* 0x0c: largest frame */
    /* 0x10: num_frames again (found even for files without audio) */
    /* 0x14: video width (max 32767, apparently can be negative but no different meaning) */
    /* 0x18: video height (max 32767) */
    /* 0x1c: fps dividend (must be set) */
    /* 0x20: fps divider (must be set)
     * - ex. 2997/100 = 29.97 fps, 30/1 = 30 fps (common values) */
  //uint32_t video_flags = read_u32le(0x24,sf); /* (scale, alpha, color modes, etc) */
    /* 0x28: audio tracks */
    /* video flags:
       - F0000000 (bits 28-31): width and height scaling bits (doubled, interlaced, etc)
       - 00100000 (bit 20): has alpha plane
       - 00040000 (bit 18): unknown, related to packet format? (seen in some bik2 n+, with and w/o audio)
       - 00020000 (bit 17): grayscale
       - 00010000 (bit 16): 12 16b numbers? (seen in binkplay)
       - 00000010 (bit 4): 1 big number? (seems always set in bik1 k+ bik2 i+, but also set in bik2 g wihtout the number)
       - 00000004 (bit 2): 6 16b (seen in some bik2 n+)
       - 00000002 (bit 1): unknown (seen in some bik2 n+, with and w/o audio)
       (from binkplay, flags 0x04 and 0x10000 can't coexist)
    */

    if (file_size != get_streamfile_size(sf))
        return false;
    if (num_frames <= 0 || num_frames > BINK_MAX_FRAMES)
        return false; /* (avoids big allocs below) */

    uint32_t signature = head_id & 0xffffff00;
    uint8_t revision = head_id & 0xFF;
    int sample_rate, channels;
    uint16_t audio_flags;

    /* multichannel/multilanguage audio is usually N streams of stereo/mono, no way to know channel layout */
    int total_subsongs = read_s32le(0x28,sf);
    if (total_subsongs < 1) {
        vgm_logi("BIK: no audio in movie (ignore)\n");
        goto fail;
    }

    if (target_subsong == 0) target_subsong = 1;
    if (target_subsong < 0 || target_subsong > total_subsongs || total_subsongs > BINK_MAX_TRACKS) goto fail;


    /* find stream info and position in offset table */
    cur_offset = 0x2c;

    if ((signature == get_id32be("BIK\0") && revision >= 'k') || (signature == get_id32be("KB2\0") && revision >= 'i'))
        cur_offset += 0x04;

  //if (video_flags & 0x000004) /* only n+? */
  //    cur_offset += 0x0c; /* 6 number: s16 * 0.00003051850944757462 (video stuff?) */
  //if (video_flags & 0x010000)
  //    cur_offset += 0x18;
  //if (video_flags & 0x000010) /* only i+? */
  //    cur_offset += 0x04;

    cur_offset += 0x04 * total_subsongs; /* skip streams max packet bytes */
    sample_rate = read_u16le(cur_offset + 0x04 * (target_subsong - 1) + 0x00, sf);
    audio_flags = read_u16le(cur_offset + 0x04 * (target_subsong - 1) + 0x02, sf);
    cur_offset += 0x04 * total_subsongs; /* skip streams info */
    cur_offset += 0x04 * total_subsongs; /* skip streams ids */

    /* audio flags:
       - 8000 (bit 15): unknown (observed in some samples)
       - 4000 (bit 14): unknown (same file may have it set for none/some/all)
       - 2000 (bit 13): stereo flag
       - 1000 (bit 12): audio type (1=DCT, 0=FFT)
    */
    channels = audio_flags & 0x2000 ? 2 : 1;


    /* read frame offsets in a buffer, to avoid fseeking to the table back and forth */
    offsets = malloc(num_frames * sizeof(uint32_t));
    if (!offsets) goto fail;

    for (int i = 0; i < num_frames; i++) {
        offsets[i] = read_u32le(cur_offset, sf) & 0xFFFFFFFE; /* mask first bit (= keyframe) */
        cur_offset += 0x4;

        if (offsets[i] > file_size)
            goto fail;
    }
    /* after the last index is the file size, validate just in case */
    if (read_u32le(cur_offset,sf) != file_size)
        goto fail;

    /* read each frame header and sum all samples
     * a frame has N audio packets with a header (one per stream) + video packet */
    for (int i = 0; i < num_frames; i++) {
        cur_offset = offsets[i];

        /* read audio packet headers per stream */
        for (int j = 0; j < total_subsongs; j++) {
            uint32_t ap_size = read_u32le(cur_offset + 0x00,sf); /* not counting this int */

            if (j + 1 == target_subsong) {
                stream_size += 0x04 + ap_size;
                if (ap_size > 0)
                    num_samples_b += read_u32le(cur_offset + 0x04,sf); /* decoded samples in bytes */
                break; /* next frame */
            }
            else { /* next stream packet or frame */
                cur_offset += 4 + ap_size; //todo sometimes ap_size doesn't include itself (+4), others it does?
            }
        }
    }


    if (p_total_subsongs) *p_total_subsongs = total_subsongs;
    if (p_stream_size)    *p_stream_size = stream_size;
    if (p_sample_rate)    *p_sample_rate = sample_rate;
    if (p_channels)  *p_channels = channels;
    //todo returns a few more samples (~48) than binkconv.exe?
    if (p_num_samples)    *p_num_samples = num_samples_b / (2 * channels);

    free(offsets);
    return true;
fail:
    free(offsets);
    return false;
}
