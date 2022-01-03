#ifndef __EMU_LOGGING_H__
#define __EMU_LOGGING_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"
#include "../common_def.h"
#include "EmuStructs.h"


typedef struct _device_logger
{
	DEVCB_LOG func;
	void* source;
	void* param;
} DEV_LOGGER;

INLINE void dev_logger_set(DEV_LOGGER* logger, void* source, DEVCB_LOG func, void* param)
{
	logger->func = func;
	logger->source = source;
	logger->param = param;
	return;
}

void emu_logf(DEV_LOGGER* logger, UINT8 level, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif	// __EMU_LOGGING_H__
