
#include <stdint.h>
#include <string.h>

#include "usf.h"
#include "cpu.h"
#include "memory.h"
#include "audio.h"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

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

static int usf_startup(usf_state_t * state)
{
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

void usf_restart(void * state)
{
    if ( USF_STATE->MemoryState )
        StartEmulationFromSave(USF_STATE, USF_STATE->savestatespace);
    
    USF_STATE->samples_in_buffer = 0;
}

void usf_shutdown(void * state)
{
	Release_Memory(USF_STATE);
#ifdef DEBUG_INFO
    fclose(USF_STATE->debug_log);
#endif
}
