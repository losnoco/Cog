#pragma once

#include "error_codes.h"
#include "structures.h"
#include "huffCodes.h"

at9_status init_decoder(atrac9_handle* handle, unsigned char * config_data, int wlength);
at9_status init_config_data(ConfigData* config, unsigned char * config_data);
at9_status read_config_data(ConfigData* config);
at9_status init_frame(atrac9_handle* handle);
at9_status init_block(block* block, frame* parent_frame, int block_index);
at9_status init_channel(channel* channel, block* parent_block, int channel_index);
void init_huffman_codebooks();
void init_huffman_set(const HuffmanCodebook* codebooks, int count);
void GenerateTrigTables(int sizeBits);
void GenerateShuffleTable(int sizeBits);
void init_mdct_tables(int frameSizePower);
void GenerateMdctWindow(int frameSizePower);
void GenerateImdctWindow(int frameSizePower);
