#ifndef _DECODE_H
#define _DECODE_H

#include "shorten.h"
#include "shn.h"

#define NUM_DEFAULT_BUFFER_BLOCKS 512L

#ifndef _SHN_CONFIG
#define _SHN_CONFIG

/* First fill out a shn_config struct... */
typedef struct _shn_config
{
	int      error_output_method;
	char    *seek_tables_path;
	char    *relative_seek_tables_path;
	int      verbose;
	int      swap_bytes;
} shn_config;
#endif

/* ... then you can load a file, normally you have to use the functions in this order */
shn_file *shn_load(char *filename, shn_config config);	/* Loads the file in filename and uses config... returns a shn_file context */
int shn_init_decoder(shn_file *this_shn);		/* inits the decoder for this_shn necessary to do shn_read() */

/* shn_get_buffer_block_size() returns the minimal size that read_buffer should have            *
 * blocks should be around 512                                                                  *
 * You have to allocate a buffer with the size returned by shn_get_buffer_block_size() yourself */
int shn_get_buffer_block_size(shn_file *this_shn, int blocks);
unsigned int shn_get_song_length(shn_file *this_shn);	/* returns song length in milliseconds */
unsigned int shn_get_samplerate(shn_file *this_shn);	/* returns the number of samples per second */
unsigned int shn_get_channels(shn_file *this_shn);	/* returns the number of channels of the audio file */
unsigned int shn_get_bitspersample(shn_file *this_shn);	/* returns the number of bits per sample */

/* Play with the shorten file */
int shn_read(shn_file *this_shn, uchar *read_buffer, int bytes_to_read);	/* bytes_to_read should be the size returned by shn_get_buffer_block_size */
int shn_seekable(shn_file *this_shn);						/* Returns 1 if file is seekables (has seek tables) otherwise 0 */
int shn_seek(shn_file *this_shn, unsigned int time);				/* Seek to position "time" in seconds */

/* Unload everything */
int shn_cleanup_decoder(shn_file *this_shn);					/* Frees some buffers */
void shn_unload(shn_file *this_shn);						/* Unloads the file */

#endif
