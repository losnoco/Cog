#include "imdct.h"
#include "tables.h"

void RunImdct(mdct* mdct, double* input, double* output)
{
	const int size = 1 << mdct->bits;
	const int half = size / 2;
	double dctOut[MAX_FRAME_SAMPLES];
	const double* window = ImdctWindow[mdct->bits - 6];
	double* previous = mdct->_imdctPrevious;

	Dct4(mdct, input, dctOut);

	for (int i = 0; i < half; i++)
	{
		output[i] = window[i] * dctOut[i + half] + previous[i];
		output[i + half] = window[i + half] * -dctOut[size - 1 - i] - previous[i + half];
		previous[i] = window[size - 1 - i] * -dctOut[half - i - 1];
		previous[i + half] = window[half - i - 1] * dctOut[i];
	}
}

void Dct4(mdct* mdct, double* input, double* output)
{
	int MdctBits = mdct->bits;
	int MdctSize = 1 << MdctBits;
	const int* shuffleTable = ShuffleTables[MdctBits];
	const double* sinTable = SinTables[MdctBits];
	const double* cosTable = CosTables[MdctBits];
	double dctTemp[MAX_FRAME_SAMPLES];

	int size = MdctSize;
	int lastIndex = size - 1;
	int halfSize = size / 2;

	for (int i = 0; i < halfSize; i++)
	{
		int i2 = i * 2;
		double a = input[i2];
		double b = input[lastIndex - i2];
		double sin = sinTable[i];
		double cos = cosTable[i];
		dctTemp[i2] = a * cos + b * sin;
		dctTemp[i2 + 1] = a * sin - b * cos;
	}
	int stageCount = MdctBits - 1;

	for (int stage = 0; stage < stageCount; stage++)
	{
		int blockCount = 1 << stage;
		int blockSizeBits = stageCount - stage;
		int blockHalfSizeBits = blockSizeBits - 1;
		int blockSize = 1 << blockSizeBits;
		int blockHalfSize = 1 << blockHalfSizeBits;
		sinTable = SinTables[blockHalfSizeBits];
		cosTable = CosTables[blockHalfSizeBits];

		for (int block = 0; block < blockCount; block++)
		{
			for (int i = 0; i < blockHalfSize; i++)
			{
				int frontPos = (block * blockSize + i) * 2;
				int backPos = frontPos + blockSize;
				double a = dctTemp[frontPos] - dctTemp[backPos];
				double b = dctTemp[frontPos + 1] - dctTemp[backPos + 1];
				double sin = sinTable[i];
				double cos = cosTable[i];
				dctTemp[frontPos] += dctTemp[backPos];
				dctTemp[frontPos + 1] += dctTemp[backPos + 1];
				dctTemp[backPos] = a * cos + b * sin;
				dctTemp[backPos + 1] = a * sin - b * cos;
			}
		}
	}

	for (int i = 0; i < MdctSize; i++)
	{
		output[i] = dctTemp[shuffleTable[i]];
	}
}