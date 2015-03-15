#include "barray.h"

#include <string.h>


void * bit_array_create(size_t size)
{
	size_t bsize = ((size + 7) >> 3) + sizeof(size_t);
	void * ret = calloc(1, bsize);
	if (ret) *(size_t *)ret = size;
	return ret;
}

void bit_array_destroy(void * array)
{
	if (array) free(array);
}

void * bit_array_dup(const void * array)
{
	if (array)
	{
		const size_t * size = (const size_t *) array;
		size_t bsize = ((*size + 7) >> 3) + sizeof(*size);
		void * ret = malloc(bsize);
		if (ret) memcpy(ret, array, bsize);
		return ret;
	}
	return NULL;
}

void bit_array_reset(void * array)
{
	if (array)
	{
		size_t * size = (size_t *) array;
		size_t bsize = (*size + 7) >> 3;
		memset(size + 1, 0, bsize);
	}
}


size_t bit_array_size(const void * array)
{
	if (array)
	{
		return *(const size_t *) array;
	}
	return 0;
}

size_t bit_array_count(const void * array)
{
	if (array)
	{
		size_t i;
		size_t count = 0;
		const size_t * size = (const size_t *) array;
		for (i = 0; i < *size; ++i)
			count += bit_array_test(array, i);
		return count;
	}
	return 0;
}

void bit_array_set(void * array, size_t bit)
{
	if (array)
	{
		size_t * size = (size_t *) array;
		if (bit < *size)
		{
			unsigned char * ptr = (unsigned char *)(size + 1);
			ptr[bit >> 3] |= (1U << (bit & 7));
		}
	}
}

void bit_array_set_range(void * array, size_t bit, size_t count)
{
    if (array && count)
    {
        size_t * size = (size_t *) array;
        if (bit < *size)
        {
            unsigned char * ptr = (unsigned char *)(size + 1);
            size_t i;
            for (i = bit; i < *size && i < bit + count; ++i)
                ptr[i >> 3] |= (1U << (i & 7));
        }
    }
}

int bit_array_test(const void * array, size_t bit)
{
	if (array)
	{
		const size_t * size = (const size_t *) array;
		if (bit < *size)
		{
			const unsigned char * ptr = (const unsigned char *)(size + 1);
			if (ptr[bit >> 3] & (1U << (bit & 7)))
			{
				return 1;
			}
		}
	}
	return 0;
}

int bit_array_test_range(const void * array, size_t bit, size_t count)
{
	if (array)
	{
		const size_t * size = (const size_t *) array;
		if (bit < *size)
		{
			const unsigned char * ptr = (const unsigned char *)(size + 1);
			if ((bit & 7) && (count > 8))
			{
				while ((bit < *size) && count && (bit & 7))
				{
					if (ptr[bit >> 3] & (1U << (bit & 7))) return 1;
					bit++;
					count--;
				}
			}
			if (!(bit & 7))
			{
				while (((*size - bit) >= 8) && (count >= 8))
				{
					if (ptr[bit >> 3]) return 1;
					bit += 8;
					count -= 8;
				}
			}
			while ((bit < *size) && count)
			{
				if (ptr[bit >> 3] & (1U << (bit & 7))) return 1;
				bit++;
				count--;
			}
		}
	}
	return 0;
}

void bit_array_clear(void * array, size_t bit)
{
	if (array)
	{
		size_t * size = (size_t *) array;
		if (bit < *size)
		{
			unsigned char * ptr = (unsigned char *)(size + 1);
			ptr[bit >> 3] &= ~(1U << (bit & 7));
		}
	}
}

void bit_array_clear_range(void * array, size_t bit, size_t count)
{
    if (array && count)
    {
        size_t * size = (size_t *) array;
        if (bit < *size)
        {
            unsigned char * ptr = (unsigned char *)(size + 1);
            size_t i;
            for (i = bit; i < *size && i < bit + count; ++i)
                ptr[i >> 3] &= ~(1U << (i & 7));
        }
    }
}

void bit_array_merge(void * dest, const void * source, size_t offset)
{
	if (dest && source)
	{
		size_t * dsize = (size_t *) dest;
		const size_t * ssize = (const size_t *) source;
		size_t soffset = 0;
		while (offset < *dsize && soffset < *ssize)
		{
			if (bit_array_test(source, soffset))
			{
				bit_array_set(dest, offset);
			}
			soffset++;
			offset++;
		}
	}
}

void bit_array_mask(void * dest, const void * source, size_t offset)
{
	if (dest && source)
	{
		size_t * dsize = (size_t *) dest;
		const size_t * ssize = (const size_t *) source;
		size_t soffset = 0;
		while (offset < *dsize && soffset < *ssize)
		{
			if (bit_array_test(source, soffset))
			{
				bit_array_clear(dest, offset);
			}
			soffset++;
			offset++;
		}
	}
}
