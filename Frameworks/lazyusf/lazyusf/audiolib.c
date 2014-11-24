#include "usf.h"
#include "usf_internal.h"

#include "cpu_hle.h"
#include "audiolib.h"
#include "os.h"

#include "main.h"
#include "memory.h"

#define N64WORD(x)		(*(uint32_t*)PageVRAM((x)))
#define N64HALF(x)		(*(uint16_t*)PageVRAM((x)))
#define N64BYTE(x)		(*(uint8_t*)PageVRAM((x)))


int alCopy(usf_state_t * state, int paddr) {
	uint32_t source = (state->GPR[4].UW[0]);
	uint32_t dest = (state->GPR[5].UW[0]);
	uint32_t len = (state->GPR[6].UW[0]);

	if(len&3)
		DisplayError(state, "OMG!!!! - alCopy length & 3\n");

	memcpyn642n64(state, dest, source, len);

	return 1;
}



int alLink(usf_state_t * state, int paddr) {
	ALLink *element = (ALLink*)PageVRAM(state->GPR[4].UW[0]);
	ALLink *after = (ALLink*)PageVRAM(state->GPR[5].UW[0]);
	ALLink *afterNext;

	element->next = after->next;
    element->prev = state->GPR[5].UW[0];

    if (after->next) {
    	afterNext = (ALLink*)PageVRAM(after->next);
        afterNext->prev = state->GPR[4].UW[0];
	}

    after->next = state->GPR[4].UW[0];
	return 1;
}


int alUnLink(usf_state_t * state, int paddr) {
	ALLink *element = (ALLink*)PageVRAM(state->GPR[4].UW[0]);
	ALLink *elementNext = (ALLink*)PageVRAM(element->next);
	ALLink *elementPrev = (ALLink*)PageVRAM(element->prev);
//	_asm int 3

	if (element->next)
        elementNext->prev = element->prev;
    if (element->prev)
        elementPrev->next = element->next;
	return 1;
}

int alEvtqPostEvent(usf_state_t * state, int paddr) {
	ALEventQueue *evtq;
	ALEvent *events;

	uint32_t A0 = state->GPR[4].UW[0];
	uint32_t A1 = state->GPR[5].UW[0];
	uint32_t DeltaTime = state->GPR[6].UW[0];

	uint32_t nodeNext = 0;
	uint32_t nextItem = 0;
	uint32_t node = 0;
	uint32_t nextDelta = 0;
	uint32_t item = 0;
	uint32_t postWhere = 0;
	uint32_t NEXT = 0;
	uint32_t nextItemDelta = 0;

	evtq = (ALEventQueue *)PageVRAM(A0);
	events = (ALEvent *)PageVRAM(A1);
//_asm int 3

	NEXT = evtq->freeList.next;

	if(NEXT == 0)
		return 1;

	//DisplayError("%08x", N64WORD(0x800533E4));
	//cprintf("%08x\t%08x\n", N64WORD(0x800533D4), N64WORD(0x800533D8));

	item = NEXT;
	state->GPR[4].UW[0] = NEXT;
	alUnLink(state, 0);

	state->GPR[4].UW[0] = A1; state->GPR[5].UW[0] = NEXT + 0xC; state->GPR[6].UW[0] = 0x10;
	alCopy(state, 0);

    postWhere = (DeltaTime==0x7FFFFFFF)?1:0;
    nodeNext = A0;
    node = nodeNext + 8;

	while(nodeNext !=0 ) {
		nodeNext = *(uint32_t*)PageVRAM(node);

		if(nodeNext != 0) {
			nextDelta = *(uint32_t*)PageVRAM(nodeNext + 8);
			nextItem = nodeNext;
			if(DeltaTime < nextDelta) {
				*(uint32_t*)PageVRAM(item + 8) = DeltaTime;
				nextItemDelta = *(uint32_t*)PageVRAM(nextItem + 8);
				*(uint32_t*)PageVRAM(nextItem + 8) = nextItemDelta - DeltaTime;

				state->GPR[4].UW[0] = item; state->GPR[5].UW[0] = node;
				alLink(state, 0);
				return 1;
			} else {
				node = nodeNext;
				DeltaTime -= nextDelta;
				if(node == 0)
					return 1;
			}
		}

	}

	if(postWhere == 0)
		*(uint32_t*)PageVRAM(item + 8) = DeltaTime;
	else
		*(uint32_t*)PageVRAM(item + 8) = 0;


	state->GPR[4].UW[0] = item; state->GPR[5].UW[0] = node;
	alLink(state, 0);
	return 1;
}

int alEvtqPostEvent_Alt(usf_state_t * state, int paddr) {
	return 0;
}

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

uint32_t __nextSampleTime(usf_state_t * state, uint32_t driver, uint32_t *client) {

	uint32_t c = 0;
	int32_t deltaTime = 0x7FFFFFFF;
	*client = 0;

	for(c = N64WORD(driver); c != 0; c = N64WORD(c)) {
		int samplesLeft = N64WORD(c + 0x10);
		int curSamples = N64WORD(driver + 0x20);
		if((samplesLeft - curSamples) < deltaTime) {
			*client = c;
			deltaTime = samplesLeft - curSamples;
		}
	}

	return N64WORD((*client)+0x10);
}

int32_t _timeToSamplesNoRound(usf_state_t * state, long synth, long micros)
{
    uint32_t outputRate = N64WORD(synth+0x44);
    float tmp = ((float)micros) * outputRate / 1000000.0 + 0.5;
    //DisplayError("Smaple rate is %d", outputRate);

    return (int32_t)tmp;
}

uint32_t byteswap(char b[4] ) {
	uint32_t out = 0;
	out += b[3];
	out += b[2] << 8;
	out += b[1] << 16;
	out += b[0] << 24;
	return out;
}

int alAudioFrame(usf_state_t * state, int paddr) {

	uint32_t alGlobals = 0;
	uint32_t driver = 0, *paramSamples, *curSamples, client = 0, dl = 0;
	uint32_t A0 = state->GPR[4].UW[0];
	uint32_t A1 = state->GPR[5].UW[0];
	uint32_t A2 = state->GPR[6].UW[0];
	uint32_t outLen = state->GPR[7].UW[0];
	uint32_t cmdlEnd = A0;
	uint32_t lOutBuf = A2;

	alGlobals = ((*(uint16_t*)PageRAM2(paddr + 0x8)) & 0xFFFF) << 16;		//alGlobals->drvr
	alGlobals += *(int16_t*)PageRAM2(paddr + 0xc);
	//alGlobals = 0x80750C74;
	driver = N64WORD(alGlobals);
	paramSamples = (uint32_t*) PageVRAM(driver + 0x1c);
	curSamples = (uint32_t*) PageVRAM(driver + 0x20);

	if(N64WORD(driver) == 0) {		// if(drvr->head == 0)
		N64WORD(A1) = 0;
		state->GPR[2].UW[0] = A0;
		return 1;
	}

	for(*paramSamples = __nextSampleTime(state, driver, &client); (*paramSamples - *curSamples) < outLen; *paramSamples = __nextSampleTime(state, driver, &client)) {
		int32_t *cSamplesLeft;
		cSamplesLeft = (int32_t *) PageVRAM(client + 0x10);
		*paramSamples &= ~0xf;

		//run handler (not-HLE'd)
		state->GPR[4].UW[0] = client;
		RunFunction(state, N64WORD(client+0x8));
    	*cSamplesLeft += _timeToSamplesNoRound(state, driver, state->GPR[2].UW[0]);
	}

	*paramSamples &= ~0xf;

	//give us some stack
	state->GPR[0x1d].UW[0] -= 0x20;
	N64WORD(state->GPR[0x1d].UW[0]+0x4) = 0; //tmp

	while (outLen > 0) {

		uint32_t maxOutSamples = 0, nOut = 0, cmdPtr = 0, output = 0, setParam = 0, handler = 0;

		maxOutSamples = N64WORD(driver + 0x48);
		nOut = MIN(maxOutSamples, outLen);
		cmdPtr = cmdlEnd;//+8;

		output = N64WORD(driver + 0x38);
		setParam = N64WORD(output+8);			 // alSaveParam

		state->GPR[4].DW = output;
		state->GPR[5].DW = 0x6; // AL_FILTER_SET_DRAM
		state->GPR[6].DW = lOutBuf;
		RunFunction(state, setParam);

		handler = N64WORD(output+4);			 // alSavePull
		state->GPR[4].DW = output;
		state->GPR[5].DW = state->GPR[0x1d].UW[0]+0x12; //&tmp
		state->GPR[6].DW = nOut;
		state->GPR[7].DW = *curSamples;
		N64WORD(state->GPR[0x1d].UW[0]+0x10) = cmdPtr;
		RunFunction(state, handler);

		curSamples = (uint32_t *) PageVRAM(driver + 0x20);

		cmdlEnd = state->GPR[2].UW[0];
		outLen -= nOut;
		lOutBuf += (nOut<<2);
		*curSamples += nOut;

	}

	state->GPR[0x1d].UW[0] += 0x20;

	N64WORD(A1) = (int32_t) ((cmdlEnd - A0) >> 3);

	state->GPR[4].UW[0] = driver;

	while( (dl = N64WORD(driver+0x14)) ) {
		state->GPR[4].UW[0] = dl;
		alUnLink(state, 0);
		state->GPR[4].UW[0] = dl;
		state->GPR[5].UW[0] = driver + 4;
		alLink(state, 0);
	}

	state->GPR[2].UW[0] = cmdlEnd;
	return 1;
}
