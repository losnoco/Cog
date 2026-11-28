//
//  Vorbis-Bridging-Header.h
//  Vorbis
//
//  The C APIs the Swift decoder reads through.
//

#import <Foundation/Foundation.h>

// config.h things
#define __MACOSX__
#define HAVE_CONFIG_H

#import <vorbis/codec.h>
#import <vorbis/vorbisfile.h>

#undef __MACOSX__
#undef HAVE_CONFIG_H

#import <FLAC/metadata.h>

#import "vorbis-tools/include/picture.h"

#import "../../Utils/NSDictionary+Merge.h"
