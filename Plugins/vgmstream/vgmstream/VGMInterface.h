//
//  VGMInterface.h
//  VGMStream
//
//  Created by Christopher Snowhill on 9/1/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import <libvgmstream/vgmstream.h>

typedef struct _COGSTREAMFILE {
    STREAMFILE sf;
    void *file;
    off_t offset;
    char name[PATH_LIMIT];
} COGSTREAMFILE;

VGMSTREAM *init_vgmstream_from_cogfile(const char *path, int subsong);
