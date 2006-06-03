//
//  ShnFile.h
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Shorten/shn_reader.h>

#import "SoundFile.h"

@interface ShnFile : SoundFile {
	//shn_file *handle;
	shn_reader *decoder;
	
	long bufferSize; //total size
	void *buffer;
	void *inputBuffer;//derek
	long bufferAmount; //amount currently in
}

@end
