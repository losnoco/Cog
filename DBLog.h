/*
 *  NSDebug.h
 *  Cog
 *
 *  Created by Zaphod Beeblebrox on 5/30/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include <Cocoa/Cocoa.h>

#ifdef __cplusplus
extern "C"
{
#endif
	
	void DBLog(NSString *format, ...);
	
#ifdef __cplusplus
};
#endif