//
//  umx.mm
//  Dumb Unreal Archive parser
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#include "umr.h"

#include "umx.h"

class umr_mem_reader : public umr::file_reader
{
	const void * ptr;
	long offset, size;
public:
	umr_mem_reader(const void * buf, unsigned p_size) : ptr(buf), size(p_size), offset(0) {}
    
	long read( void * buf, long howmany )
	{
		long max = size - offset;
		if ( max > howmany ) max = howmany;
		if ( max )
		{
			memcpy( buf, (const uint8_t *)ptr + offset, max );
			offset += max;
		}
		return max;
	}
    
	void seek( long where )
	{
		if ( where > size ) offset = size;
		else offset = where;
	}
};

void * unpackUmx( const void * in, long * size )
{
    umr_mem_reader memreader(in, *size);
    umr::upkg pkg;
    if (pkg.open(&memreader))
    {
        for (int i = 1, j = pkg.ocount(); i <= j; i++)
        {
            char * classname = pkg.oclassname(i);
            if (classname && !strcmp(pkg.oclassname(i), "Music"))
            {
                char * type = pkg.otype(i);
                if (!type) continue;
                if (!strcasecmp(type, "it") || !strcasecmp(type, "s3m") || !strcasecmp(type, "xm"))
                {
                    *size = pkg.object_size(i);
                    void * ret = malloc( *size );
                    memcpy( ret, (const uint8_t *)in + pkg.object_offset(i), *size );
                    return ret;
                }
            }
        }
    }
    
    return NULL;
}
