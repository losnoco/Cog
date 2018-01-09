#include "band_extension.h"
#include "tables.h"
#include "utility.h"
#include <math.h>

void ApplyBandExtension(block* block)
{
	if (!block->BandExtensionEnabled || !block->HasExtensionData) return;

	for (int i = 0; i < block->ChannelCount; i++)
	{
		ApplyBandExtensionChannel(&block->Channels[i]);
	}
}

void ApplyBandExtensionChannel(channel* channel)
{
	const int groupAUnit = channel->Block->QuantizationUnitCount;
	int* scaleFactors = channel->ScaleFactors;
	double* spectra = channel->Spectra;
	double scales[6];
	int* values = channel->BexValues;

	const bex_group* bex_info = &BexGroupInfo[channel->Block->QuantizationUnitCount - 13];
	const int bandCount = bex_info->band_count;
	const int groupBUnit = bex_info->group_b_unit;
	const int groupCUnit = bex_info->group_c_unit;

	const int totalUnits = max(groupCUnit, 22);
	const int bexQuantUnits = totalUnits - groupAUnit;

	const int groupABin = QuantUnitToCoeffIndex[groupAUnit];
	const int groupBBin = QuantUnitToCoeffIndex[groupBUnit];
	const int groupCBin = QuantUnitToCoeffIndex[groupCUnit];
	const int totalBins = QuantUnitToCoeffIndex[totalUnits];

	FillHighFrequencies(spectra, groupABin, groupBBin, groupCBin, totalBins);

	double groupAScale, groupBScale, groupCScale;
	double rate, scale, mult;

	switch (channel->BexMode)
	{
	case 0:
		switch (bandCount)
		{
		case 3:
			scales[0] = BexMode0Bands3[0][values[0]];
			scales[1] = BexMode0Bands3[1][values[0]];
			scales[2] = BexMode0Bands3[2][values[1]];
			scales[3] = BexMode0Bands3[3][values[2]];
			scales[4] = BexMode0Bands3[4][values[3]];
			break;
		case 4:
			scales[0] = BexMode0Bands4[0][values[0]];
			scales[1] = BexMode0Bands4[1][values[0]];
			scales[2] = BexMode0Bands4[2][values[1]];
			scales[3] = BexMode0Bands4[3][values[2]];
			scales[4] = BexMode0Bands4[4][values[3]];
			break;
		case 5:
			scales[0] = BexMode0Bands5[0][values[0]];
			scales[1] = BexMode0Bands5[1][values[1]];
			scales[2] = BexMode0Bands5[2][values[1]];
			break;
		}

		scales[bexQuantUnits - 1] = SpectrumScale[scaleFactors[groupAUnit]];

		AddNoiseToSpectrum(channel, QuantUnitToCoeffIndex[totalUnits - 1],
			QuantUnitToCoeffCount[totalUnits - 1]);
		ScaleBexQuantUnits(spectra, scales, groupAUnit, totalUnits);
		break;
	case 1:
		for (int i = groupAUnit; i < totalUnits; i++)
		{
			scales[i - groupAUnit] = SpectrumScale[scaleFactors[i]];
		}

		AddNoiseToSpectrum(channel, groupABin, totalBins - groupABin);
		ScaleBexQuantUnits(spectra, scales, groupAUnit, totalUnits);
		break;
	case 2:
		groupAScale = BexMode2Scale[values[0]];
		groupBScale = BexMode2Scale[values[1]];

		for (int i = groupABin; i < groupBBin; i++)
		{
			spectra[i] *= groupAScale;
		}

		for (int i = groupBBin; i < groupCBin; i++)
		{
			spectra[i] *= groupBScale;
		}
		return;
	case 3:
		rate = pow(2, BexMode3Rate[values[1]]);
		scale = BexMode3Initial[values[0]];
		for (int i = groupABin; i < totalBins; i++)
		{
			scale *= rate;
			spectra[i] *= scale;
		}
		return;
	case 4:
		mult = BexMode4Multiplier[values[0]];
		groupAScale = 0.7079468 * mult;
		groupBScale = 0.5011902 * mult;
		groupCScale = 0.3548279 * mult;

		for (int i = groupABin; i < groupBBin; i++)
		{
			spectra[i] *= groupAScale;
		}

		for (int i = groupBBin; i < groupCBin; i++)
		{
			spectra[i] *= groupBScale;
		}

		for (int i = groupCBin; i < totalBins; i++)
		{
			spectra[i] *= groupCScale;
		}
	}
}

void ScaleBexQuantUnits(double* spectra, double* scales, int startUnit, int totalUnits)
{
	for (int i = startUnit; i < totalUnits; i++)
	{
		for (int k = QuantUnitToCoeffIndex[i]; k < QuantUnitToCoeffIndex[i + 1]; k++)
		{
			spectra[k] *= scales[i - startUnit];
		}
	}
}

void FillHighFrequencies(double* spectra, int groupABin, int groupBBin, int groupCBin, int totalBins)
{
	for (int i = 0; i < groupBBin - groupABin; i++)
	{
		spectra[groupABin + i] = spectra[groupABin - i - 1];
	}

	for (int i = 0; i < groupCBin - groupBBin; i++)
	{
		spectra[groupBBin + i] = spectra[groupBBin - i - 1];
	}

	for (int i = 0; i < totalBins - groupCBin; i++)
	{
		spectra[groupCBin + i] = spectra[groupCBin - i - 1];
	}
}

void AddNoiseToSpectrum(channel* channel, int index, int count)
{
	if (!channel->rng.initialized)
	{
		int* sf = channel->ScaleFactors;
		const unsigned short seed = (unsigned short)(543 * (sf[8] + sf[12] + sf[15] + 1));
		rng_init(&channel->rng, seed);
	}
	for (int i = 0; i < count; i++)
	{
		channel->Spectra[i + index] = rng_next(&channel->rng) / 65535.0 * 2.0 - 1.0;
	}
}

void rng_init(rng_cxt* rng, unsigned short seed)
{
	const int startValue = 0x4D93 * (seed ^ (seed >> 14));

	rng->stateA = (unsigned short)(3 - startValue);
	rng->stateB = (unsigned short)(2 - startValue);
	rng->stateC = (unsigned short)(1 - startValue);
	rng->stateD = (unsigned short)(0 - startValue);
	rng->initialized = TRUE;
}

unsigned short rng_next(rng_cxt* rng)
{
	const unsigned short t = (unsigned short)(rng->stateD ^ (rng->stateD << 5));
	rng->stateD = rng->stateC;
	rng->stateC = rng->stateB;
	rng->stateB = rng->stateA;
	rng->stateA = (unsigned short)(t ^ rng->stateA ^ ((t ^ (rng->stateA >> 5)) >> 4));
	return rng->stateA;
}