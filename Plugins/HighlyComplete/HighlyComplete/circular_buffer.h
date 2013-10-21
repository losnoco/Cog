#ifndef _CIRCULAR_BUFFER_H_
#define _CIRCULAR_BUFFER_H_

#include <algorithm>
#include <vector>

long const silence_threshold = 8;

template <typename T>
class circular_buffer
{
    std::vector<T> buffer;
    unsigned long readptr, writeptr, used, size;
    unsigned long silence_count;
public:
    circular_buffer() : readptr( 0 ), writeptr( 0 ), size( 0 ), used( 0 ), silence_count( 0 ) { }
    unsigned long data_available() { return used; }
    unsigned long free_space() { return size - used; }
    T* get_write_ptr( unsigned long & count_out )
    {
        count_out = size - writeptr;
        if ( count_out > size - used ) count_out = size - used;
        return &buffer[writeptr];
    }
    bool samples_written( unsigned long count )
    {
        unsigned long max_count = size - writeptr;
        if ( max_count > size - used ) max_count = size - used;
        if ( count > max_count ) return false;
        silence_count += count_silent( &buffer[ 0 ] + writeptr, &buffer[ 0 ] + writeptr + count );
        used += count;
        writeptr = ( writeptr + count ) % size;
        return true;
    }
    unsigned long read( T * dst, unsigned long count )
    {
        unsigned long done = 0;
        for(;;)
        {
            unsigned long delta = size - readptr;
            if ( delta > used ) delta = used;
            if ( delta > count ) delta = count;
            if ( !delta ) break;

            if ( dst ) std::copy( buffer.begin() + readptr, buffer.begin() + readptr + delta, dst );
            silence_count -= count_silent( &buffer[ 0 ] + readptr, &buffer[ 0 ] + readptr + delta );
            if ( dst ) dst += delta;
            done += delta;
            readptr = ( readptr + delta ) % size;
            count -= delta;
            used -= delta;
        }
        return done;
    }
    void reset()
    {
        readptr = writeptr = used = 0;
    }
    void resize(unsigned long p_size)
    {
        size = p_size;
        buffer.resize( p_size );
        reset();
    }
    bool test_silence() const
    {
        return silence_count == used;
    }
    void remove_leading_silence( unsigned long mask )
    {
        T const* p;
        T const* begin;
        T const* end;
        mask = ~mask;
        if ( used )
        {
            p = begin = &buffer[ 0 ] + readptr - 1;
            end = &buffer[ 0 ] + ( writeptr > readptr ? writeptr : size );
            while ( ++p < end && ( ( unsigned long ) ( *p + silence_threshold ) <= ( unsigned long ) silence_threshold * 2 ) );
            unsigned long skipped = ( p - begin ) & mask;
            silence_count -= skipped;
            used -= skipped;
            readptr = ( readptr + skipped ) % size;
            if ( readptr == 0 && readptr != writeptr )
            {
                p = begin = &buffer[ 0 ];
                end = &buffer[ 0 ] + writeptr;
                while ( ++p < end && ( ( unsigned long ) ( *p + silence_threshold ) <= ( unsigned long ) silence_threshold * 2 ) );
                skipped = ( p - begin ) & mask;
                silence_count -= skipped;
                used -= skipped;
                readptr += skipped;
            }
        }
    }
private:
    static unsigned long count_silent(T const* begin, T const* end)
    {
        unsigned long count = 0;
        T const* p = begin - 1;
        while ( ++p < end ) count += ( ( unsigned long ) ( *p + silence_threshold ) <= ( unsigned long ) silence_threshold * 2 );
        return count;
    }
};

#endif
