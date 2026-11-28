//
//  MP3Implementation.m
//  minimp3
//
//  Compiles the header-only minimp3 implementation into the plugin. The
//  Swift decoder reaches the API through the bridging header, which imports
//  minimp3_ex.h without MINIMP3_IMPLEMENTATION.
//

#define MINIMP3_FLOAT_OUTPUT 1
#define MINIMP3_NO_STDIO 1
#define MINIMP3_IMPLEMENTATION 1

#import "ThirdParty/minimp3_ex.h"
