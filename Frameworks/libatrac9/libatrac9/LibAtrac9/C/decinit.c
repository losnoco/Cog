#include "bit_reader.h"
#include "decinit.h"
#include "error_codes.h"
#include "structures.h"
#include "tables.h"
#include <string.h>
#include <math.h>
#include "utility.h"

static int BlockTypeToChannelCount(BlockType block_type);

at9_status init_decoder(atrac9_handle* handle, unsigned char* config_data, int wlength)
{
	ERROR_CHECK(init_config_data(&handle->config, config_data));
	ERROR_CHECK(init_frame(handle));
	init_mdct_tables(handle->config.FrameSamplesPower);
	init_huffman_codebooks();
	handle->wlength = wlength;
	handle->initialized = 1;
	return ERR_SUCCESS;
}

at9_status init_config_data(ConfigData* config, unsigned char* config_data)
{
	memcpy(config->ConfigData, config_data, CONFIG_DATA_SIZE);
	ERROR_CHECK(read_config_data(config));

	config->FramesPerSuperframe = 1 << config->SuperframeIndex;
	config->SuperframeBytes = config->FrameBytes << config->SuperframeIndex;

	config->ChannelConfig = ChannelConfigs[config->ChannelConfigIndex];
	config->ChannelCount = config->ChannelConfig.ChannelCount;
	config->SampleRate = SampleRates[config->SampleRateIndex];
	config->HighSampleRate = config->SampleRateIndex > 7;
	config->FrameSamplesPower = SamplingRateIndexToFrameSamplesPower[config->SampleRateIndex];
	config->FrameSamples = 1 << config->FrameSamplesPower;
	config->SuperframeSamples = config->FrameSamples * config->FramesPerSuperframe;

	return ERR_SUCCESS;
}

at9_status read_config_data(ConfigData* config)
{
	bit_reader_cxt br;
	init_bit_reader_cxt(&br, &config->ConfigData);

	const int header = read_int(&br, 8);
	config->SampleRateIndex = read_int(&br, 4);
	config->ChannelConfigIndex = read_int(&br, 3);
	const int validation_bit = read_int(&br, 1);
	config->FrameBytes = read_int(&br, 11) + 1;
	config->SuperframeIndex = read_int(&br, 2);

	if (header != 0xFE || validation_bit != 0)
	{
		return ERR_BAD_CONFIG_DATA;
	}

	return ERR_SUCCESS;
}

at9_status init_frame(atrac9_handle* handle)
{
	const int block_count = handle->config.ChannelConfig.BlockCount;
	handle->frame.config = &handle->config;

	for (int i = 0; i < block_count; i++)
	{
		ERROR_CHECK(init_block(&handle->frame.Blocks[i], &handle->frame, i));
	}

	return ERR_SUCCESS;
}

at9_status init_block(block* block, frame* parent_frame, int block_index)
{
	block->Frame = parent_frame;
	block->BlockIndex = block_index;
	block->config = parent_frame->config;
	block->BlockType = block->config->ChannelConfig.Types[block_index];
	block->ChannelCount = BlockTypeToChannelCount(block->BlockType);

	for (int i = 0; i < block->ChannelCount; i++)
	{
		ERROR_CHECK(init_channel(&block->Channels[i], block, i));
	}

	return ERR_SUCCESS;
}

at9_status init_channel(channel* channel, block* parent_block, int channel_index)
{
	channel->Block = parent_block;
	channel->Frame = parent_block->Frame;
	channel->config = parent_block->config;
	channel->ChannelIndex = channel_index;
	channel->mdct.bits = parent_block->config->FrameSamplesPower;
	return ERR_SUCCESS;
}

void init_huffman_codebooks()
{
	init_huffman_set(HuffmanScaleFactorsUnsigned, sizeof(HuffmanScaleFactorsUnsigned) / sizeof(HuffmanCodebook));
	init_huffman_set(HuffmanScaleFactorsSigned, sizeof(HuffmanScaleFactorsSigned) / sizeof(HuffmanCodebook));
	init_huffman_set(HuffmanSpectrum, sizeof(HuffmanSpectrum) / sizeof(HuffmanCodebook));
}

void init_huffman_set(const HuffmanCodebook* codebooks, int count)
{
	for (int i = 0; i < count; i++)
	{
		InitHuffmanCodebook(&codebooks[i]);
	}
}

void init_mdct_tables(int frameSizePower)
{
	for (int i = 0; i < 9; i++)
	{
		GenerateTrigTables(i);
		GenerateShuffleTable(i);
	}
	GenerateMdctWindow(frameSizePower);
	GenerateImdctWindow(frameSizePower);
}

void GenerateTrigTables(int sizeBits)
{
	const int size = 1 << sizeBits;
	double* sinTab = SinTables[sizeBits];
	double* cosTab = CosTables[sizeBits];

	for (int i = 0; i < size; i++)
	{
		const double value = M_PI * (4 * i + 1) / (4 * size);
		sinTab[i] = sin(value);
		cosTab[i] = cos(value);
	}
}

void GenerateShuffleTable(int sizeBits)
{
	const int size = 1 << sizeBits;
	int* table = ShuffleTables[sizeBits];

	for (int i = 0; i < size; i++)
	{
		table[i] = BitReverse32(i ^ (i / 2), sizeBits);
	}
}

void GenerateMdctWindow(int frameSizePower)
{
	const int frameSize = 1 << frameSizePower;
	double* mdct = MdctWindow[frameSizePower - 6];

	for (int i = 0; i < frameSize; i++)
	{
		mdct[i] = (sin(((i + 0.5) / frameSize - 0.5) * M_PI) + 1.0) * 0.5;
	}
}

void GenerateImdctWindow(int frameSizePower)
{
	const int frameSize = 1 << frameSizePower;
	double* imdct = ImdctWindow[frameSizePower - 6];
	double* mdct = MdctWindow[frameSizePower - 6];

	for (int i = 0; i < frameSize; i++)
	{
		imdct[i] = mdct[i] / (mdct[frameSize - 1 - i] * mdct[frameSize - 1 - i] + mdct[i] * mdct[i]);
	}
}

static int BlockTypeToChannelCount(BlockType block_type)
{
	switch (block_type)
	{
	case Mono:
		return 1;
	case Stereo:
		return 2;
	case LFE:
		return 1;
	default:
		return 0;
	}
}
