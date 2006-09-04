//
//  GameFile.m
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GameFile.h"

#include "GME/Nsf_Emu.h"
#include "GME/Gbs_Emu.h"
#include "GME/Spc_Emu.h"
#include "GME/Vgm_Emu.h"
#include "GME/Gym_Emu.h"

@implementation GameFile

- (BOOL)open:(const char *)filename
{
	int i;
	const char* p;
    char ext[3];
	
	p = strrchr( (char*) filename, '.' )+1;
	NSLog(@"OPENING GAME FILE: %s", filename);
	NSLog(@"Extension: %s", p);

	if (!p || strlen(p) != 3)
		return NO;
	
	for (i = 0; i < 4; i++)
		ext[i] = toupper(p[i]);
	ext[3] = 0;
	
	if (!ext)
		emu = NULL;
	else if ( !strcmp( ext, "NSF" ) )
		emu = new Nsf_Emu;
	else if ( !strcmp( ext, "GBS" ) )
		emu = new Gbs_Emu;
	else if ( !strcmp( ext, "SPC" ) )
		emu = new Spc_Emu;
	else if ( !strcmp( ext, "VGM" ) || !strcmp( ext, "VGZ" ) )
		emu = new Vgm_Emu;
	else if ( !strcmp( ext, "GYM" ) )
		emu = new Gym_Emu;
	else
		emu = NULL;
	
	NSLog(@"EMU IS: %i", emu);
	if (!emu)
		return NO;
	
	emu->set_sample_rate(44100);
	emu->load_file(filename);
	emu->start_track( 0 );
	frequency = 44100;
	channels = 2;
	
	bitsPerSample = 8 * sizeof(Music_Emu::sample_t);

	totalSize = emu->track_count() * frequency*(bitsPerSample/8)*channels;

	isBigEndian = YES;
	
	NSLog(@"OPENED GAME FILE:");
	
	return YES;
}

- (BOOL)readInfo:(const char *)filename
{
	return [self open:filename];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numSamples = size / ((bitsPerSample/8));

	emu->play(numSamples, (short int *)buf);
	
	return size; //No such thing as EOF
}

//Cheap hack until I figure out how to actually support multiple tracks in a single file.
- (double)seekToTime:(double)milliseconds
{
	int track;
	track = (int)(milliseconds/1000.0);
	NSLog(@"Track: %i", track);
	if (track > emu->track_count())
	{
		track = emu->track_count();
	}
	emu->start_track( track );
	
	
	return -1.0;	
}

- (void)close
{
	if (emu)
		delete emu;

}


@end
