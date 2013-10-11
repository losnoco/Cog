//
//  Logging.h
//  Cog
//
//  Shamelessly stolen from stackoverflow by Dmitry Promsky on 3/7/12.
//  Copyright 2012 dmitry.promsky@gmail.com. All rights reserved.
//

#ifdef DEBUG
#   define DLog(fmt, ...) NSLog((@"%s (line %d) " fmt), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#   define DLog(...)
#endif

#define ALog(fmt, ...) NSLog((@"%s (line %d) " fmt), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);

