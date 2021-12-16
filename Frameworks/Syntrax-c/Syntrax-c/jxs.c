#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jaytrax.h"
#include "jxs.h"
#include "ioutil.h"

struct memdata
{
    const uint8_t* data;
    size_t remain;
    int error;
};

void memread(void* buf, size_t size, size_t count, void* _data)
{
    struct memdata* data = (struct memdata*)_data;
    size_t unread = 0;
    size *= count;
    if (size > data->remain)
    {
        unread = size - data->remain;
        size = data->remain;
        data->error = 1;
    }
    memcpy(buf, data->data, size);
    data->data += size;
    data->remain -= size;
    if (unread)
        memset(((uint8_t*)buf) + size, 0, unread);
}

int memerror(void* _data)
{
    struct memdata* data = (struct memdata*)_data;
    return data->error;
}

void fileread(void* buf, size_t size, size_t count, void* _file)
{
    FILE* file = (FILE*)_file;
    fread(buf, size, count, file);
}

int fileerror(void* _file)
{
    FILE* file = (FILE*)_file;
    return ferror(file);
}

typedef void (*thereader)(void*, size_t, size_t, void*);
typedef int (*theerror)(void*);

//---------------------JXS3457

static int struct_readHeader(JT1Song* dest, thereader reader, theerror iferror, void* fin) {

    f_JT1Header t;
    
    reader(&t, sizeof(f_JT1Header), 1, fin);
    dest->mugiversion = t.mugiversion;
    dest->nrofpats    = t.nrofpats;
    dest->nrofsongs   = t.nrofsongs;
    dest->nrofinst    = t.nrofinst;
    return iferror(fin);
}

static int struct_readSubsong(JT1Subsong* dest, size_t len, thereader reader, theerror iferror, void* fin) {
    uint32_t i, j, k;
    f_JT1Subsong t;
    
    for (i=0; i < len; i++) {
        reader(&t, sizeof(f_JT1Subsong), 1, fin);
        for (j=0; j < J3457_CHANS_SUBSONG; j++) dest[i].mute[j] = t.mute[j];
        dest[i].songspd         = t.songspd;
        dest[i].groove          = t.groove;
        dest[i].songpos         = t.songpos;
        dest[i].songstep        = t.songstep;
        dest[i].endpos          = t.endpos;
        dest[i].endstep         = t.endstep;
        dest[i].looppos         = t.looppos;
        dest[i].loopstep        = t.loopstep;
        dest[i].songloop        = t.songloop;
        memcpy(&dest[i].name, &t.name, 32);
        dest[i].nrofchans       = t.nrofchans;
        dest[i].delaytime       = t.delaytime;
        for (j=0; j < J3457_CHANS_SUBSONG; j++) {
            dest[i].delayamount[j] = t.delayamount[j];
        }
        dest[i].amplification   = t.amplification;
        for (j=0; j < J3457_CHANS_SUBSONG; j++) {
            for (k=0; k < J3457_ORDERS_SUBSONG; k++) {
                dest[i].orders[j][k].patnr  = t.orders[j][k].patnr;
                dest[i].orders[j][k].patlen = t.orders[j][k].patlen;
            }
        }
    }
    return iferror(fin);
}

static int struct_readPat(JT1Row* dest, size_t len, thereader reader, theerror iferror, void* fin) {
    uint32_t i, j;
    f_JT1Row t[J3457_ROWS_PAT];
    
    for (i=0; i < len; i++) {
        reader(&t, sizeof(f_JT1Row)*J3457_ROWS_PAT, 1, fin);
        for (j=0; j < J3457_ROWS_PAT; j++) {
            uint32_t off = i*J3457_ROWS_PAT + j;
            dest[off].srcnote = t[j].srcnote;
            dest[off].dstnote = t[j].dstnote;
            dest[off].inst    = t[j].inst;
            dest[off].param   = t[j].param;
            dest[off].script  = t[j].script;
        }
    }
    return iferror(fin);
}

static int struct_readInst(JT1Inst* dest, size_t len, thereader reader, theerror iferror, void* fin) {
    uint32_t i, j;
    f_JT1Inst t;
    for (i=0; i < len; i++) {
        reader(&t, sizeof(f_JT1Inst), 1, fin);
        dest[i].mugiversion     = t.mugiversion;
        memcpy(&dest[i].instname, &t.instname, 32);
        dest[i].waveform        = t.waveform;
        dest[i].wavelength      = t.wavelength;
        dest[i].mastervol       = t.mastervol;
        dest[i].amwave          = t.amwave;
        dest[i].amspd           = t.amspd;
        dest[i].amlooppoint     = t.amlooppoint;
        dest[i].finetune        = t.finetune;
        dest[i].fmwave          = t.fmwave;
        dest[i].fmspd           = t.fmspd;
        dest[i].fmlooppoint     = t.fmlooppoint;
        dest[i].fmdelay         = t.fmdelay;
        dest[i].arpeggio        = t.arpeggio;
        for (j=0; j < J3457_WAVES_INST; j++) {
            dest[i].resetwave[j] = t.resetwave[j];
        }
        dest[i].panwave         = t.panwave;
        dest[i].panspd          = t.panspd;
        dest[i].panlooppoint    = t.panlooppoint;
        for (j=0; j < J3457_EFF_INST; j++) {
            dest[i].fx[j].dsteffect     = t.fx[j].dsteffect;
            dest[i].fx[j].srceffect1    = t.fx[j].srceffect1;
            dest[i].fx[j].srceffect2    = t.fx[j].srceffect2;
            dest[i].fx[j].osceffect     = t.fx[j].osceffect;
            dest[i].fx[j].effectvar1    = t.fx[j].effectvar1;
            dest[i].fx[j].effectvar2    = t.fx[j].effectvar2;
            dest[i].fx[j].effectspd     = t.fx[j].effectspd;
            dest[i].fx[j].oscspd        = t.fx[j].oscspd;
            dest[i].fx[j].effecttype    = t.fx[j].effecttype;
            dest[i].fx[j].oscflg        = t.fx[j].oscflg;
            dest[i].fx[j].reseteffect   = t.fx[j].reseteffect;
        }
        memcpy(&dest[i].samplename, &t.samplename, 192);
        //exFnameFromPath(&dest[i].samplename, &t.samplename, SE_NAMELEN);
        dest[i].sharing       = t.sharing;
        dest[i].loopflg       = t.loopflg;
        dest[i].bidirecflg    = t.bidirecflg;
        dest[i].startpoint    = t.startpoint;
        dest[i].looppoint     = t.looppoint;
        dest[i].endpoint      = t.endpoint;
        dest[i].hasSampData   = t.hasSampData ? 1 : 0; //this was a sampdata pointer in original jaytrax
        dest[i].samplelength  = t.samplelength;
        //memcpy(&dest[i].waves, &t.waves, J3457_WAVES_INST * J3457_SAMPS_WAVE * sizeof(int16_t));
        reader(&dest->waves, 2, J3457_WAVES_INST * J3457_SAMPS_WAVE, fin);
    }
    return iferror(fin);
}

//---------------------JXS3458

/* Soon! */

//---------------------

static int jxsfile_readSongCb(thereader reader, theerror iferror, void* fin, JT1Song** sngOut) {
#define FAIL(x) {error=(x); goto _ERR;}
    JT1Song* song;
    int i;
    int error;
    
    //song
    if((song = (JT1Song*)calloc(1, sizeof(JT1Song)))) {
        int version;
        
        if (struct_readHeader(song, reader, iferror, fin)) FAIL(ERR_BADSONG);
        //version magic
        version = song->mugiversion;
        if (version >= 3456 && version <= 3457) {
            int nrSubsongs = song->nrofsongs;
            int nrPats     = song->nrofpats;
            int nrRows     = J3457_ROWS_PAT * nrPats;
            int nrInst     = song->nrofinst;
            
            //subsongs
            if ((song->subsongs = (JT1Subsong**)calloc(nrSubsongs, sizeof(JT1Subsong*)))) {
                for (i=0; i < nrSubsongs; i++) {
                    if ((song->subsongs[i] = (JT1Subsong*)calloc(1, sizeof(JT1Subsong)))) {
                        if (struct_readSubsong(song->subsongs[i], 1, reader, iferror, fin)) FAIL(ERR_BADSONG);
                    } else FAIL(ERR_MALLOC);
                }
            } else FAIL(ERR_MALLOC);
            
            //patterns
            if ((song->patterns = (JT1Row*)calloc(nrRows, sizeof(JT1Row)))) {
                if (struct_readPat(song->patterns, nrPats, reader, iferror, fin)) FAIL(ERR_BADSONG);
            } else FAIL(ERR_MALLOC);
            
            //pattern names. Length includes \0
            if ((song->patNames = (char**)calloc(nrPats, sizeof(char*)))) {
                for (i=0; i < nrPats; i++) {
                    int32_t nameLen = 0;
                    
                    reader(&nameLen, 4, 1, fin);
                    if ((song->patNames[i] = (char*)calloc(nameLen, sizeof(char)))) {
                        reader(song->patNames[i], nameLen, 1, fin);
                    } else FAIL(ERR_MALLOC);
                }
                
                if (iferror(fin)) FAIL(ERR_BADSONG);
            } else FAIL(ERR_MALLOC);
            
            //instruments
            if ((song->instruments = (JT1Inst**)calloc(nrInst, sizeof(JT1Inst*)))) {
                if (!(song->samples = (uint8_t**)calloc(nrInst, sizeof(uint8_t*)))) FAIL(ERR_MALLOC);
                for (i=0; i < nrInst; i++) {
                    if ((song->instruments[i] = (JT1Inst*)calloc(1, sizeof(JT1Inst)))) {
                        JT1Inst* inst = song->instruments[i];
                        if (struct_readInst(inst, 1, reader, iferror, fin)) FAIL(ERR_BADSONG);
                        
                        //patch old instrument to new
                        if (version == 3456) {
                            inst->sharing = 0;
                            inst->loopflg = 0;
                            inst->bidirecflg = 0;
                            inst->startpoint = 0;
                            inst->looppoint = 0;
                            inst->endpoint = 0;
                            //silly place to put a pointer
                            if (inst->hasSampData) {
                                inst->startpoint=0;
                                inst->endpoint=(inst->samplelength/2);
                                inst->looppoint=0;
                            }
                        }
                        
                        //sample data
                        if (inst->hasSampData) {
                            //inst->samplelength is in bytes, not samples
                            if(!(song->samples[i] = (uint8_t*)calloc(inst->samplelength, sizeof(uint8_t)))) FAIL(ERR_MALLOC);
                            reader(song->samples[i], 1, inst->samplelength, fin);
                            if (iferror(fin)) FAIL(ERR_BADSONG);
                        } else {
                            song->samples[i] = NULL;
                        }
                    } else FAIL(ERR_MALLOC);
                }
            } else FAIL(ERR_MALLOC);
            
            //arpeggio table
            reader(&song->arpTable, J3457_STEPS_ARP, J3457_ARPS_SONG, fin);
            
            if (iferror(fin)) FAIL(ERR_BADSONG);
        } else if (version == 3458) {
            //Soon enough!
            FAIL(ERR_BADSONG);
        } else FAIL(ERR_BADSONG);
    } else FAIL(ERR_MALLOC);
    
    *sngOut = song;
    return ERR_OK;
    #undef FAIL
    _ERR:
        jxsfile_freeSong(song);
        *sngOut = NULL;
        return error;
}

int jxsfile_readSong(const char* path, JT1Song** sngOut) {
    char buf[BUFSIZ];
    FILE *fin;
    int error;

    if (!(fin = fopen(path, "rb"))) return ERR_FILEIO;
    setbuf(fin, buf);
    
    error = jxsfile_readSongCb(fileread, fileerror, fin, sngOut);
    
    fclose(fin);
    return error;
    
    _ERR:
        if (fin) fclose(fin);
        return error;
}

int jxsfile_readSongMem(const uint8_t* data, size_t size, JT1Song** sngOut) {
    struct memdata fin;
    fin.data = data;
    fin.remain = size;
    fin.error = 0;
    
    return jxsfile_readSongCb(memread, memerror, &fin, sngOut);
}

void jxsfile_freeSong(JT1Song* song) {
    if (song) {
        int i;
        int nrSubsongs = song->nrofsongs;
        int nrPats     = song->nrofpats;
        int nrInst     = song->nrofinst;
        
        if (song->subsongs) {
            
            for (i=0; i < nrSubsongs; i++) {
                if (song->subsongs[i])
                    free(song->subsongs[i]);
            }
            
            free(song->subsongs);
        }
        
        if (song->patterns) {
            free(song->patterns);
        }

        if (song->patNames) {
            for (i=0; i < nrPats; i++) {
                if (song->patNames[i])
                    free(song->patNames[i]);
            }
            
            free(song->patNames);
        }
        
        if (song->instruments && song->samples) {
            for (i=0; i < nrInst; i++) {
                if (song->instruments[i])
                    free(song->instruments[i]);
                if (song->samples[i])
                    free(song->samples[i]);
            }
        }
        
        if (song->samples) {
            free(song->samples);
        }
        
        if (song->instruments) {
            free(song->instruments);
        }
        
        free(song);
    }
}
