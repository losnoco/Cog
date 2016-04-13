#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#include "SCCore.h"

SCCore::SCCore()
{
	duped = false;
	path = 0;
	
	handle = 0;
	
	TG_initialize = 0;
	//TG_terminate = 0;
	TG_activate = 0;
	TG_deactivate = 0;
	TG_setSampleRate = 0;
	TG_setMaxBlockSize = 0;
	TG_flushMidi = 0;
	TG_setInterruptThreadIdAtThisTime = 0;
	// TG_PMidiIn = 0;
	TG_ShortMidiIn = 0;
	TG_LongMidiIn = 0;
	// TG_isFatalError = 0;
	// TG_getErrorStrings = 0;
	TG_XPgetCurTotalRunningVoices = 0;
	// TG_XPsetSystemConfig = 0;
	// TG_XPgetCurSystemConfig = 0;
	TG_Process = 0;
}

SCCore::~SCCore()
{
	Unload();
}

void SCCore::Unload()
{
	if (handle)
	{
		dlclose(handle);
		handle = 0;
	}
	if (duped && path)
	{
		unlink(path);
		duped = false;
	}
	if (path)
	{
		free(path);
		path = 0;
	}
}

static const char name_template[] = "/tmp/SCCore.dylib.XXXXXXXX";

bool SCCore::Load(const char * _path, bool dupe)
{
    uintptr_t size = 0;
    
	if (dupe)
	{
		path = (char *) malloc(strlen(name_template) + 1);
		strcpy(path, name_template);
		mktemp(path);
		
		const char * uniq = path + strlen(path) - 8;
		
		FILE * f = fopen(_path, "rb");
		if (!f) return false;
		
		fseek(f, 0, SEEK_END);
		size_t fs = ftell(f);
		fseek(f, 0, SEEK_SET);
        
        size = fs;
		
		unsigned char * buffer = (unsigned char *) malloc(fs);
		if (!fs)
		{
			fclose(f);
			return false;
		}

		fread(buffer, 1, fs, f);
		fclose(f);
		
		for (size_t i = 0; i < fs - 14; ++i)
		{
			if (memcmp(buffer + i, "SCCore00.dylib", 14) == 0)
			{
				memcpy(buffer + i, uniq, 8);
				i += 13;
			}
		}
		
		duped = true;
		
		f = fopen(path, "wb");
		if (!f)
		{
			free(buffer);
			return false;
		}
		
		fwrite(buffer, 1, fs, f);
		fclose(f);
		
		free(buffer);
	}
	else
	{
		path = (char *) malloc(strlen(_path) + 1);
		strcpy(path, _path);
        
        FILE * f = fopen(path, "rb");
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fclose(f);
	}
	
	handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
	if (handle)
	{
		*(void**)&TG_initialize = dlsym(handle, "TG_initialize");
		//*(void**)&TG_terminate = dlsym(handle, "TG_terminate");
		*(void**)&TG_activate = dlsym(handle, "TG_activate");
		*(void**)&TG_deactivate = dlsym(handle, "TG_deactivate");
		*(void**)&TG_setSampleRate = dlsym(handle, "TG_setSampleRate");
		*(void**)&TG_setMaxBlockSize = dlsym(handle, "TG_setMaxBlockSize");
		*(void**)&TG_flushMidi = dlsym(handle, "TG_flushMidi");
		*(void**)&TG_setInterruptThreadIdAtThisTime = dlsym(handle, "TG_setInterruptThreadIdAtThisTime");
		//*(void**)&TG_PMidiIn = dlsym(handle, "TG_PMidiIn");
		*(void**)&TG_ShortMidiIn = dlsym(handle, "TG_ShortMidiIn");
		*(void**)&TG_LongMidiIn = dlsym(handle, "TG_LongMidiIn");
		//*(void**)&TG_isFatalError = dlsym(handle, "TG_isFatalError");
		//*(void**)&TG_getErrorStrings = dlsym(handle, "TG_getErrorStrings");
		*(void**)&TG_XPgetCurTotalRunningVoices = dlsym(handle, "TG_XPgetCurTotalRunningVoices");
		//*(void**)&TG_XPsetSystemConfig = dlsym(handle, "TG_XPsetSystemConfig");
		//*(void**)&TG_XPgetCurSystemConfig = dlsym(handle, "TG_XPgetCurSystemConfig");
		*(void**)&TG_Process = dlsym(handle, "TG_Process");
		
		if (TG_initialize && /*TG_terminate &&*/ TG_activate && TG_deactivate &&
		   TG_setSampleRate && TG_setMaxBlockSize && TG_flushMidi &&
		   TG_setInterruptThreadIdAtThisTime && /*TG_PMidiIn &&*/
		   TG_ShortMidiIn && TG_LongMidiIn && /*TG_isFatalError &&
		   TG_getErrorStrings &&*/ TG_XPgetCurTotalRunningVoices &&
		   /*TG_XPsetSystemConfig && TG_XPgetCurSystemConfig &&*/
		   TG_Process)
		{
			return true;
		}
		else
		{
			TG_initialize = 0;
			//TG_terminate = 0;
			TG_activate = 0;
			TG_deactivate = 0;
			TG_setSampleRate = 0;
			TG_setMaxBlockSize = 0;
			TG_flushMidi = 0;
			TG_setInterruptThreadIdAtThisTime = 0;
			// TG_PMidiIn = 0;
			TG_ShortMidiIn = 0;
			TG_LongMidiIn = 0;
			// TG_isFatalError = 0;
			// TG_getErrorStrings = 0;
			TG_XPgetCurTotalRunningVoices = 0;
			// TG_XPsetSystemConfig = 0;
			// TG_XPgetCurSystemConfig = 0;
			TG_Process = 0;
			
			dlclose(handle);
			handle = 0;
		}
	}
	
	return false;
}
