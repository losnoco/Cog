
#include <stdint.h>
#include <string.h>

#include "usf.h"
#include "cpu.h"
#include "memory.h"
#include "audio.h"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

#include "resampler.h"

#include "usf_internal.h"

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
    
	//USF_STATE->savestatespace = NULL;
	//USF_STATE->cpu_running = 0;
	USF_STATE->cpu_stopped = 1;
    
    //USF_STATE->enablecompare = 0;
    //USF_STATE->enableFIFOfull = 0;
    
    //USF_STATE->enable_hle_audio = 0;
    
    //USF_STATE->NextInstruction = 0;
    //USF_STATE->JumpToLocation = 0;
    //USF_STATE->AudioIntrReg = 0;
    //USF_STATE->CPU_Action = 0;
    //USF_STATE->Timers = 0;
    //USF_STATE->CPURunning = 0;
    //USF_STATE->SPHack = 0;
    //USF_STATE->WaitMode = 0;
    
    //USF_STATE->TLB_Map = 0;
    //USF_STATE->MemChunk = 0;
    USF_STATE->RdramSize = 0x800000;
    USF_STATE->SystemRdramSize = 0x800000;
    USF_STATE->RomFileSize = 0x4000000;
    
    //USF_STATE->N64MEM = 0;
    //USF_STATE->RDRAM = 0;
    //USF_STATE->DMEM = 0;
    //USF_STATE->IMEM = 0;

    //memset(USF_STATE->ROMPages, 0, sizeof(USF_STATE->ROMPages));
    //USF_STATE->savestatespace = 0;
    //USF_STATE->NOMEM = 0;
    
    //USF_STATE->WrittenToRom = 0;
    //USF_STATE->WroteToRom = 0;
    //USF_STATE->TempValue = 0;
    //USF_STATE->MemoryState = 0;
    //USF_STATE->EmptySpace = 0;
    
    //USF_STATE->Registers = 0;
    
    //USF_STATE->PIF_Ram = 0;

	PreAllocate_Memory(USF_STATE);
    
    USF_STATE->resampler = resampler_create();
    
#ifdef DEBUG_INFO
    USF_STATE->debug_log = fopen("/tmp/lazyusf.log", "w");
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

			while(len) {
				uint32_t page = start >> 16;
				uint32_t readLen = ( ((start + len) >> 16) > page) ? (((page + 1) << 16) - start) : len;

				if( USF_STATE->ROMPages[page] == 0 ) {
					USF_STATE->ROMPages[page] = malloc(0x10000);
                    if ( USF_STATE->ROMPages[page] == 0 )
                        return -1;
                    
					memset(USF_STATE->ROMPages[page], 0, 0x10000);
                }
                
                if ( size < readLen )
                    return -1;
                
                memcpy( USF_STATE->ROMPages[page] + (start & 0xffff), data, readLen );
                data += readLen; size -= readLen;

				start += readLen;
				len -= readLen;
			}

            if ( size < 4 ) return -1;
            len = get_le32( data ); data += 4; size -= 4;
		}
	}

    if ( size < 4 ) return -1;
    temp = get_le32( data ); data += 4; size -= 4;

	if(temp == 0x34365253) {
		uint32_t len, start;
        
        if ( size < 4 ) return -1;
        len = get_le32( data ); data += 4; size -= 4;

		while(len) {
            if ( size < 4 ) return -1;
            start = get_le32( data ); data += 4; size -= 4;

            if ( size < len ) return -1;
            memcpy( USF_STATE->savestatespace + start, data, len );
            data += len; size -= len;

            if ( size < 4 ) return -1;
            len = get_le32( data ); data += 4; size -= 4;
		}
	}

	return 0;
}

static int is_valid_rom(const unsigned char *buffer)
{
    /* Test if rom is a native .z64 image with header 0x80371240. [ABCD] */
    if((buffer[0]==0x80)&&(buffer[1]==0x37)&&(buffer[2]==0x12)&&(buffer[3]==0x40))
        return 1;
    /* Test if rom is a byteswapped .v64 image with header 0x37804012. [BADC] */
    else if((buffer[0]==0x37)&&(buffer[1]==0x80)&&(buffer[2]==0x40)&&(buffer[3]==0x12))
        return 1;
    /* Test if rom is a wordswapped .n64 image with header  0x40123780. [DCBA] */
    else if((buffer[0]==0x40)&&(buffer[1]==0x12)&&(buffer[2]==0x37)&&(buffer[3]==0x80))
        return 1;
    else
        return 0;
}

static void swap_rom(const unsigned char* signature, unsigned char* localrom, int loadlength)
{
    unsigned char temp;
    int i;
    
    /* Btyeswap if .v64 image. */
    if(signature[0]==0x37)
    {
        for (i = 0; i < loadlength; i+=2)
        {
            temp=localrom[i];
            localrom[i]=localrom[i+1];
            localrom[i+1]=temp;
        }
    }
    /* Wordswap if .n64 image. */
    else if(signature[0]==0x40)
    {
        for (i = 0; i < loadlength; i+=4)
        {
            temp=localrom[i];
            localrom[i]=localrom[i+3];
            localrom[i+3]=temp;
            temp=localrom[i+1];
            localrom[i+1]=localrom[i+2];
            localrom[i+2]=temp;
        }
    }
}

static _system_type rom_country_code_to_system_type(unsigned short country_code)
{
    switch (country_code & 0xFF)
    {
            // PAL codes
        case 0x44:
        case 0x46:
        case 0x49:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x58:
        case 0x59:
            return SYSTEM_PAL;
            
            // NTSC codes
        case 0x37:
        case 0x41:
        case 0x45:
        case 0x4a:
        default: // Fallback for unknown codes
            return SYSTEM_NTSC;
    }
}

// Get the VI (vertical interrupt) limit associated to a ROM system type.
static int rom_system_type_to_vi_limit(_system_type system_type)
{
    switch (system_type)
    {
        case SYSTEM_PAL:
        case SYSTEM_MPAL:
            return 50;
            
        case SYSTEM_NTSC:
        default:
            return 60;
    }
}

static int rom_system_type_to_ai_dac_rate(_system_type system_type)
{
    switch (system_type)
    {
        case SYSTEM_PAL:
            return 49656530;
        case SYSTEM_MPAL:
            return 48628316;
        case SYSTEM_NTSC:
        default:
            return 48681812;
    }
}

void open_rom_header(usf_state_t * state, unsigned char * header, int header_size)
{
    if (header_size >= sizeof(_rom_header))
        memcpy(&state->ROM_HEADER, header, sizeof(_rom_header));
    
    if (is_valid_rom((const unsigned char *)&state->ROM_HEADER))
        swap_rom((const unsigned char *)&state->ROM_HEADER, (unsigned char *)&state->ROM_HEADER, sizeof(_rom_header));
    
    /* add some useful properties to ROM_PARAMS */
    state->ROM_PARAMS.systemtype = rom_country_code_to_system_type(state->ROM_HEADER.Country_code);
    state->ROM_PARAMS.vilimit = rom_system_type_to_vi_limit(state->ROM_PARAMS.systemtype);
    state->ROM_PARAMS.aidacrate = rom_system_type_to_ai_dac_rate(state->ROM_PARAMS.systemtype);
    state->ROM_PARAMS.countperop = COUNT_PER_OP_DEFAULT;
}

static int usf_startup(usf_state_t * state)
{
    // Detect region
    
    open_rom_header(state, state->savestatespace + 8, sizeof(_rom_header));
    
    // Detect the Ramsize before the memory allocation
    
	if(get_le32(state->savestatespace + 4) == 0x400000) {
        void * savestate;
		state->RdramSize = 0x400000;
		savestate = realloc(state->savestatespace, 0x40275c);
        if ( savestate )
            state->savestatespace = savestate;
	} else if(get_le32(USF_STATE->savestatespace + 4) == 0x800000)
		state->RdramSize = 0x800000;

	if ( !Allocate_Memory(state) )
        return -1;

	StartEmulationFromSave(state, USF_STATE->savestatespace);
    
    return 0;
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
    
    USF_STATE->cpu_stopped = 0;
	USF_STATE->cpu_running = 1;
    
	StartInterpreterCPU(USF_STATE);
    
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
            if (i)
            {
                memmove(USF_STATE->samplebuf2, USF_STATE->samplebuf2 + i * 2, (USF_STATE->samples_in_buffer_2 - i) * sizeof(short) * 2);
                USF_STATE->samples_in_buffer_2 -= i;
            }
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
        StartEmulationFromSave(USF_STATE, USF_STATE->savestatespace);
    
    USF_STATE->samples_in_buffer = 0;
    USF_STATE->samples_in_buffer_2 = 0;
    
    resampler_clear(USF_STATE->resampler);
}

void usf_shutdown(void * state)
{
	Release_Memory(USF_STATE);
    resampler_delete(USF_STATE->resampler);
#ifdef DEBUG_INFO
    fclose(USF_STATE->debug_log);
#endif
}
