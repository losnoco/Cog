#ifndef _CPU_HLE_AUDIOLIB_
#define _CPU_HLE_AUDIOLIB_



#include "cpu_hle.h"
#include "os.h"

// a few of these structures/type were sequestered from SGI\Nindendo's code

typedef struct ALLink_s {
    uint32_t            next;
    uint32_t            prev;
} ALLink;

typedef struct {
    ALLink      freeList;
    ALLink      allocList;
    int32_t     eventCount;
} ALEventQueue;


typedef struct {
	uint16_t type;
	uint8_t msg[12];
} ALEvent;


typedef struct {
    ALLink      node;
    int32_t     delta; //microtime
    ALEvent     event;
} ALEventListItem;

int alCopy(usf_state_t *, int paddr);
int alLink(usf_state_t *, int paddr);
int alUnLink(usf_state_t *, int paddr);
int alEvtqPostEvent(usf_state_t *, int paddr) ;
int alEvtqPostEvent_Alt(usf_state_t *, int paddr);
int alAudioFrame(usf_state_t *, int paddr);

// need to remove these

typedef struct {
    uint8_t *base;
    uint8_t *cur;
    int32_t len;
    int32_t count;
} ALHeap;

typedef struct ALPlayer_s {
    struct ALPlayer_s *next;
    void *clientData;
    void *handler;
    int32_t callTime;
    int32_t samplesLeft;
} ALPlayer;


#endif
