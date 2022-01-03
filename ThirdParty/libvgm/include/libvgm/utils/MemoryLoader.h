#ifndef __MEMORYLOADER_H__
#define __MEMORYLOADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdtype.h"
#include "DataLoader.h"

DATA_LOADER *MemoryLoader_Init(const UINT8 *buffer, UINT32 length);
#define MemoryLoader_Load				DataLoader_Load
#define MemoryLoader_Reset				DataLoader_Reset
#define MemoryLoader_GetData			DataLoader_GetData
#define MemoryLoader_GetTotalSize		DataLoader_GetTotalSize
#define MemoryLoader_GetSize			DataLoader_GetSize
#define MemoryLoader_GetStatus			DataLoader_GetStatus
#define MemoryLoader_Read				DataLoader_Read
#define MemoryLoader_CancelLoading		DataLoader_CancelLoading
#define MemoryLoader_SetPreloadBytes	DataLoader_SetPreloadBytes
#define MemoryLoader_ReadUntil			DataLoader_ReadUntil
#define MemoryLoader_ReadAll			DataLoader_ReadAll
#define MemoryLoader_Deinit				DataLoader_Deinit

extern const DATA_LOADER_CALLBACKS memoryLoader;

#ifdef __cplusplus
}
#endif

#endif	// __MEMORYLOADER_H__
