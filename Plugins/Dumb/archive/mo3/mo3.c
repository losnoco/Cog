//
//  mo3.c
//  Dumb MO3 Archive parser
//
//  Created by Christopher Snowhill on 11/1/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#include "mo3.h"

#include <stdint.h>
typedef uint32_t DWORD;

#include "unmo3.h"

#include <limits.h>
#include <string.h>

void * unpackMo3( const void * in, long * size )
{
    void * data;
    unsigned int len;
    
    if ( *size < 3 || *size > UINT_MAX )
        return 0;
    
    if ( memcmp( in, "MO3", 3 ) != 0 )
        return 0;
    
    data = (void *) in;
    len = (unsigned int) *size;
    
    if ( UNMO3_Decode( &data, &len, 0 ) != 0 )
        return 0;
    
    *size = len;
    
    return data;
}

void freeMo3( void * in )
{
    UNMO3_Free( in );
}
