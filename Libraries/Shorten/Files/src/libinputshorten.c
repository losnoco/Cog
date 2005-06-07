/*
 * lamip input plugin - Shorten decoder
 *
 *
 * well... first version is full of memory leaks i guess :)
*/

/* General includes */
#include <plug_in.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

/* General includes for shorten */
#include "decode.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* we declare the functions we use in the plugin here */
static int	shorten_init(const lamipPluginHandle *);
static void	shorten_cleanup(void);
static void	shorten_decode(lamipURL *, int);
static void     shorten_songinfo(lamipURL *,lamipSonginfo *);

/* We set the functions in the InputPlugin struct... */
static InputPlugin shorten_functions;
InputPlugin	*lamip_input_info(void)
{
	shorten_functions.common.name		= "inputSHORTEN";
	shorten_functions.common.description	= "plays *.shn - Shorten";
	shorten_functions.common.init		= shorten_init;
	shorten_functions.common.cleanup	= shorten_cleanup;
	shorten_functions.decode		= shorten_decode;
	shorten_functions.set_song_info		= shorten_songinfo;

	return(&shorten_functions);
}

/* we set the module PCM format in this struct... like every input plugin should do by now */
static lamipPCMInfo pcmi;

/* some functions */

/* all things for shorten decoder */
shn_file *shnfile;
shn_config shn_cfg;
static uchar *real_buffer = (uchar *)NULL;

#define CONFIG_ERROR_OUTPUT_METHOD "error_output_method"
#define CONFIG_SEEK_TABLES_PATH "seek_tables_path"
#define CONFIG_RELATIVE_SEEK_TABLES_PATH "relative_seek_tables_path"
#define CONFIG_VERBOSE "verbose"
#define CONFIG_SWAP_BYTES "swap_bytes"

#define NUM_BUFFER_BLOCKS 4096L

static int	shorten_init(const lamipPluginHandle *handle)
{
	/* Setting when shorten decoder should get active */
	lamipPluginHandle *config = (lamipPluginHandle *)handle;
	lamip_set_mime_type(config, ".shn", NULL);	
	
	/* Initializing the shn_cfg struct, we config it later anyway */
	shn_cfg.error_output_method = ERROR_OUTPUT_DEVNULL;
	shn_cfg.seek_tables_path = NULL;
	shn_cfg.relative_seek_tables_path = NULL;
	shn_cfg.verbose = 0;
	shn_cfg.swap_bytes = 0;

	/* Necessary config variables */
	char *val_error_output_method;
	char *val_seek_tables_path;
	char *val_relative_seek_tables_path;
	int val_verbose;
	int val_swap_bytes;

	/* config: error output */
	if(!lamip_cfg_getExist(handle, CONFIG_ERROR_OUTPUT_METHOD)) {
		lamip_send_message("SHORTEN: shorten_init: no config value for %s found, resetting to default DEVNULL...\n", CONFIG_ERROR_OUTPUT_METHOD);
		lamip_send_message("SHORTEN: shorten_init: possible values for %s are \"DEVNULL\", \"STDERR\"\n", CONFIG_ERROR_OUTPUT_METHOD);
		val_error_output_method = strdup("DEVNULL");
		lamip_cfg_set(config, CONFIG_ERROR_OUTPUT_METHOD, val_error_output_method);
	} else {
		val_error_output_method = lamip_cfg_get(handle, CONFIG_ERROR_OUTPUT_METHOD);
	}
	if(strcasecmp(val_error_output_method, "DEVNULL") == 0)
		shn_cfg.error_output_method = ERROR_OUTPUT_DEVNULL;
	else if(strcasecmp(val_error_output_method, "STDERR") == 0)
		shn_cfg.error_output_method = ERROR_OUTPUT_STDERR;
	else {
		lamip_send_message("SHORTEN: shorten_init: Wrong value for %s found! Resetting to default DEVNULL...\n");
		lamip_send_message("SHORTEN: shorten_init: possible values for %s are \"DEVNULL\", \"STDERR\", \"WINDOW\"\n", CONFIG_ERROR_OUTPUT_METHOD);
		val_error_output_method = strdup("DEVNULL");
		lamip_cfg_set(config, CONFIG_ERROR_OUTPUT_METHOD, val_error_output_method);
		shn_cfg.error_output_method = ERROR_OUTPUT_DEVNULL;
	}

	/* config: absolute seek tables path */
	if(!lamip_cfg_getExist(handle, CONFIG_SEEK_TABLES_PATH)) {
		lamip_send_message("SHORTEN: shorten_init: no config value for %s found, resetting to default...\n", CONFIG_SEEK_TABLES_PATH);
		val_seek_tables_path = strdup("/tmp");
		lamip_cfg_set(config, CONFIG_SEEK_TABLES_PATH, val_seek_tables_path);
	} else {
		val_seek_tables_path = lamip_cfg_get(handle, CONFIG_SEEK_TABLES_PATH);
	}
	shn_cfg.seek_tables_path = strdup(val_seek_tables_path);

	/* config: relative seek tables path */
	if(!lamip_cfg_getExist(handle, CONFIG_RELATIVE_SEEK_TABLES_PATH)) {
		lamip_send_message("SHORTEN: shorten_init: no config value for %s found, resetting to default...\n", CONFIG_RELATIVE_SEEK_TABLES_PATH);
		val_relative_seek_tables_path = strdup("");
		lamip_cfg_set(config, CONFIG_RELATIVE_SEEK_TABLES_PATH, val_relative_seek_tables_path);
	} else {
		val_relative_seek_tables_path = lamip_cfg_get(handle, CONFIG_RELATIVE_SEEK_TABLES_PATH);
	}
	shn_cfg.relative_seek_tables_path = strdup(val_relative_seek_tables_path);

	/* config: verbose */
	if(!lamip_cfg_getExist(handle, CONFIG_VERBOSE)) {
		lamip_send_message("SHORTEN: shorten_init: no config value for %s found, resetting to default...\n", CONFIG_VERBOSE);
		val_verbose = 0;
		lamip_cfg_setBool(config, CONFIG_VERBOSE, val_verbose);
	} else {
		val_verbose = lamip_cfg_getBool(handle, CONFIG_VERBOSE);
	}
	shn_cfg.verbose = val_verbose;

	/* config: swap bytes */
	if(!lamip_cfg_getExist(handle, CONFIG_SWAP_BYTES)) {
		lamip_send_message("SHORTEN: shorten_init: no config value for %s found, resetting to default...\n", CONFIG_SWAP_BYTES);
		val_swap_bytes = 0;
		lamip_cfg_setBool(config, CONFIG_SWAP_BYTES, val_swap_bytes);
	} else {
		val_swap_bytes = lamip_cfg_getBool(handle, CONFIG_SWAP_BYTES);
	}
	shn_cfg.swap_bytes = val_swap_bytes;

	/* Config cleanup */
	free(val_error_output_method);
	free(val_seek_tables_path);
	free(val_relative_seek_tables_path);
			
	return 1;
}

static void     shorten_cleanup(void)
{
	return;
}

static void	shorten_decode(lamipURL *url, int subtrack)
{
	if(!url) {
		lamip_send_message("SHORTEN: shorten_decode: Got no url!\n");
		return;
	}
	char *filename = lamip_url_getURL(url);
	if(!filename) {
		lamip_send_message("SHORTEN: shorten_decode: Got no filename! We cannot play from stream by now!\n");
		return;
	}
	shnfile = shn_load(filename, shn_cfg);
	if(!shnfile) {
		lamip_send_message("SHORTEN: shorten_decode: Error in opening file! Give it another try with skipping id3v2...\n");
		return;
	}
	if(!shn_init_decoder(shnfile)) {
		lamip_send_message("SHORTEN: shorten_decode: shn_init_decoder() failed! Aborting...\n");
		shn_unload(shnfile);
		shnfile = (shn_file *)NULL;
		return;
	}

	pcmi.channels = shn_get_channels(shnfile);
	pcmi.samplerate = shn_get_samplerate(shnfile);
	switch(shn_get_bitspersample(shnfile)) {
		case 8:
			pcmi.format = PCM_FORMAT_U8;
			break;
		case 16:
			pcmi.format = PCM_FORMAT_S16_LE;
			break;
		/* Next two bit depths aren't supported by shorten anyway */
		case 24:
			pcmi.format = PCM_FORMAT_S24_LE;
			break;
		case 32:
			pcmi.format = PCM_FORMAT_S32_LE;
			break;
		default:
			lamip_send_message("SHORTEN: shorten_decode: Not supported bits_per_sample format!");
	}

	/* Getting a clean buffer */
	int buffer_size = shn_get_buffer_block_size(shnfile, NUM_BUFFER_BLOCKS);
	if(real_buffer) {
		free(real_buffer);
		real_buffer = (uchar *)NULL;
	}
	if(!real_buffer) {
		real_buffer = (uchar *)malloc(20000);
		if(!real_buffer) {
			lamip_send_message("SHORTEN: shorten_decode: malloc for real_buffer failed! Aborting...\n");
			shn_unload(shnfile);
			shnfile = (shn_file *)NULL;
			return;
		}
	}
	int read_buffer;
	int seekable = shn_seekable(shnfile);
	lamip_open(&pcmi, shn_get_song_length(shnfile));
	while(lamip_isContinue()) {
		read_buffer = shn_read(shnfile, real_buffer, buffer_size);
		if(read_buffer <= 0) {
			lamip_drain();
			break;
		}
		
		lamip_writeData((uchar *)real_buffer, buffer_size);
		
		/* If seeking */
		if(lamip_isSeek() && seekable)
			if(!(shn_seek(shnfile, (unsigned int)(lamip_isSeekGetAndReset() / 1000))))
				lamip_send_message("SHORTEN: shorten_decode: Seeking failed!\n");
	}
	lamip_close();
	if(!shn_cleanup_decoder(shnfile))
		lamip_send_message("SHORTEN: shorten_decode: shn_cleanup_decoder() failed!\n");
	shn_unload(shnfile);
	shnfile = NULL;
	if(real_buffer) {
		free((uchar *)real_buffer);
		real_buffer = (uchar *)NULL;
	}
	return;
}

static void shorten_songinfo(lamipURL* url, lamipSonginfo* songinfo)
{
	/* TODO - perhaps it ain't necessary... due to a changing lamip core :) */
	return; 
}

