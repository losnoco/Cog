//
//  g719.c
//  g719
//
//  Created by Christopher Snowhill on 1/24/15.
//  Copyright (c) 2015 Christopher Snowhill. All rights reserved.
//

#include <stdlib.h>

#include "g719.h"

#include "proto.h"

#include "stack_alloc.h"

struct g719_handle_s
{
    DecoderState state;
    int num_bits;
};

g719_handle * g719_init(int bytes_per_frame)
{
    g719_handle * handle = (g719_handle *) malloc(sizeof(struct g719_handle_s));
    if (handle)
    {
        handle->num_bits = bytes_per_frame * 8;
        decoder_init(&handle->state, bytes_per_frame * 8);
    }
    return handle;
}

void g719_decode_frame(g719_handle *handle, void *code_words, void *sample_buffer)
{
    int i, j;
    VARDECL(short, code_bits);
    ALLOC(code_bits, handle->num_bits, short);
    for (i = 0, j = handle->num_bits; i < j; i++)
    {
        code_bits[i] = ((((unsigned char *)code_words)[i / 8] >> (i & 7)) & 1) ? G192_BIT1 : G192_BIT0;
    }
    decode_frame((short *)code_bits, 0, (short *)sample_buffer, &handle->state);
}

void g719_reset(g719_handle *handle)
{
    decoder_reset_tables(&handle->state, handle->num_bits);
}

void g719_free(g719_handle *handle)
{
    free(handle);
}
