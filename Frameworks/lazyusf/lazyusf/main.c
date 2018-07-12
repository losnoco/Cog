
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "usf.h"
#include "cpu.h"
#include "memory.h"

#include "usf_internal.h"

void StopEmulation(usf_state_t * state)
{
	//asm("int $3");
	//printf("Arrivederci!\n\n");
	//Release_Memory();
	//exit(0);
	state->cpu_running = 0;
}

void DisplayError (usf_state_t * state, char * Message, ...) {
	va_list ap;
    
    size_t len = strlen( state->error_message );
    
    if ( len )
        state->error_message[ len++ ] = '\n';

	va_start( ap, Message );
	vsprintf( state->error_message + len, Message, ap );
	va_end( ap );
    
    state->last_error = state->error_message;
    StopEmulation( state );

	//printf("Error: %s\n", Msg);
}
