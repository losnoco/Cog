//
//  VGMInterface.h
//  VGMStream
//
//  Created by Christopher Snowhill on 9/1/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import <libvgmstream/vgmstream.h>

#import <libvgmstream/plugins.h>

/* a STREAMFILE that operates via standard IO using a buffer */
typedef struct _COGSTREAMFILE {
	STREAMFILE vt; /* callbacks */

	void* infile; /* CogSource, retained */
	char* name; /* FILE filename */
	int name_len; /* cache */

	char* archname; /* archive name */
	int archname_len; /* cache */
	int archpath_end; /* where the last / ends before archive name */
	int archfile_end; /* where the last | ends before file name */

	offv_t offset; /* last read offset (info) */
	offv_t buf_offset; /* current buffer data start */
	uint8_t* buf; /* data buffer */
	size_t buf_size; /* max buffer size */
	size_t valid_size; /* current buffer size */
	size_t file_size; /* buffered file size */
} COGSTREAMFILE;

STREAMFILE* open_cog_streamfile_from_url(NSURL* url);
STREAMFILE* open_cog_streamfile(const char* filename);

VGMSTREAM* init_vgmstream_from_cogfile(const char* path, int subsong);

void register_log_callback();
