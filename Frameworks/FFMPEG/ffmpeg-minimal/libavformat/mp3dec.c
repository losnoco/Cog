/*
 * MP3 demuxer
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "libavutil/opt.h"
#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/dict.h"
#include "libavutil/mathematics.h"
#include "avformat.h"
#include "internal.h"
#include "id3v2.h"
#include "id3v1.h"
#include "apetag.h"
#include "libavcodec/mpegaudiodecheader.h"

#define XING_FLAG_FRAMES 0x01
#define XING_FLAG_SIZE   0x02
#define XING_FLAG_TOC    0x04

#define XING_TOC_COUNT 100

typedef struct {
    AVClass *class;
    int64_t filesize;
    int64_t header_filesize;
    int xing_toc;
    int start_pad;
    int end_pad;
    int usetoc;
    int is_cbr;
} MP3DecContext;

/* mp3 read */

static int mp3_read_probe(AVProbeData *p)
{
    int max_frames, first_frames = 0;
    int fsize, frames, sample_rate;
    uint32_t header;
    const uint8_t *buf, *buf0, *buf2, *end;
    AVCodecContext avctx;

    buf0 = p->buf;
    end = p->buf + p->buf_size - sizeof(uint32_t);
    while(buf0 < end && !*buf0)
        buf0++;

    max_frames = 0;
    buf = buf0;

    for(; buf < end; buf= buf2+1) {
        buf2 = buf;

        for(frames = 0; buf2 < end; frames++) {
            header = AV_RB32(buf2);
            fsize = avpriv_mpa_decode_header(&avctx, header, &sample_rate, &sample_rate, &sample_rate, &sample_rate);
            if(fsize < 0)
                break;
            buf2 += fsize;
        }
        max_frames = FFMAX(max_frames, frames);
        if(buf == buf0)
            first_frames= frames;
    }
    // keep this in sync with ac3 probe, both need to avoid
    // issues with MPEG-files!
    if   (first_frames>=4) return AVPROBE_SCORE_EXTENSION + 1;
    else if(max_frames>200)return AVPROBE_SCORE_EXTENSION;
    else if(max_frames>=4) return AVPROBE_SCORE_EXTENSION / 2;
    else if(ff_id3v2_match(buf0, ID3v2_DEFAULT_MAGIC) && 2*ff_id3v2_tag_len(buf0) >= p->buf_size)
                           return p->buf_size < PROBE_BUF_MAX ? AVPROBE_SCORE_EXTENSION / 4 : AVPROBE_SCORE_EXTENSION - 2;
    else if(max_frames>=1) return 1;
    else                   return 0;
//mpegps_mp3_unrecognized_format.mpg has max_frames=3
}

static void read_xing_toc(AVFormatContext *s, int64_t filesize, int64_t duration)
{
    int i;
    MP3DecContext *mp3 = s->priv_data;
    int fill_index = mp3->usetoc && duration > 0;

    if (!filesize &&
        !(filesize = avio_size(s->pb))) {
        av_log(s, AV_LOG_WARNING, "Cannot determine file size, skipping TOC table.\n");
        fill_index = 0;
    }

    for (i = 0; i < XING_TOC_COUNT; i++) {
        uint8_t b = avio_r8(s->pb);
        if (fill_index)
            av_add_index_entry(s->streams[0],
                           av_rescale(b, filesize, 256),
                           av_rescale(i, duration, XING_TOC_COUNT),
                           0, 0, AVINDEX_KEYFRAME);
    }
    if (fill_index)
        mp3->xing_toc = 1;
}

/**
 * Try to find Xing/Info/VBRI tags and compute duration from info therein
 */
static int mp3_parse_vbr_tags(AVFormatContext *s, AVStream *st, int64_t base)
{
    MP3DecContext *mp3 = s->priv_data;
    uint32_t v, spf;
    unsigned frames = 0; /* Total number of frames in file */
    unsigned size = 0; /* Total number of bytes in the stream */
    static const int64_t xing_offtbl[2][2] = {{32, 17}, {17,9}};
    MPADecodeHeader c;
    int vbrtag_size = 0;
    int is_cbr;
    AVDictionaryEntry *de;
    uint64_t duration = 0;

    v = avio_rb32(s->pb);
    if(ff_mpa_check_header(v) < 0)
      return -1;

    if (avpriv_mpegaudio_decode_header(&c, v) == 0)
        vbrtag_size = c.frame_size;
    if(c.layer != 3)
        return -1;
    
    mp3->start_pad = 0;
    mp3->end_pad = 0;
    
    spf = c.lsf ? 576 : 1152; /* Samples per frame, layer 3 */

    /* Check for Xing / Info tag */
    avio_skip(s->pb, xing_offtbl[c.lsf == 1][c.nb_channels == 1]);
    v = avio_rb32(s->pb);
    is_cbr = v == MKBETAG('I', 'n', 'f', 'o');
    if (v == MKBETAG('X', 'i', 'n', 'g') || is_cbr) {
        v = avio_rb32(s->pb);
        if(v & XING_FLAG_FRAMES)
            frames = avio_rb32(s->pb);
        if(v & XING_FLAG_SIZE)
            size = avio_rb32(s->pb);
        if (v & XING_FLAG_TOC)
            read_xing_toc(s, size, av_rescale_q(frames, (AVRational){spf, c.sample_rate},
                                    st->time_base));
        if(v & 8)
            avio_skip(s->pb, 4);

        v = avio_rb32(s->pb);
        if(v == MKBETAG('L', 'A', 'M', 'E') || v == MKBETAG('L', 'a', 'v', 'f')) {
            avio_skip(s->pb, 21-4);
            v= avio_rb24(s->pb);
            mp3->start_pad = v>>12;
            mp3->  end_pad = v&4095;
            st->skip_samples = mp3->start_pad + 528 + 1;
            if (mp3->end_pad >= 528 + 1)
                mp3->end_pad -= 528 + 1;
            av_log(s, AV_LOG_DEBUG, "pad %d %d\n", mp3->start_pad, mp3->  end_pad);
        }
    }

    /* Check for VBRI tag (always 32 bytes after end of mpegaudio header) */
    avio_seek(s->pb, base + 4 + 32, SEEK_SET);
    v = avio_rb32(s->pb);
    if(v == MKBETAG('V', 'B', 'R', 'I')) {
        /* Check tag version */
        if(avio_rb16(s->pb) == 1) {
            /* skip delay and quality */
            avio_skip(s->pb, 4);
            size = avio_rb32(s->pb);
            frames = avio_rb32(s->pb);
        }
    }
    
    if (!frames)
        vbrtag_size = 0;

    if (s->metadata && (de = av_dict_get(s->metadata, "iTunSMPB", NULL, 0))) {
        uint32_t zero, start_pad, end_pad;
        uint64_t last_eight_frames_offset;
        if (sscanf(de->value, "%x %x %x %llx %x %llx", &zero, &start_pad, &end_pad, &duration, &zero, &last_eight_frames_offset) < 6) {
            duration = 0;
        }
        else {
            mp3->start_pad = start_pad;
            mp3->end_pad = end_pad;
            if (end_pad >= 528 + 1)
                mp3->end_pad = end_pad - (528 + 1);
            st->skip_samples = mp3->start_pad + 528 + 1;
            av_log(s, AV_LOG_DEBUG, "pad %d %d\n", mp3->start_pad, mp3->end_pad);
            if (s->pb->seekable) {
                int i;
                size = last_eight_frames_offset;
                avio_seek(s->pb, base + vbrtag_size + last_eight_frames_offset, SEEK_SET);
                for (i = 0; i < 8; ++i) {
                    v = avio_rb32(s->pb);
                    if (ff_mpa_check_header(v) < 0)
                        return -1;
                    if (avpriv_mpegaudio_decode_header(&c, v) != 0)
                        break;
                    size += c.frame_size;
                    avio_skip(s->pb, c.frame_size - 4);
                }
            }
        }
    }
    
    if(!frames && !size && !duration)
        return -1;

    /* Skip the vbr tag frame */
    avio_seek(s->pb, base + vbrtag_size, SEEK_SET);

    if (duration)
        st->duration = av_rescale_q(duration, (AVRational){1, c.sample_rate}, st->time_base);
    else if(frames)
        st->duration = av_rescale_q(frames, (AVRational){spf, c.sample_rate},
                                    st->time_base) - av_rescale_q(mp3->end_pad, (AVRational){1, c.sample_rate}, st->time_base);
    
    if (size) {
        if (duration)
            st->codec->bit_rate = av_rescale(size, 8 * c.sample_rate, duration);
        else if (frames)
            st->codec->bit_rate = av_rescale(size, 8 * c.sample_rate, frames * (int64_t)spf);
    }

    mp3->is_cbr          = is_cbr;
    mp3->header_filesize = size;

    return 0;
}

static int mp3_read_header(AVFormatContext *s)
{
    MP3DecContext *mp3 = s->priv_data;
    AVStream *st;
    int64_t off;

    st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);

    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_id = AV_CODEC_ID_MP3;
    st->need_parsing = AVSTREAM_PARSE_FULL_RAW;
    st->start_time = 0;

    // lcm of all mp3 sample rates
    avpriv_set_pts_info(st, 64, 1, 14112000);

    s->pb->maxsize = -1;
    off = avio_tell(s->pb);

    if (!av_dict_get(s->metadata, "", NULL, AV_DICT_IGNORE_SUFFIX))
        ff_id3v1_read(s);

    if(s->pb->seekable)
        mp3->filesize = avio_size(s->pb);

    if (mp3_parse_vbr_tags(s, st, off) < 0)
    {
        uint64_t duration = 0;
        uint8_t buf[8];
        int sample_rate = 0;
        /* Time for a full parse! */
        avio_seek(s->pb, -128, SEEK_END);
        avio_read(s->pb, buf, 3);
        if (buf[0] == 'T' && buf[1] == 'A' && buf[2] == 'G')
            mp3->filesize -= 128;
        avio_seek(s->pb, mp3->filesize - APE_TAG_FOOTER_BYTES, SEEK_SET);
        avio_read(s->pb, buf, 8);
        if (memcmp(buf, APE_TAG_PREAMBLE, 8) == 0)
        {
            avio_seek(s->pb, 4, SEEK_CUR);
            mp3->filesize -= avio_rl32(s->pb) + APE_TAG_FOOTER_BYTES;
        }
        avio_seek(s->pb, off, SEEK_SET);
        while (avio_tell(s->pb) < mp3->filesize)
        {
            MPADecodeHeader c;
            uint32_t v, spf;
            
            v = avio_rb32(s->pb);
            
            if(ff_mpa_check_header(v) < 0)
                break;
            
            if (avpriv_mpegaudio_decode_header(&c, v) != 0)
                break;
            
            if (!sample_rate)
                sample_rate = c.sample_rate;
            
            spf = c.lsf ? 576 : 1152; /* Samples per frame, layer 3 */
            
            duration += spf;
            
            avio_skip(s->pb, c.frame_size - 4);
        }
        avio_seek(s->pb, off, SEEK_SET);
        st->duration = av_rescale_q(duration, (AVRational){1, sample_rate}, st->time_base);
    }

    /* the parameters will be extracted from the compressed bitstream */
    return 0;
}

#define MP3_PACKET_SIZE 1024

static int mp3_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    MP3DecContext *mp3 = s->priv_data;
    int ret, size;
    int64_t pos;

    size= MP3_PACKET_SIZE;
    pos = avio_tell(s->pb);
    if(mp3->filesize > ID3v1_TAG_SIZE && pos < mp3->filesize)
        size= FFMIN(size, mp3->filesize - pos);

    ret= av_get_packet(s->pb, pkt, size);
    if (ret <= 0) {
        if(ret<0)
            return ret;
        return AVERROR_EOF;
    }

    pkt->flags &= ~AV_PKT_FLAG_CORRUPT;
    pkt->stream_index = 0;

    if (ret >= ID3v1_TAG_SIZE &&
        memcmp(&pkt->data[ret - ID3v1_TAG_SIZE], "TAG", 3) == 0)
        ret -= ID3v1_TAG_SIZE;

    /* note: we need to modify the packet size here to handle the last
       packet */
    pkt->size = ret;
    return ret;
}

static int check(AVFormatContext *s, int64_t pos)
{
    int64_t ret = avio_seek(s->pb, pos, SEEK_SET);
    unsigned header;
    MPADecodeHeader sd;
    if (ret < 0)
        return ret;
    header = avio_rb32(s->pb);
    if (ff_mpa_check_header(header) < 0)
        return -1;
    if (avpriv_mpegaudio_decode_header(&sd, header) == 1)
        return -1;
    return sd.frame_size;
}

static int mp3_seek(AVFormatContext *s, int stream_index, int64_t timestamp,
                    int flags)
{
    MP3DecContext *mp3 = s->priv_data;
    AVStream *st = s->streams[0];
    int64_t timestamp_samples = 0;
    uint32_t v, spf;

    timestamp = av_clip64(timestamp, 0, st->duration);
    
    /* Screw it, we're doing a full stream parse! */
    avio_seek(s->pb, s->data_offset, SEEK_SET);
    
    st->skip_samples = 0;
    
    if (timestamp > 0) {
        int64_t skipped = 0;
        int64_t skip_extra = 0;
        do {
            MPADecodeHeader c;
            
            v = avio_rb32(s->pb);
            avio_seek(s->pb, -4, SEEK_CUR);
            
            if(ff_mpa_check_header(v) < 0)
                return -1;
            
            if (avpriv_mpegaudio_decode_header(&c, v) != 0)
                return -1;

            spf = c.lsf ? 576 : 1152; /* Samples per frame, layer 3 */
            
            if (!timestamp_samples) {
                timestamp_samples = av_rescale_q(timestamp, st->time_base, (AVRational){1, c.sample_rate}) + mp3->start_pad + 528 + 1;
                if (timestamp_samples >= spf * 8 ) {
                    timestamp_samples -= spf * 8;
                    skip_extra = spf * 8;
                }
                else {
                    skip_extra = timestamp_samples;
                    timestamp_samples = 0;
                }
            }
            
            if ( skipped + spf > timestamp_samples ) break;
            
            skipped += spf;
            
            avio_skip(s->pb, c.frame_size);
        } while ( skipped < timestamp_samples );
        
        st->skip_samples = timestamp_samples - skipped + skip_extra;
    }
    
    ff_update_cur_dts(s, st, timestamp);
    
    return 0;
}

static const AVOption options[] = {
    { "usetoc", "use table of contents", offsetof(MP3DecContext, usetoc), AV_OPT_TYPE_INT, {.i64 = -1}, -1, 1, AV_OPT_FLAG_DECODING_PARAM},
    { NULL },
};

static const AVClass demuxer_class = {
    .class_name = "mp3",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEMUXER,
};

AVInputFormat ff_mp3_demuxer = {
    .name           = "mp3",
    .long_name      = NULL_IF_CONFIG_SMALL("MP2/3 (MPEG audio layer 2/3)"),
    .read_probe     = mp3_read_probe,
    .read_header    = mp3_read_header,
    .read_packet    = mp3_read_packet,
    .read_seek      = mp3_seek,
    .priv_data_size = sizeof(MP3DecContext),
    .flags          = AVFMT_GENERIC_INDEX,
    .extensions     = "mp2,mp3,m2a,mpa", /* XXX: use probe */
    .priv_class     = &demuxer_class,
};
