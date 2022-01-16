//
//  ConverterNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "ConverterNode.h"

#import "BufferChain.h"
#import "OutputNode.h"

#import "Logging.h"

#import <audio/conversion/s16_to_float.h>
#import <audio/conversion/s32_to_float.h>

#import "lpc.h"

#import <TargetConditionals.h>

#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#include <emmintrin.h>
#elif TARGET_CPU_ARM || TARGET_CPU_ARM64
#include <arm_neon.h>
#endif

void PrintStreamDesc (AudioStreamBasicDescription *inDesc)
{
	if (!inDesc) {
		DLog (@"Can't print a NULL desc!\n");
		return;
	}
	DLog (@"- - - - - - - - - - - - - - - - - - - -\n");
	DLog (@"  Sample Rate:%f\n", inDesc->mSampleRate);
	DLog (@"  Format ID:%s\n", (char*)&inDesc->mFormatID);
	DLog (@"  Format Flags:%X\n", inDesc->mFormatFlags);
	DLog (@"  Bytes per Packet:%d\n", inDesc->mBytesPerPacket);
	DLog (@"  Frames per Packet:%d\n", inDesc->mFramesPerPacket);
	DLog (@"  Bytes per Frame:%d\n", inDesc->mBytesPerFrame);
	DLog (@"  Channels per Frame:%d\n", inDesc->mChannelsPerFrame);
	DLog (@"  Bits per Channel:%d\n", inDesc->mBitsPerChannel);
	DLog (@"- - - - - - - - - - - - - - - - - - - -\n");
}

@implementation ConverterNode

@synthesize inputFormat;

- (id)initWithController:(id)c previous:(id)p
{
    self = [super initWithController:c previous:p];
    if (self)
    {
        rgInfo = nil;
        
        resampler = NULL;
        resampler_data = NULL;
        inputBuffer = NULL;
        inputBufferSize = 0;
        floatBuffer = NULL;
        floatBufferSize = 0;
        
        stopping = NO;
        convertEntered = NO;
        paused = NO;
        outputFormatChanged = NO;
        
        skipResampler = NO;
        
        latencyStarted = -1;
        latencyEaten = 0;
        latencyPostfill = NO;
        
        refillNode = nil;
        originalPreviousNode = nil;
        
        extrapolateBuffer = NULL;
        extrapolateBufferSize = 0;
        
        dsd2pcm = NULL;
        dsd2pcmCount = 0;
        
        outputResampling = @"";

        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling"		options:0 context:nil];
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.outputResampling" options:0 context:nil];
    }
    
    return self;
}

static const float STEREO_DOWNMIX[8-2][8][2]={
    /*3.0*/
    {
        {0.5858F,0.0F},{0.0F,0.5858F},{0.4142F,0.4142F}
    },
    /*quadrophonic*/
    {
        {0.4226F,0.0F},{0.0F,0.4226F},{0.366F,0.2114F},{0.2114F,0.336F}
    },
    /*5.0*/
    {
        {0.651F,0.0F},{0.0F,0.651F},{0.46F,0.46F},{0.5636F,0.3254F},
        {0.3254F,0.5636F}
    },
    /*5.1*/
    {
        {0.529F,0.0F},{0.0F,0.529F},{0.3741F,0.3741F},{0.3741F,0.3741F},{0.4582F,0.2645F},
        {0.2645F,0.4582F}
    },
    /*6.1*/
    {
        {0.4553F,0.0F},{0.0F,0.4553F},{0.322F,0.322F},{0.322F,0.322F},{0.2788F,0.2788F},
        {0.3943F,0.2277F},{0.2277F,0.3943F}
    },
    /*7.1*/
    {
        {0.3886F,0.0F},{0.0F,0.3886F},{0.2748F,0.2748F},{0.2748F,0.2748F},{0.3366F,0.1943F},
        {0.1943F,0.3366F},{0.3366F,0.1943F},{0.1943F,0.3366F}
    }
};

static void downmix_to_stereo(float * buffer, int channels, size_t count)
{
    if (channels >= 3 && channels <= 8)
    for (size_t i = 0; i < count; ++i)
    {
        float left = 0, right = 0;
        for (int j = 0; j < channels; ++j)
        {
            left += buffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][0];
            right += buffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][1];
        }
        buffer[i * 2 + 0] = left;
        buffer[i * 2 + 1] = right;
    }
}

static void downmix_to_mono(float * buffer, int channels, size_t count)
{
    if (channels >= 3 && channels <= 8)
    {
        downmix_to_stereo(buffer, channels, count);
        channels = 2;
    }
    float invchannels = 1.0 / (float)channels;
    for (size_t i = 0; i < count; ++i)
    {
        float sample = 0;
        for (int j = 0; j < channels; ++j)
        {
            sample += buffer[i * channels + j];
        }
        buffer[i] = sample * invchannels;
    }
}

static void upmix(float * buffer, int inchannels, int outchannels, size_t count)
{
    for (ssize_t i = count - 1; i >= 0; --i)
    {
        if (inchannels == 1 && outchannels == 2)
        {
            // upmix mono to stereo
            float sample = buffer[i];
            buffer[i * 2 + 0] = sample;
            buffer[i * 2 + 1] = sample;
        }
        else if (inchannels == 1 && outchannels == 4)
        {
            // upmix mono to quad
            float sample = buffer[i];
            buffer[i * 4 + 0] = sample;
            buffer[i * 4 + 1] = sample;
            buffer[i * 4 + 2] = 0;
            buffer[i * 4 + 3] = 0;
        }
        else if (inchannels == 1 && (outchannels == 3 || outchannels >= 5))
        {
            // upmix mono to center channel
            float sample = buffer[i];
            buffer[i * outchannels + 2] = sample;
            for (int j = 0; j < 2; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
            for (int j = 3; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
        else if (inchannels == 4 && outchannels >= 5)
        {
            float fl = buffer[i * 4 + 0];
            float fr = buffer[i * 4 + 1];
            float bl = buffer[i * 4 + 2];
            float br = buffer[i * 4 + 3];
            const int skipclfe = (outchannels == 5) ? 1 : 2;
            buffer[i * outchannels + 0] = fl;
            buffer[i * outchannels + 1] = fr;
            buffer[i * outchannels + skipclfe + 2] = bl;
            buffer[i * outchannels + skipclfe + 3] = br;
            for (int j = 0; j < skipclfe; ++j)
            {
                buffer[i * outchannels + 2 + j] = 0;
            }
            for (int j = 4 + skipclfe; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
        else if (inchannels == 5 && outchannels >= 6)
        {
            float fl = buffer[i * 5 + 0];
            float fr = buffer[i * 5 + 1];
            float c = buffer[i * 5 + 2];
            float bl = buffer[i * 5 + 3];
            float br = buffer[i * 5 + 4];
            buffer[i * outchannels + 0] = fl;
            buffer[i * outchannels + 1] = fr;
            buffer[i * outchannels + 2] = c;
            buffer[i * outchannels + 3] = 0;
            buffer[i * outchannels + 4] = bl;
            buffer[i * outchannels + 5] = br;
            for (int j = 6; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
        else if (inchannels == 7 && outchannels == 8)
        {
            float fl = buffer[i * 7 + 0];
            float fr = buffer[i * 7 + 1];
            float c = buffer[i * 7 + 2];
            float lfe = buffer[i * 7 + 3];
            float sl = buffer[i * 7 + 4];
            float sr = buffer[i * 7 + 5];
            float bc = buffer[i * 7 + 6];
            buffer[i * 8 + 0] = fl;
            buffer[i * 8 + 1] = fr;
            buffer[i * 8 + 2] = c;
            buffer[i * 8 + 3] = lfe;
            buffer[i * 8 + 4] = bc;
            buffer[i * 8 + 5] = bc;
            buffer[i * 8 + 6] = sl;
            buffer[i * 8 + 7] = sr;
        }
        else
        {
            // upmix N channels to N channels plus silence the empty channels
            float samples[inchannels];
            for (int j = 0; j < inchannels; ++j)
            {
                samples[j] = buffer[i * inchannels + j];
            }
            for (int j = 0; j < inchannels; ++j)
            {
                buffer[i * outchannels + j] = samples[j];
            }
            for (int j = inchannels; j < outchannels; ++j)
            {
                buffer[i * outchannels + j] = 0;
            }
        }
    }
}

void scale_by_volume(float * buffer, size_t count, float volume)
{
    if ( volume != 1.0 )
    {
#if TARGET_CPU_X86 || TARGET_CPU_X86_64
        if ( count >= 8 )
        {
            __m128 vgf = _mm_set1_ps(volume);
            while ( count >= 8 )
            {
                __m128 input   = _mm_loadu_ps(buffer);
                __m128 input2  = _mm_loadu_ps(buffer + 4);
                __m128 output  = _mm_mul_ps(input, vgf);
                __m128 output2 = _mm_mul_ps(input2, vgf);

                _mm_storeu_ps(buffer + 0, output);
                _mm_storeu_ps(buffer + 4, output2);
                
                buffer        += 8;
                count         -= 8;
            }
        }
#elif TARGET_CPU_ARM || TARGET_CPU_ARM64
        if ( count >= 8 )
        {
            float32x4_t vgf           = vdupq_n_f32(volume);
            while ( count >= 8 )
            {
                float32x4x2_t oreg;
                float32x4x2_t inreg   = vld1q_f32_x2(buffer);
                oreg.val[0]           = vmulq_f32(inreg.val[0], vgf);
                oreg.val[1]           = vmulq_f32(inreg.val[1], vgf);
                vst1q_f32_x2(buffer, oreg);
                buffer               += 8;
                count                -= 8;
            }
        }
#endif
        
        for (size_t i = 0; i < count; ++i)
            buffer[i] *= volume;
    }
}

/**
 * DSD 2 PCM: Stage 1:
 * Decimate by factor 8
 * (one byte (8 samples) -> one float sample)
 * The bits are processed from least signicifant to most signicicant.
 * @author Sebastian Gesemann
 */

#define dsd2pcm_FILTER_COEFFS_COUNT 64
static const float dsd2pcm_FILTER_COEFFS[64] =
{
    0.09712411121659f, 0.09613438994044f, 0.09417884216316f, 0.09130441727307f,
    0.08757947648990f, 0.08309142055179f, 0.07794369263673f, 0.07225228745463f,
    0.06614191680338f, 0.05974199351302f, 0.05318259916599f, 0.04659059631228f,
    0.04008603356890f, 0.03377897290478f, 0.02776684382775f, 0.02213240062966f,
    0.01694232798846f, 0.01224650881275f, 0.00807793792573f, 0.00445323755944f,
    0.00137370697215f,-0.00117318019994f,-0.00321193033831f,-0.00477694265140f,
    -0.00591028841335f,-0.00665946056286f,-0.00707518873201f,-0.00720940203988f,
    -0.00711340642819f,-0.00683632603227f,-0.00642384017266f,-0.00591723006715f,
    -0.00535273320457f,-0.00476118922548f,-0.00416794965654f,-0.00359301524813f,
    -0.00305135909510f,-0.00255339111833f,-0.00210551956895f,-0.00171076760278f,
    -0.00136940723130f,-0.00107957856005f,-0.00083786862365f,-0.00063983084245f,
    -0.00048043272086f,-0.00035442550015f,-0.00025663481039f,-0.00018217573430f,
    -0.00012659899635f,-0.00008597726991f,-0.00005694188820f,-0.00003668060332f,
    -0.00002290670286f,-0.00001380895679f,-0.00000799057558f,-0.00000440385083f,
    -0.00000228567089f,-0.00000109760778f,-0.00000047286430f,-0.00000017129652f,
    -0.00000004282776f, 0.00000000119422f, 0.00000000949179f, 0.00000000747450f
};

struct dsd2pcm_state {
    /*
    * This is the 2nd half of an even order symmetric FIR
    * lowpass filter (to be used on a signal sampled at 44100*64 Hz)
    * Passband is 0-24 kHz (ripples +/- 0.025 dB)
    * Stopband starts at 176.4 kHz (rejection: 170 dB)
    * The overall gain is 2.0
    */

    /* These remain constant for the duration */
    int FILT_LOOKUP_PARTS;
    float * FILT_LOOKUP_TABLE;
    uint8_t * REVERSE_BITS;
    int FIFO_LENGTH;
    int FIFO_OFS_MASK;

    /* These are altered */
    int * fifo;
    int fpos;
};

static void dsd2pcm_free(void *);
static void dsd2pcm_reset(void *);

static void * dsd2pcm_alloc()
{
    struct dsd2pcm_state * state = (struct dsd2pcm_state *) calloc(1, sizeof(struct dsd2pcm_state));
    
    if (!state)
        return NULL;
    
    state->FILT_LOOKUP_PARTS = ( dsd2pcm_FILTER_COEFFS_COUNT + 7 ) / 8;
    const int FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
    // The current 128 tap FIR leads to an 8 KB lookup table
    state->FILT_LOOKUP_TABLE = (float*) calloc(sizeof(float), FILT_LOOKUP_PARTS << 8);
    if (!state->FILT_LOOKUP_TABLE)
        goto fail;
    float* FILT_LOOKUP_TABLE = state->FILT_LOOKUP_TABLE;
    double * temp = (double*) calloc(sizeof(double), 0x100);
    if (!temp)
        goto fail;
    for ( int part=0, sofs=0, dofs=0; part < FILT_LOOKUP_PARTS; )
    {
        memset( temp, 0, 0x100 * sizeof( double ) );
        for ( int bit=0, bitmask=0x80; bit<8 && sofs+bit < dsd2pcm_FILTER_COEFFS_COUNT; )
        {
            double coeff = dsd2pcm_FILTER_COEFFS[ sofs + bit ];
            for ( int bite=0; bite < 0x100; bite++ )
            {
                if ( ( bite & bitmask ) == 0 )
                {
                    temp[ bite ] -= coeff;
                } else {
                    temp[ bite ] += coeff;
                }
            }
            bit++;
            bitmask >>= 1;
        }
        for ( int s = 0; s < 0x100; ) {
            FILT_LOOKUP_TABLE[dofs++] = (float) temp[s++];
        }
        part++;
        sofs += 8;
    }
    free(temp);
    { // calculate FIFO stuff
        int k = 1;
        while (k<FILT_LOOKUP_PARTS*2) k<<=1;
        state->FIFO_LENGTH = k;
        state->FIFO_OFS_MASK = k-1;
    }
    state->REVERSE_BITS = (uint8_t*) calloc(1, 0x100);
    if (!state->REVERSE_BITS)
        goto fail;
    uint8_t* REVERSE_BITS = state->REVERSE_BITS;
    for (int i=0, j=0; i<0x100; i++) {
        REVERSE_BITS[i] = ( uint8_t ) j;
        // "reverse-increment" of j
        for (int bitmask=0x80;;) {
            if (((j^=bitmask) & bitmask)!=0) break;
            if (bitmask==1) break;
            bitmask >>= 1;
        }
    }

    state->fifo = (int*) calloc(sizeof(int), state->FIFO_LENGTH);
    if (!state->fifo)
        goto fail;

    dsd2pcm_reset(state);
    
    return (void*) state;
    
fail:
    dsd2pcm_free(state);
    return NULL;
}

static void * dsd2pcm_dup(void * _state)
{
    struct dsd2pcm_state * state = (struct dsd2pcm_state *) _state;
    if (state)
    {
        struct dsd2pcm_state * newstate = (struct dsd2pcm_state *) calloc(1, sizeof(struct dsd2pcm_state));
        if (newstate) {
            newstate->FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
            newstate->FIFO_LENGTH = state->FIFO_LENGTH;
            newstate->FIFO_OFS_MASK = state->FIFO_OFS_MASK;
            newstate->fpos = state->fpos;
            
            newstate->FILT_LOOKUP_TABLE = (float*) calloc(sizeof(float), state->FILT_LOOKUP_PARTS << 8);
            if (!newstate->FILT_LOOKUP_TABLE)
                goto fail;
            
            memcpy(newstate->FILT_LOOKUP_TABLE, state->FILT_LOOKUP_TABLE, sizeof(float) * (state->FILT_LOOKUP_PARTS << 8));
            
            newstate->REVERSE_BITS = (uint8_t*) calloc(1, 0x100);
            if (!newstate->REVERSE_BITS)
                goto fail;
            
            memcpy(newstate->REVERSE_BITS, state->REVERSE_BITS, 0x100);
            
            newstate->fifo = (int*) calloc(sizeof(int), state->FIFO_LENGTH);
            if (!newstate->fifo)
                goto fail;
            
            memcpy(newstate->fifo, state->fifo, sizeof(int) * state->FIFO_LENGTH);

            return (void*) newstate;
        }
        
    fail:
        dsd2pcm_free(newstate);
        return NULL;
    }
    
    return NULL;
}

static void dsd2pcm_free(void * _state)
{
    struct dsd2pcm_state * state = (struct dsd2pcm_state *) _state;
    if (state)
    {
        free(state->fifo);
        free(state->REVERSE_BITS);
        free(state->FILT_LOOKUP_TABLE);
        free(state);
    }
}

static void dsd2pcm_reset(void * _state)
{
    struct dsd2pcm_state * state = (struct dsd2pcm_state *) _state;
    const int FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
    int* fifo = state->fifo;
    for (int i=0; i<FILT_LOOKUP_PARTS; i++) {
        fifo[i] = 0x55;
        fifo[i+FILT_LOOKUP_PARTS] = 0xAA;
    }
    state->fpos = FILT_LOOKUP_PARTS;
}

static void dsd2pcm_process(void * _state, uint8_t * src, size_t sofs, size_t sinc, float * dest, size_t dofs, size_t dinc, size_t len)
{
    struct dsd2pcm_state * state = (struct dsd2pcm_state *) _state;
    int bite1, bite2, temp;
    float sample;
    int* fifo = state->fifo;
    const uint8_t* REVERSE_BITS = state->REVERSE_BITS;
    const float* FILT_LOOKUP_TABLE = state->FILT_LOOKUP_TABLE;
    const int FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
    const int FIFO_OFS_MASK = state->FIFO_OFS_MASK;
    int fpos = state->fpos;
    while ( len > 0 )
    {
        fifo[ fpos ] = REVERSE_BITS[ fifo[ fpos ] ] & 0xFF;
        fifo[ ( fpos + FILT_LOOKUP_PARTS ) & FIFO_OFS_MASK ] = src[ sofs ] & 0xFF;
        sofs += sinc;
        temp = ( fpos + 1 ) & FIFO_OFS_MASK;
        sample = 0;
        for ( int k=0, lofs=0; k < FILT_LOOKUP_PARTS; )
        {
            bite1 = fifo[ ( fpos - k ) & FIFO_OFS_MASK ];
            bite2 = fifo[ ( temp + k ) & FIFO_OFS_MASK ];
            sample += FILT_LOOKUP_TABLE[ lofs + bite1 ] + FILT_LOOKUP_TABLE[ lofs + bite2 ];
            k++;
            lofs += 0x100;
        }
        fpos = temp;
        dest[ dofs ] = sample;
        dofs += dinc;
        len--;
    }
    state->fpos = fpos;
}

static int dsd2pcm_latency(void * _state)
{
    struct dsd2pcm_state * state = (struct dsd2pcm_state *) _state;
    if (state) return state->FIFO_LENGTH;
    else return 0;
}

static void convert_dsd_to_f32(float *output, uint8_t *input, size_t count, size_t channels, void ** dsd2pcm)
{
    for (size_t channel = 0; channel < channels; ++channel)
    {
        dsd2pcm_process(dsd2pcm[channel], input, channel, channels, output, channel, channels, count);
    }
}

static void convert_u8_to_s16(int16_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t sample = (input[i] << 8) | input[i];
        sample ^= 0x8080;
        output[i] = (int16_t)(sample);
    }
}

static void convert_s8_to_s16(int16_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t sample = (input[i] << 8) | input[i];
        output[i] = (int16_t)(sample);
    }
}

static void convert_u16_to_s16(int16_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        buffer[i] ^= 0x8000;
    }
}

static void convert_s24_to_s32(int32_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        int32_t sample = (input[i * 3] << 8) | (input[i * 3 + 1] << 16) | (input[i * 3 + 2] << 24);
        output[i] = sample;
    }
}

static void convert_u24_to_s32(int32_t *output, uint8_t *input, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        int32_t sample = (input[i * 3] << 8) | (input[i * 3 + 1] << 16) | (input[i * 3 + 2] << 24);
        output[i] = sample ^ 0x80000000;
    }
}

static void convert_u32_to_s32(int32_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        buffer[i] ^= 0x80000000;
    }
}

static void convert_f64_to_f32(float *output, double *input, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        output[i] = (float)(input[i]);
    }
}

static void convert_be_to_le(uint8_t *buffer, size_t bitsPerSample, size_t bytes)
{
    size_t i;
    uint8_t temp;
    bitsPerSample = (bitsPerSample + 7) / 8;
    switch (bitsPerSample) {
        case 2:
            for (i = 0; i < bytes; i += 2)
            {
                temp = buffer[1];
                buffer[1] = buffer[0];
                buffer[0] = temp;
                buffer += 2;
            }
            break;
            
        case 3:
            for (i = 0; i < bytes; i += 3)
            {
                temp = buffer[2];
                buffer[2] = buffer[0];
                buffer[0] = temp;
                buffer += 3;
            }
            break;
            
        case 4:
            for (i = 0; i < bytes; i += 4)
            {
                temp = buffer[3];
                buffer[3] = buffer[0];
                buffer[0] = temp;
                temp = buffer[2];
                buffer[2] = buffer[1];
                buffer[1] = temp;
                buffer += 4;
            }
            break;
            
        case 8:
            for (i = 0; i < bytes; i += 8)
            {
                temp = buffer[7];
                buffer[7] = buffer[0];
                buffer[0] = temp;
                temp = buffer[6];
                buffer[6] = buffer[1];
                buffer[1] = temp;
                temp = buffer[5];
                buffer[5] = buffer[2];
                buffer[2] = temp;
                temp = buffer[4];
                buffer[4] = buffer[3];
                buffer[3] = temp;
                buffer += 8;
            }
            break;
    }
}

static const int extrapolate_order = 16;

static void extrapolate(float *buffer, ssize_t channels, ssize_t frameSize, ssize_t size, BOOL backward, void ** extrapolateBuffer, size_t * extrapolateSize)
{
    const ssize_t delta = (backward ? -1 : 1) * channels;

    size_t lpc_size = sizeof(float) * extrapolate_order;
    size_t my_work_size = sizeof(float) * frameSize;
    size_t their_work_size = sizeof(float) * (extrapolate_order + size);
    
    size_t newSize = lpc_size + my_work_size + their_work_size;
    if (!*extrapolateBuffer || *extrapolateSize < newSize)
    {
        *extrapolateBuffer = realloc(*extrapolateBuffer, newSize);
        *extrapolateSize = newSize;
    }

    float *lpc = (float*)(*extrapolateBuffer);
    float *work = (float*)(*extrapolateBuffer + lpc_size);
    float *their_work = (float*)(*extrapolateBuffer + lpc_size + my_work_size);
    
    for (size_t ch = 0; ch < channels; ch++)
    {
        if (frameSize - size > extrapolate_order * 2)
        {
            float *chPcmBuf = buffer + ch + (backward ? frameSize : -1) * channels;
            for (size_t i = 0; i < frameSize; i++) work[i] = *(chPcmBuf += delta);
            
            vorbis_lpc_from_data(work, lpc, (int)(frameSize - size), extrapolate_order);
            
            vorbis_lpc_predict(lpc, work + frameSize - size - extrapolate_order, extrapolate_order, work + frameSize - size, size, their_work);
            
            chPcmBuf = buffer + ch + (backward ? frameSize : -1) * channels;
            for (size_t i = 0; i < frameSize; i++) *(chPcmBuf += delta) = work[i];
        }
    }
}

-(void)process
{
	char writeBuf[CHUNK_SIZE];	
	
    // Removed endOfStream check from here, since we want to be able to flush the converter
    // when the end of stream is reached. Convert function instead processes what it can,
    // and returns 0 samples when it has nothing more to process at the end of stream.
	while ([self shouldContinue] == YES)
	{
		int amountConverted = [self convert:writeBuf amount:CHUNK_SIZE];
        if (!amountConverted)
        {
            if (paused)
            {
                while (paused)
                    usleep(500);
                continue;
            }
            else if (refillNode)
            {
                // refill node just ended, file resumes
                [self setPreviousNode:originalPreviousNode];
                [self setEndOfStream:NO];
                [self setShouldContinue:YES];
                refillNode = nil;
                [self cleanUp];
                [self setupWithInputFormat:rememberedInputFormat outputFormat:outputFormat];
                continue;
            }
            else break;
        }
		[self writeData:writeBuf amount:amountConverted];
	}
}

- (int)convert:(void *)dest amount:(int)amount
{	
	UInt32 ioNumberPackets;
    int amountReadFromFC;
    int amountRead = 0;
    int extrapolateStart = 0;
    int extrapolateEnd = 0;
    int eatFromEnd = 0;
    size_t amountToSkip = 0;

    if (stopping)
        return 0;
    
    convertEntered = YES;
    
tryagain:
    if (stopping || [self shouldContinue] == NO)
    {
        convertEntered = NO;
        return amountRead;
    }
    
    amountReadFromFC = 0;
    
    if (floatOffset == floatSize) // skip this step if there's still float buffered
    while (inpOffset == inpSize) {
        size_t samplesRead = 0;
        
        BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
        BOOL isUnsigned = !isFloat && !(inputFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger);

        uint8_t fillByte = (dsd2pcm) ? 0x55 : ((isUnsigned) ? 0x80 : 0x00);
        
        // Approximately the most we want on input
        ioNumberPackets = (amount - amountRead) / outputFormat.mBytesPerPacket;
        
        // We want to upscale this count if the ratio is below zero
        if (sampleRatio < 1.0)
        {
            ioNumberPackets = ((uint32_t)(ioNumberPackets / sampleRatio) + 15) & ~15;
        }
        
        size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
        if (!inputBuffer || inputBufferSize < newSize)
            inputBuffer = realloc( inputBuffer, inputBufferSize = newSize * 3 );
        
        ssize_t amountToWrite = ioNumberPackets * inputFormat.mBytesPerPacket;
        amountToSkip = 0;
        
        if (!skipResampler)
        {
            if (latencyStarted < 0)
            {
                latencyStarted = resampler->latency(resampler_data);
                extrapolateStart = (int)latencyStarted;
                if (dsd2pcm) extrapolateStart += dsd2pcmLatency;
            }

            if (latencyStarted)
            {
                ssize_t latencyToWrite = latencyStarted * inputFormat.mBytesPerPacket;
                if (latencyToWrite > amountToWrite)
                    latencyToWrite = amountToWrite;
                
                memset(inputBuffer, fillByte, latencyToWrite);

                amountToSkip = latencyToWrite;
                amountToWrite -= amountToSkip;

                latencyEaten = (int)ceil(extrapolateStart * sampleRatio);

                latencyStarted -= latencyToWrite / inputFormat.mBytesPerPacket;
            }
        }
        
        ssize_t bytesReadFromInput = 0;
        
        while (bytesReadFromInput < amountToWrite && !stopping && [self shouldContinue] == YES && [self endOfStream] == NO)
        {
            size_t bytesRead = [self readData:inputBuffer + amountToSkip + bytesReadFromInput amount:(int)(amountToWrite - bytesReadFromInput)];
            bytesReadFromInput += bytesRead;
            if (!bytesRead)
            {
                if (refillNode)
                    [self setEndOfStream:YES];
                else
                    usleep(500);
            }
        }
        
        // Pad end of track with input format silence
        
        if (stopping || [self shouldContinue] == NO || [self endOfStream] == YES)
        {
            if (!skipResampler && !latencyPostfill)
            {
                ioNumberPackets = (int)resampler->latency(resampler_data);
                if (dsd2pcm) ioNumberPackets += dsd2pcmLatency;
                newSize = ioNumberPackets * inputFormat.mBytesPerPacket;
                newSize += bytesReadFromInput;
                if (!inputBuffer || inputBufferSize < newSize)
                    inputBuffer = realloc( inputBuffer, inputBufferSize = newSize * 3);
                
                latencyPostfill = YES;
                extrapolateEnd = ioNumberPackets;

                memset(inputBuffer + bytesReadFromInput, fillByte, extrapolateEnd * inputFormat.mBytesPerPacket);

                bytesReadFromInput = newSize;

                if (dsd2pcm)
                {
                    eatFromEnd = (int)ceil(dsd2pcmLatency * sampleRatio);
                }
            }
        }
        
        if (!bytesReadFromInput) {
            convertEntered = NO;
            return amountRead;
        }

        bytesReadFromInput += amountToSkip;

        if (bytesReadFromInput &&
            (inputFormat.mFormatFlags & kAudioFormatFlagIsBigEndian))
        {
            // Time for endian swap!
            convert_be_to_le(inputBuffer, inputFormat.mBitsPerChannel, bytesReadFromInput);
        }
        
        if (bytesReadFromInput && isFloat && inputFormat.mBitsPerChannel == 64)
        {
            // Time for precision loss from weird inputs
            samplesRead = bytesReadFromInput / sizeof(double);
            convert_f64_to_f32(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
            memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * sizeof(float));
            bytesReadFromInput = samplesRead * sizeof(float);
        }
        
        if (bytesReadFromInput && !isFloat)
        {
            size_t bitsPerSample = inputFormat.mBitsPerChannel;
            if (bitsPerSample == 1) {
                samplesRead = bytesReadFromInput / inputFormat.mBytesPerPacket;
                convert_dsd_to_f32(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead, inputFormat.mChannelsPerFrame, dsd2pcm);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * inputFormat.mChannelsPerFrame * sizeof(float));
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * inputFormat.mChannelsPerFrame * sizeof(float);
                isFloat = YES;
            }
            else if (bitsPerSample <= 8) {
                samplesRead = bytesReadFromInput;
                if (!isUnsigned)
                    convert_s8_to_s16(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                else
                    convert_u8_to_s16(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * 2);
                bitsPerSample = 16;
                bytesReadFromInput = samplesRead * 2;
                isUnsigned = NO;
            }
            if (bitsPerSample <= 16) {
                samplesRead = bytesReadFromInput / 2;
                if (isUnsigned)
                    convert_u16_to_s16(inputBuffer, samplesRead);
                convert_s16_to_float(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead, 1.0);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * sizeof(float));
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * sizeof(float);
                isUnsigned = NO;
                isFloat = YES;
            }
            else if (bitsPerSample <= 24) {
                samplesRead = bytesReadFromInput / 3;
                if (isUnsigned)
                    convert_u24_to_s32(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                else
                    convert_s24_to_s32(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * 4);
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * 4;
                isUnsigned = NO;
            }
            if (!isFloat && bitsPerSample <= 32) {
                samplesRead = bytesReadFromInput / 4;
                if (isUnsigned)
                    convert_u32_to_s32(inputBuffer, samplesRead);
                convert_s32_to_float(inputBuffer + bytesReadFromInput, inputBuffer, samplesRead, 1.0);
                memmove(inputBuffer, inputBuffer + bytesReadFromInput, samplesRead * sizeof(float));
                bitsPerSample = 32;
                bytesReadFromInput = samplesRead * sizeof(float);
                isUnsigned = NO;
                isFloat = YES;
            }
        }
        
        // Extrapolate start
        if (extrapolateStart)
        {
            extrapolate( inputBuffer, floatFormat.mChannelsPerFrame, bytesReadFromInput / floatFormat.mBytesPerPacket, extrapolateStart, YES, &extrapolateBuffer, &extrapolateBufferSize);
            extrapolateStart = 0;
        }

        if (extrapolateEnd)
        {
            extrapolate( inputBuffer, floatFormat.mChannelsPerFrame, bytesReadFromInput / floatFormat.mBytesPerPacket, extrapolateEnd, NO, &extrapolateBuffer, &extrapolateBufferSize);
            extrapolateEnd = 0;
        }
        
        // Input now contains bytesReadFromInput worth of floats, in the input sample rate
        inpSize = bytesReadFromInput;
        inpOffset = 0;
    }
    
    if (inpOffset != inpSize && floatOffset == floatSize)
    {
        struct resampler_data src_data;
        
        size_t inputSamples = (inpSize - inpOffset) / floatFormat.mBytesPerPacket;
        
        ioNumberPackets = (UInt32)inputSamples;
        
        ioNumberPackets = (UInt32)ceil((float)ioNumberPackets * sampleRatio);
        ioNumberPackets = (ioNumberPackets + 255) & ~255;
        
        size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
        if (newSize < (ioNumberPackets * dmFloatFormat.mBytesPerPacket))
            newSize = ioNumberPackets * dmFloatFormat.mBytesPerPacket;
        if (!floatBuffer || floatBufferSize < newSize)
            floatBuffer = realloc( floatBuffer, floatBufferSize = newSize * 3 );
        
        if (stopping)
        {
            convertEntered = NO;
            return 0;
        }

        src_data.data_out      = floatBuffer;
        src_data.output_frames = 0;
        
        src_data.data_in       = (float*)(((uint8_t*)inputBuffer) + inpOffset);
        src_data.input_frames  = inputSamples;
        
        src_data.ratio         = sampleRatio;
        
        if (!skipResampler)
        {
            resampler->process(resampler_data, &src_data);
        }
        else
        {
            memcpy(src_data.data_out, src_data.data_in, inputSamples * floatFormat.mBytesPerPacket);
            src_data.output_frames = inputSamples;
        }
        
        inpOffset += inputSamples * floatFormat.mBytesPerPacket;
        
        if (!skipResampler)
        {
            if (latencyEaten)
            {
                if (src_data.output_frames > latencyEaten)
                {
                    src_data.output_frames -= latencyEaten;
                    memmove(src_data.data_out, src_data.data_out + latencyEaten * inputFormat.mChannelsPerFrame, src_data.output_frames * floatFormat.mBytesPerPacket);
                    latencyEaten = 0;
                }
                else
                {
                    latencyEaten -= src_data.output_frames;
                    src_data.output_frames = 0;
                }
            }
            else if (eatFromEnd)
            {
                if (src_data.output_frames > eatFromEnd)
                {
                    src_data.output_frames -= eatFromEnd;
                }
                else
                {
                    src_data.output_frames = 0;
                }
                eatFromEnd = 0;
            }
        }
        
        amountReadFromFC = (int)(src_data.output_frames * floatFormat.mBytesPerPacket);
        
        scale_by_volume( (float*) floatBuffer, amountReadFromFC / sizeof(float), volumeScale);
        
        if ( inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2 )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            downmix_to_stereo( (float*) floatBuffer, inputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float) * 2;
        }
        else if ( inputFormat.mChannelsPerFrame > 1 && outputFormat.mChannelsPerFrame == 1 )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            downmix_to_mono( (float*) floatBuffer, inputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float);
        }
        else if ( inputFormat.mChannelsPerFrame < outputFormat.mChannelsPerFrame )
        {
            int samples = amountReadFromFC / floatFormat.mBytesPerFrame;
            upmix( (float*) floatBuffer, inputFormat.mChannelsPerFrame, outputFormat.mChannelsPerFrame, samples );
            amountReadFromFC = samples * sizeof(float) * outputFormat.mChannelsPerFrame;
        }
        
        floatSize = amountReadFromFC;
        floatOffset = 0;
    }
    
    if (floatOffset == floatSize)
        goto tryagain;

    ioNumberPackets = (amount - amountRead);
    if (ioNumberPackets > (floatSize - floatOffset))
        ioNumberPackets = (UInt32)(floatSize - floatOffset);
    
    memcpy(dest + amountRead, floatBuffer + floatOffset, ioNumberPackets);
    
    floatOffset += ioNumberPackets;
    amountRead += ioNumberPackets;
    
    convertEntered = NO;
	return amountRead;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
	DLog(@"SOMETHING CHANGED!");
    if ([keyPath isEqual:@"values.volumeScaling"]) {
        //User reset the volume scaling option
        [self refreshVolumeScaling];
    }
    else if ([keyPath isEqual:@"values.outputResampling"]) {
        // Reset resampler
        if (resampler && resampler_data) {
            NSString *value = [[NSUserDefaults standardUserDefaults] stringForKey:@"outputResampling"];
            if (![value isEqualToString:outputResampling])
                [self inputFormatDidChange:inputFormat];
        }
    }
}

static float db_to_scale(float db)
{
    return pow(10.0, db / 20);
}

- (void)refreshVolumeScaling
{
    if (rgInfo == nil)
    {
        volumeScale = 1.0;
        return;
    }
    
    NSString * scaling = [[NSUserDefaults standardUserDefaults] stringForKey:@"volumeScaling"];
    BOOL useAlbum = [scaling hasPrefix:@"albumGain"];
    BOOL useTrack = useAlbum || [scaling hasPrefix:@"trackGain"];
    BOOL useVolume = useAlbum || useTrack || [scaling isEqualToString:@"volumeScale"];
    BOOL usePeak = [scaling hasSuffix:@"WithPeak"];
    float scale = 1.0;
    float peak = 0.0;
    if (useVolume) {
        id pVolumeScale = [rgInfo objectForKey:@"volume"];
        if (pVolumeScale != nil)
            scale = [pVolumeScale floatValue];
    }
    if (useTrack) {
        id trackGain = [rgInfo objectForKey:@"replayGainTrackGain"];
        id trackPeak = [rgInfo objectForKey:@"replayGainTrackPeak"];
        if (trackGain != nil)
            scale = db_to_scale([trackGain floatValue]);
        if (trackPeak != nil)
            peak = [trackPeak floatValue];
    }
    if (useAlbum) {
        id albumGain = [rgInfo objectForKey:@"replayGainAlbumGain"];
        id albumPeak = [rgInfo objectForKey:@"replayGainAlbumPeak"];
        if (albumGain != nil)
            scale = db_to_scale([albumGain floatValue]);
        if (albumPeak != nil)
            peak = [albumPeak floatValue];
    }
    if (usePeak) {
        if (scale * peak > 1.0)
            scale = 1.0 / peak;
    }
    volumeScale = scale;
}


- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf outputFormat:(AudioStreamBasicDescription)outf
{
	//Make the converter
	inputFormat = inf;
	outputFormat = outf;
    
    // These are the only sample formats we support translating
    BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
    if ((!isFloat && !(inputFormat.mBitsPerChannel >= 1 && inputFormat.mBitsPerChannel <= 32))
        || (isFloat && !(inputFormat.mBitsPerChannel == 32 || inputFormat.mBitsPerChannel == 64)))
        return NO;
    
    // These are really placeholders, as we're doing everything internally now
    
    floatFormat = inputFormat;
    floatFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    floatFormat.mBitsPerChannel = 32;
    floatFormat.mBytesPerFrame = (32/8)*floatFormat.mChannelsPerFrame;
    floatFormat.mBytesPerPacket = floatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;
    
    if (inputFormat.mBitsPerChannel == 1) {
        // Decimate this for speed
        floatFormat.mSampleRate *= 1.0 / 8.0;
        dsd2pcmCount = floatFormat.mChannelsPerFrame;
        dsd2pcm = calloc(dsd2pcmCount, sizeof(void*));
        dsd2pcm[0] = dsd2pcm_alloc();
        dsd2pcmLatency = dsd2pcm_latency(dsd2pcm[0]);
        for (size_t i = 1; i < dsd2pcmCount; ++i)
        {
            dsd2pcm[i] = dsd2pcm_dup(dsd2pcm[0]);
        }
    }
    
    inpOffset = 0;
    inpSize = 0;
    
    floatOffset = 0;
    floatSize = 0;
    
    // This is a post resampler, post-down/upmix format
    
    dmFloatFormat = floatFormat;
    dmFloatFormat.mSampleRate = outputFormat.mSampleRate;
    dmFloatFormat.mChannelsPerFrame = outputFormat.mChannelsPerFrame;
    dmFloatFormat.mBytesPerFrame = (32/8)*dmFloatFormat.mChannelsPerFrame;
    dmFloatFormat.mBytesPerPacket = dmFloatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;

    convert_s16_to_float_init_simd();
    convert_s32_to_float_init_simd();
    
    skipResampler = outputFormat.mSampleRate == floatFormat.mSampleRate;
    
    sampleRatio = (double)outputFormat.mSampleRate / (double)floatFormat.mSampleRate;
    
    if (!skipResampler)
    {
        enum resampler_quality quality = RESAMPLER_QUALITY_DONTCARE;
        
        NSString * resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"outputResampling"];
        if ([resampling isEqualToString:@"lowest"])
            quality = RESAMPLER_QUALITY_LOWEST;
        else if ([resampling isEqualToString:@"lower"])
            quality = RESAMPLER_QUALITY_LOWER;
        else if ([resampling isEqualToString:@"normal"])
            quality = RESAMPLER_QUALITY_NORMAL;
        else if ([resampling isEqualToString:@"higher"])
            quality = RESAMPLER_QUALITY_HIGHER;
        else if ([resampling isEqualToString:@"highest"])
            quality = RESAMPLER_QUALITY_HIGHEST;
        
        outputResampling = resampling;
        
        if (!retro_resampler_realloc(&resampler_data, &resampler, "sinc", quality, inputFormat.mChannelsPerFrame, sampleRatio))
        {
            return NO;
        }
    
        latencyStarted = -1;
        latencyEaten = 0;
        latencyPostfill = NO;
    }

	PrintStreamDesc(&inf);
	PrintStreamDesc(&outf);

    [self refreshVolumeScaling];
    
    // Move this here so process call isn't running the resampler until it's allocated
    stopping = NO;
    convertEntered = NO;
    paused = NO;
    outputFormatChanged = NO;
    
	return YES;
}

- (void)dealloc
{
	DLog(@"Decoder dealloc");

    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeScaling"];
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.outputResampling"];
    
    paused = NO;
	[self cleanUp];
}


- (void)setOutputFormat:(AudioStreamBasicDescription)format
{
	DLog(@"SETTING OUTPUT FORMAT!");
    previousOutputFormat = outputFormat;
	outputFormat = format;
    outputFormatChanged = YES;
}

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format
{
	DLog(@"FORMAT CHANGED");
    paused = YES;
	[self cleanUp];
    if (outputFormatChanged && ![buffer isEmpty] &&
        memcmp(&outputFormat, &previousOutputFormat, sizeof(outputFormat)) != 0)
    {
        // Transfer previously buffered data, remember input format
        rememberedInputFormat = format;
        originalPreviousNode = previousNode;
        refillNode = [[RefillNode alloc] initWithController:controller previous:nil];
        [self setPreviousNode:refillNode];

        int dataRead = 0;

        for (;;)
        {
            void * ptr;
            dataRead = [buffer lengthAvailableToReadReturningPointer:&ptr];
            if (dataRead) {
                [refillNode writeData:(float*)ptr floatCount:dataRead / sizeof(float)];
                [buffer didReadLength:dataRead];
            }
            else
                break;
        }
        
        for (;;)
        {
            void * ptr;
            BufferChain * bufferChain = controller;
            AudioPlayer * audioPlayer = [bufferChain controller];
            VirtualRingBuffer * buffer = [[audioPlayer output] buffer];
            dataRead = [buffer lengthAvailableToReadReturningPointer:&ptr];
            if (dataRead) {
                [refillNode writeData:(float*)ptr floatCount:dataRead / sizeof(float)];
                [buffer didReadLength:dataRead];
            }
            else
                break;
        }
        
        [self setupWithInputFormat:previousOutputFormat outputFormat:outputFormat];
    }
    else
    {
        [self setupWithInputFormat:format outputFormat:outputFormat];
    }
}

- (void)setRGInfo:(NSDictionary *)rgi
{
    DLog(@"Setting ReplayGain info");
    rgInfo = rgi;
    [self refreshVolumeScaling];
}

- (void)cleanUp
{
    stopping = YES;
    while (convertEntered)
    {
        usleep(500);
    }
    if (resampler && resampler_data)
    {
        resampler->free(resampler, resampler_data);
        resampler = NULL;
        resampler_data = NULL;
    }
    if (dsd2pcm && dsd2pcmCount)
    {
        for (size_t i = 0; i < dsd2pcmCount; ++i)
        {
            dsd2pcm_free(dsd2pcm[i]);
            dsd2pcm[i] = NULL;
        }
        free(dsd2pcm);
        dsd2pcm = NULL;
    }
    if (extrapolateBuffer)
    {
        free(extrapolateBuffer);
        extrapolateBuffer = NULL;
        extrapolateBufferSize = 0;
    }
    if (floatBuffer)
    {
        free(floatBuffer);
        floatBuffer = NULL;
        floatBufferSize = 0;
    }
	if (inputBuffer) {
		free(inputBuffer);
		inputBuffer = NULL;
        inputBufferSize = 0;
	}
    floatOffset = 0;
    floatSize = 0;
}

- (double) secondsBuffered
{
    return ((double)[buffer bufferedLength] / (outputFormat.mSampleRate * outputFormat.mBytesPerPacket));
}

@end
