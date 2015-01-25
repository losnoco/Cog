//
//  g7221.c
//  g7221
//
//  Created by Christopher Snowhill on 1/24/15.
//  Copyright (c) 2015 Christopher Snowhill. All rights reserved.
//

#include "g7221.h"

#include "libsiren/siren7.h"

g7221_handle * g7221_init(int bytes_per_frame, int bandwidth)
{
    g7221_handle * handle = 0;
    int sample_rate = bytes_per_frame * 8 * 50;
    if ( sample_rate >= 16000 && sample_rate <= 48000 )
    {
        if ( ( ( sample_rate / 800 ) * 800 ) == sample_rate )
        {
            if ( bandwidth == 7000 || bandwidth == 14000 )
            {
                handle = (g7221_handle *) Siren7_NewDecoder( sample_rate, ( bandwidth == 7000 ) ? 1 : 2 );
            }
        }
    }
    return handle;
}

void g7221_decode_frame(g7221_handle *handle, void *code_words, void *sample_buffer)
{
    Siren7_DecodeFrame((SirenDecoder)handle, (unsigned char *)code_words, (unsigned char *)sample_buffer);
}

void g7221_reset(g7221_handle *handle)
{
    Siren7_ResetDecoder ((SirenDecoder)handle);
}

void g7221_free(g7221_handle *handle)
{
    Siren7_CloseDecoder((SirenDecoder) handle);
}
