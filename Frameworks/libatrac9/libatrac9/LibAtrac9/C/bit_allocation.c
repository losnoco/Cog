#include "bit_allocation.h"
#include "tables.h"
#include "utility.h"
#include <string.h>

at9_status CreateGradient(block* block)
{
	int valueCount = block->GradientEndValue - block->GradientStartValue;
	int unitCount = block->GradientEndUnit - block->GradientStartUnit;

	for (int i = 0; i < block->GradientEndUnit; i++)
	{
		block->Gradient[i] = block->GradientStartValue;
	}

	for (int i = block->GradientEndUnit; i <= block->QuantizationUnitCount; i++)
	{
		block->Gradient[i] = block->GradientEndValue;
	}
	if (unitCount <= 0) return ERR_SUCCESS;
	if (valueCount == 0) return ERR_SUCCESS;

	const unsigned char* curve = GradientCurves[unitCount - 1];
	if (valueCount <= 0)
	{
		double scale = (-valueCount - 1) / 31.0;
		int baseVal = block->GradientStartValue - 1;
		for (int i = block->GradientStartUnit; i < block->GradientEndUnit; i++)
		{
			block->Gradient[i] = baseVal - (int)(curve[i - block->GradientStartUnit] * scale);
		}
	}
	else
	{
		double scale = (valueCount - 1) / 31.0;
		int baseVal = block->GradientStartValue + 1;
		for (int i = block->GradientStartUnit; i < block->GradientEndUnit; i++)
		{
			block->Gradient[i] = baseVal + (int)(curve[i - block->GradientStartUnit] * scale);
		}
	}

	return ERR_SUCCESS;
}

void CalculateMask(channel* channel)
{
	memset(channel->PrecisionMask, 0, sizeof(channel->PrecisionMask));
	for (int i = 1; i < channel->Block->QuantizationUnitCount; i++)
	{
		const int delta = channel->ScaleFactors[i] - channel->ScaleFactors[i - 1];
		if (delta > 1)
		{
			channel->PrecisionMask[i] += min(delta - 1, 5);
		}
		else if (delta < -1)
		{
			channel->PrecisionMask[i - 1] += min(delta * -1 - 1, 5);
		}
	}
}

void CalculatePrecisions(channel* channel)
{
	block* block = channel->Block;

	if (block->GradientMode != 0)
	{
		for (int i = 0; i < block->QuantizationUnitCount; i++)
		{
			channel->Precisions[i] = channel->ScaleFactors[i] + channel->PrecisionMask[i] - block->Gradient[i];
			if (channel->Precisions[i] > 0)
			{
				switch (block->GradientMode)
				{
				case 1:
					channel->Precisions[i] /= 2;
					break;
				case 2:
					channel->Precisions[i] = 3 * channel->Precisions[i] / 8;
					break;
				case 3:
					channel->Precisions[i] /= 4;
					break;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < block->QuantizationUnitCount; i++)
		{
			channel->Precisions[i] = channel->ScaleFactors[i] - block->Gradient[i];
		}
	}

	for (int i = 0; i < block->QuantizationUnitCount; i++)
	{
		if (channel->Precisions[i] < 1)
		{
			channel->Precisions[i] = 1;
		}
	}

	for (int i = 0; i < block->GradientBoundary; i++)
	{
		channel->Precisions[i]++;
	}

	for (int i = 0; i < block->QuantizationUnitCount; i++)
	{
		channel->PrecisionsFine[i] = 0;
		if (channel->Precisions[i] > 15)
		{
			channel->PrecisionsFine[i] = channel->Precisions[i] - 15;
			channel->Precisions[i] = 15;
		}
	}
}