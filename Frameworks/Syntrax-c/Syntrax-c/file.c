#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

size_t filesize;

Song* File_loadSong(const char *path)
{
    Song *synSong;
    FILE *f;
    uint8_t *buffer;
    size_t size;
    
    if (!(f = fopen(path, "rb"))) return NULL;
    
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (!(buffer = (uint8_t *) malloc(size))) {
        fclose(f);
        return NULL;
    }
    
    if (fread(buffer, 1, size, f) != size) {
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    
    synSong = File_loadSongMem(buffer, size);
    
    free(buffer);
    
    return synSong;
}

uint16_t get_le16(const void *p)
{
    return (((const uint8_t*)p)[0]) +
        (((const uint8_t*)p)[1]) * 0x100;
}

uint32_t get_le32(const void *p)
{
    return (((const uint8_t*)p)[0]) +
        (((const uint8_t*)p)[1]) * 0x100 +
        (((const uint8_t*)p)[2]) * 0x10000 +
        (((const uint8_t*)p)[3]) * 0x1000000;
}

static long File_readHeader(SongHeader *h, const uint8_t *buffer, size_t size)
{
    if (size < 52) return -1;

    h->version    = get_le16(buffer);
    h->UNK00      = get_le16(buffer + 2);
    h->patNum     = get_le32(buffer + 4);
    h->subsongNum = get_le32(buffer + 8);
    h->instrNum   = get_le32(buffer + 12);
    h->UNK01      = get_le32(buffer + 16);
    h->UNK02      = get_le16(buffer + 20);
    h->UNK03      = get_le16(buffer + 22);
    h->UNK04      = get_le16(buffer + 24);
    h->UNK05      = get_le16(buffer + 26);
    h->UNK06      = get_le16(buffer + 28);
    h->UNK07      = get_le16(buffer + 30);
    h->UNK08      = get_le16(buffer + 32);
    h->UNK09      = get_le16(buffer + 34);
    h->UNK0A      = get_le16(buffer + 36);
    h->UNK0B      = get_le16(buffer + 38);
    h->UNK0C      = get_le16(buffer + 40);
    h->UNK0D      = get_le16(buffer + 42);
    h->UNK0E      = get_le16(buffer + 44);
    h->UNK0F      = get_le16(buffer + 46);
    h->UNK10      = get_le16(buffer + 48);
    h->UNK11      = get_le16(buffer + 50);

    return 52;
}

static long File_readSubSong(Subsong *subSong, const uint8_t *buffer, size_t size)
{
    int i, j;
    
    if (size < 16564) return -1;
    
    for (i = 0; i < 16; i++)
        subSong->UNK00[i] = get_le32(buffer + i * 4);
    
    for (i = 0; i < SE_MAXCHANS; i++)
        subSong->mutedChans[i] = buffer[64 + i];
    
    subSong->tempo          = get_le32(buffer + 80);
    subSong->groove         = get_le32(buffer + 84);
    subSong->startPosCoarse = get_le32(buffer + 88);
    subSong->startPosFine   = get_le32(buffer + 92);
    subSong->endPosCoarse   = get_le32(buffer + 96);
    subSong->endPosFine     = get_le32(buffer + 100);
    subSong->loopPosCoarse  = get_le32(buffer + 104);
    subSong->loopPosFine    = get_le32(buffer + 108);
    subSong->isLooping      = get_le16(buffer + 112);

    memcpy(subSong->m_Name, buffer + 114, 32);
    subSong->m_Name[32] = '\0';
    
    subSong->channelNumber  = get_le16(buffer + 146);
    subSong->delayTime      = get_le16(buffer + 148);
    
    for (i = 0; i < SE_MAXCHANS; i++)
        subSong->chanDelayAmt[i] = buffer[150 + i];
    
    subSong->amplification  = get_le16(buffer + 166);

    subSong->UNK01          = get_le16(buffer + 168);
    subSong->UNK02          = get_le16(buffer + 170);
    subSong->UNK03          = get_le16(buffer + 172);
    subSong->UNK04          = get_le16(buffer + 174);
    subSong->UNK05          = get_le16(buffer + 176);
    subSong->UNK06          = get_le16(buffer + 178);

    for (i = 0; i < SE_MAXCHANS; i++) {
        for (j = 0; j < 0x100; j++) {
            subSong->orders[i][j].patIndex = get_le16(buffer + 180 + i * 1024 + j * 4);
            subSong->orders[i][j].patLen   = get_le16(buffer + 180 + i * 1024 + j * 4 + 2);
        }
    }
    
    return 16564;
}

long File_readRow(Row *r, const uint8_t *buffer, size_t size)
{
    if (size < 5) return -1;
    
    r->note    = buffer[0];
    r->dest    = buffer[1];
    r->instr   = buffer[2];
    r->spd     = buffer[3];
    r->command = buffer[4];
    
    return 5;
}

static long File_readInstrumentEffect(InstrumentEffect *effect, const uint8_t *buffer, size_t size)
{
    if (size < 40) return -1;
    
    effect->destWave    = get_le32(buffer);
    effect->srcWave1    = get_le32(buffer + 4);
    effect->srcWave2    = get_le32(buffer + 8);
    effect->oscWave     = get_le32(buffer + 12);
    effect->variable1   = get_le32(buffer + 16);
    effect->variable2   = get_le32(buffer + 20);
    effect->fxSpeed     = get_le32(buffer + 24);
    effect->oscSpeed    = get_le32(buffer + 28);
    effect->effectType  = get_le32(buffer + 32);
    effect->oscSelect   = buffer[36];
    effect->resetEffect = buffer[37];
    effect->UNK00       = get_le16(buffer + 38);
    
    return 40;
}

static long File_readInstrument(Instrument *instr, const uint8_t *buffer, size_t size)
{
    int i, j;
    long sizeRead;
    
    if (size < 8712) return -1;

    instr->version = get_le16(buffer);

    memcpy(instr->name, buffer + 2, 32);
    instr->name[32] = '\0';
    
    instr->waveform     = get_le16(buffer + 34);
    instr->wavelength   = get_le16(buffer + 36);
    instr->masterVolume = get_le16(buffer + 38);
    instr->amWave       = get_le16(buffer + 40);
    instr->amSpeed      = get_le16(buffer + 42);
    instr->amLoopPoint  = get_le16(buffer + 44);
    instr->finetune     = get_le16(buffer + 46);
    instr->fmWave       = get_le16(buffer + 48);
    instr->fmSpeed      = get_le16(buffer + 50);
    instr->fmLoopPoint  = get_le16(buffer + 52);
    instr->fmDelay      = get_le16(buffer + 54);
    instr->arpIndex     = get_le16(buffer + 56);
    
    for (i = 0; i < SE_MAXCHANS; i++)
        instr->m_ResetWave[i] = buffer[58 + i];
    
    instr->panWave      = get_le16(buffer + 74);
    instr->panSpeed     = get_le16(buffer + 76);
    instr->panLoopPoint = get_le16(buffer + 78);
    instr->UNK00        = get_le16(buffer + 80);
    instr->UNK01        = get_le16(buffer + 82);
    instr->UNK02        = get_le16(buffer + 84);
    instr->UNK03        = get_le16(buffer + 86);
    instr->UNK04        = get_le16(buffer + 88);
    instr->UNK05        = get_le16(buffer + 90);
    
    buffer += 92; size -= 92;
    
    for (i = 0; i < 4; i++) {
        sizeRead = File_readInstrumentEffect(&instr->effects[i], buffer, size);
        if (sizeRead < 0) return -1;
        buffer += sizeRead; size -= sizeRead;
    }
    
    memcpy(instr->smpFullImportPath, buffer, 192);
    instr->smpFullImportPath[192] = '\0';
    
    buffer += 192; size -= 192;

    instr->UNK06        = get_le32(buffer);
    instr->UNK07        = get_le32(buffer + 4);
    instr->UNK08        = get_le32(buffer + 8);
    instr->UNK09        = get_le32(buffer + 12);
    instr->UNK0A        = get_le32(buffer + 16);
    instr->UNK0B        = get_le32(buffer + 20);
    instr->UNK0C        = get_le32(buffer + 24);
    instr->UNK0D        = get_le32(buffer + 28);
    instr->UNK0E        = get_le32(buffer + 32);
    instr->UNK0F        = get_le32(buffer + 36);
    instr->UNK10        = get_le32(buffer + 40);
    instr->UNK11        = get_le32(buffer + 44);
    instr->UNK12        = get_le16(buffer + 48);
    
    buffer += 50; size -= 50;
    
    instr->shareSmpDataFromInstr = get_le16(buffer);
    instr->hasLoop               = get_le16(buffer + 2);
    instr->hasBidiLoop           = get_le16(buffer + 4);

    buffer += 6; size -= 6;
    
    instr->smpStartPoint         = get_le32(buffer);
    instr->smpLoopPoint          = get_le32(buffer + 4);
    instr->smpEndPoint           = get_le32(buffer + 8);
    instr->hasSample             = get_le32(buffer + 12);
    instr->smpLength             = get_le32(buffer + 16);
    
    buffer += 20; size -= 20;
    
    for (i = 0; i < SE_MAXCHANS; i++) {
        for (j = 0; j < 0x100; j++) {
            instr->synthBuffers[i][j] = get_le16(buffer + i * 512 + j * 2);
        }
    }
    
    return 8712;
}

Song* File_loadSongMem(const uint8_t *buffer, size_t size)
{
    int i, j, k;
    int songVer;
    Subsong *subs;
    Order *orderCol;
    Order *order;
    Row *row;
    Instrument *instr;
    Song *synSong;
    long sizeRead;

    synSong = (Song *) calloc(1, sizeof(Song));
    if (!synSong) return NULL;

    /*
    //unused vars
    int8_t _local5[] = [0, 0, 0, 0, 0, 0];
    bool _local2 = false;
    int _local7 = 0;
    bool _local8 = true;
    */

    sizeRead = File_readHeader(&synSong->h, buffer, size);
    if (sizeRead < 0) goto FAIL;
    buffer += sizeRead; size -= sizeRead;
    
    songVer = synSong->h.version;
    if ((songVer >= 3456) && (songVer <= 3457)){
        if (synSong->h.subsongNum > 0){
            synSong->subsongs = (Subsong *) malloc(synSong->h.subsongNum *sizeof(Subsong));
            if (!synSong->subsongs) goto FAIL;
            
            for (i = 0; i < synSong->h.subsongNum; i++) {
                sizeRead = File_readSubSong(synSong->subsongs + i, buffer, size);
                if (sizeRead < 0) goto FAIL;
                buffer += sizeRead; size -= sizeRead;
            }

            synSong->rows = (Row *) malloc(synSong->h.patNum * 64 *sizeof(Row));
            if (!synSong->rows) goto FAIL;
            
            for (i = 0, j = synSong->h.patNum * 64; i < j; i++) {
                sizeRead = File_readRow(synSong->rows + i, buffer, size);
                if (sizeRead < 0) goto FAIL;
                buffer += sizeRead; size -= sizeRead;
            }

            synSong->patNameSizes = (uint32_t *) malloc(synSong->h.patNum * sizeof(uint32_t));
            if (!synSong->patNameSizes) goto FAIL;
            synSong->patternNames = calloc(sizeof(char *), synSong->h.patNum);
            if (!synSong->patternNames) goto FAIL;
            
            for (i = 0; i < synSong->h.patNum; i++) {
                if (size < 4) goto FAIL;
                j = synSong->patNameSizes[i] = get_le32(buffer);
                buffer += 4; size -= 4;
                
                if (size < j) goto FAIL;
                
                synSong->patternNames[i] = malloc(j + sizeof(char));
                if (!synSong->patternNames[i]) goto FAIL;
                
                memcpy(synSong->patternNames[i], buffer, j);
                synSong->patternNames[i][j] = '\0';
                
                buffer += j; size -= j;
            }

            synSong->instruments = malloc(synSong->h.instrNum * sizeof(Instrument));
            if (!synSong->instruments) goto FAIL;
            synSong->samples = calloc(sizeof(int16_t *), synSong->h.instrNum);
            if (!synSong->samples) goto FAIL;
            
            for (i = 0; i < synSong->h.instrNum; i++) {
                instr = &synSong->instruments[i];
                sizeRead = File_readInstrument(instr, buffer, size);
                if (sizeRead < 0) goto FAIL;
                buffer += sizeRead; size -= sizeRead;
                
                if (songVer == 3456){
                    instr->shareSmpDataFromInstr = 0;
                    instr->hasLoop = 0;
                    instr->hasBidiLoop = 0;
                    instr->smpStartPoint = 0;
                    instr->smpLoopPoint = 0;
                    instr->smpEndPoint = 0;
                    if (instr->hasSample){
                        instr->smpStartPoint = 0;
                        instr->smpEndPoint = (instr->smpLength / 2);
                        instr->smpLoopPoint = 0;
                    }
                }
                if (instr->hasSample){
                    //instr->smpLength is in bytes, I think
                    if (size < instr->smpLength) goto FAIL;
                    synSong->samples[i] = malloc(instr->smpLength);
                    if (!synSong->samples[i]) goto FAIL;
                    memcpy(synSong->samples[i], buffer, instr->smpLength);
                    buffer += instr->smpLength; size -= instr->smpLength;
                } else {
                    synSong->samples[i] = NULL;
                }

            }
            memcpy(synSong->arpTable, buffer, 0x100);
            buffer += 0x100; size -= 0x100;
        } else goto FAIL;
    } else goto FAIL;

    return synSong;

FAIL:
    File_freeSong(synSong);
    return NULL;
}

void File_freeSong(Song *synSong)
{
    int i;
    
    if (synSong) {
        if (synSong->subsongs)     free(synSong->subsongs);
        if (synSong->rows)         free(synSong->rows);
        if (synSong->patNameSizes) free(synSong->patNameSizes);
        if (synSong->patternNames) {
            for (i = 0; i < synSong->h.patNum; i++) {
                if (synSong->patternNames[i]) free(synSong->patternNames[i]);
            }
            free(synSong->patternNames);
        }
        if (synSong->instruments)  free(synSong->instruments);
        if (synSong->samples) {
            for (i = 0; i < synSong->h.instrNum; i++) {
                if (synSong->samples[i]) free(synSong->samples[i]);
            }
            free(synSong->samples);
        }
        free(synSong);
    }
}
