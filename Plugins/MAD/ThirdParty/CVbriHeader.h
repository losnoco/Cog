#ifndef _CVbriHeader_
#define _CVbriHeader_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct VbriHeader;

int readVbriHeader(struct VbriHeader **, const unsigned char *Hbuffer, size_t length);
void freeVbriHeader(struct VbriHeader *);

// int VbriSeekPointByTime(float EntryTimeInSeconds);
// float VbriSeekTimeByPoint(unsigned int EntryPointInBytes);
// int VbriSeekPointByPercent(float percent);
// float VbriTotalDuration();

int VbriTotalFrames(struct VbriHeader *);

#ifdef __cplusplus
}
#endif

#endif
