#pragma once

#include "structures.h"

void DequantizeSpectra(block* block);
void DequantizeQuantUnit(channel* channel, int band);
void ScaleSpectrumBlock(block* block);
void ScaleSpectrumChannel(channel* channel);