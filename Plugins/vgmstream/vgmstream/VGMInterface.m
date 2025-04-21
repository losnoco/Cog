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

static void log_callback(int level, const char* str) {
	ALog(@"%s", str);
}

void register_log_callback() {
	libvgmstream_set_log(LIBVGMSTREAM_LOG_LEVEL_ALL, &log_callback);
}

typedef struct {
	void* infile;
	int64_t offset, file_size;
	char name[0x4000];
} vfs_priv_t;

static size_t vfs_read(void* user_data, uint8_t* dst, int64_t offset, int length) {
	vfs_priv_t* priv = (vfs_priv_t*)user_data;
	size_t read_total = 0;

	NSObject* _file = (__bridge NSObject*)(priv->infile);
	id<CogSource> __unsafe_unretained file = (id)_file;

	if(!dst || length <= 0 || offset < 0)
		return 0;
	
	if(priv->offset != offset) {
		BOOL ok = offset <= priv->file_size && [file seek:offset whence:SEEK_SET];
		if(!ok)
			return 0;
		priv->offset = offset;
	}
	
	size_t bytes_read = [file read:dst amount:length];
	priv->offset += bytes_read;
	
	return bytes_read;
}

static size_t vfs_get_size(void* user_data) {
	vfs_priv_t* priv = (vfs_priv_t*)user_data;
	return priv->file_size;
}

static const char* vfs_get_name(void* user_data) {
	vfs_priv_t* priv = (vfs_priv_t*)user_data;
	return priv->name;
}

static libstreamfile_t* vfs_open(void* user_data, const char* filename) {
	if(!filename)
		return NULL;

	return open_vfs(filename);
}

static void vfs_close(libstreamfile_t* libsf) {
	if(libsf->user_data) {
		vfs_priv_t* priv = (vfs_priv_t*)libsf->user_data;
		CFBridgingRelease(priv->infile);
		free(priv);
	}
	free(libsf);
}

static libstreamfile_t* open_vfs_by_cogsource(id<CogSource> file, const char* path) {
	vfs_priv_t* priv = NULL;
	libstreamfile_t* libsf = (libstreamfile_t*)calloc(1, sizeof(libstreamfile_t));
	if(!libsf) return NULL;

	libsf->read = (int (*)(void*, uint8_t*, int64_t, int)) vfs_read;
	libsf->get_size = (int64_t (*)(void*)) vfs_get_size;
	libsf->get_name = (const char* (*)(void*)) vfs_get_name;
	libsf->open = (libstreamfile_t* (*)(void*, const char* const)) vfs_open;
	libsf->close = (void (*)(libstreamfile_t*)) vfs_close;

	libsf->user_data = (vfs_priv_t*)calloc(1, sizeof(vfs_priv_t));
	if(!libsf->user_data) goto fail;

	priv = (vfs_priv_t*)libsf->user_data;
	priv->infile = CFBridgingRetain(file);
	priv->offset = 0;
	strncpy(priv->name, path, sizeof(priv->name));
	priv->name[sizeof(priv->name) - 1] = '\0';

	if(![file seekable]) {
		goto fail;
	}

	[file seek:0 whence:SEEK_END];
	priv->file_size = [file tell];
	[file seek:0 whence:SEEK_SET];

	return libsf;

fail:
	vfs_close(libsf);
	return NULL;
}

libstreamfile_t* open_vfs(const char* path) {
	NSString* urlString = [NSString stringWithUTF8String:path];
	NSURL* url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];

	if([url fragment]) {
		// .TXTP fragments need an override here
		NSString* frag = [url fragment];
		NSUInteger len = [frag length];
		if(len > 5 && [[frag substringFromIndex:len - 5] isEqualToString:@".txtp"]) {
			urlString = [urlString stringByReplacingOccurrencesOfString:@"#" withString:@"%23"];
			url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];
		}
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> infile = [audioSourceClass audioSourceForURL:url];

	if(![infile open:url]) {
		return NULL;
	}

	return open_vfs_by_cogsource(infile, path);
}
