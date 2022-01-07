//
//  VGMInterface.m
//  VGMStream
//
//  Created by Christopher Snowhill on 9/1/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import "VGMInterface.h"

#import "Plugin.h"

#import "Logging.h"

static void log_callback(int level, const char *str) {
    ALog(@"%@", str);
}

void register_log_callback() {
    vgmstream_set_log_callback(VGM_LOG_LEVEL_ALL, &log_callback);
}

static STREAMFILE* open_cog_streamfile_buffer(const char* const filename, size_t buf_size);
static STREAMFILE* open_cog_streamfile_buffer_by_file(id infile, const char* const filename, size_t buf_size);

static size_t cogsf_read(COGSTREAMFILE* sf, uint8_t* dst, offv_t offset, size_t length) {
    size_t read_total = 0;

    if (!sf || !sf->infile || !dst || length <= 0 || offset < 0)
        return 0;

    //;VGM_LOG("cogsf: read %lx + %x (buf %lx + %x)\n", offset, length, sf->buf_offset, sf->valid_size);

    /* is the part of the requested length in the buffer? */
    if (offset >= sf->buf_offset && offset < sf->buf_offset + sf->valid_size) {
        size_t buf_limit;
        int buf_into = (int)(offset - sf->buf_offset);

        buf_limit = sf->valid_size - buf_into;
        if (buf_limit > length)
            buf_limit = length;

        //;VGM_LOG("cogsf: copy buf %lx + %x (+ %x) (buf %lx + %x)\n", offset, length_to_read, (length - length_to_read), sf->buf_offset, sf->valid_size);

        memcpy(dst, sf->buf + buf_into, buf_limit);
        read_total += buf_limit;
        length -= buf_limit;
        offset += buf_limit;
        dst += buf_limit;
    }

#ifdef VGM_DEBUG_OUTPUT
    if (offset < sf->buf_offset && length > 0) {
        //VGM_LOG("cogsf: rebuffer, requested %x vs %x (sf %x)\n", (uint32_t)offset, (uint32_t)sf->buf_offset, (uint32_t)sf);
        //sf->rebuffer++;
        //if (rebuffer > N) ...
    }
#endif
    NSObject* _file = (__bridge NSObject *)(sf->infile);
    id<CogSource> __unsafe_unretained file = (id) _file;
    
    /* read the rest of the requested length */
    while (length > 0) {
        size_t length_to_read;

        /* ignore requests at EOF */
        if (offset >= sf->file_size) {
            //offset = sf->file_size; /* seems fseek doesn't clamp offset */
            VGM_ASSERT_ONCE(offset > sf->file_size, "COGSF: reading over file_size 0x%x @ 0x%x + 0x%x\n", sf->file_size, (uint32_t)offset, length);
            break;
        }

        /* position to new offset */
        if (![file seek:offset whence:SEEK_SET]) {
            break; /* this shouldn't happen in our code */
        }

        /* fill the buffer (offset now is beyond buf_offset) */
        sf->buf_offset = offset;
        sf->valid_size = [file read:sf->buf amount:sf->buf_size];
        //;VGM_LOG("cogsf: read buf %lx + %x\n", sf->buf_offset, sf->valid_size);

        /* decide how much must be read this time */
        if (length > sf->buf_size)
            length_to_read = sf->buf_size;
        else
            length_to_read = length;

        /* give up on partial reads (EOF) */
        if (sf->valid_size < length_to_read) {
            memcpy(dst, sf->buf, sf->valid_size);
            offset += sf->valid_size;
            read_total += sf->valid_size;
            break;
        }

        /* use the new buffer */
        memcpy(dst, sf->buf, length_to_read);
        offset += length_to_read;
        read_total += length_to_read;
        length -= length_to_read;
        dst += length_to_read;
    }

    sf->offset = offset; /* last fread offset */
    return read_total;
}
static size_t cogsf_get_size(COGSTREAMFILE* sf) {
    return sf->file_size;
}
static offv_t cogsf_get_offset(COGSTREAMFILE* sf) {
    return sf->offset;
}
static void cogsf_get_name(COGSTREAMFILE* sf, char* name, size_t name_size) {
    int copy_size = sf->name_len + 1;
    if (copy_size > name_size)
        copy_size = name_size;

    memcpy(name, sf->name, copy_size);
    name[copy_size - 1] = '\0';
}

static STREAMFILE* cogsf_open(COGSTREAMFILE* sf, const char* const filename, size_t buf_size) {
    if (!filename)
        return NULL;

    // a normal open, open a new file
    return open_cog_streamfile_buffer(filename, buf_size);
}

static void cogsf_close(COGSTREAMFILE* sf) {
    if (sf->infile)
        CFBridgingRelease(sf->infile);
    free(sf->buf);
    free(sf);
}


static STREAMFILE* open_cog_streamfile_buffer_by_file(id<CogSource> infile, const char* const filename, size_t buf_size) {
    uint8_t* buf = NULL;
    COGSTREAMFILE* this_sf = NULL;
    int infile_retained = 0;

    buf = calloc(buf_size, sizeof(uint8_t));
    if (!buf) goto fail;

    this_sf = calloc(1, sizeof(COGSTREAMFILE));
    if (!this_sf) goto fail;

    this_sf->vt.read = (void*)cogsf_read;
    this_sf->vt.get_size = (void*)cogsf_get_size;
    this_sf->vt.get_offset = (void*)cogsf_get_offset;
    this_sf->vt.get_name = (void*)cogsf_get_name;
    this_sf->vt.open = (void*)cogsf_open;
    this_sf->vt.close = (void*)cogsf_close;

    if (infile) {
        this_sf->infile = (void*)CFBridgingRetain(infile);
        infile_retained = 1;
    }
    
    this_sf->buf_size = buf_size;
    this_sf->buf = buf;

    this_sf->name_len = strlen(filename);
    if (this_sf->name_len >= sizeof(this_sf->name))
        goto fail;
    memcpy(this_sf->name, filename, this_sf->name_len);
    this_sf->name[this_sf->name_len] = '\0';

    /* cache file_size */
    if (infile) {
        [infile seek:0 whence:SEEK_END];
        this_sf->file_size = [infile tell];
        [infile seek:0 whence:SEEK_SET];
    }
    else {
        this_sf->file_size = 0; /* allow virtual, non-existing files */
    }

    /* Typically fseek(o)/ftell(o) may only handle up to ~2.14GB, signed 32b = 0x7FFFFFFF
     * (happens in banks like FSB, though rarely). Should work if configured properly, log otherwise. */
    if (this_sf->file_size == 0xFFFFFFFF) { /* -1 on error */
        vgm_logi("STREAMFILE: file size too big (report)\n");
        goto fail; /* can be ignored but may result in strange/unexpected behaviors */
    }

    return &this_sf->vt;

fail:
    if (infile_retained) CFBridgingRelease(this_sf->infile);
    free(buf);
    free(this_sf);
    return NULL;
}

static STREAMFILE* open_cog_streamfile_buffer_from_url(NSURL* url, const char* const filename, size_t bufsize) {
    id<CogSource> infile;
    STREAMFILE* sf = NULL;

    id audioSourceClass = NSClassFromString(@"AudioSource");
    infile = [audioSourceClass audioSourceForURL:url];
    
    if (![infile open:url]) {
        /* allow non-existing files in some cases */
        if (!vgmstream_is_virtual_filename(filename))
            return NULL;
    }

    if (![infile seekable])
        return NULL;
    
    return open_cog_streamfile_buffer_by_file(infile, filename, bufsize);
}

static STREAMFILE* open_cog_streamfile_buffer(const char* const filename, size_t bufsize) {
    NSString * urlString = [NSString stringWithUTF8String:filename];
    NSURL * url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];
    
    if ([url fragment]) {
        // .TXTP fragments need an override here
        NSString * frag = [url fragment];
        NSUInteger len = [frag length];
        if (len > 5 && [[frag substringFromIndex:len - 5] isEqualToString:@".txtp"]) {
            urlString = [urlString stringByReplacingOccurrencesOfString:@"#" withString:@"%23"];
            url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];
        }
    }

    return open_cog_streamfile_buffer_from_url(url, filename, bufsize);
}

STREAMFILE* open_cog_streamfile_from_url(NSURL* url) {
    return open_cog_streamfile_buffer_from_url(url, [[[url absoluteString] stringByRemovingPercentEncoding] UTF8String], STREAMFILE_DEFAULT_BUFFER_SIZE);
}

STREAMFILE* open_cog_streamfile(const char* filename) {
    return open_cog_streamfile_buffer(filename, STREAMFILE_DEFAULT_BUFFER_SIZE);
}

//STREAMFILE* open_cog_streamfile_by_file(id<CogSource> file, const char* filename) {
//    return open_cog_streamfile_buffer_by_file(file, filename, STREAMFILE_DEFAULT_BUFFER_SIZE);
//}

VGMSTREAM *init_vgmstream_from_cogfile(const char *path, int subsong) {
    STREAMFILE *sf;
    VGMSTREAM *vgm = NULL;
    
    if (!subsong)
        subsong = 1;
    
    sf = open_cog_streamfile(path);
    
    if (sf) {
        sf->stream_index = subsong;
        vgm = init_vgmstream_from_STREAMFILE(sf);
        cogsf_close((COGSTREAMFILE *)sf);
    }
    
    return vgm;
}
