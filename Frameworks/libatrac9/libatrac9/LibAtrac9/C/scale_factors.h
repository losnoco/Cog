#pragma once
#include "bit_allocation.h"

at9_status read_scale_factors(channel* channel, bit_reader_cxt* br);
void ReadClcOffset(channel* channel, bit_reader_cxt* br);
void ReadVlcDeltaOffset(channel* channel, bit_reader_cxt* br);
void ReadVlcDistanceToBaseline(channel* channel, bit_reader_cxt* br, int* baseline, int baselineLength);
void ReadVlcDeltaOffsetWithBaseline(channel* channel, bit_reader_cxt* br, int* baseline, int baselineLength);
