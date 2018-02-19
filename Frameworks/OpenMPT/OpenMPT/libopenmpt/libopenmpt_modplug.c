/*
 * libopenmpt_modplug.c
 * --------------------
 * Purpose: libopenmpt emulation of the libmodplug interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef NO_LIBMODPLUG

#ifdef LIBOPENMPT_BUILD_DLL
#undef LIBOPENMPT_BUILD_DLL
#endif

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif /* _MSC_VER */

#include "libopenmpt.h"

#include <limits.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* define to emulate 0.8.7 API/ABI instead of 0.8.8 API/ABI */
/* #define LIBOPENMPT_MODPLUG_0_8_7 */

#ifdef _MSC_VER
/* msvc errors when seeing dllexport declarations after prototypes have been declared in modplug.h */
#define LIBOPENMPT_MODPLUG_API
#else /* !_MSC_VER */
#define LIBOPENMPT_MODPLUG_API LIBOPENMPT_API_HELPER_EXPORT
#endif /* _MSC_VER */
#ifdef LIBOPENMPT_MODPLUG_0_8_7
#include "libmodplug/modplug_0.8.7.h"
#else
#include "libmodplug/modplug.h"
#endif

/* from libmodplug/sndfile.h */
/* header is not c clean */
#define MIXING_ATTENUATION 4
#define MOD_TYPE_NONE		0x0
#define MOD_TYPE_MOD		0x1
#define MOD_TYPE_S3M		0x2
#define MOD_TYPE_XM		0x4
#define MOD_TYPE_MED		0x8
#define MOD_TYPE_MTM		0x10
#define MOD_TYPE_IT		0x20
#define MOD_TYPE_669		0x40
#define MOD_TYPE_ULT		0x80
#define MOD_TYPE_STM		0x100
#define MOD_TYPE_FAR		0x200
#define MOD_TYPE_WAV		0x400
#define MOD_TYPE_AMF		0x800
#define MOD_TYPE_AMS		0x1000
#define MOD_TYPE_DSM		0x2000
#define MOD_TYPE_MDL		0x4000
#define MOD_TYPE_OKT		0x8000
#define MOD_TYPE_MID		0x10000
#define MOD_TYPE_DMF		0x20000
#define MOD_TYPE_PTM		0x40000
#define MOD_TYPE_DBM		0x80000
#define MOD_TYPE_MT2		0x100000
#define MOD_TYPE_AMF0		0x200000
#define MOD_TYPE_PSM		0x400000
#define MOD_TYPE_J2B		0x800000
#define MOD_TYPE_ABC		0x1000000
#define MOD_TYPE_PAT		0x2000000
#define MOD_TYPE_UMX		0x80000000 // Fake type

#define BUFFER_COUNT 1024

struct _ModPlugFile {
	openmpt_module* mod;
	signed short* buf;
	signed int* mixerbuf;
	char* name;
	char* message;
	ModPlug_Settings settings;
	ModPlugMixerProc mixerproc;
	ModPlugNote** patterns;
};

static ModPlug_Settings globalsettings = {
	MODPLUG_ENABLE_OVERSAMPLING|MODPLUG_ENABLE_NOISE_REDUCTION,
	2,
	16,
	44100,
	MODPLUG_RESAMPLE_LINEAR,
#ifndef LIBOPENMPT_MODPLUG_0_8_7
	128,
	256,
#endif
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

static int32_t modplugresamplingmode_to_filterlength(int mode)
{
	if(mode<0){
		return 1;
	}
	switch(mode){
	case MODPLUG_RESAMPLE_NEAREST: return 1; break;
	case MODPLUG_RESAMPLE_LINEAR: return 2; break;
	case MODPLUG_RESAMPLE_SPLINE: return 4; break;
	case MODPLUG_RESAMPLE_FIR: return 8; break;
	}
	return 8;
}

LIBOPENMPT_MODPLUG_API ModPlugFile* ModPlug_Load(const void* data, int size)
{
	ModPlugFile* file = malloc(sizeof(ModPlugFile));
	const char* name = NULL;
	const char* message = NULL;
	if(!file) return NULL;
	memset(file,0,sizeof(ModPlugFile));
	memcpy(&file->settings,&globalsettings,sizeof(ModPlug_Settings));
	file->mod = openmpt_module_create_from_memory2(data,size,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	if(!file->mod){
		free(file);
		return NULL;
	}
	file->buf = malloc(BUFFER_COUNT*sizeof(signed short)*4);
	if(!file->buf){
		openmpt_module_destroy(file->mod);
		free(file);
		return NULL;
	}
	openmpt_module_set_repeat_count(file->mod,file->settings.mLoopCount);
	name = openmpt_module_get_metadata(file->mod,"title");
	if(name){
		file->name = malloc(strlen(name)+1);
		if(file->name){
			strcpy(file->name,name);
		}
		openmpt_free_string(name);
		name = NULL;
	}else{
		file->name = malloc(strlen("")+1);
		if(file->name){
			strcpy(file->name,"");
		}
	}
	message = openmpt_module_get_metadata(file->mod,"message");
	if(message){
		file->message = malloc(strlen(message)+1);
		if(file->message){
			strcpy(file->message,message);
		}
		openmpt_free_string(message);
		message = NULL;
	}else{
		file->message = malloc(strlen("")+1);
		if(file->message){
			strcpy(file->message,"");
		}
	}
#ifndef LIBOPENMPT_MODPLUG_0_8_7
	openmpt_module_set_render_param(file->mod,OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT,file->settings.mStereoSeparation*100/128);
#endif
	openmpt_module_set_render_param(file->mod,OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH,modplugresamplingmode_to_filterlength(file->settings.mResamplingMode));
	return file;
}

LIBOPENMPT_MODPLUG_API void ModPlug_Unload(ModPlugFile* file)
{
	int p;
	if(!file) return;
	if(file->patterns){
		for(p=0;p<openmpt_module_get_num_patterns(file->mod);p++){
			if(file->patterns[p]){
				free(file->patterns[p]);
				file->patterns[p] = NULL;
			}
		}
		free(file->patterns);
		file->patterns = NULL;
	}
	if(file->mixerbuf){
		free(file->mixerbuf);
		file->mixerbuf = NULL;
	}
	openmpt_module_destroy(file->mod);
	file->mod = NULL;
	free(file->name);
	file->name = NULL;
	free(file->message);
	file->message = NULL;
	free(file->buf);
	file->buf = NULL;
	free(file);
}

LIBOPENMPT_MODPLUG_API int ModPlug_Read(ModPlugFile* file, void* buffer, int size)
{
	int framesize;
	int framecount;
	int frames;
	int rendered;
	int frame;
	int channel;
	int totalrendered;
	signed short* in;
	signed int* mixbuf;
	unsigned char* buf8;
	signed short* buf16;
	signed int* buf32;
	if(!file) return 0;
	framesize = file->settings.mBits/8*file->settings.mChannels;
	framecount = size/framesize;
	buf8 = buffer;
	buf16 = buffer;
	buf32 = buffer;
	totalrendered = 0;
	while(framecount>0){
		frames = framecount;
		if(frames>BUFFER_COUNT){
			frames = BUFFER_COUNT;
		}
		if(file->settings.mChannels==1){
			rendered = (int)openmpt_module_read_mono(file->mod,file->settings.mFrequency,frames,&file->buf[frames*0]);
		}else if(file->settings.mChannels==2){
			rendered = (int)openmpt_module_read_stereo(file->mod,file->settings.mFrequency,frames,&file->buf[frames*0],&file->buf[frames*1]);
		}else if(file->settings.mChannels==4){
			rendered = (int)openmpt_module_read_quad(file->mod,file->settings.mFrequency,frames,&file->buf[frames*0],&file->buf[frames*1],&file->buf[frames*2],&file->buf[frames*3]);
		}else{
			return 0;
		}
		in = file->buf;
		if(file->mixerproc&&file->mixerbuf){
			mixbuf=file->mixerbuf;
			for(frame=0;frame<frames;frame++){
				for(channel=0;channel<file->settings.mChannels;channel++){
					*mixbuf = in[frames*channel+frame]<<(32-16-1-MIXING_ATTENUATION);
					mixbuf++;
				}
			}
			file->mixerproc(file->mixerbuf,file->settings.mChannels*frames,file->settings.mChannels);
			mixbuf=file->mixerbuf;
			for(frame=0;frame<frames;frame++){
				for(channel=0;channel<file->settings.mChannels;channel++){
					in[frames*channel+frame] = *mixbuf>>(32-16-1-MIXING_ATTENUATION);
					mixbuf++;
				}
			}
		}
		if(file->settings.mBits==8){
			for(frame=0;frame<frames;frame++){
				for(channel=0;channel<file->settings.mChannels;channel++){
					*buf8 = in[frames*channel+frame]/256+0x80;
					buf8++;
				}
			}
		}else if(file->settings.mBits==16){
			for(frame=0;frame<frames;frame++){
				for(channel=0;channel<file->settings.mChannels;channel++){
					*buf16 = in[frames*channel+frame];
					buf16++;
				}
			}
		}else if(file->settings.mBits==32){
			for(frame=0;frame<frames;frame++){
				for(channel=0;channel<file->settings.mChannels;channel++){
					*buf32 = in[frames*channel+frame] << (32-16-1-MIXING_ATTENUATION);
					buf32++;
				}
			}
		}else{
			return 0;
		}
		totalrendered += rendered;
		framecount -= frames;
		if(!rendered) break;
	}
	memset(((char*)buffer)+totalrendered*framesize,0,size-totalrendered*framesize);
	return totalrendered*framesize;
}

LIBOPENMPT_MODPLUG_API const char* ModPlug_GetName(ModPlugFile* file)
{
	if(!file) return NULL;
	return file->name;
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetLength(ModPlugFile* file)
{
	if(!file) return 0;
	return (int)(openmpt_module_get_duration_seconds(file->mod)*1000.0);
}

LIBOPENMPT_MODPLUG_API void ModPlug_Seek(ModPlugFile* file, int millisecond)
{
	if(!file) return;
	openmpt_module_set_position_seconds(file->mod,(double)millisecond*0.001);
}

LIBOPENMPT_MODPLUG_API void ModPlug_GetSettings(ModPlug_Settings* settings)
{
	if(!settings) return;
	memcpy(settings,&globalsettings,sizeof(ModPlug_Settings));
}

LIBOPENMPT_MODPLUG_API void ModPlug_SetSettings(const ModPlug_Settings* settings)
{
	if(!settings) return;
	memcpy(&globalsettings,settings,sizeof(ModPlug_Settings));
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_GetMasterVolume(ModPlugFile* file)
{
	int32_t val;
	if(!file) return 0;
	val = 0;
	if(!openmpt_module_get_render_param(file->mod,OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL,&val)) return 128;
	return (unsigned int)(128.0*pow(10.0,val*0.0005));
}

LIBOPENMPT_MODPLUG_API void ModPlug_SetMasterVolume(ModPlugFile* file,unsigned int cvol)
{
	if(!file) return;
	openmpt_module_set_render_param(file->mod,OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL,(int32_t)(2000.0*log10(cvol/128.0)));
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetCurrentSpeed(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_current_speed(file->mod);
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetCurrentTempo(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_current_tempo(file->mod);
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetCurrentOrder(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_current_order(file->mod);
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetCurrentPattern(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_current_pattern(file->mod);
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetCurrentRow(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_current_row(file->mod);
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetPlayingChannels(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_current_playing_channels(file->mod);
}

LIBOPENMPT_MODPLUG_API void ModPlug_SeekOrder(ModPlugFile* file,int order)
{
	if(!file) return;
	openmpt_module_set_position_order_row(file->mod,order,0);
}

LIBOPENMPT_MODPLUG_API int ModPlug_GetModuleType(ModPlugFile* file)
{
	const char* type;
	int retval;
	if(!file) return 0;
	type = openmpt_module_get_metadata(file->mod,"type");
	retval = MOD_TYPE_NONE;
	if(!type){
		return retval;
	}
	if(!strcmp(type,"mod")){
		retval = MOD_TYPE_MOD;
	}else if(!strcmp(type,"s3m")){
		retval = MOD_TYPE_S3M;
	}else if(!strcmp(type,"xm")){
		retval = MOD_TYPE_XM;
	}else if(!strcmp(type,"med")){
		retval = MOD_TYPE_MED;
	}else if(!strcmp(type,"mtm")){
		retval = MOD_TYPE_MTM;
	}else if(!strcmp(type,"it")){
		retval = MOD_TYPE_IT;
	}else if(!strcmp(type,"669")){
		retval = MOD_TYPE_669;
	}else if(!strcmp(type,"ult")){
		retval = MOD_TYPE_ULT;
	}else if(!strcmp(type,"stm")){
		retval = MOD_TYPE_STM;
	}else if(!strcmp(type,"far")){
		retval = MOD_TYPE_FAR;
	}else if(!strcmp(type,"s3m")){
		retval = MOD_TYPE_WAV;
	}else if(!strcmp(type,"amf")){
		retval = MOD_TYPE_AMF;
	}else if(!strcmp(type,"ams")){
		retval = MOD_TYPE_AMS;
	}else if(!strcmp(type,"dsm")){
		retval = MOD_TYPE_DSM;
	}else if(!strcmp(type,"mdl")){
		retval = MOD_TYPE_MDL;
	}else if(!strcmp(type,"okt")){
		retval = MOD_TYPE_OKT;
	}else if(!strcmp(type,"mid")){
		retval = MOD_TYPE_MID;
	}else if(!strcmp(type,"dmf")){
		retval = MOD_TYPE_DMF;
	}else if(!strcmp(type,"ptm")){
		retval = MOD_TYPE_PTM;
	}else if(!strcmp(type,"dbm")){
		retval = MOD_TYPE_DBM;
	}else if(!strcmp(type,"mt2")){
		retval = MOD_TYPE_MT2;
	}else if(!strcmp(type,"amf0")){
		retval = MOD_TYPE_AMF0;
	}else if(!strcmp(type,"psm")){
		retval = MOD_TYPE_PSM;
	}else if(!strcmp(type,"j2b")){
		retval = MOD_TYPE_J2B;
	}else if(!strcmp(type,"abc")){
		retval = MOD_TYPE_ABC;
	}else if(!strcmp(type,"pat")){
		retval = MOD_TYPE_PAT;
	}else if(!strcmp(type,"umx")){
		retval = MOD_TYPE_UMX;
	}else{
		retval = MOD_TYPE_IT; /* fallback, most complex type */
	}
	openmpt_free_string(type);
	return retval;
}

LIBOPENMPT_MODPLUG_API char* ModPlug_GetMessage(ModPlugFile* file)
{
	if(!file) return NULL;
	return file->message;
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_NumInstruments(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_num_instruments(file->mod);
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_NumSamples(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_num_samples(file->mod);
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_NumPatterns(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_num_patterns(file->mod);
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_NumChannels(ModPlugFile* file)
{
	if(!file) return 0;
	return openmpt_module_get_num_channels(file->mod);
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_SampleName(ModPlugFile* file, unsigned int qual, char* buff)
{
	const char* str;
	unsigned int retval;
	size_t tmpretval;
	if(!file) return 0;
	str = openmpt_module_get_sample_name(file->mod,qual-1);
	if(!str){
		if(buff){
			*buff = '\0';
		}
		return 0;
	}
	tmpretval = strlen(str);
	if(tmpretval>=INT_MAX){
		tmpretval = INT_MAX-1;
	}
	retval = (int)tmpretval;
	if(buff){
		strncpy(buff,str,retval+1);
	}
	openmpt_free_string(str);
	return retval;
}

LIBOPENMPT_MODPLUG_API unsigned int ModPlug_InstrumentName(ModPlugFile* file, unsigned int qual, char* buff)
{
	const char* str;
	unsigned int retval;
	size_t tmpretval;
	if(!file) return 0;
	str = openmpt_module_get_instrument_name(file->mod,qual-1);
	if(!str){
		if(buff){
			*buff = '\0';
		}
		return 0;
	}
	tmpretval = strlen(str);
	if(tmpretval>=INT_MAX){
		tmpretval = INT_MAX-1;
	}
	retval = (int)tmpretval;
	if(buff){
		strncpy(buff,str,retval+1);
	}
	openmpt_free_string(str);
	return retval;
}

LIBOPENMPT_MODPLUG_API ModPlugNote* ModPlug_GetPattern(ModPlugFile* file, int pattern, unsigned int* numrows)
{
	int c;
	int r;
	int numr;
	int numc;
	ModPlugNote note;
	if(!file) return NULL;
	if(numrows){
		*numrows = openmpt_module_get_pattern_num_rows(file->mod,pattern);
	}
	if(pattern<0||pattern>=openmpt_module_get_num_patterns(file->mod)){
		return NULL;
	}
	if(!file->patterns){
		file->patterns = malloc(sizeof(ModPlugNote*)*openmpt_module_get_pattern_num_rows(file->mod,pattern));
		if(!file->patterns) return NULL;
		memset(file->patterns,0,sizeof(ModPlugNote*)*openmpt_module_get_pattern_num_rows(file->mod,pattern));
	}
	if(!file->patterns[pattern]){
		file->patterns[pattern] = malloc(sizeof(ModPlugNote)*openmpt_module_get_pattern_num_rows(file->mod,pattern)*openmpt_module_get_num_channels(file->mod));
		if(!file->patterns[pattern]) return NULL;
		memset(file->patterns[pattern],0,sizeof(ModPlugNote)*openmpt_module_get_pattern_num_rows(file->mod,pattern)*openmpt_module_get_num_channels(file->mod));
	}
	numr = openmpt_module_get_pattern_num_rows(file->mod,pattern);
	numc = openmpt_module_get_num_channels(file->mod);
	for(r=0;r<numr;r++){
		for(c=0;c<numc;c++){
			memset(&note,0,sizeof(ModPlugNote));
			note.Note = openmpt_module_get_pattern_row_channel_command(file->mod,pattern,r,c,OPENMPT_MODULE_COMMAND_NOTE);
			note.Instrument = openmpt_module_get_pattern_row_channel_command(file->mod,pattern,r,c,OPENMPT_MODULE_COMMAND_INSTRUMENT);
			note.VolumeEffect = openmpt_module_get_pattern_row_channel_command(file->mod,pattern,r,c,OPENMPT_MODULE_COMMAND_VOLUMEEFFECT);
			note.Effect = openmpt_module_get_pattern_row_channel_command(file->mod,pattern,r,c,OPENMPT_MODULE_COMMAND_EFFECT);
			note.Volume = openmpt_module_get_pattern_row_channel_command(file->mod,pattern,r,c,OPENMPT_MODULE_COMMAND_VOLUME);
			note.Parameter = openmpt_module_get_pattern_row_channel_command(file->mod,pattern,r,c,OPENMPT_MODULE_COMMAND_PARAMETER);
			memcpy(&file->patterns[pattern][r*numc+c],&note,sizeof(ModPlugNote));
		}
	}
	return file->patterns[pattern];
}

LIBOPENMPT_MODPLUG_API void ModPlug_InitMixerCallback(ModPlugFile* file,ModPlugMixerProc proc)
{
	if(!file) return;
	if(!file->mixerbuf){
		file->mixerbuf = malloc(BUFFER_COUNT*sizeof(signed int)*4);
	}
	file->mixerproc = proc;
}

LIBOPENMPT_MODPLUG_API void ModPlug_UnloadMixerCallback(ModPlugFile* file)
{
	if(!file) return;
	file->mixerproc = NULL;
	if(file->mixerbuf){
		free(file->mixerbuf);
		file->mixerbuf = NULL;
	}
}

LIBOPENMPT_MODPLUG_API char ModPlug_ExportS3M(ModPlugFile* file, const char* filepath)
{
	(void)file;
	/* not implemented */
	fprintf(stderr,"libopenmpt-modplug: error: ModPlug_ExportS3M(%s) not implemented.\n",filepath);
	return 0;
}

LIBOPENMPT_MODPLUG_API char ModPlug_ExportXM(ModPlugFile* file, const char* filepath)
{
	(void)file;
	/* not implemented */
	fprintf(stderr,"libopenmpt-modplug: error: ModPlug_ExportXM(%s) not implemented.\n",filepath);
	return 0;
}

LIBOPENMPT_MODPLUG_API char ModPlug_ExportMOD(ModPlugFile* file, const char* filepath)
{
	(void)file;
	/* not implemented */
	fprintf(stderr,"libopenmpt-modplug: error: ModPlug_ExportMOD(%s) not implemented.\n",filepath);
	return 0;
}

LIBOPENMPT_MODPLUG_API char ModPlug_ExportIT(ModPlugFile* file, const char* filepath)
{
	(void)file;
	/* not implemented */
	fprintf(stderr,"libopenmpt-modplug: error: ModPlug_ExportIT(%s) not implemented.\n",filepath);
	return 0;
}

#ifdef _MSC_VER
#ifdef _M_IX86
#pragma comment(linker, "/EXPORT:ModPlug_Load=_ModPlug_Load")
#pragma comment(linker, "/EXPORT:ModPlug_Unload=_ModPlug_Unload")
#pragma comment(linker, "/EXPORT:ModPlug_Read=_ModPlug_Read")
#pragma comment(linker, "/EXPORT:ModPlug_GetName=_ModPlug_GetName")
#pragma comment(linker, "/EXPORT:ModPlug_GetLength=_ModPlug_GetLength")
#pragma comment(linker, "/EXPORT:ModPlug_Seek=_ModPlug_Seek")
#pragma comment(linker, "/EXPORT:ModPlug_GetSettings=_ModPlug_GetSettings")
#pragma comment(linker, "/EXPORT:ModPlug_SetSettings=_ModPlug_SetSettings")
#pragma comment(linker, "/EXPORT:ModPlug_GetMasterVolume=_ModPlug_GetMasterVolume")
#pragma comment(linker, "/EXPORT:ModPlug_SetMasterVolume=_ModPlug_SetMasterVolume")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentSpeed=_ModPlug_GetCurrentSpeed")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentTempo=_ModPlug_GetCurrentTempo")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentOrder=_ModPlug_GetCurrentOrder")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentPattern=_ModPlug_GetCurrentPattern")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentRow=_ModPlug_GetCurrentRow")
#pragma comment(linker, "/EXPORT:ModPlug_GetPlayingChannels=_ModPlug_GetPlayingChannels")
#pragma comment(linker, "/EXPORT:ModPlug_SeekOrder=_ModPlug_SeekOrder")
#pragma comment(linker, "/EXPORT:ModPlug_GetModuleType=_ModPlug_GetModuleType")
#pragma comment(linker, "/EXPORT:ModPlug_GetMessage=_ModPlug_GetMessage")
#pragma comment(linker, "/EXPORT:ModPlug_NumInstruments=_ModPlug_NumInstruments")
#pragma comment(linker, "/EXPORT:ModPlug_NumSamples=_ModPlug_NumSamples")
#pragma comment(linker, "/EXPORT:ModPlug_NumPatterns=_ModPlug_NumPatterns")
#pragma comment(linker, "/EXPORT:ModPlug_NumChannels=_ModPlug_NumChannels")
#pragma comment(linker, "/EXPORT:ModPlug_SampleName=_ModPlug_SampleName")
#pragma comment(linker, "/EXPORT:ModPlug_InstrumentName=_ModPlug_InstrumentName")
#pragma comment(linker, "/EXPORT:ModPlug_GetPattern=_ModPlug_GetPattern")
#pragma comment(linker, "/EXPORT:ModPlug_InitMixerCallback=_ModPlug_InitMixerCallback")
#pragma comment(linker, "/EXPORT:ModPlug_UnloadMixerCallback=_ModPlug_UnloadMixerCallback")
#pragma comment(linker, "/EXPORT:ModPlug_ExportS3M=_ModPlug_ExportS3M")
#pragma comment(linker, "/EXPORT:ModPlug_ExportXM=_ModPlug_ExportXM")
#pragma comment(linker, "/EXPORT:ModPlug_ExportMOD=_ModPlug_ExportMOD")
#pragma comment(linker, "/EXPORT:ModPlug_ExportIT=_ModPlug_ExportIT")
#else /* !_M_IX86 */
#pragma comment(linker, "/EXPORT:ModPlug_Load")
#pragma comment(linker, "/EXPORT:ModPlug_Unload")
#pragma comment(linker, "/EXPORT:ModPlug_Read")
#pragma comment(linker, "/EXPORT:ModPlug_GetName")
#pragma comment(linker, "/EXPORT:ModPlug_GetLength")
#pragma comment(linker, "/EXPORT:ModPlug_Seek")
#pragma comment(linker, "/EXPORT:ModPlug_GetSettings")
#pragma comment(linker, "/EXPORT:ModPlug_SetSettings")
#pragma comment(linker, "/EXPORT:ModPlug_GetMasterVolume")
#pragma comment(linker, "/EXPORT:ModPlug_SetMasterVolume")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentSpeed")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentTempo")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentOrder")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentPattern")
#pragma comment(linker, "/EXPORT:ModPlug_GetCurrentRow")
#pragma comment(linker, "/EXPORT:ModPlug_GetPlayingChannels")
#pragma comment(linker, "/EXPORT:ModPlug_SeekOrder")
#pragma comment(linker, "/EXPORT:ModPlug_GetModuleType")
#pragma comment(linker, "/EXPORT:ModPlug_GetMessage")
#pragma comment(linker, "/EXPORT:ModPlug_NumInstruments")
#pragma comment(linker, "/EXPORT:ModPlug_NumSamples")
#pragma comment(linker, "/EXPORT:ModPlug_NumPatterns")
#pragma comment(linker, "/EXPORT:ModPlug_NumChannels")
#pragma comment(linker, "/EXPORT:ModPlug_SampleName")
#pragma comment(linker, "/EXPORT:ModPlug_InstrumentName")
#pragma comment(linker, "/EXPORT:ModPlug_GetPattern")
#pragma comment(linker, "/EXPORT:ModPlug_InitMixerCallback")
#pragma comment(linker, "/EXPORT:ModPlug_UnloadMixerCallback")
#pragma comment(linker, "/EXPORT:ModPlug_ExportS3M")
#pragma comment(linker, "/EXPORT:ModPlug_ExportXM")
#pragma comment(linker, "/EXPORT:ModPlug_ExportMOD")
#pragma comment(linker, "/EXPORT:ModPlug_ExportIT")
#endif /* _M_IX86 */
#endif /* _MSC_VER */

#endif /* NO_LIBMODPLUG */
