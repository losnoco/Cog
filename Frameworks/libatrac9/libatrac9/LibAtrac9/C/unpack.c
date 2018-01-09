#include "tables.h"
#include "unpack.h"
#include "bit_allocation.h"
#include <string.h>
#include "scale_factors.h"
#include "utility.h"
#include "huffCodes.h"

at9_status UnpackFrame(frame* frame, bit_reader_cxt* br)
{
	const int block_count = frame->config->ChannelConfig.BlockCount;

	for (int i = 0; i < block_count; i++)
	{
		ERROR_CHECK(UnpackBlock(&frame->Blocks[i], br));
	}
	return ERR_SUCCESS;
}

at9_status UnpackBlock(block* block, bit_reader_cxt* br)
{
	ERROR_CHECK(ReadBlockHeader(block, br));

	if (block->BlockType == LFE)
	{
		ERROR_CHECK(UnpackLfeBlock(block, br));
	}
	else
	{
		ERROR_CHECK(UnpackStandardBlock(block, br));
	}

	align_position(br, 8);
	return ERR_SUCCESS;
}

at9_status ReadBlockHeader(block* block, bit_reader_cxt* br)
{
	int firstInSuperframe = block->Frame->FrameIndex == 0;
	block->FirstInSuperframe = !read_int(br, 1);
	block->ReuseBandParams = read_int(br, 1);

	if (block->FirstInSuperframe && block->ReuseBandParams && block->BlockType != LFE)
	{
		return ERR_UNPACK_REUSE_BAND_PARAMS_INVALID;
	}

	return ERR_SUCCESS;
}

at9_status UnpackStandardBlock(block* block, bit_reader_cxt* br)
{
	if (!block->ReuseBandParams)
	{
		ERROR_CHECK(ReadBandParams(block, br));
	}

	ERROR_CHECK(ReadGradientParams(block, br));
	ERROR_CHECK(CreateGradient(block));
	ERROR_CHECK(ReadStereoParams(block, br));
	ERROR_CHECK(ReadExtensionParams(block, br));

	for (int i = 0; i < block->ChannelCount; i++)
	{
		channel* channel = &block->Channels[i];
		UpdateCodedUnits(channel);

		ERROR_CHECK(read_scale_factors(channel, br));
		CalculateMask(channel);
		CalculatePrecisions(channel);
		CalculateSpectrumCodebookIndex(channel);

		ERROR_CHECK(ReadSpectra(channel, br));
		ERROR_CHECK(ReadSpectraFine(channel, br));
	}

	block->QuantizationUnitsPrev = block->BandExtensionEnabled ? block->ExtensionUnit : block->QuantizationUnitCount;
	return ERR_SUCCESS;
}

at9_status ReadBandParams(block* block, bit_reader_cxt* br)
{
	const int minBandCount = MinBandCount[block->config->HighSampleRate];
	const int maxExtensionBand = MaxExtensionBand[block->config->HighSampleRate];
	block->BandCount = read_int(br, 4) + minBandCount;
	block->QuantizationUnitCount = BandToQuantUnitCount[block->BandCount];

	if (block->BandCount < minBandCount || block->BandCount >
		MaxBandCount[block->config->SampleRateIndex])
	{
		return ERR_SUCCESS;
	}

	if (block->BlockType == Stereo)
	{
		block->StereoBand = read_int(br, 4);
		block->StereoBand += minBandCount;
		block->StereoQuantizationUnit = BandToQuantUnitCount[block->StereoBand];
	}
	else
	{
		block->StereoBand = block->BandCount;
	}

	block->BandExtensionEnabled = read_int(br, 1);
	if (block->BandExtensionEnabled)
	{
		block->ExtensionBand = read_int(br, 4);
		block->ExtensionBand += minBandCount;

		if (block->ExtensionBand < block->BandCount || block->ExtensionBand > maxExtensionBand)
		{
			return ERR_UNPACK_BAND_PARAMS_INVALID;
		}

		block->ExtensionUnit = BandToQuantUnitCount[block->ExtensionBand];
	}
	else
	{
		block->ExtensionBand = block->BandCount;
		block->ExtensionUnit = block->QuantizationUnitCount;
	}

	return ERR_SUCCESS;
}

at9_status ReadGradientParams(block* block, bit_reader_cxt* br)
{
	block->GradientMode = read_int(br, 2);
	if (block->GradientMode > 0)
	{
		block->GradientEndUnit = 31;
		block->GradientEndValue = 31;
		block->GradientStartUnit = read_int(br, 5);
		block->GradientStartValue = read_int(br, 5);
	}
	else
	{
		block->GradientStartUnit = read_int(br, 6);
		block->GradientEndUnit = read_int(br, 6) + 1;
		block->GradientStartValue = read_int(br, 5);
		block->GradientEndValue = read_int(br, 5);
	}
	block->GradientBoundary = read_int(br, 4);

	if (block->GradientBoundary > block->QuantizationUnitCount)
	{
		return ERR_UNPACK_GRAD_BOUNDARY_INVALID;
	}
	if (block->GradientStartUnit < 1 || block->GradientStartUnit >= 48)
	{
		return ERR_UNPACK_GRAD_START_UNIT_OOB;
	}
	if (block->GradientEndUnit < 1 || block->GradientEndUnit >= 48)
	{
		return ERR_UNPACK_GRAD_END_UNIT_OOB;
	}
	if (block->GradientStartUnit > block->GradientEndUnit)
	{
		return ERR_UNPACK_GRAD_END_UNIT_INVALID;
	}
	if (block->GradientStartValue < 0 || block->GradientStartValue >= 32)
	{
		return ERR_UNPACK_GRAD_START_VALUE_OOB;
	}
	if (block->GradientEndValue < 0 || block->GradientEndValue >= 32)
	{
		return ERR_UNPACK_GRAD_END_VALUE_OOB;
	}

	return ERR_SUCCESS;
}

at9_status ReadStereoParams(block* block, bit_reader_cxt* br)
{
	if (block->BlockType != Stereo) return ERR_SUCCESS;

	block->PrimaryChannelIndex = read_int(br, 1);
	block->HasJointStereoSigns = read_int(br, 1);
	if (block->HasJointStereoSigns)
	{
		for (int i = block->StereoQuantizationUnit; i < block->QuantizationUnitCount; i++)
		{
			block->JointStereoSigns[i] = read_int(br, 1);
		}
	}
	else
	{
		memset(block->JointStereoSigns, 0, sizeof(block->JointStereoSigns));
	}

	return ERR_SUCCESS;
}

void BexReadHeader(channel* channel, bit_reader_cxt* br, int bexBand)
{
	const int bexMode = read_int(br, 2);
	channel->BexMode = bexBand > 2 ? bexMode : 4;
	channel->BexValueCount = BexEncodedValueCounts[channel->BexMode][bexBand];
}

void BexReadData(channel* channel, bit_reader_cxt* br, int bexBand)
{
	for (int i = 0; i < channel->BexValueCount; i++)
	{
		const int dataLength = BexDataLengths[channel->BexMode][bexBand][i];
		channel->BexValues[i] = read_int(br, dataLength);
	}
}

at9_status ReadExtensionParams(block* block, bit_reader_cxt* br)
{
	int bexBand = 0;
	if (block->BandExtensionEnabled)
	{
		bexBand = BexGroupInfo[block->QuantizationUnitCount - 13].band_count;
		if (block->BlockType == Stereo)
		{
			BexReadHeader(&block->Channels[1], br, bexBand);
		}
		else
		{
			br->position += 1;
		}
	}
	block->HasExtensionData = read_int(br, 1);

	if (!block->HasExtensionData) return ERR_SUCCESS;
	if (!block->BandExtensionEnabled)
	{
		block->BexMode = read_int(br, 2);
		block->BexDataLength = read_int(br, 5);
		br->position += block->BexDataLength;
		return ERR_SUCCESS;
	}

	BexReadHeader(&block->Channels[0], br, bexBand);

	block->BexDataLength = read_int(br, 5);
	if (block->BexDataLength <= 0) return ERR_SUCCESS;
	const int bexDataEnd = br->position + block->BexDataLength;

	BexReadData(&block->Channels[0], br, bexBand);

	if (block->BlockType == Stereo)
	{
		BexReadData(&block->Channels[1], br, bexBand);
	}

	// Make sure we didn't read too many bits
	if (br->position > bexDataEnd)
	{
		return ERR_UNPACK_EXTENSION_DATA_INVALID;
	}

	return ERR_SUCCESS;
}

void UpdateCodedUnits(channel* channel)
{
	if (channel->Block->PrimaryChannelIndex == channel->ChannelIndex)
	{
		channel->CodedQuantUnits = channel->Block->QuantizationUnitCount;
	}
	else
	{
		channel->CodedQuantUnits = channel->Block->StereoQuantizationUnit;
	}
}

void CalculateSpectrumCodebookIndex(channel* channel)
{
	memset(channel->CodebookSet, 0, sizeof(channel->CodebookSet));
	const int quantUnits = channel->CodedQuantUnits;
	int* sf = channel->ScaleFactors;

	if (quantUnits <= 1) return;
	if (channel->config->HighSampleRate) return;

	// Temporarily setting this value allows for simpler code by
	// making the last value a non-special case.
	const int originalScaleTmp = sf[quantUnits];
	sf[quantUnits] = sf[quantUnits - 1];

	int avg = 0;
	if (quantUnits > 12)
	{
		for (int i = 0; i < 12; i++)
		{
			avg += sf[i];
		}
		avg = (avg + 6) / 12;
	}

	for (int i = 8; i < quantUnits; i++)
	{
		const int prevSf = sf[i - 1];
		const int nextSf = sf[i + 1];
		const int minSf = min(prevSf, nextSf);
		if (sf[i] - minSf >= 3 || sf[i] - prevSf + sf[i] - nextSf >= 3)
		{
			channel->CodebookSet[i] = 1;
		}
	}

	for (int i = 12; i < quantUnits; i++)
	{
		if (channel->CodebookSet[i] == 0)
		{
			const int minSf = min(sf[i - 1], sf[i + 1]);
			if (sf[i] - minSf >= 2 && sf[i] >= avg - (QuantUnitToCoeffCount[i] == 16 ? 1 : 0))
			{
				channel->CodebookSet[i] = 1;
			}
		}
	}

	sf[quantUnits] = originalScaleTmp;
}

at9_status ReadSpectra(channel* channel, bit_reader_cxt* br)
{
	int values[16];
	memset(channel->QuantizedSpectra, 0, sizeof(channel->QuantizedSpectra));
	const int maxHuffPrecision = MaxHuffPrecision[channel->config->HighSampleRate];

	for (int i = 0; i < channel->CodedQuantUnits; i++)
	{
		const int subbandCount = QuantUnitToCoeffCount[i];
		const int precision = channel->Precisions[i] + 1;
		if (precision <= maxHuffPrecision)
		{
			const HuffmanCodebook* huff = &HuffmanSpectrum[channel->CodebookSet[i]][precision][QuantUnitToCodebookIndex[i]];
			const int groupCount = subbandCount >> huff->ValueCountPower;
			for (int j = 0; j < groupCount; j++)
			{
				values[j] = ReadHuffmanValue(huff, br, FALSE);
			}

			DecodeHuffmanValues(channel->QuantizedSpectra, QuantUnitToCoeffIndex[i], subbandCount, huff, values);
		}
		else
		{
			const int subbandIndex = QuantUnitToCoeffIndex[i];
			for (int j = subbandIndex; j < QuantUnitToCoeffIndex[i + 1]; j++)
			{
				channel->QuantizedSpectra[j] = read_signed_int(br, precision);
			}
		}
	}

	return ERR_SUCCESS;
}

at9_status ReadSpectraFine(channel* channel, bit_reader_cxt* br)
{
	memset(channel->QuantizedSpectraFine, 0, sizeof(channel->QuantizedSpectraFine));

	for (int i = 0; i < channel->CodedQuantUnits; i++)
	{
		if (channel->PrecisionsFine[i] > 0)
		{
			const int overflowBits = channel->PrecisionsFine[i] + 1;
			const int startSubband = QuantUnitToCoeffIndex[i];
			const int endSubband = QuantUnitToCoeffIndex[i + 1];

			for (int j = startSubband; j < endSubband; j++)
			{
				channel->QuantizedSpectraFine[j] = read_signed_int(br, overflowBits);
			}
		}
	}
	return ERR_SUCCESS;
}

at9_status UnpackLfeBlock(block* block, bit_reader_cxt* br)
{
	channel* channel = &block->Channels[0];
	block->QuantizationUnitCount = 2;

	DecodeLfeScaleFactors(channel, br);
	CalculateLfePrecision(channel);
	channel->CodedQuantUnits = block->QuantizationUnitCount;
	ReadLfeSpectra(channel, br);

	return ERR_SUCCESS;
}

void DecodeLfeScaleFactors(channel* channel, bit_reader_cxt* br)
{
	memset(channel->ScaleFactors, 0, sizeof(channel->ScaleFactors));
	for (int i = 0; i < channel->Block->QuantizationUnitCount; i++)
	{
		channel->ScaleFactors[i] = read_int(br, 5);
	}
}

void CalculateLfePrecision(channel* channel)
{
	block* block = channel->Block;
	const int precision = block->ReuseBandParams ? 8 : 4;
	for (int i = 0; i < block->QuantizationUnitCount; i++)
	{
		channel->Precisions[i] = precision;
		channel->PrecisionsFine[i] = 0;
	}
}

void ReadLfeSpectra(channel* channel, bit_reader_cxt* br)
{
	memset(channel->QuantizedSpectra, 0, sizeof(channel->QuantizedSpectra));

	for (int i = 0; i < channel->CodedQuantUnits; i++)
	{
		if (channel->Precisions[i] <= 0) continue;

		const int precision = channel->Precisions[i] + 1;
		for (int j = QuantUnitToCoeffIndex[i]; j < QuantUnitToCoeffIndex[i + 1]; j++)
		{
			channel->QuantizedSpectra[j] = read_signed_int(br, precision);
		}
	}
}
