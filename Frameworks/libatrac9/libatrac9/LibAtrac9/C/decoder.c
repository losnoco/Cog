#include <string.h>
#include "decoder.h"
#include "unpack.h"
#include "quantization.h"
#include "tables.h"
#include "imdct.h"
#include <math.h>
#include "utility.h"
#include "band_extension.h"

at9_status Decode(atrac9_handle* handle, const unsigned char* audio, unsigned char* pcm, int* bytesUsed)
{
	handle->frame.frameNum++;
	bit_reader_cxt br;
	init_bit_reader_cxt(&br, audio);
	ERROR_CHECK(DecodeFrame(&handle->frame, &br));

	PcmFloatToShort(&handle->frame, (short*)pcm);

	*bytesUsed = br.position / 8;
	return ERR_SUCCESS;
}

at9_status DecodeFrame(frame* frame, bit_reader_cxt* br)
{
	ERROR_CHECK(UnpackFrame(frame, br));

	for (int i = 0; i < frame->config->ChannelConfig.BlockCount; i++)
	{
		block* block = &frame->Blocks[i];

		DequantizeSpectra(block);
		ApplyIntensityStereo(block);
		ScaleSpectrumBlock(block);
		ApplyBandExtension(block);
		ImdctBlock(block);
	}

	return ERR_SUCCESS;
}

void PcmFloatToShort(frame* frame, short* pcmOut)
{
	const int endSample = frame->config->FrameSamples;
	short* dest = pcmOut;
	for (int d = 0, s = 0; s < endSample; d++, s++)
	{
		for (int i = 0; i < frame->config->ChannelConfig.BlockCount; i++)
		{
			block* block = &frame->Blocks[i];

			for (int j = 0; j < block->ChannelCount; j++)
			{
				channel* channel = &block->Channels[j];
				double* pcmSrc = channel->Pcm;

				const double sample = pcmSrc[d];
				const int roundedSample = (int)floor(sample + 0.5);
				*dest++ = Clamp16(roundedSample);
			}
		}
	}
}

void ImdctBlock(block* block)
{
	for (int i = 0; i < block->ChannelCount; i++)
	{
		channel* channel = &block->Channels[i];

		RunImdct(&channel->mdct, channel->Spectra, channel->Pcm);
	}
}

void ApplyIntensityStereo(block* block)
{
	if (block->BlockType != Stereo) return;

	const int totalUnits = block->QuantizationUnitCount;
	const int stereoUnits = block->StereoQuantizationUnit;
	if (stereoUnits >= totalUnits) return;

	channel* source = &block->Channels[block->PrimaryChannelIndex == 0 ? 0 : 1];
	channel* dest = &block->Channels[block->PrimaryChannelIndex == 0 ? 1 : 0];

	for (int i = stereoUnits; i < totalUnits; i++)
	{
		const int sign = block->JointStereoSigns[i];
		for (int sb = QuantUnitToCoeffIndex[i]; sb < QuantUnitToCoeffIndex[i + 1]; sb++)
		{
			if (sign > 0)
			{
				dest->Spectra[sb] = -source->Spectra[sb];
			}
			else
			{
				dest->Spectra[sb] = source->Spectra[sb];
			}
		}
	}
}

int GetCodecInfo(atrac9_handle* handle, CodecInfo * pCodecInfo)
{
	pCodecInfo->channels = handle->config.ChannelCount;
	pCodecInfo->channelConfigIndex = handle->config.ChannelConfigIndex;
	pCodecInfo->samplingRate = handle->config.SampleRate;
	pCodecInfo->superframeSize = handle->config.SuperframeBytes;
	pCodecInfo->framesInSuperframe = handle->config.FramesPerSuperframe;
	pCodecInfo->frameSamples = handle->config.FrameSamples;
	pCodecInfo->wlength = handle->wlength;
	memcpy(pCodecInfo->configData, handle->config.ConfigData, CONFIG_DATA_SIZE);
	return ERR_SUCCESS;
}
