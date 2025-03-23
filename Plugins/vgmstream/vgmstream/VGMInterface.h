//
//  VGMInterface.h
//  VGMStream
//
//  Created by Christopher Snowhill on 9/1/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import <libvgmstream/libvgmstream.h>

libstreamfile_t* open_vfs(const char *path);

void register_log_callback();
