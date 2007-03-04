//
//  SourceCIO.m
//  MonkeysAudio
//
//  Created by Zaphod Beeblebrox on 3/4/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "SourceIO.h"

#import <wchar.h>
#include <MAC/NoWindows.h>

SourceIO::SourceIO(id<CogSource> s)
{
	source = [s retain];
}

SourceIO::~SourceIO()
{
	[source release];
}

int SourceIO::Open(const wchar_t * pName)
{
	return -1;
}

int SourceIO::Close()
{
	[source close];

	return 0;
}

// read / write
int SourceIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
	int l = [source read:pBuffer amount:nBytesToRead];

	if (l < 0) {
		*pBytesRead = 0;
		NSLog(@"Error!");
		return -1;
	}
	*pBytesRead = l;
	
	return 0;
}
int SourceIO::Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten)
{
	*pBytesWritten = 0;

	return -1;
}

// seek
int SourceIO::Seek(int nDistance, unsigned int nMoveMode)
{
	return ([source seekable] && [source seek:nDistance whence:nMoveMode] ? 0 : -1);
}

// creation / destruction
int SourceIO::Create(const wchar_t * pName)
{
	return -1;
}

int SourceIO::Delete()
{
	return -1;
}

// other functions
int SourceIO::SetEOF()
{
	return -1;
}

// attributes
int SourceIO::GetPosition()
{
	return [source tell];
}

int SourceIO::GetSize()
{
	if ([source seekable]) {
		long currentPos = [source tell];
		
		[source seek:0 whence:SEEK_END];
		long size = [source tell];
		
		[source seek:currentPos whence:SEEK_SET];
		
		return size;
	}
	else
	{
		return -1;
	}
}

int SourceIO::GetName(wchar_t * pBuffer)
{
	wcscpy(pBuffer,(const wchar_t*)[[[[source url] path] substringWithRange:NSMakeRange(0, MAX_PATH)] UTF8String]);
	
	return 0;
}

