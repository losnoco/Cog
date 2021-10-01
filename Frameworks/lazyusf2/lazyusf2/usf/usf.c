
#include <stdint.h>
#include <string.h>

#include "usf.h"

#include <stdio.h>
#include <stdlib.h>

#include "usf_internal.h"

#include "api/callbacks.h"
#include "main/savestates.h"
#include "r4300/cached_interp.h"
#include "r4300/r4300.h"

#include "resampler.h"

#include "barray.h"

size_t usf_get_state_size()
{
    return sizeof(usf_state_t) + 8192;
}

void usf_clear(void * state)
{
    size_t offset;
    memset(state, 0, usf_get_state_size());
    offset = 4096 - (((uintptr_t)state) & 4095);
    USF_STATE_HELPER->offset_to_structure = offset;

    //USF_STATE->enablecompare = 0;
    //USF_STATE->enableFIFOfull = 0;

    //USF_STATE->enable_hle_audio = 0;

    // Constants, never written to
    USF_STATE->trunc_mode = 0xF3F;
    USF_STATE->round_mode = 0x33F;
    USF_STATE->ceil_mode = 0xB3F;
    USF_STATE->floor_mode = 0x73F;
#ifdef DYNAREC
	USF_STATE->precomp_instr_size = sizeof(precomp_instr);
#endif

    // USF_STATE->g_rom = 0;
    // USF_STATE->g_rom_size = 0;

    USF_STATE->save_state = calloc( 1, 0x80275c );
    USF_STATE->save_state_size = 0x80275c;

    for (offset = 0; offset < 0x10000; offset += 4)
    {
        USF_STATE->EmptySpace[offset / 4] = (uint32_t)((offset << 16) | offset);
    }

    USF_STATE->resampler = resampler_create();

#ifdef DEBUG_INFO
    USF_STATE->debug_log = NULL;
#endif
}

void usf_set_compare(void * state, int enable)
{
    USF_STATE->enablecompare = enable;
}

void usf_set_fifo_full(void * state, int enable)
{
    USF_STATE->enableFIFOfull = enable;
}

void usf_set_hle_audio(void * state, int enable)
{
    USF_STATE->enable_hle_audio = enable;
}

void usf_set_trimming_mode(void * state, int enable)
{
    USF_STATE->enable_trimming_mode = enable;
}

static uint32_t get_le32( const void * _p )
{
    const uint8_t * p = (const uint8_t *) _p;
    return p[0] + p[1] * 0x100 + p[2] * 0x10000 + p[3] * 0x1000000;
}

int usf_upload_section(void * state, const uint8_t * data, size_t size)
{
    uint32_t temp;

    if ( size < 4 ) return -1;
    temp = get_le32( data ); data += 4; size -= 4;

    if(temp == 0x34365253) { //there is a rom section
        uint32_t len, start;

        if ( size < 4 ) return -1;
        len = get_le32( data ); data += 4; size -= 4;

		while(len) {
            if ( size < 4 ) return -1;
            start = get_le32( data ); data += 4; size -= 4;

            if ( start + len > USF_STATE->g_rom_size )
            {
                void * new_rom;
                int old_rom_size = USF_STATE->g_rom_size;
                while ( start + len > USF_STATE->g_rom_size )
                {
                    if (!USF_STATE->g_rom_size)
                        USF_STATE->g_rom_size = 1024 * 1024;
                    else
                        USF_STATE->g_rom_size *= 2;
                }

                new_rom = realloc( USF_STATE->g_rom, USF_STATE->g_rom_size );
                if ( !new_rom )
                    return -1;

                USF_STATE->g_rom = (unsigned char *) new_rom;
                memset(USF_STATE->g_rom + old_rom_size, 0, USF_STATE->g_rom_size - old_rom_size);
            }

            memcpy( USF_STATE->g_rom + start, data, len );
            data += len; size -= len;

            if ( size < 4 ) return -1;
            len = get_le32( data ); data += 4; size -= 4;
		}
	}

    if ( size < 4 ) return -1;
    temp = get_le32( data ); data += 4; size -= 4;

	if(temp == 0x34365253) {
		uint32_t len, start;

        if ( !USF_STATE->save_state ) return -1;

        if ( size < 4 ) return -1;
        len = get_le32( data ); data += 4; size -= 4;

		while(len) {
            if ( size < 4 ) return -1;
            start = get_le32( data ); data += 4; size -= 4;

            if ( size < len ) return -1;
            memcpy( USF_STATE->save_state + start, data, len );
            data += len; size -= len;

            if ( size < 4 ) return -1;
            len = get_le32( data ); data += 4; size -= 4;
		}
	}

	return 0;
}

void usf_upload_rom(void * state, const uint8_t * data, size_t size)
{
    if (USF_STATE->g_rom)
        free(USF_STATE->g_rom);

    USF_STATE->g_rom = (unsigned char *) malloc(size);
    if (USF_STATE->g_rom)
        memcpy(USF_STATE->g_rom, data, size);
    USF_STATE->g_rom_size = (uint32_t) size;
}

void usf_upload_save_state(void * state, const uint8_t * data, size_t size)
{
    if (USF_STATE->save_state)
        free(USF_STATE->save_state);

    USF_STATE->save_state = (unsigned char *) malloc(size);
    if (USF_STATE->save_state)
        memcpy(USF_STATE->save_state, data, size);
    USF_STATE->save_state_size = (uint32_t) size;
}

static int usf_startup(usf_state_t * state)
{
    if (state->g_rom == NULL)
    {
        state->g_rom_size = 0;
    }

    if (state->save_state == NULL)
    {
        DebugMessage(state, 1, "Save State is missing\n");
        return -1;
    }

    // Detect the Ramsize before the memory allocation
	if(get_le32(state->save_state + 4) == 0x400000) {
        void * savestate;
		savestate = realloc(state->save_state, 0x40275c);
        if ( savestate )
            state->save_state = savestate;
        state->save_state_size = 0x40275c;
	}

    open_rom(state);

    if (main_start(state) != M64ERR_SUCCESS)
    {
        DebugMessage(state, 1, "Invalid Project64 Save State\n");
        return -1;
    }

    if (state->enable_trimming_mode)
    {
        state->barray_rom = bit_array_create(state->g_rom_size / 4);
        state->barray_ram_read = bit_array_create(get_le32(state->save_state + 4) / 4);
        state->barray_ram_written_first = bit_array_create(get_le32(state->save_state + 4) / 4);
    }

    state->MemoryState = 1;

    return 0;
}

void usf_set_audio_format(void *opaque, unsigned int frequency, unsigned int bits)
{
    usf_state_t * state = (usf_state_t *)opaque;

    state->SampleRate = frequency;
}

void usf_push_audio_samples(void *opaque, const void * buffer, size_t size)
{
    usf_state_t * state = (usf_state_t *)opaque;

    int16_t * samplePtr = (int16_t *)buffer;
    int16_t * samplesOut;
    size_t samplesTodo;
    size_t i;
    size /= 4;

    samplesTodo = size;
    if (samplesTodo > state->sample_buffer_count)
        samplesTodo = state->sample_buffer_count;
    state->sample_buffer_count -= samplesTodo;

    samplesOut = state->sample_buffer;
    size -= samplesTodo;

    if (samplesOut)
    {
        for (i = 0; i < samplesTodo; ++i)
        {
            *samplesOut++ = samplePtr[1];
            *samplesOut++ = samplePtr[0];
            samplePtr += 2;
        }
        state->sample_buffer = samplesOut;
    }
    else
        samplePtr += samplesTodo * 2;

    if (size)
    {
        samplesTodo = 8192 - state->samples_in_buffer;
        if (samplesTodo > size)
            samplesTodo = size;

        samplesOut = state->samplebuf + state->samples_in_buffer * 2;
        size -= samplesTodo;
        state->samples_in_buffer += samplesTodo;

        for (i = 0; i < samplesTodo; ++i)
        {
            *samplesOut++ = samplePtr[1];
            *samplesOut++ = samplePtr[0];
            samplePtr += 2;
        }

        state->stop = 1;
    }

    if (size)
        DebugMessage(state, 1, "Sample buffer full!");
}

const char * usf_render(void * state, int16_t * buffer, size_t count, int32_t * sample_rate)
{
    USF_STATE->last_error = 0;
    USF_STATE->error_message[0] = '\0';

    if ( !USF_STATE->MemoryState )
    {
        if ( usf_startup( USF_STATE ) < 0 )
            return USF_STATE->last_error;
    }

    if ( USF_STATE->samples_in_buffer )
    {
        size_t do_max = USF_STATE->samples_in_buffer;
        if ( do_max > count )
            do_max = count;

        if ( buffer )
            memcpy( buffer, USF_STATE->samplebuf, sizeof(int16_t) * 2 * do_max );

        USF_STATE->samples_in_buffer -= do_max;

        if ( sample_rate )
            *sample_rate = USF_STATE->SampleRate;

        if ( USF_STATE->samples_in_buffer )
        {
            memmove( USF_STATE->samplebuf, USF_STATE->samplebuf + do_max * 2, sizeof(int16_t) * 2 * USF_STATE->samples_in_buffer );
            return 0;
        }

        if ( buffer )
            buffer += 2 * do_max;
        count -= do_max;
    }

    USF_STATE->sample_buffer = buffer;
    USF_STATE->sample_buffer_count = count;

    USF_STATE->stop = 0;

    main_run(USF_STATE);

    if ( sample_rate )
        *sample_rate = USF_STATE->SampleRate;

    return USF_STATE->last_error;
}

const char * usf_render_resampled(void * state, int16_t * buffer, size_t count, int32_t sample_rate)
{
    if ( !buffer )
    {
        unsigned long samples_buffered = resampler_get_sample_count( USF_STATE->resampler );
        resampler_clear(USF_STATE->resampler);
        if (samples_buffered)
        {
            unsigned long samples_to_remove = samples_buffered;
            if (samples_to_remove > count)
                samples_to_remove = count;
            count -= samples_to_remove;
            while (samples_to_remove--)
                resampler_remove_sample(USF_STATE->resampler);
            if (!count)
                return 0;
        }
        count = (size_t)((uint64_t)count * USF_STATE->SampleRate / sample_rate);
        if (count > USF_STATE->samples_in_buffer_2)
        {
            count -= USF_STATE->samples_in_buffer_2;
            USF_STATE->samples_in_buffer_2 = 0;
        }
        else if (count)
        {
            USF_STATE->samples_in_buffer_2 -= count;
            memmove(USF_STATE->samplebuf2, USF_STATE->samplebuf2 + 8192 - USF_STATE->samples_in_buffer_2 * 2, USF_STATE->samples_in_buffer_2 * sizeof(short) * 2);
            return 0;
        }
        return usf_render(state, buffer, count, NULL);
    }
    while ( count )
    {
        const char * err;

        while ( USF_STATE->samples_in_buffer_2 && resampler_get_free_count(USF_STATE->resampler) )
        {
            int i = 0, j = resampler_get_free_count(USF_STATE->resampler);
            if (j > USF_STATE->samples_in_buffer_2)
                j = (int)USF_STATE->samples_in_buffer_2;
            for (i = 0; i < j; ++i)
            {
                resampler_write_sample(USF_STATE->resampler, USF_STATE->samplebuf2[i*2], USF_STATE->samplebuf2[i*2+1]);
            }
            memmove(USF_STATE->samplebuf2, USF_STATE->samplebuf2 + i * 2, (USF_STATE->samples_in_buffer_2 - i) * sizeof(short) * 2);
            USF_STATE->samples_in_buffer_2 -= i;
        }

        while ( count && resampler_get_sample_count(USF_STATE->resampler) )
        {
            resampler_get_sample(USF_STATE->resampler, buffer, buffer + 1);
            resampler_remove_sample(USF_STATE->resampler);
            buffer += 2;
            --count;
        }

        if (!count)
            break;

        if (USF_STATE->samples_in_buffer_2)
            continue;

        err = usf_render(state, USF_STATE->samplebuf2, 4096, 0);
        if (err)
            return err;

        USF_STATE->samples_in_buffer_2 = 4096;

        resampler_set_rate(USF_STATE->resampler, (float)USF_STATE->SampleRate / (float)sample_rate);
    }

    return 0;
}

void usf_restart(void * state)
{
    if ( USF_STATE->MemoryState )
    {
        r4300_end(USF_STATE);
        if (USF_STATE->enable_trimming_mode)
        {
            bit_array_destroy(USF_STATE->barray_rom);
            bit_array_destroy(USF_STATE->barray_ram_read);
            bit_array_destroy(USF_STATE->barray_ram_written_first);
            USF_STATE->barray_rom = 0;
            USF_STATE->barray_ram_read = 0;
            USF_STATE->barray_ram_written_first = 0;
        }
        USF_STATE->MemoryState = 0;
    }

    USF_STATE->samples_in_buffer = 0;
    USF_STATE->samples_in_buffer_2 = 0;

    resampler_clear(USF_STATE->resampler);
}

void usf_shutdown(void * state)
{
    r4300_end(USF_STATE);
    if (USF_STATE->enable_trimming_mode)
    {
        if (USF_STATE->barray_rom)
            bit_array_destroy(USF_STATE->barray_rom);
        if (USF_STATE->barray_ram_read)
            bit_array_destroy(USF_STATE->barray_ram_read);
        if (USF_STATE->barray_ram_written_first)
            bit_array_destroy(USF_STATE->barray_ram_written_first);
        USF_STATE->barray_rom = 0;
        USF_STATE->barray_ram_read = 0;
        USF_STATE->barray_ram_written_first = 0;
    }
    USF_STATE->MemoryState = 0;
    free(USF_STATE->save_state);
    USF_STATE->save_state = 0;
    close_rom(USF_STATE);
#ifdef DEBUG_INFO
    if (USF_STATE->debug_log)
      fclose(USF_STATE->debug_log);
#endif
    resampler_delete(USF_STATE->resampler);
    USF_STATE->resampler = 0;
}

void * usf_get_rom_coverage_barray(void * state)
{
    return USF_STATE->barray_rom;
}

void * usf_get_ram_coverage_barray(void * state)
{
    return USF_STATE->barray_ram_read;
}

#ifdef DEBUG_INFO
void usf_log_start(void * state)
{
  USF_STATE->debug_log = fopen("/tmp/lazyusf.log", "w");
}

void usf_log_stop(void * state)
{
  if (USF_STATE->debug_log)
  {
    fclose(USF_STATE->debug_log);
    USF_STATE->debug_log = 0;
  }
}
#endif
