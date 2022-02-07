#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SCCore.h"

static unsigned long g_serial = 0;

SCCore::SCCore() {
	serial = g_serial++;
	path = 0;

	handle = 0;

	TG_initialize = 0;
	// TG_terminate = 0;
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

SCCore::~SCCore() {
	Unload();
}

void SCCore::Unload() {
	if(handle) {
		dlclose(handle);
		handle = 0;
	}
	if(path) {
		free(path);
		path = 0;
	}
}

bool SCCore::Load(const char *_path, bool dupe) {
	path = (char *)malloc(strlen(_path) + 1);
	strcpy(path, _path);

	if(dupe) {
		char *name = strstr(path, "SCCore00");
		if(name) {
			unsigned long serial_wrapped = serial % 32;
			name[6] = '0' + (serial_wrapped / 10);
			name[7] = '0' + (serial_wrapped % 10);
		}
	}

	handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
	if(handle) {
		*(void **)&TG_initialize = dlsym(handle, "TG_initialize");
		//*(void**)&TG_terminate = dlsym(handle, "TG_terminate");
		*(void **)&TG_activate = dlsym(handle, "TG_activate");
		*(void **)&TG_deactivate = dlsym(handle, "TG_deactivate");
		*(void **)&TG_setSampleRate = dlsym(handle, "TG_setSampleRate");
		*(void **)&TG_setMaxBlockSize = dlsym(handle, "TG_setMaxBlockSize");
		*(void **)&TG_flushMidi = dlsym(handle, "TG_flushMidi");
		*(void **)&TG_setInterruptThreadIdAtThisTime = dlsym(handle, "TG_setInterruptThreadIdAtThisTime");
		//*(void**)&TG_PMidiIn = dlsym(handle, "TG_PMidiIn");
		*(void **)&TG_ShortMidiIn = dlsym(handle, "TG_ShortMidiIn");
		*(void **)&TG_LongMidiIn = dlsym(handle, "TG_LongMidiIn");
		//*(void**)&TG_isFatalError = dlsym(handle, "TG_isFatalError");
		//*(void**)&TG_getErrorStrings = dlsym(handle, "TG_getErrorStrings");
		*(void **)&TG_XPgetCurTotalRunningVoices = dlsym(handle, "TG_XPgetCurTotalRunningVoices");
		//*(void**)&TG_XPsetSystemConfig = dlsym(handle, "TG_XPsetSystemConfig");
		//*(void**)&TG_XPgetCurSystemConfig = dlsym(handle, "TG_XPgetCurSystemConfig");
		*(void **)&TG_Process = dlsym(handle, "TG_Process");

		if(TG_initialize && /*TG_terminate &&*/ TG_activate && TG_deactivate &&
		   TG_setSampleRate && TG_setMaxBlockSize && TG_flushMidi &&
		   TG_setInterruptThreadIdAtThisTime && /*TG_PMidiIn &&*/
		   TG_ShortMidiIn && TG_LongMidiIn && /*TG_isFatalError &&
		   TG_getErrorStrings &&*/
		   TG_XPgetCurTotalRunningVoices &&
		   /*TG_XPsetSystemConfig && TG_XPgetCurSystemConfig &&*/
		   TG_Process) {
			return true;
		} else {
			TG_initialize = 0;
			// TG_terminate = 0;
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
