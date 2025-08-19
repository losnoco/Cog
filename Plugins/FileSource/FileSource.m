//
//  FileSource.m
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileSource.h"

#import "SandboxBroker.h"

#import "Logging.h"

@implementation FileSource

+ (void)initialize {
	fex_init();
}

- (id)init {
	self = [super init];
	if(self) {
		fex = NULL;
		_fd = NULL;
	}
	return self;
}

- (BOOL)open:(NSURL *)url {
	[self setURL:url];

	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
	id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];

	sbHandle = [sandboxBroker beginFolderAccess:url];

	NSString *path = [url path];
	if(!path) {
		ALog(@"Invalid URL: %@", url);
		return NO;
	}

	fex_type_t type;
	fex_err_t error = fex_identify_file(&type, [path UTF8String]);

	if(!error && type && strcmp(fex_type_name(type), "file")) {
		error = fex_open_type(&fex, [path UTF8String], type);
		if(!error) {
			while(!fex_done(fex)) {
				NSString *name = [[NSString stringWithUTF8String:fex_name(fex)] lowercaseString];
				if([name hasSuffix:@".diz"] || [name hasSuffix:@".txt"] || [name hasSuffix:@".nfo"]) {
					error = fex_next(fex);
					if(error)
						return NO;
					continue;
				}
				break;
			}
			if(fex_done(fex))
				return NO;

			error = fex_data(fex, &data);
			if(error)
				return NO;

			size = fex_size(fex);
			offset = 0;

			return YES;
		} else
			return NO;
	}

	_fd = fopen([path UTF8String], "r");

	return (_fd != NULL);
}

- (BOOL)seekable {
	return YES;
}

- (BOOL)seek:(long)position whence:(int)whence {
	if(fex) {
		switch(whence) {
			case SEEK_CUR:
				position += offset;
				break;

			case SEEK_END:
				position += size;
				break;
		}

		offset = position;

		return (position >= 0) && (position < size);
	}

	if(_fd == NULL) {
		offset = 0;
		size = 0;
		return NO;
	}

	return (fseeko(_fd, position, whence) == 0);
}

- (long)tell {
	if(fex)
		return offset;
	else
		return ftello(_fd);
}

- (long)read:(void *)buffer amount:(long)amount {
	if(fex) {
		if(amount + offset > size)
			amount = size - offset;
		memcpy(buffer, (const char *)data + offset, amount);
		offset += amount;
		return amount;
	} else {
		return fread(buffer, 1, amount, _fd);
	}
}

- (void)close {
	if(_fd) {
		fclose(_fd);
		_fd = NULL;
	}

	if(fex) {
		fex_close(fex);
		fex = NULL;
	}

	if(sbHandle) {
		id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
		id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];

		[sandboxBroker endFolderAccess:sbHandle];
		sbHandle = NULL;
	}
}

- (NSURL *)url {
	return _url;
}

- (NSString *)mimeType {
	return nil;
}

- (void)setURL:(NSURL *)url {
	_url = url;
}

+ (NSArray *)schemes {
	return @[@"file"];
}

- (void)dealloc {
	[self close];
}

@end
