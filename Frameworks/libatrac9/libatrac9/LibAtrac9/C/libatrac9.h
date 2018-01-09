#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define ATRAC9_CONFIG_DATA_SIZE 4

typedef struct {
	int channels;
	int channelConfigIndex;
	int samplingRate;
	int superframeSize;
	int framesInSuperframe;
	int frameSamples;
	int wlength;
	unsigned char configData[ATRAC9_CONFIG_DATA_SIZE];
    
    double MdctWindow[3][256];
    double ImdctWindow[3][256];
} Atrac9CodecInfo;

void* Atrac9GetHandle(void);
void Atrac9ReleaseHandle(void* handle);

int Atrac9InitDecoder(void* handle, unsigned char *pConfigData);
int Atrac9Decode(void* handle, const unsigned char *pAtrac9Buffer, short *pPcmBuffer, int *pNBytesUsed);

int Atrac9GetCodecInfo(void* handle, Atrac9CodecInfo *pCodecInfo);

#ifdef __cplusplus
}
#endif
