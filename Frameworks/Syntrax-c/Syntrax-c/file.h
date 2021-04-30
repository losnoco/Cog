#ifndef FILE_H
#define FILE_H

#include <Syntrax_c/syntrax.h>

#ifdef __cplusplus
extern "C" {
#endif

Song* File_loadSong(const char *path);
Song* File_loadSongMem(const uint8_t *buffer, size_t size);

void File_freeSong(Song *synSong);

#ifdef __cplusplus
}
#endif

#endif
