#pragma once
#include "bit_reader.h"
#include "error_codes.h"
#include "structures.h"

at9_status UnpackFrame(frame* frame, bit_reader_cxt* br);
at9_status UnpackBlock(block* block, bit_reader_cxt* br);
at9_status ReadBlockHeader(block* block, bit_reader_cxt* br);
at9_status UnpackStandardBlock(block* block, bit_reader_cxt* br);
at9_status ReadBandParams(block* block, bit_reader_cxt* br);
at9_status ReadGradientParams(block* block, bit_reader_cxt* br);
at9_status ReadStereoParams(block* block, bit_reader_cxt* br);
at9_status ReadExtensionParams(block* block, bit_reader_cxt* br);
void UpdateCodedUnits(channel* channel);
void CalculateSpectrumCodebookIndex(channel* channel);

at9_status ReadSpectra(channel* channel, bit_reader_cxt* br);
at9_status ReadSpectraFine(channel* channel, bit_reader_cxt* br);

at9_status UnpackLfeBlock(block* block, bit_reader_cxt* br);
void DecodeLfeScaleFactors(channel* channel, bit_reader_cxt* br);
void CalculateLfePrecision(channel* channel);
void ReadLfeSpectra(channel* channel, bit_reader_cxt* br);
