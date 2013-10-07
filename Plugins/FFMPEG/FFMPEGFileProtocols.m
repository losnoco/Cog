//
//  FFMPEGFileProtocols.m
//  FFMPEG
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#include "Plugin.h"

#define __FRAMEWORK__
#import <FFMPEG/avformat.h>
#import <FFMPEG/url.h>
#import <FFMPEG/opt.h>
#undef __FRAMEWORK__

/* standard file protocol */

typedef struct FileContext {
    const AVClass *class;
    id<CogSource> fd;
} FileContext;

static const AVOption file_options[] = {
    { NULL }
};

#define LIBAVUTIL_VERSION_MAJOR  52
#define LIBAVUTIL_VERSION_MINOR  46
#define LIBAVUTIL_VERSION_MICRO 100

#define AV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)

#define LIBAVUTIL_VERSION_INT   AV_VERSION_INT(LIBAVUTIL_VERSION_MAJOR, \
LIBAVUTIL_VERSION_MINOR, \
LIBAVUTIL_VERSION_MICRO)

static const AVClass file_class = {
    .class_name = "file",
    .item_name  = av_default_item_name,
    .option     = file_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

static const AVClass http_class = {
    .class_name = "http",
    .item_name  = av_default_item_name,
    .option     = file_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

static const AVClass unpack_class = {
    .class_name = "unpack",
    .item_name  = av_default_item_name,
    .option     = file_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

static int file_read(URLContext *h, unsigned char *buf, int size)
{
    FileContext *c = h->priv_data;
    return [c->fd read:buf amount:size];
}

static int file_check(URLContext *h, int mask)
{
    return mask & AVIO_FLAG_READ;
}

static int file_open(URLContext *h, const char *filename, int flags)
{
    FileContext *c = h->priv_data;
    id<CogSource> fd;
    
    if (flags & AVIO_FLAG_WRITE) {
        return -1;
    }
    
    NSString * urlString = [NSString stringWithUTF8String:filename];
    NSURL * url = [NSURL URLWithString:[urlString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    
    id audioSourceClass = NSClassFromString(@"AudioSource");
    fd = [audioSourceClass audioSourceForURL:url];
    
    if (![fd open:url])
        return -1;
    
    c->fd = [fd retain];
    
    return 0;
}

/* XXX: use llseek */
static int64_t file_seek(URLContext *h, int64_t pos, int whence)
{
    FileContext *c = h->priv_data;
    return [c->fd seek:pos whence:whence] ? [c->fd tell] : -1;
}

static int file_close(URLContext *h)
{
    FileContext *c = h->priv_data;
    [c->fd release];
    return 0;
}

URLProtocol ff_file_protocol = {
    .name                = "file",
    .url_open            = file_open,
    .url_read            = file_read,
    .url_seek            = file_seek,
    .url_close           = file_close,
    .url_check           = file_check,
    .priv_data_size      = sizeof(FileContext),
    .priv_data_class     = &file_class,
};

URLProtocol ff_http_protocol = {
    .name                = "http",
    .url_open            = file_open,
    .url_read            = file_read,
    .url_seek            = file_seek,
    .url_close           = file_close,
    .url_check           = file_check,
    .priv_data_size      = sizeof(FileContext),
    .priv_data_class     = &file_class,
};

URLProtocol ff_unpack_protocol = {
    .name                = "unpack",
    .url_open            = file_open,
    .url_read            = file_read,
    .url_seek            = file_seek,
    .url_close           = file_close,
    .url_check           = file_check,
    .priv_data_size      = sizeof(FileContext),
    .priv_data_class     = &file_class,
};

void registerCogProtocols()
{
    ffurl_register_protocol(&ff_file_protocol, sizeof(ff_file_protocol));
    ffurl_register_protocol(&ff_http_protocol, sizeof(ff_http_protocol));
    ffurl_register_protocol(&ff_unpack_protocol, sizeof(ff_unpack_protocol));
}
