#pragma once

#include "structures.h"

void ApplyBandExtension(block* block);
void ApplyBandExtensionChannel(channel* channel);

void ScaleBexQuantUnits(double* spectra, double* scales, int startUnit, int totalUnits);
void FillHighFrequencies(double* spectra, int groupABin, int groupBBin, int groupCBin, int totalBins);
void AddNoiseToSpectrum(channel* channel, int index, int count);

void rng_init(rng_cxt* rng, unsigned short seed);
unsigned short rng_next(rng_cxt* rng);