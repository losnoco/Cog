#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#include "decode.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

int shn_seek(shn_file *this_shn, unsigned int time);
int shn_seekable(shn_file *this_shn);
shn_file *shn_load(char *filename, shn_config config);
int shn_init_decoder(shn_file *this_shn);
int shn_cleanup_decoder(shn_file *this_shn);
unsigned int shn_get_song_length(shn_file *this_shn);
int shn_get_buffer_block_size(shn_file *this_shn, int blocks);
int shn_read(shn_file *this_shn, uchar *read_buffer, int bytes_to_read);
void shn_unload(shn_file *this_shn);
unsigned int shn_get_samplerate(shn_file *this_shn);
unsigned int shn_get_channels(shn_file *this_shn);
unsigned int shn_get_bitspersample(shn_file *this_shn);
static void swap_bytes(shn_file *this_shn,int bytes);
static int get_wave_header(shn_file *this_shn);
static int shn_init_decode_state(shn_file *this_shn);
static int write_to_buffer(shn_file *this_shn, uchar *read_buffer, int bytes_to_read);

static int buffer_is_full = 0;
static int buffer_ret = 0;

static slong **buffer = NULL, **offset = NULL;
static slong lpcqoffset = 0;
static int version = FORMAT_VERSION, bitshift = 0;
static int ftype = TYPE_EOF;
static char *magic = MAGIC;
static int blocksize = DEFAULT_BLOCK_SIZE, nchan = DEFAULT_NCHAN;
static int i, chan, nwrap, nskip = DEFAULT_NSKIP;
static int *qlpc = NULL, maxnlpc = DEFAULT_MAXNLPC, nmean = UNDEFINED_UINT;
static int cmd;
static int internal_ftype;
static int cklen;
static uchar tmp;
static ulong seekto_offset;

int shn_seekable(shn_file *this_shn) {
	if(!this_shn)
		return 0;

	if(this_shn->vars.seek_table_entries == NO_SEEK_TABLE) {
		shn_debug(this_shn->config, "File not seekable");
		return 0;
	}

	/* File is seekable */
	return 1;
}

int shn_seek(shn_file *this_shn, unsigned int time)
{
	if (NULL == this_shn)
		return 0;

	if (this_shn->vars.seek_table_entries == NO_SEEK_TABLE)
	{
		shn_error(this_shn->config, "Cannot seek to %d:%02d because there is no seek information for this file.",time/60,time%60);
		return 0;
	}

	if(time > (unsigned int)(shn_get_song_length(this_shn) / 1000)) {
		shn_error(this_shn->config, "You cannot seek to this position! It is out of range!");
		return 0;
	}
	this_shn->vars.seek_to = time;

	this_shn->vars.eof = FALSE;
	
	return 1;
}

static void swap_bytes(shn_file *this_shn,int bytes)
{
	int i;
	uchar tmp;

	for (i=0;i<bytes;i=i+2) {
		tmp = this_shn->vars.buffer[i+1];
		this_shn->vars.buffer[i+1] = this_shn->vars.buffer[i];
		this_shn->vars.buffer[i] = tmp;
	}
}

static int get_wave_header(shn_file *this_shn)
{
	if(!this_shn)
		return 0;
	slong  **buffer = NULL, **offset = NULL;
	slong  lpcqoffset = 0;
	int   version = FORMAT_VERSION, bitshift = 0;
	int   ftype = TYPE_EOF;
	char  *magic = MAGIC;
	int   blocksize = DEFAULT_BLOCK_SIZE, nchan = DEFAULT_NCHAN;
	int   i, chan, nwrap, nskip = DEFAULT_NSKIP;
	int   *qlpc = NULL, maxnlpc = DEFAULT_MAXNLPC, nmean = UNDEFINED_UINT;
	int   cmd;
	int   internal_ftype;
	int   cklen;
	int   retval = 0;

	if (!shn_init_decode_state(this_shn))
		return 0;

    /***********************/
    /* EXTRACT starts here */
    /***********************/

    /* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
    if(FORMAT_VERSION < 2)
    {
      for(i = 0; i < strlen(magic); i++) {
        if(getc_exit(this_shn->vars.fd) != magic[i])
          return 0;
        this_shn->vars.bytes_read++;
      }

      /* get version number */
      version = getc_exit(this_shn->vars.fd);
      this_shn->vars.bytes_read++;
    }
    else
#endif /* STRICT_FORMAT_COMPATABILITY */
    {
      int nscan = 0;

      version = MAX_VERSION + 1;
      while(version > MAX_VERSION)
      {
        int byte = getc(this_shn->vars.fd);
        this_shn->vars.bytes_read++;
        if(byte == EOF)
          return 0;
        if(magic[nscan] != '\0' && byte == magic[nscan])
          nscan++;
        else
          if(magic[nscan] == '\0' && byte <= MAX_VERSION)
            version = byte;
        else
        {
          if(byte == magic[0])
            nscan = 1;
          else
          {
            nscan = 0;
          }
         version = MAX_VERSION + 1;
        }
      }
    }

    /* check version number */
    if(version > MAX_SUPPORTED_VERSION)
      return 0;

    /* set up the default nmean, ignoring the command line state */
    nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

    /* initialise the variable length file read for the compressed stream */
    var_get_init(this_shn);
    if (this_shn->vars.fatal_error)
      return 0;

    /* initialise the fixed length file write for the uncompressed stream */
    fwrite_type_init(this_shn);

    /* get the internal file type */
    internal_ftype = UINT_GET(TYPESIZE, this_shn);

    /* has the user requested a change in file type? */
    if(internal_ftype != ftype) {
      if(ftype == TYPE_EOF) {
        ftype = internal_ftype;    /*  no problems here */
      }
      else {           /* check that the requested conversion is valid */
        if(internal_ftype == TYPE_AU1 || internal_ftype == TYPE_AU2 ||
           internal_ftype == TYPE_AU3 || ftype == TYPE_AU1 ||ftype == TYPE_AU2 || ftype == TYPE_AU3)
        {
          retval = 0;
          goto got_enough_data;
        }
      }
    }

    nchan = UINT_GET(CHANSIZE, this_shn);

    /* get blocksize if version > 0 */
    if(version > 0)
    {
      int byte;
      blocksize = UINT_GET((int) (log((double) DEFAULT_BLOCK_SIZE) / M_LN2),this_shn);
      maxnlpc = UINT_GET(LPCQSIZE, this_shn);
      nmean = UINT_GET(0, this_shn);
      nskip = UINT_GET(NSKIPSIZE, this_shn);
      for(i = 0; i < nskip; i++)
      {
	byte = uvar_get(XBYTESIZE,this_shn);
      }
    }
    else
      blocksize = DEFAULT_BLOCK_SIZE;

    nwrap = MAX(NWRAP, maxnlpc);

    /* grab some space for the input buffer */
    buffer  = long2d((ulong) nchan, (ulong) (blocksize + nwrap),this_shn);
    if (this_shn->vars.fatal_error)
      return 0;
    offset  = long2d((ulong) nchan, (ulong) MAX(1, nmean),this_shn);
    if (this_shn->vars.fatal_error) {
      if (buffer) {
        free(buffer);
        buffer = NULL;
      }
      return 0;
    }

    for(chan = 0; chan < nchan; chan++)
    {
      for(i = 0; i < nwrap; i++)
      	buffer[chan][i] = 0;
      buffer[chan] += nwrap;
    }

    if(maxnlpc > 0) {
      qlpc = (int*) pmalloc((ulong) (maxnlpc * sizeof(*qlpc)),this_shn);
      if (this_shn->vars.fatal_error) {
        if (buffer) {
          free(buffer);
          buffer = NULL;
        }
        if (offset) {
          free(offset);
          buffer = NULL;
        }
        return 0;
      }
    }

    if(version > 1)
      lpcqoffset = V2LPCQOFFSET;

    init_offset(offset, nchan, MAX(1, nmean), internal_ftype);

    /* get commands from file and execute them */
    chan = 0;
    while(1)
    {
      this_shn->vars.reading_function_code = 1;
      cmd = uvar_get(FNSIZE,this_shn);
      this_shn->vars.reading_function_code = 0;

        switch(cmd)
        {
          case FN_ZERO:
          case FN_DIFF0:
          case FN_DIFF1:
          case FN_DIFF2:
          case FN_DIFF3:
          case FN_QLPC:
          {
            slong coffset, *cbuffer = buffer[chan];
            int resn = 0, nlpc, j;

            if(cmd != FN_ZERO)
            {
              resn = uvar_get(ENERGYSIZE,this_shn);
              if (this_shn->vars.fatal_error) {
                retval = 0;
                goto got_enough_data;
              }
              /* this is a hack as version 0 differed in definition of var_get */
              if(version == 0)
                resn--;
            }

            /* find mean offset : N.B. this code duplicated */
            if(nmean == 0)
              coffset = offset[chan][0];
            else
            {
              slong sum = (version < 2) ? 0 : nmean / 2;
              for(i = 0; i < nmean; i++)
                sum += offset[chan][i];
              if(version < 2)
                coffset = sum / nmean;
              else
                coffset = ROUNDEDSHIFTDOWN(sum / nmean, bitshift);
            }

            switch(cmd)
            {
              case FN_ZERO:
                for(i = 0; i < blocksize; i++)
                  cbuffer[i] = 0;
                break;
              case FN_DIFF0:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn,this_shn) + coffset;
                  if (this_shn->vars.fatal_error) {
                    retval = 0;
                    goto got_enough_data;
                  }
                }
                break;
              case FN_DIFF1:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn,this_shn) + cbuffer[i - 1];
                  if (this_shn->vars.fatal_error) {
                    retval = 0;
                    goto got_enough_data;
                  }
                }
                break;
              case FN_DIFF2:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn,this_shn) + (2 * cbuffer[i - 1] -	cbuffer[i - 2]);
                  if (this_shn->vars.fatal_error) {
                    retval = 0;
                    goto got_enough_data;
                  }
                }
                break;
              case FN_DIFF3:
                for(i = 0; i < blocksize; i++) {
                  cbuffer[i] = var_get(resn,this_shn) + 3 * (cbuffer[i - 1] -  cbuffer[i - 2]) + cbuffer[i - 3];
                  if (this_shn->vars.fatal_error) {
                    retval = 0;
                    goto got_enough_data;
                  }
                }
                break;
              case FN_QLPC:
                nlpc = uvar_get(LPCQSIZE,this_shn);
                if (this_shn->vars.fatal_error) {
                  retval = 0;
                  goto got_enough_data;
                }

                for(i = 0; i < nlpc; i++) {
                  qlpc[i] = var_get(LPCQUANT,this_shn);
                  if (this_shn->vars.fatal_error) {
                    retval = 0;
                    goto got_enough_data;
                  }
                }
                for(i = 0; i < nlpc; i++)
                  cbuffer[i - nlpc] -= coffset;
                for(i = 0; i < blocksize; i++)
                {
                  slong sum = lpcqoffset;

                  for(j = 0; j < nlpc; j++)
                    sum += qlpc[j] * cbuffer[i - j - 1];
                  cbuffer[i] = var_get(resn,this_shn) + (sum >> LPCQUANT);
                  if (this_shn->vars.fatal_error) {
                    retval = 0;
                    goto got_enough_data;
                  }
                }
                if(coffset != 0)
                  for(i = 0; i < blocksize; i++)
                    cbuffer[i] += coffset;
                break;
            }

            /* store mean value if appropriate : N.B. Duplicated code */
            if(nmean > 0)
            {
              slong sum = (version < 2) ? 0 : blocksize / 2;

              for(i = 0; i < blocksize; i++)
                sum += cbuffer[i];

              for(i = 1; i < nmean; i++)
                offset[chan][i - 1] = offset[chan][i];
              if(version < 2)
                offset[chan][nmean - 1] = sum / blocksize;
              else
                offset[chan][nmean - 1] = (sum / blocksize) << bitshift;
            }

			if (0 == chan) {
              this_shn->vars.initial_file_position = this_shn->vars.last_file_position_no_really;
              goto got_enough_data;
            }

            /* do the wrap */
            for(i = -nwrap; i < 0; i++)
              cbuffer[i] = cbuffer[i + blocksize];

            fix_bitshift(cbuffer, blocksize, bitshift, internal_ftype);

            if(chan == nchan - 1)
            {
              fwrite_type(buffer, ftype, nchan, blocksize, this_shn);
              this_shn->vars.bytes_in_buf = 0;
            }

            chan = (chan + 1) % nchan;
            break;
          }
          break;

          case FN_BLOCKSIZE:
            UINT_GET((int) (log((double) blocksize) / M_LN2), this_shn);
            break;

          case FN_VERBATIM:
            cklen = uvar_get(VERBATIM_CKSIZE_SIZE,this_shn);

            while (cklen--) {
              if (this_shn->vars.bytes_in_header >= OUT_BUFFER_SIZE) {
                  shn_debug(this_shn->config,"Unexpectedly large header - " PACKAGE " can only handle a maximum of %d bytes",OUT_BUFFER_SIZE);
                  goto got_enough_data;
              }
              this_shn->vars.bytes_in_buf = 0;
              this_shn->vars.header[this_shn->vars.bytes_in_header++] = (char)uvar_get(VERBATIM_BYTE_SIZE,this_shn);
            }
            retval = 1;
            break;

          case FN_BITSHIFT:
            bitshift = uvar_get(BITSHIFTSIZE,this_shn);
            this_shn->vars.bitshift = bitshift;
            break;

          default:
            goto got_enough_data;
        }
    }

got_enough_data:

    /* wind up */
    var_get_quit(this_shn);
    fwrite_type_quit(this_shn);

    if (buffer) free((void *) buffer);
    if (offset) free((void *) offset);
    if(maxnlpc > 0 && qlpc)
      free((void *) qlpc);

    this_shn->vars.bytes_in_buf = 0;

    return retval;
}


void shn_unload(shn_file *this_shn)
{
	if (this_shn)
	{
		if (this_shn->vars.fd)
		{
			fclose(this_shn->vars.fd);
			this_shn->vars.fd = NULL;
		}

		if (this_shn->decode_state)
		{
			if (this_shn->decode_state->getbuf)
			{
				free(this_shn->decode_state->getbuf);
				this_shn->decode_state->getbuf = NULL;
			}

			if (this_shn->decode_state->writebuf)
			{
				free(this_shn->decode_state->writebuf);
				this_shn->decode_state->writebuf = NULL;
			}

			if (this_shn->decode_state->writefub)
			{
				free(this_shn->decode_state->writefub);
				this_shn->decode_state->writefub = NULL;
			}

			free(this_shn->decode_state);
			this_shn->decode_state = NULL;
		}

		if (this_shn->seek_table)
		{
			free(this_shn->seek_table);
			this_shn->seek_table = NULL;
		}

		free(this_shn);
		this_shn = NULL;
	}
}

shn_file *shn_load(char *filename, shn_config config)
{
	shn_file *tmp_file;
	shn_seek_entry *first_seek_table;

	if (!(tmp_file = malloc(sizeof(shn_file))))
	{
		fprintf(stderr, "Could not allocate memory for SHN data structure");
		return NULL;
	}

	memset(tmp_file, 0, sizeof(shn_file));

	/*Copying config */
	tmp_file->config = config;
	
	tmp_file->vars.fd = NULL;
	tmp_file->vars.seek_to = -1;
	tmp_file->vars.eof = 0;
	tmp_file->vars.going = 0;
	tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
	tmp_file->vars.bytes_in_buf = 0;
	tmp_file->vars.bytes_in_header = 0;
	tmp_file->vars.reading_function_code = 0;
	tmp_file->vars.initial_file_position = 0;
	tmp_file->vars.last_file_position = 0;
	tmp_file->vars.last_file_position_no_really = 0;
	tmp_file->vars.bytes_read = 0;
	tmp_file->vars.bitshift = 0;
	tmp_file->vars.seek_offset = 0;

	tmp_file->decode_state = NULL;

	tmp_file->wave_header.filename = filename;
	tmp_file->wave_header.wave_format = 0;
	tmp_file->wave_header.channels = 0;
	tmp_file->wave_header.block_align = 0;
	tmp_file->wave_header.bits_per_sample = 0;
	tmp_file->wave_header.samples_per_sec = 0;
	tmp_file->wave_header.avg_bytes_per_sec = 0;
	tmp_file->wave_header.rate = 0;
	tmp_file->wave_header.header_size = 0;
	tmp_file->wave_header.data_size = 0;
	tmp_file->wave_header.file_has_id3v2_tag = 0;
	tmp_file->wave_header.id3v2_tag_size = 0;

	tmp_file->seek_header.version = NO_SEEK_TABLE;
	tmp_file->seek_header.shnFileSize = 0;

	tmp_file->seek_trailer.seekTableSize = 0;

	tmp_file->seek_table = NULL;

	if (!(tmp_file->vars.fd = shn_open_and_discard_id3v2_tag(filename,&tmp_file->wave_header.file_has_id3v2_tag,&tmp_file->wave_header.id3v2_tag_size)))
	{
		shn_debug(tmp_file->config, "Could not open file: '%s'",filename);
		shn_unload(tmp_file);
		return NULL;
	}

	if (0 == get_wave_header(tmp_file))
	{
		shn_debug(tmp_file->config, "Unable to read WAVE header from file '%s'",filename);
		shn_unload(tmp_file);
		return NULL;
	}

	if (tmp_file->wave_header.file_has_id3v2_tag)
	{
		fseek(tmp_file->vars.fd,tmp_file->wave_header.id3v2_tag_size,SEEK_SET);
		tmp_file->vars.bytes_read += tmp_file->wave_header.id3v2_tag_size;
		tmp_file->vars.seek_offset = tmp_file->wave_header.id3v2_tag_size;
	}
    else
	{
		fseek(tmp_file->vars.fd,0,SEEK_SET);
	}

	if (0 == shn_verify_header(tmp_file))
	{
		shn_debug(tmp_file->config, "Invalid WAVE header in file: '%s'",filename);
		shn_unload(tmp_file);
		return NULL;
	}

	if (tmp_file->decode_state)
	{
		free(tmp_file->decode_state);
		tmp_file->decode_state = NULL;
	}

	shn_load_seek_table(tmp_file,filename);

	if (NO_SEEK_TABLE != tmp_file->vars.seek_table_entries)
	{
		first_seek_table = (shn_seek_entry *)tmp_file->seek_table;
	
		/* check for broken seek tables - if found, disable seeking */
		if (0 == tmp_file->seek_header.version)
		{
			/* test, if the bitshift value in the file is identical to the bitshift value of the first seektable entry */
			if (tmp_file->vars.bitshift != shn_uchar_to_ushort_le(first_seek_table->data+22))
			{
				shn_debug(tmp_file->config, "Broken seek table detected - seeking disabled for file '%s'.",tmp_file->wave_header.filename);
				tmp_file->vars.seek_table_entries = NO_SEEK_TABLE;
			}
		}

		tmp_file->vars.seek_offset += tmp_file->vars.initial_file_position - shn_uchar_to_ulong_le(first_seek_table->data+8);

		if (0 != tmp_file->vars.seek_offset)
		{
			shn_debug(tmp_file->config, "Adjusting seek table offsets by %ld bytes due to mismatch between seek table values and input file - seeking might not work correctly.",
				tmp_file->vars.seek_offset);
		}
	} 

	fseek(tmp_file->vars.fd,0,SEEK_SET);
	tmp_file->vars.going = 1;
	tmp_file->vars.seek_to = -1;

	shn_debug(tmp_file->config, "Successfully loaded file: '%s'",filename);
	
	return tmp_file;
}

static int shn_init_decode_state(shn_file *this_shn)
{
	if (this_shn->decode_state)
	{
		if (this_shn->decode_state->getbuf)
		{
			free(this_shn->decode_state->getbuf);
			this_shn->decode_state->getbuf = NULL;
		}

		if (this_shn->decode_state->writebuf)
		{
			free(this_shn->decode_state->writebuf);
			this_shn->decode_state->writebuf = NULL;
		}

		if (this_shn->decode_state->writefub)
		{
			free(this_shn->decode_state->writefub);
			this_shn->decode_state->writefub = NULL;
		}

		free(this_shn->decode_state);
		this_shn->decode_state = NULL;
	}

	if (!(this_shn->decode_state = malloc(sizeof(shn_decode_state))))
	{
		shn_debug(this_shn->config, "Could not allocate memory for decode state data structure");
		return 0;
	}

	this_shn->decode_state->getbuf = NULL;
	this_shn->decode_state->getbufp = NULL;
	this_shn->decode_state->nbitget = 0;
	this_shn->decode_state->nbyteget = 0;
	this_shn->decode_state->gbuffer = 0;
	this_shn->decode_state->writebuf = NULL;
	this_shn->decode_state->writefub = NULL;
	this_shn->decode_state->nwritebuf = 0;

	this_shn->vars.bytes_in_buf = 0;

	return 1;
}

int shn_init_decoder(shn_file *this_shn) {
	if(!this_shn)
		return 0;
	if(!shn_init_decode_state(this_shn)) {
		shn_error(this_shn->config, "shn_init_decode state failed!\n");
		return 0;
	}
	/* read magic number */
#ifdef STRICT_FORMAT_COMPATABILITY
	if(FORMAT_VERSION < 2) {
		for(i = 0; i < strlen(magic); i++)
			if(getc_exit(this_shn->vars.fd) != magic[i]) {
				shn_error_fatal(this_shn,"Bad magic number");
				return 0;
			}

		/* get version number */
		version = getc_exit(this_shn->vars.fd);
	}
	else
#endif /* STRICT_FORMAT_COMPATABILITY */
	{
		int nscan = 0;

		version = MAX_VERSION + 1;
		while(version > MAX_VERSION) {
			int byte = getc(this_shn->vars.fd);
			if(byte == EOF) {
				shn_error_fatal(this_shn,"No magic number");
				return 0;
			}
			if(magic[nscan] != '\0' && byte == magic[nscan])
				nscan++;
			else
				if(magic[nscan] == '\0' && byte <= MAX_VERSION)
					version = byte;
			else {
				if(byte == magic[0])
					nscan = 1;
				else {
					nscan = 0;
				}
				version = MAX_VERSION + 1;
			}
		}
	}

	/* check version number */
	if(version > MAX_SUPPORTED_VERSION) {
		shn_error_fatal(this_shn,"Can't decode version %d", version);
		return 0;
	}

	/* set up the default nmean, ignoring the command line state */
	nmean = (version < 2) ? DEFAULT_V0NMEAN : DEFAULT_V2NMEAN;

	/* initialise the variable length file read for the compressed stream */
	var_get_init(this_shn);
	if (this_shn->vars.fatal_error)
		return 0;

	/* initialise the fixed length file write for the uncompressed stream */
	fwrite_type_init(this_shn);

	/* get the internal file type */
	internal_ftype = UINT_GET(TYPESIZE, this_shn);

	/* has the user requested a change in file type? */
	if(internal_ftype != ftype) {
		if(ftype == TYPE_EOF)
			ftype = internal_ftype;    /*  no problems here */
		else             /* check that the requested conversion is valid */
			if(internal_ftype == TYPE_AU1 || internal_ftype == TYPE_AU2 || internal_ftype == TYPE_AU3 || ftype == TYPE_AU1 ||ftype == TYPE_AU2 || ftype == TYPE_AU3) {
				shn_error_fatal(this_shn,"Not able to perform requested output format conversion");
				return 0;
			}
	}

	nchan = UINT_GET(CHANSIZE, this_shn);

	/* get blocksize if version > 0 */
	if(version > 0) {
		int byte;
		blocksize = UINT_GET((int) (log((double) DEFAULT_BLOCK_SIZE) / M_LN2),this_shn);
		maxnlpc = UINT_GET(LPCQSIZE, this_shn);
		nmean = UINT_GET(0, this_shn);
		nskip = UINT_GET(NSKIPSIZE, this_shn);
		for(i = 0; i < nskip; i++) {
			byte = uvar_get(XBYTESIZE,this_shn);
		}
	}
	else
		blocksize = DEFAULT_BLOCK_SIZE;

	nwrap = MAX(NWRAP, maxnlpc);

	/* grab some space for the input buffer */
	buffer  = long2d((ulong) nchan, (ulong) (blocksize + nwrap),this_shn);
	if (this_shn->vars.fatal_error)
		return 0;
	offset  = long2d((ulong) nchan, (ulong) MAX(1, nmean),this_shn);
	if (this_shn->vars.fatal_error) {
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}
		return 0;
	}

	for(chan = 0; chan < nchan; chan++) {
		for(i = 0; i < nwrap; i++)
			buffer[chan][i] = 0;
		buffer[chan] += nwrap;
	}

	if(maxnlpc > 0) {
		qlpc = (int*) pmalloc((ulong) (maxnlpc * sizeof(*qlpc)),this_shn);
		if (this_shn->vars.fatal_error) {
			if (buffer) {
				free(buffer);
				buffer = NULL;
			}
			if (offset) {
				free(offset);
				buffer = NULL;
			}
			return 0;
		}
	}

	if(version > 1)
		lpcqoffset = V2LPCQOFFSET;

	init_offset(offset, nchan, MAX(1, nmean), internal_ftype);

	this_shn->vars.eof = FALSE;
	chan = 0;

	/* Success */
	return 1;
}

int shn_cleanup_decoder(shn_file *this_shn) {
	if(!this_shn)
		return 0;
	this_shn->vars.seek_to = -1;
	this_shn->vars.eof = TRUE;

/* wind up */
	var_get_quit(this_shn);
	fwrite_type_quit(this_shn);

	if (buffer) free((void *) buffer);
	if (offset) free((void *) offset);
	if(maxnlpc > 0 && qlpc)
	free((void *) qlpc);

	return 1;
}

static int write_to_buffer(shn_file *this_shn, uchar *read_buffer, int block_size)
{
	int bytes_to_write,bytes_in_block,i;

	if (this_shn->vars.bytes_in_buf < block_size)
		return 0;

	bytes_in_block = min(this_shn->vars.bytes_in_buf, block_size);

	if (bytes_in_block <= 0)
		return 0;

	bytes_to_write = bytes_in_block;
	while ((bytes_to_write + bytes_in_block) <= this_shn->vars.bytes_in_buf)
		bytes_to_write += bytes_in_block;

	if(this_shn->vars.going && this_shn->vars.seek_to == -1) {
		if (this_shn->config.swap_bytes)
			swap_bytes(this_shn, bytes_to_write);
		memcpy((uchar *)read_buffer, (uchar *)(this_shn->vars.buffer), bytes_to_write);
	} else
		return 0;

	/* shift data from end of buffer to the front */
	this_shn->vars.bytes_in_buf -= bytes_to_write;

	for(i=0;i<this_shn->vars.bytes_in_buf;i++)
		this_shn->vars.buffer[i] = this_shn->vars.buffer[i+bytes_to_write];

	return bytes_to_write;
}

int shn_get_buffer_block_size(shn_file *this_shn, int blocks) {
	int blk_size = blocks * (this_shn->wave_header.bits_per_sample / 8) * this_shn->wave_header.channels;
	if(blk_size > OUT_BUFFER_SIZE) {
		shn_debug(this_shn->config, "Resetting to default blk_size!\n");
		blk_size = NUM_DEFAULT_BUFFER_BLOCKS * (this_shn->wave_header.bits_per_sample / 8) * this_shn->wave_header.channels;
	}

	return blk_size;
}

unsigned int shn_get_song_length(shn_file *this_shn) {
	if(this_shn) {
		if(this_shn->wave_header.length > 0)
			return (unsigned int)(1000 * this_shn->wave_header.length);
	}
	/* Something failed or just isn't correct */
	return (unsigned int)0;
}
int shn_read(shn_file *this_shn, uchar *read_buffer, int bytes_to_read) {
	if(!this_shn)
		return 0;
	if(!read_buffer)
		return 0;

	/***********************/
	/* EXTRACT starts here */
	/***********************/

	buffer_is_full = 0;
	while(!buffer_is_full) {
		cmd = uvar_get(FNSIZE,this_shn);
		if (this_shn->vars.fatal_error)
			goto cleanup;

		switch(cmd) {
			case FN_ZERO:
			case FN_DIFF0:
			case FN_DIFF1:
			case FN_DIFF2:
			case FN_DIFF3:
			case FN_QLPC:
				{
					slong coffset, *cbuffer = buffer[chan];
					int resn = 0, nlpc, j;

					if(cmd != FN_ZERO) {
						resn = uvar_get(ENERGYSIZE,this_shn);
						if (this_shn->vars.fatal_error)
							goto cleanup;
						/* this is a hack as version 0 differed in definition of var_get */
						if(version == 0)
							resn--;
					}

					/* find mean offset : N.B. this code duplicated */
					if(nmean == 0)
						coffset = offset[chan][0];
					else
					{
						slong sum = (version < 2) ? 0 : nmean / 2;
						for(i = 0; i < nmean; i++)
							sum += offset[chan][i];
						if(version < 2)
							coffset = sum / nmean;
						else
							coffset = ROUNDEDSHIFTDOWN(sum / nmean, bitshift);
					}

					switch(cmd) {
						case FN_ZERO:
							for(i = 0; i < blocksize; i++)
								cbuffer[i] = 0;
							break;
						case FN_DIFF0:
							for(i = 0; i < blocksize; i++) {
								cbuffer[i] = var_get(resn,this_shn) + coffset;
								if (this_shn->vars.fatal_error)
									goto cleanup;
							}
							break;
						case FN_DIFF1:
							for(i = 0; i < blocksize; i++) {
								cbuffer[i] = var_get(resn,this_shn) + cbuffer[i - 1];
								if (this_shn->vars.fatal_error)
									goto cleanup;
							}
							break;
						case FN_DIFF2:
							for(i = 0; i < blocksize; i++) {
								cbuffer[i] = var_get(resn,this_shn) + (2 * cbuffer[i - 1] -	cbuffer[i - 2]);
								if (this_shn->vars.fatal_error)
									goto cleanup;
							}
							break;
						case FN_DIFF3:
							for(i = 0; i < blocksize; i++) {
								cbuffer[i] = var_get(resn,this_shn) + 3 * (cbuffer[i - 1] -  cbuffer[i - 2]) + cbuffer[i - 3];
								if (this_shn->vars.fatal_error)
									goto cleanup;
							}
							break;
						case FN_QLPC:
							nlpc = uvar_get(LPCQSIZE,this_shn);
							if (this_shn->vars.fatal_error)
								goto cleanup;
							
							for(i = 0; i < nlpc; i++) {
								qlpc[i] = var_get(LPCQUANT,this_shn);
								if (this_shn->vars.fatal_error)
									goto cleanup;
							}
							for(i = 0; i < nlpc; i++)
								cbuffer[i - nlpc] -= coffset;
							for(i = 0; i < blocksize; i++) {
								slong sum = lpcqoffset;
								
								for(j = 0; j < nlpc; j++)
									sum += qlpc[j] * cbuffer[i - j - 1];
								cbuffer[i] = var_get(resn,this_shn) + (sum >> LPCQUANT);
								if (this_shn->vars.fatal_error)
									goto cleanup;
							}
							if(coffset != 0)
								for(i = 0; i < blocksize; i++)
									cbuffer[i] += coffset;
							break;
					}

					/* store mean value if appropriate : N.B. Duplicated code */
					if(nmean > 0) {
						slong sum = (version < 2) ? 0 : blocksize / 2;
						
						for(i = 0; i < blocksize; i++)
							sum += cbuffer[i];
						
						for(i = 1; i < nmean; i++)
							offset[chan][i - 1] = offset[chan][i];
						if(version < 2)
							offset[chan][nmean - 1] = sum / blocksize;
						else
							offset[chan][nmean - 1] = (sum / blocksize) << bitshift;
					}
					
					/* do the wrap */
					for(i = -nwrap; i < 0; i++)
						cbuffer[i] = cbuffer[i + blocksize];
					
					fix_bitshift(cbuffer, blocksize, bitshift, internal_ftype);
					
					if(chan == nchan - 1) {
						if (!this_shn->vars.going || this_shn->vars.fatal_error)
							goto cleanup;
						
						fwrite_type(buffer, ftype, nchan, blocksize, this_shn);
						
						buffer_ret = write_to_buffer(this_shn,read_buffer,bytes_to_read);
						if(buffer_ret == bytes_to_read)
							buffer_is_full = 1;
						
						if (this_shn->vars.seek_to != -1) {
							shn_seek_entry *seek_info;
							
							shn_debug(this_shn->config, "Seeking to %d:%02d",this_shn->vars.seek_to/60,this_shn->vars.seek_to%60);
							
							seek_info = shn_seek_entry_search(this_shn->config,this_shn->seek_table,this_shn->vars.seek_to * (ulong)this_shn->wave_header.samples_per_sec,0,(ulong)(this_shn->vars.seek_table_entries - 1),this_shn->vars.seek_resolution);
							
							buffer[0][-1] = shn_uchar_to_slong_le(seek_info->data+24);
							buffer[0][-2] = shn_uchar_to_slong_le(seek_info->data+28);
							buffer[0][-3] = shn_uchar_to_slong_le(seek_info->data+32);
							offset[0][0]  = shn_uchar_to_slong_le(seek_info->data+48);
							offset[0][1]  = shn_uchar_to_slong_le(seek_info->data+52);
							offset[0][2]  = shn_uchar_to_slong_le(seek_info->data+56);
							offset[0][3]  = shn_uchar_to_slong_le(seek_info->data+60);
							if (nchan > 1) {
								buffer[1][-1] = shn_uchar_to_slong_le(seek_info->data+36);
								buffer[1][-2] = shn_uchar_to_slong_le(seek_info->data+40);
								buffer[1][-3] = shn_uchar_to_slong_le(seek_info->data+44);
								offset[1][0]  = shn_uchar_to_slong_le(seek_info->data+64);
								offset[1][1]  = shn_uchar_to_slong_le(seek_info->data+68);
								offset[1][2]  = shn_uchar_to_slong_le(seek_info->data+72);
								offset[1][3]  = shn_uchar_to_slong_le(seek_info->data+76);
							}
							
							bitshift = shn_uchar_to_ushort_le(seek_info->data+22);
							
							seekto_offset = shn_uchar_to_ulong_le(seek_info->data+8) + this_shn->vars.seek_offset;
							
							fseek(this_shn->vars.fd,(slong)seekto_offset,SEEK_SET);
							fread((uchar*) this_shn->decode_state->getbuf, 1, BUFSIZ, this_shn->vars.fd);
							
							this_shn->decode_state->getbufp = this_shn->decode_state->getbuf + shn_uchar_to_ushort_le(seek_info->data+14);
							this_shn->decode_state->nbitget = shn_uchar_to_ushort_le(seek_info->data+16);
							this_shn->decode_state->nbyteget = shn_uchar_to_ushort_le(seek_info->data+12);
							this_shn->decode_state->gbuffer = shn_uchar_to_ulong_le(seek_info->data+18);
							
							this_shn->vars.bytes_in_buf = 0;
							
							this_shn->vars.seek_to = -1;
						}
					}
					chan = (chan + 1) % nchan;
					break;
				}
				break;
			
			case FN_QUIT:
				/* empty out last of buffer */
				buffer_ret = write_to_buffer(this_shn,read_buffer,this_shn->vars.bytes_in_buf);
				if(buffer_ret == bytes_to_read) 
					buffer_is_full = 1;
				
				this_shn->vars.eof = TRUE;
				
				goto cleanup;
				break;
			
			case FN_BLOCKSIZE:
				blocksize = UINT_GET((int) (log((double) blocksize) / M_LN2), this_shn);
				if (this_shn->vars.fatal_error)
					goto cleanup;
				break;
			
			case FN_BITSHIFT:
				bitshift = uvar_get(BITSHIFTSIZE,this_shn);
				if (this_shn->vars.fatal_error)
					goto cleanup;
				break;
			
			case FN_VERBATIM:
				cklen = uvar_get(VERBATIM_CKSIZE_SIZE,this_shn);
				if (this_shn->vars.fatal_error)
					goto cleanup;
				
				while (cklen--) {
					tmp = (uchar)uvar_get(VERBATIM_BYTE_SIZE,this_shn);
					if (this_shn->vars.fatal_error)
						goto cleanup;
				}
				break;
			
			default:
				shn_error_fatal(this_shn,"Sanity check fails trying to decode function: %d",cmd);
				goto cleanup;
		}
	}
	goto exit_func;

cleanup:

    buffer_ret = write_to_buffer(this_shn,read_buffer,this_shn->vars.bytes_in_buf);

exit_func:
    return buffer_ret;

}

unsigned int shn_get_samplerate(shn_file *this_shn) {
	if(this_shn) {
		if(this_shn->wave_header.samples_per_sec > 0)
			return (unsigned int)this_shn->wave_header.samples_per_sec;
	}
	/* Something failed or just isn't correct */
	return (unsigned int)0;
}

unsigned int shn_get_channels(shn_file *this_shn) {
	if(this_shn) {
		if(this_shn->wave_header.channels > 0)
			return (unsigned int)this_shn->wave_header.channels;
	}
	/* Something failed or just isn't correct */
	return (unsigned int)0;
}

unsigned int shn_get_bitspersample(shn_file *this_shn) {
	if(this_shn) {
		if(this_shn->wave_header.bits_per_sample > 0)
			return (unsigned int)this_shn->wave_header.bits_per_sample;
	}
	/* Something failed or just isn't correct */
	return (unsigned int)0;
}

