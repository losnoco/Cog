/*
 *  NSDebug.c
 *  Cog
 *
 *  Created by Vincent Spader on 5/30/05.
 *  Copyright 2005 Vincent Spader All rights reserved.
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
