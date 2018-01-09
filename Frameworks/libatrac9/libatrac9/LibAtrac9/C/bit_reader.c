#include "bit_reader.h"
#include "utility.h"

static int peek_int_fallback(bit_reader_cxt* br, int bit_count);

void init_bit_reader_cxt(bit_reader_cxt* br, const void * buffer)
{
	br->buffer = buffer;
	br->position = 0;
}

int read_int(bit_reader_cxt* br, const int bits)
{
	const int value = peek_int(br, bits);
	br->position += bits;
	return value;
}

int read_signed_int(bit_reader_cxt* br, const int bits)
{
	const int value = peek_int(br, bits);
	br->position += bits;
	return SignExtend32(value, bits);
}

int read_offset_binary(bit_reader_cxt* br, const int bits)
{
	const int offset = 1 << (bits - 1);
	const int value = peek_int(br, bits) - offset;
	br->position += bits;
	return value;
}

int peek_int(bit_reader_cxt* br, const int bits)
{
	const int byte_index = br->position / 8;
	const int bit_index = br->position % 8;
	const unsigned char* buffer = br->buffer;

	if (bits <= 9)
	{
		int value = buffer[byte_index] << 8 | buffer[byte_index + 1];
		value &= 0xFFFF >> bit_index;
		value >>= 16 - bits - bit_index;
		return value;
	}

	if (bits <= 17)
	{
		int value = buffer[byte_index] << 16 | buffer[byte_index + 1] << 8 | buffer[byte_index + 2];
		value &= 0xFFFFFF >> bit_index;
		value >>= 24 - bits - bit_index;
		return value;
	}

	if (bits <= 25)
	{
		int value = buffer[byte_index] << 24
			| buffer[byte_index + 1] << 16
			| buffer[byte_index + 2] << 8
			| buffer[byte_index + 3];

		value &= (int)(0xFFFFFFFF >> bit_index);
		value >>= 32 - bits - bit_index;
		return value;
	}
	return peek_int_fallback(br, bits);
}

void align_position(bit_reader_cxt* br, const unsigned int multiple)
{
	const int position = br->position;
	if (position % multiple == 0)
	{
		return;
	}

	br->position = position + multiple - position % multiple;
}

static int peek_int_fallback(bit_reader_cxt* br, int bit_count)
{
	int value = 0;
	int byte_index = br->position / 8;
	int bit_index = br->position % 8;
	const unsigned char* buffer = br->buffer;

	while (bit_count > 0)
	{
		if (bit_index >= 8)
		{
			bit_index = 0;
			byte_index++;
		}

		int bits_to_read = bit_count;
		if (bits_to_read > 8 - bit_index)
		{
			bits_to_read = 8 - bit_index;
		}

		const int mask = 0xFF >> bit_index;
		const int current_byte = (mask & buffer[byte_index]) >> (8 - bit_index - bits_to_read);

		value = (value << bits_to_read) | current_byte;
		bit_index += bits_to_read;
		bit_count -= bits_to_read;
	}
	return value;
}