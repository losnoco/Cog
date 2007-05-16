/*
 *  $Id$
 *
 *  Copyright (C) 2006 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "CoreAudioUtils.h"

#include <AudioToolbox/AudioFile.h>

BOOL hostIsBigEndian()
{
#ifdef __BIG_ENDIAN__
	return YES;
#else
	return NO;
#endif
}

AudioStreamBasicDescription propertiesToASBD(NSDictionary *properties)
{
	AudioStreamBasicDescription asbd;
	asbd.mFormatID = kAudioFormatLinearPCM;
	asbd.mFormatFlags = 0;

	asbd.mSampleRate = [[properties objectForKey:@"sampleRate"] doubleValue];

	asbd.mBitsPerChannel = [[properties objectForKey:@"bitsPerSample"] intValue];

	asbd.mChannelsPerFrame = [[properties objectForKey:@"channels"] intValue];;
	asbd.mBytesPerFrame = (asbd.mBitsPerChannel/8)*asbd.mChannelsPerFrame;
	
	asbd.mFramesPerPacket = 1;
	asbd.mBytesPerPacket = asbd.mBytesPerFrame * asbd.mFramesPerPacket;
	asbd.mReserved = 0;
	
	if ([[properties objectForKey:@"endian"] isEqualToString:@"big"] || ([[properties objectForKey:@"endian"] isEqualToString:@"host"] && hostIsBigEndian() ))
	{
		asbd.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
		asbd.mFormatFlags |= kLinearPCMFormatFlagIsAlignedHigh;
	}
	
	if ([[properties objectForKey:@"unsigned"] boolValue] == NO) {
		asbd.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
	}
	
	return asbd;
}


