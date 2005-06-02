/*
 *  NSDebug.c
 *  Cog
 *
 *  Created by Zaphod Beeblebrox on 5/30/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include "DBLog.h"

void DBLog(NSString *format, ...)
{
#ifdef DEBUG
	
	va_list ap;
	
	va_start(ap, format);

	NSLogv(format, ap);
	
	va_end(ap);
#endif
}
