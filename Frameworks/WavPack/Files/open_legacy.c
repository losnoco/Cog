////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2016 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// open_legacy.c

// This code provides an interface between the new reader callback mechanism that
// WavPack uses internally and the old reader callback functions that did not
// provide large file support.

#include <stdlib.h>
#include <string.h>

#include "wavpack_local.h"

typedef struct {
    WavpackStreamReader *reader;
    void *id;
} WavpackReaderTranslator;

static int32_t trans_read_bytes (void *id, void *data, int32_t bcount)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->read_bytes (trans->id, data, bcount);
}

static int32_t trans_write_bytes (void *id, void *data, int32_t bcount)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->write_bytes (trans->id, data, bcount);
}

static int64_t trans_get_pos (void *id)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->get_pos (trans->id);
}

static int trans_set_pos_abs (void *id, int64_t pos)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->set_pos_abs (trans->id, (uint32_t) pos);
}

static int trans_set_pos_rel (void *id, int64_t delta, int mode)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->set_pos_rel (trans->id, (int32_t) delta, mode);
}

static int trans_push_back_byte (void *id, int c)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->push_back_byte (trans->id, c);
}

static int64_t trans_get_length (void *id)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->get_length (trans->id);
}

static int trans_can_seek (void *id)
{
    WavpackReaderTranslator *trans = id;
    return trans->reader->can_seek (trans->id);
}

static int trans_close_stream (void *id)
{
    free (id);
    return 0;
}

//  int32_t (*read_bytes)(void *id, void *data, int32_t bcount);
//  int32_t (*write_bytes)(void *id, void *data, int32_t bcount);
//  int64_t (*get_pos)(void *id);                               // new signature for large files
//  int (*set_pos_abs)(void *id, int64_t pos);                  // new signature for large files
//  int (*set_pos_rel)(void *id, int64_t delta, int mode);      // new signature for large files
//  int (*push_back_byte)(void *id, int c);
//  int64_t (*get_length)(void *id);                            // new signature for large files
//  int (*can_seek)(void *id);
//  int (*truncate_here)(void *id);                             // new function to truncate file at current position
//  int (*close)(void *id);                                     // new function to close file

static WavpackStreamReader64 trans_reader = {
    trans_read_bytes, trans_write_bytes, trans_get_pos, trans_set_pos_abs, trans_set_pos_rel,
    trans_push_back_byte, trans_get_length, trans_can_seek, NULL, trans_close_stream
};

// This function is identical to WavpackOpenFileInput() except that instead
// of providing a filename to open, the caller provides a pointer to a set of
// reader callbacks and instances of up to two streams. The first of these
// streams is required and contains the regular WavPack data stream; the second
// contains the "correction" file if desired. Unlike the standard open
// function which handles the correction file transparently, in this case it
// is the responsibility of the caller to be aware of correction files.

WavpackContext *WavpackOpenFileInputEx (WavpackStreamReader *reader, void *wv_id, void *wvc_id, char *error, int flags, int norm_offset)
{
    WavpackReaderTranslator *trans_wv = NULL, *trans_wvc = NULL;

    if (wv_id) {
        trans_wv = malloc (sizeof (WavpackReaderTranslator));
        trans_wv->reader = reader;
        trans_wv->id = wv_id;
    }

    if (wvc_id) {
        trans_wvc = malloc (sizeof (WavpackReaderTranslator));
        trans_wvc->reader = reader;
        trans_wvc->id = wvc_id;
    }

    return WavpackOpenFileInputEx64 (&trans_reader, trans_wv, trans_wvc, error, flags, norm_offset);
}
