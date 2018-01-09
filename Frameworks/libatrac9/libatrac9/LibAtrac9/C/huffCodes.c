#include "huffCodes.h"
#include "utility.h"

int ReadHuffmanValue(const HuffmanCodebook* huff, bit_reader_cxt* br, int is_signed)
{
	const int code = peek_int(br, huff->MaxBitSize);
	const unsigned char value = huff->Lookup[code];
	const int bits = huff->Bits[value];
	br->position += bits;
	return is_signed ? SignExtend32(value, huff->ValueBits) : value;
}

void DecodeHuffmanValues(int* spectrum, int index, int bandCount, const HuffmanCodebook* huff, const int* values)
{
	const int valueCount = bandCount >> huff->ValueCountPower;
	const int mask = (1 << huff->ValueBits) - 1;

	for (int i = 0; i < valueCount; i++)
	{
		int value = values[i];
		for (int j = 0; j < huff->ValueCount; j++)
		{
			spectrum[index++] = SignExtend32(value & mask, huff->ValueBits);
			value >>= huff->ValueBits;
		}
	}
}

void InitHuffmanCodebook(const HuffmanCodebook* codebook)
{
	const int huffLength = codebook->Length;
	if (huffLength == 0) return;

	unsigned char* dest = codebook->Lookup;

	for (int i = 0; i < huffLength; i++)
	{
		if (codebook->Bits[i] == 0) continue;
		const int unusedBits = codebook->MaxBitSize - codebook->Bits[i];

		const int start = codebook->Codes[i] << unusedBits;
		const int length = 1 << unusedBits;
		const int end = start + length;

		for (int j = start; j < end; j++)
		{
			dest[j] = i;
		}
	}
}

HuffmanCodebook HuffmanScaleFactorsUnsigned[7] = {
	{0},
	{ScaleFactorsA1Bits, ScaleFactorsA1Codes, ScaleFactorsA1Lookup, 2, 1, 0, 1, 2, 1},
	{ScaleFactorsA2Bits, ScaleFactorsA2Codes, ScaleFactorsA2Lookup, 4, 1, 0, 2, 4, 3},
	{ScaleFactorsA3Bits, ScaleFactorsA3Codes, ScaleFactorsA3Lookup, 8, 1, 0, 3, 8, 6},
	{ScaleFactorsA4Bits, ScaleFactorsA4Codes, ScaleFactorsA4Lookup, 16, 1, 0, 4, 16, 8},
	{ScaleFactorsA5Bits, ScaleFactorsA5Codes, ScaleFactorsA5Lookup, 32, 1, 0, 5, 32, 8},
	{ScaleFactorsA6Bits, ScaleFactorsA6Codes, ScaleFactorsA6Lookup, 64, 1, 0, 6, 64, 8},
};

HuffmanCodebook HuffmanScaleFactorsSigned[6] = {
	{0},
	{0},
	{ScaleFactorsB2Bits, ScaleFactorsB2Codes, ScaleFactorsB2Lookup, 4, 1, 0, 2, 4, 2},
	{ScaleFactorsB3Bits, ScaleFactorsB3Codes, ScaleFactorsB3Lookup, 8, 1, 0, 3, 8, 6},
	{ScaleFactorsB4Bits, ScaleFactorsB4Codes, ScaleFactorsB4Lookup, 16, 1, 0, 4, 16, 8},
	{ScaleFactorsB5Bits, ScaleFactorsB5Codes, ScaleFactorsB5Lookup, 32, 1, 0, 5, 32, 8},
};

HuffmanCodebook HuffmanSpectrum[2][8][4] = {
	{
		{0},
		{0},
		{
			{SpectrumA21Bits, SpectrumA21Codes, SpectrumA21Lookup, 16, 2, 1, 2, 4, 3},
			{SpectrumA22Bits, SpectrumA22Codes, SpectrumA22Lookup, 256, 4, 2, 2, 4, 8},
			{SpectrumA23Bits, SpectrumA23Codes, SpectrumA23Lookup, 256, 4, 2, 2, 4, 9},
			{SpectrumA24Bits, SpectrumA24Codes, SpectrumA24Lookup, 256, 4, 2, 2, 4, 10}
		},
		{
			{SpectrumA31Bits, SpectrumA31Codes, SpectrumA31Lookup, 64, 2, 1, 3, 8, 7},
			{SpectrumA32Bits, SpectrumA32Codes, SpectrumA32Lookup, 64, 2, 1, 3, 8, 7},
			{SpectrumA33Bits, SpectrumA33Codes, SpectrumA33Lookup, 64, 2, 1, 3, 8, 8},
			{SpectrumA34Bits, SpectrumA34Codes, SpectrumA34Lookup, 64, 2, 1, 3, 8, 10}
		},
		{
			{SpectrumA41Bits, SpectrumA41Codes, SpectrumA41Lookup, 256, 2, 1, 4, 16, 9},
			{SpectrumA42Bits, SpectrumA42Codes, SpectrumA42Lookup, 256, 2, 1, 4, 16, 10},
			{SpectrumA43Bits, SpectrumA43Codes, SpectrumA43Lookup, 256, 2, 1, 4, 16, 10},
			{SpectrumA44Bits, SpectrumA44Codes, SpectrumA44Lookup, 256, 2, 1, 4, 16, 10}
		},
		{
			{SpectrumA51Bits, SpectrumA51Codes, SpectrumA51Lookup, 32, 1, 0, 5, 32, 6},
			{SpectrumA52Bits, SpectrumA52Codes, SpectrumA52Lookup, 32, 1, 0, 5, 32, 6},
			{SpectrumA53Bits, SpectrumA53Codes, SpectrumA53Lookup, 32, 1, 0, 5, 32, 7},
			{SpectrumA54Bits, SpectrumA54Codes, SpectrumA54Lookup, 32, 1, 0, 5, 32, 8}
		},
		{
			{SpectrumA61Bits, SpectrumA61Codes, SpectrumA61Lookup, 64, 1, 0, 6, 64, 7},
			{SpectrumA62Bits, SpectrumA62Codes, SpectrumA62Lookup, 64, 1, 0, 6, 64, 7},
			{SpectrumA63Bits, SpectrumA63Codes, SpectrumA63Lookup, 64, 1, 0, 6, 64, 8},
			{SpectrumA64Bits, SpectrumA64Codes, SpectrumA64Lookup, 64, 1, 0, 6, 64, 9}
		},
		{
			{SpectrumA71Bits, SpectrumA71Codes, SpectrumA71Lookup, 128, 1, 0, 7, 128, 8},
			{SpectrumA72Bits, SpectrumA72Codes, SpectrumA72Lookup, 128, 1, 0, 7, 128, 8},
			{SpectrumA73Bits, SpectrumA73Codes, SpectrumA73Lookup, 128, 1, 0, 7, 128, 9},
			{SpectrumA74Bits, SpectrumA74Codes, SpectrumA74Lookup, 128, 1, 0, 7, 128, 10}
		}
	},
	{
		{0},
		{0},
		{
			{0},
			{SpectrumB22Bits, SpectrumB22Codes, SpectrumB22Lookup, 256, 4, 2, 2, 4, 10},
			{SpectrumB23Bits, SpectrumB23Codes, SpectrumB23Lookup, 256, 4, 2, 2, 4, 10},
			{SpectrumB24Bits, SpectrumB24Codes, SpectrumB24Lookup, 256, 4, 2, 2, 4, 10}
		},
		{
			{0},
			{SpectrumB32Bits, SpectrumB32Codes, SpectrumB32Lookup, 64, 2, 1, 3, 8, 9},
			{SpectrumB33Bits, SpectrumB33Codes, SpectrumB33Lookup, 64, 2, 1, 3, 8, 10},
			{SpectrumB34Bits, SpectrumB34Codes, SpectrumB34Lookup, 64, 2, 1, 3, 8, 10}
		},
		{
			{0},
			{SpectrumB42Bits, SpectrumB42Codes, SpectrumB42Lookup, 256, 2, 1, 4, 16, 10},
			{SpectrumB43Bits, SpectrumB43Codes, SpectrumB43Lookup, 256, 2, 1, 4, 16, 10},
			{SpectrumB44Bits, SpectrumB44Codes, SpectrumB44Lookup, 256, 2, 1, 4, 16, 10}
		},
		{
			{0},
			{SpectrumB52Bits, SpectrumB52Codes, SpectrumB52Lookup, 32, 1, 0, 5, 32, 7},
			{SpectrumB53Bits, SpectrumB53Codes, SpectrumB53Lookup, 32, 1, 0, 5, 32, 8},
			{SpectrumB54Bits, SpectrumB54Codes, SpectrumB54Lookup, 32, 1, 0, 5, 32, 9}
		},
		{
			{0},
			{SpectrumB62Bits, SpectrumB62Codes, SpectrumB62Lookup, 64, 1, 0, 6, 64, 8},
			{SpectrumB63Bits, SpectrumB63Codes, SpectrumB63Lookup, 64, 1, 0, 6, 64, 9},
			{SpectrumB64Bits, SpectrumB64Codes, SpectrumB64Lookup, 64, 1, 0, 6, 64, 10}
		},
		{
			{0},
			{SpectrumB72Bits, SpectrumB72Codes, SpectrumB72Lookup, 128, 1, 0, 7, 128, 9},
			{SpectrumB73Bits, SpectrumB73Codes, SpectrumB73Lookup, 128, 1, 0, 7, 128, 10},
			{SpectrumB74Bits, SpectrumB74Codes, SpectrumB74Lookup, 128, 1, 0, 7, 128, 10}
		}
	}
};
