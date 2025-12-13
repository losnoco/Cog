//
//  HCDecoder.m
//  HighlyComplete
//
//  Created by Christopher Snowhill on 9/30/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "HCDecoder.h"

#import "Logging.h"

#import "hebios.h"

#import <psflib/psf2fs.h>
#import <psflib/psflib.h>

#import <HighlyExperimental/bios.h>
#import <HighlyExperimental/iop.h>
#import <HighlyExperimental/psx.h>
#import <HighlyExperimental/r3000.h>

#import <HighlyTheoretical/sega.h>

#import <HighlyQuixotic/qsound.h>

#import <mgba-util/vfs.h>
#import <mgba/core/core.h>
#import <mgba-util/audio-buffer.h>
#import <mgba/core/log.h>

#import <SSEQPlayer/Player.h>
#import <SSEQPlayer/SDAT.h>
#include <vector>

#import <vio2sf/state.h>

#import <lazyusf2/usf.h>

#include <zlib.h>

#include <dlfcn.h>

#import "PlaylistController.h"

#include <signal.h>

// #define USF_LOG

@interface psf_file_container : NSObject {
	NSLock *lock;
	NSMutableDictionary *list;
}
+ (psf_file_container *)instance;
- (void)add_hint:(NSString *)path source:(id)source;
- (void)remove_hint:(NSString *)path;
- (BOOL)try_hint:(NSString *)path source:(id *)source;
@end

@implementation psf_file_container
+ (psf_file_container *)instance {
	static psf_file_container *instance;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		instance = [self new];
	});
	return instance;
}
- (psf_file_container *)init {
	if((self = [super init])) {
		lock = [NSLock new];
		list = [NSMutableDictionary new];
	}
	return self;
}
- (void)add_hint:(NSString *)path source:(id)source {
	[lock lock];
	[list setObject:source forKey:path];
	[lock unlock];
}
- (void)remove_hint:(NSString *)path {
	[lock lock];
	[list removeObjectForKey:path];
	[lock unlock];
}
- (BOOL)try_hint:(NSString *)path source:(id *)source {
	[lock lock];
	*source = [list objectForKey:path];
	[lock unlock];
	if(*source) {
		[*source seek:0 whence:0];
		return YES;
	} else {
		return NO;
	}
}
@end

void *source_fopen(const char *path) {
	id<CogSource> source;
	if(![[psf_file_container instance] try_hint:[NSString stringWithUTF8String:path] source:&source]) {
		NSString *urlString = [NSString stringWithUTF8String:path];
		urlString = [urlString stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLFragmentAllowedCharacterSet];
		NSURL *url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];

		id audioSourceClass = NSClassFromString(@"AudioSource");
		source = [audioSourceClass audioSourceForURL:url];

		if(![source open:url])
			return 0;

		if(![source seekable])
			return 0;
	}

	return (void *)CFBridgingRetain(source);
}

size_t source_fread(void *buffer, size_t size, size_t count, void *handle) {
	NSObject *_handle = (__bridge NSObject *)(handle);
	id<CogSource> __unsafe_unretained source = (id)_handle;

	return [source read:buffer amount:(size * count)] / size;
}

int source_fseek(void *handle, int64_t offset, int whence) {
	NSObject *_handle = (__bridge NSObject *)(handle);
	id<CogSource> __unsafe_unretained source = (id)_handle;

	return [source seek:(long)offset whence:whence] ? 0 : -1;
}

int source_fclose(void *handle) {
	CFBridgingRelease(handle);

	return 0;
}

long source_ftell(void *handle) {
	NSObject *_handle = (__bridge NSObject *)(handle);
	id<CogSource> __unsafe_unretained source = (id)_handle;

	return [source tell];
}

static psf_file_callbacks source_callbacks = {
	"/|\\",
	source_fopen,
	source_fread,
	source_fseek,
	source_fclose,
	source_ftell
};

@implementation HCDecoder

+ (void)initialize {
	if(self == [HCDecoder class]) {
		bios_set_image(hebios, HEBIOS_SIZE);
		psx_init();
		sega_init();
		qsound_init();
		mLogSetDefaultLogger(&gsf_logger);
	}
}

- (id)init {
	self = [super init];
	if(self) {
		hintAdded = NO;
		type = 0;
		emulatorCore = NULL;
		emulatorExtra = NULL;
	}
	return self;
}

- (NSDictionary *)metadata {
	return metadataList;
}

- (long)retrieveFrameCount:(long)ms {
	return ms * (sampleRate / 100) / 10;
}

struct psf_info_meta_state {
	NSMutableDictionary *info;

	bool utf8;

	int tag_length_ms;
	int tag_fade_ms;

	float albumGain;
	float albumPeak;
	float trackGain;
	float trackPeak;
	float volume;
};

static int parse_time_crap(NSString *value) {
	NSArray *crapFix = [value componentsSeparatedByString:@"\n"];
	NSArray *components = [[crapFix objectAtIndex:0] componentsSeparatedByString:@":"];

	float totalSeconds = 0;
	float multiplier = 1000;
	bool first = YES;
	for(id component in [components reverseObjectEnumerator]) {
		if(first) {
			first = NO;
			totalSeconds += [component floatValue] * multiplier;
		} else {
			totalSeconds += [component integerValue] * multiplier;
		}
		multiplier *= 60;
	}

	return totalSeconds;
}

static void setDictionary(NSMutableDictionary *dict, NSString *tag, NSString *value) {
	NSString *realKey = [tag stringByReplacingOccurrencesOfString:@"." withString:@"․"];
	NSMutableArray *array = [dict valueForKey:realKey];
	if(!array) {
		array = [NSMutableArray new];
		[dict setObject:array forKey:realKey];
	}
	if([array count]) {
		NSString *existing = array[0];
		array[0] = [existing stringByAppendingFormat:@"\r\n%@", value];
	} else {
		[array addObject:value];
	}
}

static int psf_info_meta(void *context, const char *name, const char *value) {
	struct psf_info_meta_state *state = (struct psf_info_meta_state *)context;

	NSString *tag = guess_encoding_of_string(name);
	NSString *taglc = [tag lowercaseString];
	NSString *svalue = guess_encoding_of_string(value);

	if(svalue == nil)
		return 0;

	if([taglc isEqualToString:@"game"]) {
		taglc = @"album";
	}

	if([taglc isEqualToString:@"length"]) {
		state->tag_length_ms = parse_time_crap(svalue);
		setDictionary(state->info, @"psf_length", svalue);
	} else if([taglc isEqualToString:@"fade"]) {
		state->tag_fade_ms = parse_time_crap(svalue);
		setDictionary(state->info, @"psf_fade", svalue);
	} else if([taglc isEqualToString:@"utf8"]) {
		state->utf8 = true;
	} else {
		setDictionary(state->info, taglc, svalue);
	}

	return 0;
}

struct psf1_load_state {
	void *emu;
	bool first;
	unsigned refresh;
};

typedef struct {
	uint32_t pc0;
	uint32_t gp0;
	uint32_t t_addr;
	uint32_t t_size;
	uint32_t d_addr;
	uint32_t d_size;
	uint32_t b_addr;
	uint32_t b_size;
	uint32_t s_ptr;
	uint32_t s_size;
	uint32_t sp, fp, gp, ret, base;
} exec_header_t;

typedef struct {
	char key[8];
	uint32_t text;
	uint32_t data;
	exec_header_t exec;
	char title[60];
} psxexe_hdr_t;

static int psf1_info(void *context, const char *name, const char *value) {
	struct psf1_load_state *state = (struct psf1_load_state *)context;

	NSString *sname = [guess_encoding_of_string(name) lowercaseString];
	NSString *svalue = guess_encoding_of_string(value);

	if(!state->refresh && [sname isEqualToString:@"_refresh"]) {
		state->refresh = [svalue intValue];
	}

	return 0;
}

unsigned get_be16(void const *p) {
	return (unsigned)((unsigned char const *)p)[0] << 8 |
	       (unsigned)((unsigned char const *)p)[1];
}

unsigned get_le32(void const *p) {
	return (unsigned)((unsigned char const *)p)[3] << 24 |
	       (unsigned)((unsigned char const *)p)[2] << 16 |
	       (unsigned)((unsigned char const *)p)[1] << 8 |
	       (unsigned)((unsigned char const *)p)[0];
}

unsigned get_be32(void const *p) {
	return (unsigned)((unsigned char const *)p)[0] << 24 |
	       (unsigned)((unsigned char const *)p)[1] << 16 |
	       (unsigned)((unsigned char const *)p)[2] << 8 |
	       (unsigned)((unsigned char const *)p)[3];
}

void set_le32(void *p, unsigned n) {
	((unsigned char *)p)[0] = (unsigned char)n;
	((unsigned char *)p)[1] = (unsigned char)(n >> 8);
	((unsigned char *)p)[2] = (unsigned char)(n >> 16);
	((unsigned char *)p)[3] = (unsigned char)(n >> 24);
}

int psf1_loader(void *context, const uint8_t *exe, size_t exe_size,
                const uint8_t *reserved, size_t reserved_size) {
	struct psf1_load_state *state = (struct psf1_load_state *)context;

	psxexe_hdr_t *psx = (psxexe_hdr_t *)exe;

	if(exe_size < 0x800) return -1;
	if(exe_size > UINT_MAX) return -1;

	uint32_t addr = get_le32(&psx->exec.t_addr);
	uint32_t size = (uint32_t)exe_size - 0x800;

	addr &= 0x1fffff;
	if((addr < 0x10000) || (size > 0x1f0000) || (addr + size > 0x200000)) return -1;

	void *pIOP = psx_get_iop_state(state->emu);
	iop_upload_to_ram(pIOP, addr, exe + 0x800, size);

	if(!state->refresh) {
		if(!strncasecmp((const char *)exe + 113, "Japan", 5))
			state->refresh = 60;
		else if(!strncasecmp((const char *)exe + 113, "Europe", 6))
			state->refresh = 50;
		else if(!strncasecmp((const char *)exe + 113, "North America", 13))
			state->refresh = 60;
	}

	if(state->first) {
		void *pR3000 = iop_get_r3000_state(pIOP);
		r3000_setreg(pR3000, R3000_REG_PC, get_le32(&psx->exec.pc0));
		r3000_setreg(pR3000, R3000_REG_GEN + 29, get_le32(&psx->exec.s_ptr));
		state->first = false;
	}

	return 0;
}

static int EMU_CALL virtual_readfile(void *context, const char *path, int offset, char *buffer, int length) {
	return psf2fs_virtual_readfile(context, path, offset, buffer, length);
}

struct sdsf_loader_state {
	uint8_t *data;
	size_t data_size;
};

int sdsf_loader(void *context, const uint8_t *exe, size_t exe_size,
                const uint8_t *reserved, size_t reserved_size) {
	if(exe_size < 4) return -1;

	struct sdsf_loader_state *state = (struct sdsf_loader_state *)context;

	uint8_t *dst = state->data;

	if(state->data_size < 4) {
		state->data = dst = (uint8_t *)malloc(exe_size);
		state->data_size = exe_size;
		memcpy(dst, exe, exe_size);
		return 0;
	}

	uint32_t dst_start = get_le32(dst);
	uint32_t src_start = get_le32(exe);
	dst_start &= 0x7fffff;
	src_start &= 0x7fffff;
	size_t dst_len = state->data_size - 4;
	size_t src_len = exe_size - 4;
	if(dst_len > 0x800000) dst_len = 0x800000;
	if(src_len > 0x800000) src_len = 0x800000;

	if(src_start < dst_start) {
		uint32_t diff = dst_start - src_start;
		state->data_size = dst_len + 4 + diff;
		state->data = dst = (uint8_t *)realloc(dst, state->data_size);
		memmove(dst + 4 + diff, dst + 4, dst_len);
		memset(dst + 4, 0, diff);
		dst_len += diff;
		dst_start = src_start;
		set_le32(dst, dst_start);
	}
	if((src_start + src_len) > (dst_start + dst_len)) {
		size_t diff = (src_start + src_len) - (dst_start + dst_len);
		state->data_size = dst_len + 4 + diff;
		state->data = dst = (uint8_t *)realloc(dst, state->data_size);
		memset(dst + 4 + dst_len, 0, diff);
	}

	memcpy(dst + 4 + (src_start - dst_start), exe + 4, src_len);

	return 0;
}

struct qsf_loader_state {
	uint8_t *key;
	uint32_t key_size;

	uint8_t *z80_rom;
	uint32_t z80_size;

	uint8_t *sample_rom;
	uint32_t sample_size;
};

static int upload_qsf_section(struct qsf_loader_state *state, const char *section, uint32_t start,
                              const uint8_t *data, uint32_t size) {
	uint8_t **array = NULL;
	uint32_t *array_size = NULL;
	uint32_t max_size = 0x7fffffff;

	if(!strcmp(section, "KEY")) {
		array = &state->key;
		array_size = &state->key_size;
		max_size = 11;
	} else if(!strcmp(section, "Z80")) {
		array = &state->z80_rom;
		array_size = &state->z80_size;
	} else if(!strcmp(section, "SMP")) {
		array = &state->sample_rom;
		array_size = &state->sample_size;
	} else
		return -1;

	if((start + size) < start) return -1;

	uint32_t new_size = start + size;
	uint32_t old_size = *array_size;
	if(new_size > max_size) return -1;

	if(new_size > old_size) {
		*array = (uint8_t *)realloc(*array, new_size);
		*array_size = new_size;
		memset((*array) + old_size, 0, new_size - old_size);
	}

	memcpy((*array) + start, data, size);

	return 0;
}

static int qsf_loader(void *context, const uint8_t *exe, size_t exe_size,
                      const uint8_t *reserved, size_t reserved_size) {
	struct qsf_loader_state *state = (struct qsf_loader_state *)context;

	for(;;) {
		char s[4];
		if(exe_size < 11) break;
		memcpy(s, exe, 3);
		exe += 3;
		exe_size -= 3;
		s[3] = 0;
		uint32_t dataofs = get_le32(exe);
		exe += 4;
		exe_size -= 4;
		uint32_t datasize = get_le32(exe);
		exe += 4;
		exe_size -= 4;
		if(datasize > exe_size)
			return -1;

		if(upload_qsf_section(state, s, dataofs, exe, datasize) < 0)
			return -1;

		exe += datasize;
		exe_size -= datasize;
	}

	return 0;
}

struct gsf_loader_state {
	int entry_set;
	uint32_t entry;
	uint8_t *data;
	size_t data_size;
};

static int gsf_loader(void *context, const uint8_t *exe, size_t exe_size,
                      const uint8_t *reserved, size_t reserved_size) {
	if(exe_size < 12) return -1;

	struct gsf_loader_state *state = (struct gsf_loader_state *)context;

	unsigned char *iptr;
	size_t isize;
	unsigned char *xptr;
	unsigned xentry = get_le32(exe + 0);
	unsigned xsize = get_le32(exe + 8);
	unsigned xofs = get_le32(exe + 4) & 0x1ffffff;
	if(xsize < exe_size - 12) return -1;
	if(!state->entry_set) {
		state->entry = xentry;
		state->entry_set = 1;
	}
	{
		iptr = state->data;
		isize = state->data_size;
		state->data = 0;
		state->data_size = 0;
	}
	if(!iptr) {
		size_t rsize = xofs + xsize;
		{
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		iptr = (unsigned char *)malloc(rsize + 10);
		if(!iptr)
			return -1;
		memset(iptr, 0, rsize + 10);
		isize = rsize;
	} else if(isize < xofs + xsize) {
		size_t rsize = xofs + xsize;
		{
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		xptr = (unsigned char *)realloc(iptr, xofs + rsize + 10);
		if(!xptr) {
			free(iptr);
			return -1;
		}
		iptr = xptr;
		isize = rsize;
	}
	memcpy(iptr + xofs, exe + 12, xsize);
	{
		state->data = iptr;
		state->data_size = isize;
	}
	return 0;
}

struct gsf_running_state {
	struct mAVStream stream;
	void *rom;
};

void GSFLogger(struct mLogger *logger, int category, enum mLogLevel level, const char *format, va_list args) {
	(void)logger;
	(void)category;
	(void)level;
	(void)format;
	(void)args;
}

static struct mLogger gsf_logger = {
	.log = GSFLogger,
};

struct ncsf_loader_state {
	uint32_t sseq;
	std::vector<uint8_t> sdatData;
	std::unique_ptr<SDAT> sdat;

	std::vector<uint8_t> outputBuffer;

	ncsf_loader_state()
	: sseq(0) {
	}
};

static int ncsf_loader(void *context, const uint8_t *exe, size_t exe_size,
                       const uint8_t *reserved, size_t reserved_size) {
	struct ncsf_loader_state *state = (struct ncsf_loader_state *)context;

	if(reserved_size >= 4) {
		state->sseq = get_le32(reserved);
	}

	if(exe_size >= 12) {
		uint32_t sdat_size = get_le32(exe + 8);
		if(sdat_size > exe_size) return -1;

		if(state->sdatData.empty())
			state->sdatData.resize(sdat_size, 0);
		else if(state->sdatData.size() < sdat_size)
			state->sdatData.resize(sdat_size);
		memcpy(&state->sdatData[0], exe, sdat_size);
	}

	return 0;
}

struct twosf_loader_state {
	uint8_t *rom;
	uint8_t *state;
	size_t rom_size;
	size_t state_size;

	int initial_frames;
	int sync_type;
	int clockdown;
	int arm9_clockdown_level;
	int arm7_clockdown_level;
};

static int load_twosf_map(struct twosf_loader_state *state, int issave, const unsigned char *udata, unsigned usize) {
	if(usize < 8) return -1;

	unsigned char *iptr;
	size_t isize;
	unsigned char *xptr;
	unsigned xsize = get_le32(udata + 4);
	unsigned xofs = get_le32(udata + 0);
	if(issave) {
		iptr = state->state;
		isize = state->state_size;
		state->state = 0;
		state->state_size = 0;
	} else {
		iptr = state->rom;
		isize = state->rom_size;
		state->rom = 0;
		state->rom_size = 0;
	}
	if(!iptr) {
		size_t rsize = xofs + xsize;
		if(!issave) {
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		iptr = (unsigned char *)malloc(rsize + 10);
		if(!iptr)
			return -1;
		memset(iptr, 0, rsize + 10);
		isize = rsize;
	} else if(isize < xofs + xsize) {
		size_t rsize = xofs + xsize;
		if(!issave) {
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		xptr = (unsigned char *)realloc(iptr, xofs + rsize + 10);
		if(!xptr) {
			free(iptr);
			return -1;
		}
		iptr = xptr;
		isize = rsize;
	}
	memcpy(iptr + xofs, udata + 8, xsize);
	if(issave) {
		state->state = iptr;
		state->state_size = isize;
	} else {
		state->rom = iptr;
		state->rom_size = isize;
	}
	return 0;
}

static int load_twosf_mapz(struct twosf_loader_state *state, int issave, const unsigned char *zdata, unsigned zsize, unsigned zcrc) {
	int ret;
	int zerr;
	uLongf usize = 8;
	uLongf rsize = usize;
	unsigned char *udata;
	unsigned char *rdata;

	udata = (unsigned char *)malloc(usize);
	if(!udata)
		return -1;

	while(Z_OK != (zerr = uncompress(udata, &usize, zdata, zsize))) {
		if(Z_MEM_ERROR != zerr && Z_BUF_ERROR != zerr) {
			free(udata);
			return -1;
		}
		if(usize >= 8) {
			usize = get_le32(udata + 4) + 8;
			if(usize < rsize) {
				rsize += rsize;
				usize = rsize;
			} else
				rsize = usize;
		} else {
			rsize += rsize;
			usize = rsize;
		}
		rdata = (unsigned char *)realloc(udata, usize);
		if(!rdata) {
			free(udata);
			return -1;
		}
		udata = rdata;
	}

	rdata = (unsigned char *)realloc(udata, usize);
	if(!rdata) {
		free(udata);
		return -1;
	}

	{
		uLong ccrc = crc32(crc32(0L, Z_NULL, 0), rdata, (uInt)usize);
		if(ccrc != zcrc) {
			free(rdata);
			return -1;
		}
	}

	ret = load_twosf_map(state, issave, rdata, (unsigned)usize);
	free(rdata);
	return ret;
}

static int twosf_loader(void *context, const uint8_t *exe, size_t exe_size,
                        const uint8_t *reserved, size_t reserved_size) {
	struct twosf_loader_state *state = (struct twosf_loader_state *)context;

	if(exe_size >= 8) {
		if(load_twosf_map(state, 0, exe, (unsigned)exe_size))
			return -1;
	}

	if(reserved_size) {
		size_t resv_pos = 0;
		if(reserved_size < 16)
			return -1;
		while(resv_pos + 12 < reserved_size) {
			unsigned save_size = get_le32(reserved + resv_pos + 4);
			unsigned save_crc = get_le32(reserved + resv_pos + 8);
			if(get_le32(reserved + resv_pos + 0) == 0x45564153) {
				if(resv_pos + 12 + save_size > reserved_size)
					return -1;
				if(load_twosf_mapz(state, 1, reserved + resv_pos + 12, save_size, save_crc))
					return -1;
			}
			resv_pos += 12 + save_size;
		}
	}

	return 0;
}

static int twosf_info(void *context, const char *name, const char *value) {
	struct twosf_loader_state *state = (struct twosf_loader_state *)context;

	NSString *sname = [guess_encoding_of_string(name) lowercaseString];
	NSString *svalue = guess_encoding_of_string(value);

	if([sname isEqualToString:@"_frames"]) {
		state->initial_frames = [svalue intValue];
	} else if([sname isEqualToString:@"_clockdown"]) {
		state->clockdown = [svalue intValue];
	} else if([sname isEqualToString:@"_vio2sf_sync_type"]) {
		state->sync_type = [svalue intValue];
	} else if([sname isEqualToString:@"_vio2sf_arm9_clockdown_level"]) {
		state->arm9_clockdown_level = [svalue intValue];
	} else if([sname isEqualToString:@"_vio2sf_arm7_clockdown_level"]) {
		state->arm7_clockdown_level = [svalue intValue];
	}

	return 0;
}

struct usf_loader_state {
	uint32_t enablecompare;
	uint32_t enablefifofull;

	void *emu_state;
};

static int usf_loader(void *context, const uint8_t *exe, size_t exe_size,
                      const uint8_t *reserved, size_t reserved_size) {
	struct usf_loader_state *uUsf = (struct usf_loader_state *)context;
	if(exe && exe_size > 0) return -1;

	return usf_upload_section(uUsf->emu_state, reserved, reserved_size);
}

static int usf_info(void *context, const char *name, const char *value) {
	struct usf_loader_state *uUsf = (struct usf_loader_state *)context;

	NSString *sname = [guess_encoding_of_string(name) lowercaseString];
	NSString *svalue = guess_encoding_of_string(value);

	if([sname isEqualToString:@"_enablecompare"] && [svalue length])
		uUsf->enablecompare = 1;
	else if([sname isEqualToString:@"_enablefifofull"] && [svalue length])
		uUsf->enablefifofull = 1;

	return 0;
}

- (BOOL)initializeDecoder {
	silenceSeconds = 5;

	usfRemoveSilence = NO;

	if(type == 1) {
		emulatorCore = (uint8_t *)malloc(psx_get_state_size(1));

		psx_clear_state(emulatorCore, 1);

		struct psf1_load_state state;

		state.emu = emulatorCore;
		state.first = true;
		state.refresh = 0;

		if(psf_load([currentUrl UTF8String], &source_callbacks, 1, psf1_loader, &state, psf1_info, &state, 1) <= 0)
			return NO;

		if(state.refresh)
			psx_set_refresh(emulatorCore, state.refresh);

		silenceSeconds = 30;
	} else if(type == 2) {
		emulatorExtra = psf2fs_create();

		struct psf1_load_state state;

		state.refresh = 0;

		if(psf_load([currentUrl UTF8String], &source_callbacks, 2, psf2fs_load_callback, emulatorExtra, psf1_info, &state, 1) <= 0)
			return NO;

		emulatorCore = (uint8_t *)malloc(psx_get_state_size(2));

		psx_clear_state(emulatorCore, 2);

		if(state.refresh)
			psx_set_refresh(emulatorCore, state.refresh);

		psx_set_readfile(emulatorCore, virtual_readfile, emulatorExtra);

		silenceSeconds = 30;
	} else if(type == 0x11 || type == 0x12) {
		struct sdsf_loader_state state;
		memset(&state, 0, sizeof(state));

		if(psf_load([currentUrl UTF8String], &source_callbacks, type, sdsf_loader, &state, 0, 0, 0) <= 0)
			return NO;

		emulatorCore = (uint8_t *)malloc(sega_get_state_size(type - 0x10));

		sega_clear_state(emulatorCore, type - 0x10);

		sega_enable_dry(emulatorCore, 1);
		sega_enable_dsp(emulatorCore, 1);

		sega_enable_dsp_dynarec(emulatorCore, 0);

		uint32_t start = *(uint32_t *)state.data;
		size_t length = state.data_size;
		const size_t max_length = (type == 0x12) ? 0x800000 : 0x80000;
		if((start + (length - 4)) > max_length) {
			length = max_length - start + 4;
		}
		sega_upload_program(emulatorCore, state.data, (uint32_t)length);

		free(state.data);
	} else if(type == 0x21) {
		struct usf_loader_state state;
		memset(&state, 0, sizeof(state));

		state.emu_state = malloc(usf_get_state_size());
		if(!state.emu_state)
			return NO;

		usf_clear(state.emu_state);

		usf_set_hle_audio(state.emu_state, 1);

		emulatorCore = (uint8_t *)state.emu_state;

		if(psf_load([currentUrl UTF8String], &source_callbacks, 0x21, usf_loader, &state, usf_info, &state, 1) <= 0)
			return NO;

		usf_set_compare(state.emu_state, state.enablecompare);
		usf_set_fifo_full(state.emu_state, state.enablefifofull);

		usfRemoveSilence = YES;

		silenceSeconds = 10;
	} else if(type == 0x22) {
		struct gsf_loader_state state;
		memset(&state, 0, sizeof(state));

		if(psf_load([currentUrl UTF8String], &source_callbacks, 0x22, gsf_loader, &state, 0, 0, 0) <= 0)
			return NO;

		if(state.data_size > UINT_MAX)
			return NO;

		/*FILE * f = fopen("/tmp/rom.gba", "wb");
		fwrite(state.data, 1, state.data_size, f);
		fclose(f);*/

		struct VFile *rom = VFileFromConstMemory(state.data, state.data_size);
		if(!rom) {
			free(state.data);
			return NO;
		}

		struct mCore *core = mCoreFindVF(rom);
		if(!core) {
			free(state.data);
			return NO;
		}

		struct gsf_running_state *rstate = (struct gsf_running_state *)calloc(1, sizeof(*rstate));
		if(!rstate) {
			core->deinit(core);
			free(state.data);
			return NO;
		}

		rstate->rom = state.data;

		core->init(core);
		core->setAVStream(core, &rstate->stream);
		mCoreInitConfig(core, NULL);

		core->setAudioBufferSize(core, 2048);

		struct mCoreOptions opts = {
			.useBios = false,
			.skipBios = true,
			.volume = 0x100,
			.sampleRate = 32768,
		};

		mCoreConfigLoadDefaults(&core->config, &opts);

		core->loadROM(core, rom);
		core->reset(core);

		emulatorCore = (uint8_t *)core;
		emulatorExtra = rstate;

		sampleRate = 65536; // XXX
	} else if(type == 0x24) {
		struct twosf_loader_state state;
		memset(&state, 0, sizeof(state));
		state.initial_frames = -1;

		if(psf_load([currentUrl UTF8String], &source_callbacks, 0x24, twosf_loader, &state, twosf_info, &state, 1) <= 0) {
			if(state.rom) free(state.rom);
			if(state.state) free(state.state);
			return NO;
		}

		if(state.rom_size > UINT_MAX || state.state_size > UINT_MAX) {
			if(state.rom) free(state.rom);
			if(state.state) free(state.state);
			return NO;
		}

		NDS_state *core = (NDS_state *)calloc(1, sizeof(NDS_state));
		if(!core) {
			if(state.rom) free(state.rom);
			if(state.state) free(state.state);
			return NO;
		}

		if(state_init(core)) {
			state_deinit(core);
			if(state.rom) free(state.rom);
			if(state.state) free(state.state);
			return NO;
		}

		int resampling_int = -1;
		NSString *resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
		if([resampling isEqualToString:@"zoh"])
			resampling_int = 0;
		else if([resampling isEqualToString:@"blep"])
			resampling_int = 1;
		else if([resampling isEqualToString:@"linear"])
			resampling_int = 2;
		else if([resampling isEqualToString:@"blam"])
			resampling_int = 3;
		else if([resampling isEqualToString:@"cubic"])
			resampling_int = 4;
		else if([resampling isEqualToString:@"sinc"])
			resampling_int = 5;

		core->dwInterpolation = resampling_int;
		core->dwChannelMute = 0;

		if(!state.arm7_clockdown_level)
			state.arm7_clockdown_level = state.clockdown;
		if(!state.arm9_clockdown_level)
			state.arm9_clockdown_level = state.clockdown;

		core->initial_frames = state.initial_frames;
		core->sync_type = state.sync_type;
		core->arm7_clockdown_level = state.arm7_clockdown_level;
		core->arm9_clockdown_level = state.arm9_clockdown_level;

		emulatorCore = (uint8_t *)core;
		emulatorExtra = state.rom;

		if(state.rom)
			state_setrom(core, state.rom, (u32)state.rom_size, 0);

		state_loadstate(core, state.state, (u32)state.state_size);

		if(state.state) free(state.state);
	} else if(type == 0x25) {
		struct ncsf_loader_state *state = NULL;
		Player *player = NULL;

		try {
			state = new struct ncsf_loader_state;

			if(psf_load([currentUrl UTF8String], &source_callbacks, 0x25, ncsf_loader, state, 0, 0, 0) <= 0) {
				delete state;
				return NO;
			}

			player = new Player;

			player->interpolation = INTERPOLATION_SINC;

			PseudoFile file;
			file.data = &state->sdatData;

			state->sdat.reset(new SDAT(file, state->sseq));

			auto *sseqToPlay = state->sdat->sseq.get();

			player->sampleRate = 44100;
			player->Setup(sseqToPlay);
			player->Timer();

			state->outputBuffer.resize(1024 * sizeof(int16_t) * 2);

			emulatorCore = (uint8_t *)player;
			emulatorExtra = state;
		} catch (std::exception &e) {
			ALog(@"Exception caught creating NCSF player: %s", e.what());
			emulatorCore = NULL;
			emulatorExtra = NULL;
			delete player;
			delete state;
			return NO;
		}
	} else if(type == 0x41) {
		struct qsf_loader_state *state = (struct qsf_loader_state *)calloc(1, sizeof(*state));

		emulatorExtra = state;

		if(psf_load([currentUrl UTF8String], &source_callbacks, 0x41, qsf_loader, state, 0, 0, 0) <= 0)
			return NO;

		emulatorCore = (uint8_t *)malloc(qsound_get_state_size());

		qsound_clear_state(emulatorCore);

		if(state->key_size == 11) {
			uint8_t *ptr = state->key;
			uint32_t swap_key1 = get_be32(ptr + 0);
			uint32_t swap_key2 = get_be32(ptr + 4);
			uint32_t addr_key = get_be16(ptr + 8);
			uint8_t xor_key = *(ptr + 10);
			qsound_set_kabuki_key(emulatorCore, swap_key1, swap_key2, addr_key, xor_key);
		} else {
			qsound_set_kabuki_key(emulatorCore, 0, 0, 0, 0);
		}
		qsound_set_z80_rom(emulatorCore, state->z80_rom, state->z80_size);
		qsound_set_sample_rom(emulatorCore, state->sample_rom, state->sample_size);
	} else
		return NO;

	if(type == 1 || type == 2) {
		void *pIOP = psx_get_iop_state(emulatorCore);
		iop_set_compat(pIOP, IOP_COMPAT_HARSH);
	}

	framesRead = 0;

	silence_test_buffer.resize(sampleRate * silenceSeconds * 2);

	if(![self fillBuffer])
		return NO;

	if(type == 0x22) {
		// XXX needs to emulate a bit before this becomes correct
		struct mCore *core = (struct mCore *)emulatorCore;
		sampleRate = core->audioSampleRate(core);
	}

	silence_test_buffer.remove_leading_silence();

	return YES;
}

- (BOOL)open:(id<CogSource>)source {
	if(![source seekable]) {
		return NO;
	}

	currentSource = source;

	struct psf_info_meta_state info;

	info.info = [NSMutableDictionary dictionary];
	info.utf8 = false;
	info.tag_length_ms = 0;
	info.tag_fade_ms = 0;

	info.albumGain = 0;
	info.albumPeak = 0;
	info.trackGain = 0;
	info.trackPeak = 0;
	info.volume = 1;

	currentUrl = [[[source url] absoluteString] stringByRemovingPercentEncoding];

	[[psf_file_container instance] add_hint:currentUrl source:currentSource];
	hintAdded = YES;

	type = psf_load([currentUrl UTF8String], &source_callbacks, 0, 0, 0, psf_info_meta, &info, 0);

	if(type <= 0)
		return NO;

	switch(type) {
		case 1:
		case 2:
		case 0x11:
		case 0x12:
		case 0x21:
		case 0x22:
		case 0x24:
		case 0x25:
		case 0x41:
			break;

		default:
			return NO;
	}

	emulatorCore = nil;
	emulatorExtra = nil;

	sampleRate = 44100;

	if(type == 2)
		sampleRate = 48000;
	else if(type == 0x22) {
		if(![self initializeDecoder])
			return NO;
		/* Move these into the above
		struct mCore *core = (struct mCore *)emulatorCore;
		sampleRate = core->audioSampleRate(core); */
	} else if(type == 0x41)
		sampleRate = 24038;

	tagLengthMs = info.tag_length_ms;
	tagFadeMs = info.tag_fade_ms;

	if(!tagLengthMs) {
		double defaultLength = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultSeconds"] doubleValue];
		double defaultFade = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
		if(defaultLength < 0) {
			defaultLength = 150.0;
		}
		if(defaultFade < 0) {
			defaultFade = 0;
		}

		tagLengthMs = (int)ceil(defaultLength * 1000.0);
		tagFadeMs = (int)ceil(defaultFade * 1000.0);
	}

	metadataList = info.info;

	framesLength = [self retrieveFrameCount:tagLengthMs];
	totalFrames = [self retrieveFrameCount:tagLengthMs + tagFadeMs];

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (BOOL)fillBuffer {
	long _totalFrames = totalFrames;
	if(!_totalFrames) // likely GSF early init
		_totalFrames = silenceSeconds * sampleRate;
	long frames_left = _totalFrames - framesRead - silence_test_buffer.data_available() / 2;
	long free_space = silence_test_buffer.free_space() / 2;
	if(IsRepeatOneSet())
		frames_left = free_space;
	if(free_space > frames_left)
		free_space = frames_left;
	while(free_space > 0) {
		unsigned long samples_to_write = 0;
		int16_t *buf = silence_test_buffer.get_write_ptr(samples_to_write);
		int samples_read = [self readAudioInternal:buf frames:(UInt32)samples_to_write / 2];
		if(!samples_read) break;
		silence_test_buffer.samples_written(samples_read * 2);
		free_space -= samples_read;
	}
	return !silence_test_buffer.test_silence();
}

- (int)readAudioInternal:(void *)buf frames:(UInt32)frames {
	if(type == 1 || type == 2) {
		uint32_t howmany = frames;
		psx_execute(emulatorCore, 0x7fffffff, (int16_t *)buf, &howmany, 0);
		frames = howmany;
	} else if(type == 0x11 || type == 0x12) {
		uint32_t howmany = frames;
		sega_execute(emulatorCore, 0x7fffffff, (int16_t *)buf, &howmany);
		frames = howmany;
	} else if(type == 0x21) {
		const char *err;
		if((err = usf_render_resampled(emulatorCore, (int16_t *)buf, frames, sampleRate)) != 0) {
			DLog(@"USF Error: %s", err);
			return 0;
		}
	} else if(type == 0x22) {
		struct mCore *core = (struct mCore *)emulatorCore;

		size_t howmany = frames;

		do {
			size_t frames_to_render = howmany;
			if(frames_to_render > 2048)
				frames_to_render = 2048;
			struct mAudioBuffer *buffer = core->getAudioBuffer(core);
			size_t frames_rendered = mAudioBufferRead(buffer, (int16_t *)buf, frames_to_render);
			if(frames_rendered) {
				buf = (void *)(((int16_t *)buf) + frames_rendered * 2);
				frames_to_render -= frames_rendered;
				howmany -= frames_rendered;
			}

			if(howmany) {
				while(!mAudioBufferAvailable(buffer)) {
					core->runFrame(core);
				}
			}
		} while(howmany);
	} else if(type == 0x24) {
		NDS_state *state = (NDS_state *)emulatorCore;
		state_render(state, (s16 *)buf, frames);
	} else if(type == 0x25) {
		try {
			Player *player = (Player *)emulatorCore;
			ncsf_loader_state *state = (ncsf_loader_state *)emulatorExtra;
			std::vector<uint8_t> &buffer = state->outputBuffer;
			unsigned long frames_to_do = frames;
			while(frames_to_do) {
				unsigned frames_this_run = 1024;
				if(frames_this_run > frames_to_do)
					frames_this_run = (unsigned int)frames_to_do;
				player->GenerateSamples(buffer, 0, frames_this_run);
				memcpy(buf, &buffer[0], frames_this_run * sizeof(int16_t) * 2);
				buf = ((uint8_t *)buf) + frames_this_run * sizeof(int16_t) * 2;
				frames_to_do -= frames_this_run;
			}
		} catch (std::exception &e) {
			ALog(@"Exception caught while playing NCSF: %s", e.what());
			return 0;
		}
	} else if(type == 0x41) {
		uint32_t howmany = frames;
		qsound_execute(emulatorCore, 0x7fffffff, (int16_t *)buf, &howmany);
		frames = howmany;
	}

	return frames;
}

- (AudioChunk *)readAudio {
	if(!emulatorCore) {
		if(![self initializeDecoder])
			return 0;
	} else if(![self fillBuffer])
		return 0;

	if(usfRemoveSilence) {
		silence_test_buffer.remove_leading_silence();
		usfRemoveSilence = NO;
	}

	int frames = 1024;
	int16_t buffer[frames * 2];
	void *buf = (void *)buffer;

	unsigned long written = silence_test_buffer.data_available() / 2;
	if(written > frames)
		written = frames;
	if(!IsRepeatOneSet() && written > totalFrames - framesRead)
		written = totalFrames - framesRead;
	if(written == 0)
		return 0;

	silence_test_buffer.read((int16_t *)buf, written * 2);

	if(!IsRepeatOneSet() && framesRead + written > framesLength) {
		long fadeStart = (framesLength > framesRead) ? framesLength : framesRead;
		long fadeEnd = framesRead + written;
		long fadeTotal = totalFrames - framesLength;
		long fadePos;

		int16_t *buf16 = (int16_t *)buf;

		for(fadePos = fadeStart; fadePos < fadeEnd; ++fadePos) {
			long scale = totalFrames - fadePos;
			buf16[0] = buf16[0] * scale / fadeTotal;
			buf16[1] = buf16[1] * scale / fadeTotal;
			buf16 += 2;
		}
	}

	double streamTimestamp = (double)(framesRead) / (double)(sampleRate);

	framesRead += written;

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	[chunk setStreamTimestamp:streamTimestamp];
	[chunk assignSamples:buffer frameCount:written];

	return chunk;
}

- (void)closeDecoder {
	if(emulatorCore) {
		if(type == 0x21) {
			usf_shutdown(emulatorCore);
			free(emulatorCore);
		} else if(type == 0x22) {
			struct mCore *core = (struct mCore *)emulatorCore;
			mCoreConfigDeinit(&core->config);
			core->deinit(core);
		} else if(type == 0x24) {
			NDS_state *state = (NDS_state *)emulatorCore;
			state_deinit(state);
			free(state);
		} else if(type == 0x25) {
			try {
				Player *player = (Player *)emulatorCore;
				delete player;
			} catch (std::exception &e) {
				ALog(@"Exception caught deleting NCSF player: %s", e.what());
			}
		} else {
			free(emulatorCore);
		}
		emulatorCore = nil;
	}

	if(emulatorExtra) {
		if(type == 2) {
			psf2fs_delete(emulatorExtra);
		} else if(type == 0x22) {
			struct gsf_running_state *rstate = (struct gsf_running_state *)emulatorExtra;
			free(rstate->rom);
			free(rstate);
		} else if(type == 0x24) {
			free(emulatorExtra);
		} else if(type == 0x25) {
			try {
				struct ncsf_loader_state *state = (struct ncsf_loader_state *)emulatorExtra;
				delete state;
			} catch (std::exception &e) {
				ALog(@"Exception caught deleting NCSF state: %s", e.what());
			}
		} else if(type == 0x41) {
			struct qsf_loader_state *state = (struct qsf_loader_state *)emulatorExtra;
			free(state->key);
			free(state->z80_rom);
			free(state->sample_rom);
			free(state);
		}
		emulatorExtra = nil;
	}
}

- (void)close {
	[self closeDecoder];
	currentSource = nil;
	if(hintAdded) {
		[[psf_file_container instance] remove_hint:currentUrl];
		hintAdded = NO;
	}
	currentUrl = nil;
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	if(frame < framesRead || emulatorCore == NULL) {
		[self closeDecoder];
		if(![self initializeDecoder])
			return -1;
		if(usfRemoveSilence) {
			silence_test_buffer.remove_leading_silence();
			usfRemoveSilence = NO;
		}
	}

	unsigned long buffered_samples = silence_test_buffer.data_available() / 2;
	if(buffered_samples >= (frame - framesRead)) {
		frame -= framesRead;
		silence_test_buffer.read(NULL, frame * 2);
		framesRead += frame;
		return framesRead;
	} else if(buffered_samples) {
		silence_test_buffer.read(NULL, buffered_samples * 2);
		framesRead += buffered_samples;
	}

	if(type == 1 || type == 2) {
		do {
			uint32_t howmany = (uint32_t)(frame - framesRead);
			if(psx_execute(emulatorCore, 0x7fffffff, 0, &howmany, 0) < 0) break;
			framesRead += howmany;
		} while(framesRead < frame);
	} else if(type == 0x11 || type == 0x12) {
		do {
			uint32_t howmany = (uint32_t)(frame - framesRead);
			if(sega_execute(emulatorCore, 0x7fffffff, 0, &howmany) < 0) break;
			framesRead += howmany;
		} while(framesRead < frame);
	} else if(type == 0x21) {
		do {
			ssize_t howmany = frame - framesRead;
			if(howmany > 1024) howmany = 1024;
			if(usf_render_resampled(emulatorCore, NULL, howmany, sampleRate) != 0)
				return -1;
			framesRead += howmany;
		} while(framesRead < frame);
	} else if(type == 0x22) {
		struct mCore *core = (struct mCore *)emulatorCore;

		size_t howmany = frame - framesRead;

		struct mAudioBuffer *buffer = core->getAudioBuffer(core);

		int16_t temp[2048 * 2];

		do {
			size_t frames_to_run = howmany;
			if(frames_to_run > 2048)
				frames_to_run = 2048;

			size_t removed = mAudioBufferRead(buffer, temp, frames_to_run);
			howmany -= removed;

			if(howmany) {
				while(!mAudioBufferAvailable(buffer))
					core->runFrame(core);
			}
		} while(howmany);

		framesRead = frame;
	} else if(type == 0x24) {
		NDS_state *state = (NDS_state *)emulatorCore;
		s16 temp[2048];

		long frames_to_run = frame - framesRead;

		while(frames_to_run) {
			unsigned frames_this_run = 1024;
			if(frames_this_run > frames_to_run)
				frames_this_run = (unsigned)frames_to_run;

			state_render(state, temp, frames_this_run);

			frames_to_run -= frames_this_run;
		}

		framesRead = frame;
	} else if(type == 0x25) {
		try {
			Player *player = (Player *)emulatorCore;
			ncsf_loader_state *state = (ncsf_loader_state *)emulatorExtra;
			std::vector<uint8_t> &buffer = state->outputBuffer;

			long frames_to_run = frame - framesRead;

			while(frames_to_run) {
				int frames_to_render = 1024;
				if(frames_to_render > frames_to_run) frames_to_render = (int)frames_to_run;

				player->GenerateSamples(buffer, 0, frames_to_render);

				frames_to_run -= frames_to_render;
			}

			framesRead = frame;
		} catch (std::exception &e) {
			ALog(@"Exception caught while seeking in NCSF: %s", e.what());
			framesRead = 0;
			return -1;
		}
	} else if(type == 0x41) {
		do {
			uint32_t howmany = (uint32_t)(frame - framesRead);
			if(qsound_execute(emulatorCore, 0x7fffffff, 0, &howmany) < 0) break;
			framesRead += howmany;
		} while(framesRead < frame);
	}

	return framesRead;
}

- (NSDictionary *)properties {
	NSString *codec = @"";
	switch(type) {
		case 1:
			codec = @"PSF";
			break;
		case 2:
			codec = @"PSF2";
			break;
		case 0x11:
			codec = @"SSF";
			break;
		case 0x12:
			codec = @"DSF";
			break;
		case 0x21:
			codec = @"USF";
			break;
		case 0x22:
			codec = @"GSF";
			break;
		case 0x24:
			codec = @"2SF";
			break;
		case 0x25:
			codec = @"NCSF";
			break;
		case 0x41:
			codec = @"QSF";
			break;
	}

	return @{ @"channels": @(2),
		      @"bitsPerSample": @(16),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(totalFrames),
		      @"bitrate": @(0),
		      @"seekable": @(YES),
		      @"codec": codec,
		      @"endian": @"host",
		      @"encoding": @"synthesized" };
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	struct psf_info_meta_state info;

	info.info = [NSMutableDictionary dictionary];
	info.utf8 = false;
	info.tag_length_ms = 0;
	info.tag_fade_ms = 0;

	NSString *decodedUrl = [[url absoluteString] stringByRemovingPercentEncoding];

	psf_load([decodedUrl UTF8String], &source_callbacks, 0, 0, 0, psf_info_meta, &info, 0);

	return [NSDictionary dictionaryWithDictionary:info.info];
}

+ (NSArray *)fileTypes {
	return @[@"psf", @"minipsf", @"psf2", @"minipsf2", @"ssf", @"minissf", @"dsf", @"minidsf", @"qsf", @"miniqsf", @"gsf", @"minigsf", @"ncsf", @"minincsf", @"2sf", @"mini2sf", @"usf", @"miniusf"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-psf"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	NSMutableArray *ret = [NSMutableArray new];
	[ret addObject:@"PSF Format Files"];
	[ret addObject:@"vg.icns"];
	[ret addObjectsFromArray:[self fileTypes]];

	return @[[NSArray arrayWithArray:ret]];
}

@end
