#ifndef __FILELOADER_H__
#define __FILELOADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define HAVE_FILELOADER_W	1
#endif

#include "DataLoader.h"

DATA_LOADER *FileLoader_Init(const char *fileName);
#ifdef HAVE_FILELOADER_W
#include <wchar.h>
DATA_LOADER *FileLoader_InitW(const wchar_t *fileName);
#endif

#define FileLoader_Load				DataLoader_Load
#define FileLoader_Reset			DataLoader_Reset
#define FileLoader_GetData			DataLoader_GetData
#define FileLoader_GetTotalSize		DataLoader_GetTotalSize
#define FileLoader_GetSize			DataLoader_GetSize
#define FileLoader_GetStatus		DataLoader_GetStatus
#define FileLoader_Read				DataLoader_Read
#define FileLoader_CancelLoading	DataLoader_CancelLoading
#define FileLoader_SetPreloadBytes	DataLoader_SetPreloadBytes
#define FileLoader_ReadUntil		DataLoader_ReadUntil
#define FileLoader_ReadAll			DataLoader_ReadAll
#define FileLoader_Deinit			DataLoader_Deinit

extern const DATA_LOADER_CALLBACKS fileLoader;

#ifdef __cplusplus
}
#endif

#endif	// __FILELOADER_H__
