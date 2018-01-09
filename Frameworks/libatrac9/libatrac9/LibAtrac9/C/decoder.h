#pragma once
#include "bit_reader.h"
#include "error_codes.h"
#include "structures.h"

at9_status Decode(atrac9_handle* handle, const unsigned char* audio, unsigned char* pcm, int* bytesUsed);
at9_status DecodeFrame(frame* frame, bit_reader_cxt* br);
void ImdctBlock(block* block);
void ApplyIntensityStereo(block* block);
void PcmFloatToShort(frame* frame, short* pcmOut);
int GetCodecInfo(atrac9_handle* handle, CodecInfo* pCodecInfo);
