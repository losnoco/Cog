////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2005 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wvunpack.c

// This is the main module for the WavPack command-line decompressor.

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/param.h>
#if defined (__GNUC__)
#include <unistd.h>
#include <glob.h>
#endif
#endif

#ifdef __BORLANDC__
#include <dir.h>
#elif defined(__GNUC__) && !defined(WIN32)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include "wavpack.h"
#include "md5.h"

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
static char *strdup (const char *s)
 { char *d = malloc (strlen (s) + 1); return strcpy (d, s); }
#endif

///////////////////////////// local variable storage //////////////////////////

static const char *sign_on = "\n"
" WVUNPACK  Hybrid Lossless Wavefile Decompressor  %s Version %s %s\n"
" Copyright (c) 1998 - 2005 Conifer Software.  All Rights Reserved.\n\n";

static const char *usage =
" Usage:   WVUNPACK [-options] [@]infile[.wv]|- [[@]outfile[.wav]|outpath|-]\n"
"             (infile may contain wildcards: ?,*)\n\n"
" Options: -d  = delete source file if successful (use with caution!)\n"
"          -i  = ignore .wvc file (forces hybrid lossy decompression)\n"
"          -m  = calculate and display MD5 signature; verify if lossless\n"
"          -q  = quiet (keep console output to a minimum)\n"
"          -r  = force raw audio decode (skip RIFF headers & trailers)\n"
"          -s  = display summary information only to stdout (no decode)\n"
#if defined (WIN32)
"          -t  = copy input file's time stamp to output file(s)\n"
#endif
"          -v  = verify source data only (no output file created)\n"
"          -y  = yes to overwrite warning (use with caution!)\n\n"
" Web:     Visit www.wavpack.com for latest version and info\n";

static char overwrite_all = 0, delete_source = 0, raw_decode = 0, summary = 0,
    ignore_wvc = 0, quiet_mode = 0, calc_md5 = 0, copy_time = 0;

static int num_files, file_index, outbuf_k;

/////////////////////////// local function declarations ///////////////////////

static int unpack_file (char *infilename, char *outfilename);
static void display_progress (double file_progress);

#define NO_ERROR 0L
#define SOFT_ERROR 1
#define HARD_ERROR 2

//////////////////////////////////////////////////////////////////////////////
// The "main" function for the command-line WavPack decompressor.           //
//////////////////////////////////////////////////////////////////////////////

int main (argc, argv) int argc; char **argv;
{
    int verify_only = 0, usage_error = 0, filelist = 0, add_extension = 0;
    char *infilename = NULL, *outfilename = NULL;
    char outpath, **matches = NULL;
    int result, i;

#ifdef __BORLANDC__
    struct ffblk ffblk;
#elif defined(WIN32)
    struct _finddata_t _finddata_t;
#else
    glob_t globs;
    struct stat fstats;
#endif

    // loop through command-line arguments

    while (--argc) {
#if defined (WIN32)
	if ((**++argv == '-' || **argv == '/') && (*argv)[1])
#else
	if ((**++argv == '-') && (*argv)[1])
#endif
	    while (*++*argv)
		switch (**argv) {
		    case 'Y': case 'y':
			overwrite_all = 1;
			break;

		    case 'D': case 'd':
			delete_source = 1;
			break;
#if defined (WIN32)
		    case 'T': case 't':
			copy_time = 1;
			break;
#endif
		    case 'V': case 'v':
			verify_only = 1;
			break;

		    case 'S': case 's':
			summary = 1;
			break;

		    case 'K': case 'k':
			outbuf_k = strtol (++*argv, argv, 10);
			--*argv;
			break;

		    case 'M': case 'm':
			calc_md5 = 1;
			break;

		    case 'R': case 'r':
			raw_decode = 1;
			break;

		    case 'Q': case 'q':
			quiet_mode = 1;
			break;

		    case 'I': case 'i':
			ignore_wvc = 1;
			break;

		    default:
			error_line ("illegal option: %c !", **argv);
			usage_error = 1;
		}
	else {
	    if (!infilename) {
		infilename = malloc (strlen (*argv) + PATH_MAX);
		strcpy (infilename, *argv);
	    }
	    else if (!outfilename) {
		outfilename = malloc (strlen (*argv) + PATH_MAX);
		strcpy (outfilename, *argv);
	    }
	    else {
		error_line ("extra unknown argument: %s !", *argv);
		usage_error = 1;
	    }
	}
    }

   // check for various command-line argument problems

    if (verify_only && delete_source) {
	error_line ("can't delete in verify mode!");
	delete_source = 0;
    }

    if (verify_only && outfilename) {
	error_line ("outfile specification and verify mode are incompatible!");
	usage_error = 1;
    }

    if (!quiet_mode && !usage_error)
	fprintf (stderr, sign_on, VERSION_OS, VERSION_STR, DATE_STR);

    if (!infilename) {
	printf ("%s", usage);
	return 0;
    }

    if (usage_error) {
	free (infilename);
	return 0;
    }

    setup_break ();

    // If the infile specification begins with a '@', then it actually points
    // to a file that contains the names of the files to be converted. This
    // was included for use by Wim Speekenbrink's frontends, but could be used
    // for other purposes.

    if (infilename [0] == '@') {
	FILE *list = fopen (infilename+1, "rt");
	int c;

	if (list == NULL) {
	    error_line ("file %s not found!", infilename+1);
	    free(infilename);
	    return 1;
	}

	while ((c = getc (list)) != EOF) {

	    while (c == '\n')
		c = getc (list);

	    if (c != EOF) {
		char *fname = malloc (PATH_MAX);
		int ci = 0;

		do
		    fname [ci++] = c;
		while ((c = getc (list)) != '\n' && c != EOF && ci < PATH_MAX);

		fname [ci++] = '\0';
		matches = realloc (matches, ++num_files * sizeof (*matches));
		matches [num_files - 1] = realloc (fname, ci);
	    }
	}

	fclose (list);
	free (infilename);
	infilename = NULL;
	filelist = 1;
    }
    else if (*infilename != '-') {	// skip this if infile is stdin (-)
	if (!filespec_ext (infilename))
	    strcat (infilename, ".wv");

#ifdef NO_WILDCARDS
	matches = malloc (sizeof (*matches));
	matches [num_files++] = infilename;
	filelist = 1;
#else
	// search for and store any filenames that match the user supplied spec

#ifdef __BORLANDC__
	if (findfirst (infilename, &ffblk, 0) == 0) {
	    do {
		matches = realloc (matches, ++num_files * sizeof (*matches));
		matches [num_files - 1] = strdup (ffblk.ff_name);
	    } while (findnext (&ffblk) == 0);
	}
#elif defined (WIN32)
	if ((i = _findfirst (infilename, &_finddata_t)) != -1L) {
	    do {
		if (!(_finddata_t.attrib & _A_SUBDIR)) {
		    matches = realloc (matches, ++num_files * sizeof (*matches));
		    matches [num_files - 1] = strdup (_finddata_t.name);
		}
	    } while (_findnext (i, &_finddata_t) == 0);

	    _findclose (i);
	}
#else
        i = 0;
        if (glob(infilename, 0, NULL, &globs) == 0 && globs.gl_pathc > 0) {
            do {
                if (stat(globs.gl_pathv[i], &fstats) == 0 && !(fstats.st_mode & S_IFDIR)) {
		    matches = realloc (matches, ++num_files * sizeof (*matches));
		    matches [num_files - 1] = strdup (globs.gl_pathv[i]);
                }
            } while (i++ < globs.gl_pathc);
        }
        globfree(&globs);
#endif
#endif
    }
    else {	// handle case of stdin (-)
	matches = malloc (sizeof (*matches));
	matches [num_files++] = infilename;
    }

    // If the outfile specification begins with a '@', then it actually points
    // to a file that contains the output specification. This was included for
    // use by Wim Speekenbrink's frontends because certain filenames could not
    // be passed on the command-line, but could be used for other purposes.

    if (outfilename && outfilename [0] == '@') {
	FILE *list = fopen (outfilename+1, "rt");
	int c;

	if (list == NULL) {
	    error_line ("file %s not found!", outfilename+1);
	    free(outfilename);
	    return 1;
	}

	while ((c = getc (list)) == '\n');

	if (c != EOF) {
	    int ci = 0;

	    do
		outfilename [ci++] = c;
	    while ((c = getc (list)) != '\n' && c != EOF && ci < PATH_MAX);

	    outfilename [ci] = '\0';
	}
	else {
	    error_line ("output spec file is empty!");
	    free(outfilename);
	    fclose (list);
	    return 1;
	}

	fclose (list);
    }

    // if we found any files to process, this is where we start

    if (num_files) {
	int soft_errors = 0;

	if (outfilename && *outfilename != '-') {
	    outpath = (filespec_path (outfilename) != NULL);

	    if (num_files > 1 && !outpath) {
		error_line ("%s is not a valid output path", outfilename);
		free(outfilename);
		return 1;
	    }
	}
	else
	    outpath = 0;

	add_extension = !outfilename || outpath || !filespec_ext (outfilename);

	// loop through and process files in list

	for (file_index = 0; file_index < num_files; ++file_index) {
	    if (check_break ())
		break;

	    // get input filename from list

	    if (filelist)
		infilename = matches [file_index];
	    else if (*infilename != '-') {
		*filespec_name (infilename) = '\0';
		strcat (infilename, matches [file_index]);
	    }

	    // generate output filename

	    if (outpath) {
		strcat (outfilename, filespec_name (matches [file_index]));

		if (filespec_ext (outfilename))
		    *filespec_ext (outfilename) = '\0';
	    }
	    else if (!outfilename) {
		outfilename = malloc (strlen (infilename) + 10);
		strcpy (outfilename, infilename);

		if (filespec_ext (outfilename))
		    *filespec_ext (outfilename) = '\0';
	    }

	    if (outfilename && *outfilename != '-' && add_extension)
		strcat (outfilename, raw_decode ? ".raw" : ".wav");

	    if (num_files > 1)
		fprintf (stderr, "\n%s:\n", infilename);

	    result = unpack_file (infilename, verify_only ? NULL : outfilename);

	    if (result == HARD_ERROR)
		break;
	    else if (result == SOFT_ERROR)
		++soft_errors;

	    // clean up in preparation for potentially another file

	    if (outpath)
		*filespec_name (outfilename) = '\0';
	    else if (*outfilename != '-') {
		free (outfilename);
		outfilename = NULL;
	    }

	    free (matches [file_index]);
	}

	if (num_files > 1) {
	    if (soft_errors)
		fprintf (stderr, "\n **** warning: errors occurred in %d of %d files! ****\n", soft_errors, num_files);
	    else if (!quiet_mode)
		fprintf (stderr, "\n **** %d files successfully processed ****\n", num_files);
	}

	free (matches);
    }
    else
	error_line (filespec_wild (infilename) ? "nothing to do!" :
	    "file %s not found!", infilename);

    if (outfilename)
	free(outfilename);

#ifdef DEBUG_ALLOC
    error_line ("malloc_count = %d", dump_alloc ());
#endif

    return 0;
}

// Unpack the specified WavPack input file into the specified output file name.
// This function uses the library routines provided in wputils.c to do all
// unpacking. This function takes care of reformatting the data (which is
// returned in native-endian longs) to the standard little-endian format. This
// function also handles optionally calculating and displaying the MD5 sum of
// the resulting audio data and verifying the sum if a sum was stored in the
// source and lossless compression is used.

static uchar *format_samples (int bps, uchar *dst, int32_t *src, uint32_t samcnt);
static void dump_summary (WavpackContext *wpc, char *name, FILE *dst);

extern int delta_blocks [8];

static int unpack_file (char *infilename, char *outfilename)
{
    int result = NO_ERROR, md5_diff = FALSE, open_flags = 0, bytes_per_sample, num_channels, wvc_mode, bps;
    uint32_t outfile_length, output_buffer_size, bcount, total_unpacked_samples = 0;
    uchar *output_buffer = NULL, *output_pointer = NULL;
    double dtime, progress = -1.0;
    MD5_CTX md5_context;
    WavpackContext *wpc;
    int32_t *temp_buffer;
    char error [80];
    FILE *outfile;

#ifdef __BORLANDC__
    struct time time1, time2;
#elif defined(WIN32)
    struct _timeb time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    // use library to open WavPack file

    if (outfilename && !raw_decode)
	open_flags |= OPEN_WRAPPER;

    if (raw_decode)
	open_flags |= OPEN_STREAMING;

    if (!ignore_wvc)
	open_flags |= OPEN_WVC;

    wpc = WavpackOpenFileInput (infilename, error, open_flags, 0);

    if (!wpc) {
	error_line (error);
	return SOFT_ERROR;
    }

    if (calc_md5)
	MD5Init (&md5_context);

    wvc_mode = WavpackGetMode (wpc) & MODE_WVC;
    num_channels = WavpackGetNumChannels (wpc);
    bps = WavpackGetBytesPerSample (wpc);
    bytes_per_sample = num_channels * bps;

    if (summary) {
	dump_summary (wpc, infilename, stdout);
	WavpackCloseFile (wpc);
	return NO_ERROR;
    }

    if (outfilename) {
	if (*outfilename != '-') {

	    // check the output file for overwrite warning required

	    if (!overwrite_all && (outfile = fopen (outfilename, "rb")) != NULL) {
		DoCloseHandle (outfile);
		fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (outfilename));
		SetConsoleTitle ("overwrite?");

		switch (yna ()) {

		    case 'n':
			result = SOFT_ERROR;
			break;

		    case 'a':
			overwrite_all = 1;
		}

		if (result != NO_ERROR) {
		    WavpackCloseFile (wpc);
		    return result;
		}
	    }

	    // open output file for writing

	    if ((outfile = fopen (outfilename, "wb")) == NULL) {
		error_line ("can't create file %s!", outfilename);
		WavpackCloseFile (wpc);
		return SOFT_ERROR;
	    }
	    else if (!quiet_mode)
		fprintf (stderr, "restoring %s,", FN_FIT (outfilename));
	}
	else {	// come here to open stdout as destination

	    outfile = stdout;
#if defined(WIN32)
	    setmode (fileno (stdout), O_BINARY);
#endif

	    if (!quiet_mode)
		fprintf (stderr, "unpacking %s%s to stdout,", *infilename == '-' ?
		    "stdin" : FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
	}

	if (outbuf_k)
	    output_buffer_size = outbuf_k * 1024;
	else
	    output_buffer_size = 1024 * 256;

	output_pointer = output_buffer = malloc (output_buffer_size);
    }
    else {	// in verify only mode we don't worry about headers
	outfile = NULL;

	if (!quiet_mode)
	    fprintf (stderr, "verifying %s%s,", *infilename == '-' ? "stdin" :
		FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
    }

#ifdef __BORLANDC__
    gettime (&time1);
#elif defined(WIN32)
    _ftime (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    if (WavpackGetWrapperBytes (wpc)) {
	if (outfile && (!DoWriteFile (outfile, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), &bcount) ||
	    bcount != WavpackGetWrapperBytes (wpc))) {
		error_line ("can't write .WAV data, disk probably full!");
		DoTruncateFile (outfile);
		result = HARD_ERROR;
	}

	WavpackFreeWrapper (wpc);
    }

    temp_buffer = malloc (4096L * num_channels * 4);

    while (result == NO_ERROR) {
	uint32_t samples_to_unpack, samples_unpacked;

	if (output_buffer) {
	    samples_to_unpack = (output_buffer_size - (output_pointer - output_buffer)) / bytes_per_sample;

	    if (samples_to_unpack > 4096)
		samples_to_unpack = 4096;
	}
	else
	    samples_to_unpack = 4096;

	samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, samples_to_unpack);
	total_unpacked_samples += samples_unpacked;

	if (output_buffer) {
	    if (samples_unpacked)
		output_pointer = format_samples (bps, output_pointer, temp_buffer, samples_unpacked * num_channels);

	    if (!samples_unpacked || (output_buffer_size - (output_pointer - output_buffer)) < bytes_per_sample) {
		if (!DoWriteFile (outfile, output_buffer, output_pointer - output_buffer, &bcount) ||
		    bcount != output_pointer - output_buffer) {
			error_line ("can't write .WAV data, disk probably full!");
			DoTruncateFile (outfile);
			result = HARD_ERROR;
			break;
		}

		output_pointer = output_buffer;
	    }
	}

	if (calc_md5 && samples_unpacked) {
	    format_samples (bps, (uchar *) temp_buffer, temp_buffer, samples_unpacked * num_channels);
	    MD5Update (&md5_context, temp_buffer, bps * samples_unpacked * num_channels);
	}

	if (!samples_unpacked)
	    break;

	if (check_break ()) {
	    fprintf (stderr, "^C\n");
	    DoTruncateFile (outfile);
	    result = SOFT_ERROR;
	    break;
	}

	if (WavpackGetProgress (wpc) != -1.0 &&
	    progress != floor (WavpackGetProgress (wpc) * 100.0 + 0.5)) {
		int nobs = progress == -1.0;

		progress = WavpackGetProgress (wpc);
		display_progress (progress);
		progress = floor (progress * 100.0 + 0.5);

		if (!quiet_mode)
		    fprintf (stderr, "%s%3d%% done...",
			nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
	}
    }

    free (temp_buffer);

    if (output_buffer)
	free (output_buffer);

if (0) {
int i;
for (i = 0; i < 8; ++i)
    error_line ("delta = %d, count = %d", i, delta_blocks [i]);
}

    if (!check_break () && calc_md5) {
	char md5_string1 [] = "00000000000000000000000000000000";
	char md5_string2 [] = "00000000000000000000000000000000";
	uchar md5_original [16], md5_unpacked [16];
	int i;

	MD5Final (md5_unpacked, &md5_context);

	if (WavpackGetMD5Sum (wpc, md5_original)) {

	    for (i = 0; i < 16; ++i)
		sprintf (md5_string1 + (i * 2), "%02x", md5_original [i]);

	    error_line ("original md5:  %s", md5_string1);

	    if (memcmp (md5_unpacked, md5_original, 16))
		md5_diff = TRUE;
	}

	for (i = 0; i < 16; ++i)
	    sprintf (md5_string2 + (i * 2), "%02x", md5_unpacked [i]);

	error_line ("unpacked md5:  %s", md5_string2);
    }

    if (WavpackGetWrapperBytes (wpc)) {
	if (outfile && result == NO_ERROR &&
	    (!DoWriteFile (outfile, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), &bcount) ||
	    bcount != WavpackGetWrapperBytes (wpc))) {
		error_line ("can't write .WAV data, disk probably full!");
		DoTruncateFile (outfile);
		result = HARD_ERROR;
	}

	WavpackFreeWrapper (wpc);
    }

    // if we are not just in verify only mode, grab the size of the output
    // file and close the file

    if (outfile != NULL) {
	fflush (outfile);
	outfile_length = DoGetFileSize (outfile);

	if (!DoCloseHandle (outfile)) {
	    error_line ("can't close file!");
	    result = SOFT_ERROR;
	}

	if (outfilename && *outfilename != '-' && !outfile_length)
	    DoDeleteFile (outfilename);
    }

#if defined (WIN32)
    if (result == NO_ERROR && copy_time && outfilename &&
	!copy_timestamp (infilename, outfilename))
	    error_line ("failure copying time stamp!");
#endif

    if (result == NO_ERROR && WavpackGetNumSamples (wpc) != (uint32_t) -1 &&
	total_unpacked_samples != WavpackGetNumSamples (wpc)) {
	    error_line ("incorrect number of samples!");
	    result = SOFT_ERROR;
    }

    if (result == NO_ERROR && WavpackGetNumErrors (wpc)) {
	error_line ("crc errors detected in %d block(s)!", WavpackGetNumErrors (wpc));
	result = SOFT_ERROR;
    }
    else if (result == NO_ERROR && md5_diff && (WavpackGetMode (wpc) & MODE_LOSSLESS)) {
	error_line ("MD5 signatures should match, but do not!");
	result = SOFT_ERROR;
    }

    // Compute and display the time consumed along with some other details of
    // the unpacking operation (assuming there was no error).

#ifdef __BORLANDC__
    gettime (&time2);
    dtime = time2.ti_sec * 100.0 + time2.ti_hund + time2.ti_min * 6000.0 + time2.ti_hour * 360000.00;
    dtime -= time1.ti_sec * 100.0 + time1.ti_hund + time1.ti_min * 6000.0 + time1.ti_hour * 360000.00;

    if ((dtime /= 100.0) < 0.0)
	dtime += 86400.0;
#elif defined(WIN32)
    _ftime (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if (result == NO_ERROR && !quiet_mode) {
	char *file, *fext, *oper, *cmode, cratio [16] = "";

	if (outfilename && *outfilename != '-') {
	    file = FN_FIT (outfilename);
	    fext = "";
	    oper = "restored";
	}
	else {
	    file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
	    fext = wvc_mode ? " (+.wvc)" : "";
	    oper = outfilename ? "unpacked" : "verified";
	}

	if (WavpackGetMode (wpc) & MODE_LOSSLESS) {
	    cmode = "lossless";

	    if (WavpackGetRatio (wpc) != 0.0)
		sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (wpc) * 100.0);
	}
	else {
	    cmode = "lossy";

	    if (WavpackGetAverageBitrate (wpc, TRUE) != 0.0)
		sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (wpc, TRUE) / 1000.0));
	}

	error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (wpc);

    if (result == NO_ERROR && delete_source) {
	error_line ("%s source file %s", DoDeleteFile (infilename) ?
	    "deleted" : "can't delete", infilename);

	if (wvc_mode) {
	    char in2filename [PATH_MAX];

	    strcpy (in2filename, infilename);
	    strcat (in2filename, "c");

	    error_line ("%s source file %s", DoDeleteFile (in2filename) ?
		"deleted" : "can't delete", in2filename);
	}
    }

    return result;
}

// Reformat samples from longs in processor's native endian mode to
// little-endian data with (possibly) less than 4 bytes / sample.

static uchar *format_samples (int bps, uchar *dst, int32_t *src, uint32_t samcnt)
{
    int32_t temp;

    switch (bps) {

	case 1:
	    while (samcnt--)
		*dst++ = *src++ + 128;

	    break;

	case 2:
	    while (samcnt--) {
		*dst++ = (uchar) (temp = *src++);
		*dst++ = (uchar) (temp >> 8);
	    }

	    break;

	case 3:
	    while (samcnt--) {
		*dst++ = (uchar) (temp = *src++);
		*dst++ = (uchar) (temp >> 8);
		*dst++ = (uchar) (temp >> 16);
	    }

	    break;

	case 4:
	    while (samcnt--) {
		*dst++ = (uchar) (temp = *src++);
		*dst++ = (uchar) (temp >> 8);
		*dst++ = (uchar) (temp >> 16);
		*dst++ = (uchar) (temp >> 24);
	    }

	    break;
    }

    return dst;
}

static void dump_summary (WavpackContext *wpc, char *name, FILE *dst)
{
    int num_channels = WavpackGetNumChannels (wpc);
    uchar md5_sum [16], modes [80];

    fprintf (dst, "\n");

    if (name && *name != '-') {
	fprintf (dst, "file name:         %s%s\n", name, (WavpackGetMode (wpc) & MODE_WVC) ? " (+wvc)" : "");
	fprintf (dst, "file size:         %lu bytes\n", WavpackGetFileSize (wpc));
    }

    fprintf (dst, "source:            %d-bit %s at %ld Hz\n", WavpackGetBitsPerSample (wpc),
	(WavpackGetMode (wpc) & MODE_FLOAT) ? "floats" : "ints",
	WavpackGetSampleRate (wpc));

    fprintf (dst, "channels:          %d (%s)\n", num_channels,
	num_channels > 2 ? "multichannel" : (num_channels == 1 ? "mono" : "stereo"));

    if (WavpackGetNumSamples (wpc) != (uint32_t) -1) {
	double seconds = (double) WavpackGetNumSamples (wpc) / WavpackGetSampleRate (wpc);
	int minutes = (int) floor (seconds / 60.0);
	int hours = (int) floor (seconds / 3600.0);

	seconds -= minutes * 60.0;
	minutes -= hours * 60.0;

	fprintf (dst, "duration:          %d:%02d:%0.2f\n", hours, minutes, seconds);
    }

    modes [0] = 0;

    if (WavpackGetMode (wpc) & MODE_HYBRID)
	strcat (modes, "hybrid ");

    strcat (modes, (WavpackGetMode (wpc) & MODE_LOSSLESS) ? "lossless" : "lossy");

    if (WavpackGetMode (wpc) & MODE_FAST)
	strcat (modes, ", fast");
    else if (WavpackGetMode (wpc) & MODE_HIGH)
	strcat (modes, ", high");

    if (WavpackGetMode (wpc) & MODE_EXTRA)
	strcat (modes, ", extra");

    if (WavpackGetMode (wpc) & MODE_SFX)
	strcat (modes, ", sfx");

    fprintf (dst, "modalities:        %s\n", modes);

    if (WavpackGetRatio (wpc) != 0.0) {
	fprintf (dst, "compression:       %.2f%%\n", 100.0 - (100 * WavpackGetRatio (wpc)));
	fprintf (dst, "ave bitrate:       %d kbps\n", (int) ((WavpackGetAverageBitrate (wpc, TRUE) + 500.0) / 1000.0));

	if (WavpackGetMode (wpc) & MODE_WVC)
	    fprintf (dst, "ave lossy bitrate: %d kbps\n", (int) ((WavpackGetAverageBitrate (wpc, FALSE) + 500.0) / 1000.0));
    }

    if (WavpackGetVersion (wpc))
	fprintf (dst, "encoder version:   %d\n", WavpackGetVersion (wpc));

    if (WavpackGetMD5Sum (wpc, md5_sum)) {
	char md5_string [] = "00000000000000000000000000000000";
	int i;

	for (i = 0; i < 16; ++i)
	    sprintf (md5_string + (i * 2), "%02x", md5_sum [i]);

	fprintf (dst, "original md5:      %s\n", md5_string);
    }
}

//////////////////////////////////////////////////////////////////////////////
// This function displays the progress status on the title bar of the DOS   //
// window that WavPack is running in. The "file_progress" argument is for   //
// the current file only and ranges from 0 - 1; this function takes into    //
// account the total number of files to generate a batch progress number.   //
//////////////////////////////////////////////////////////////////////////////

void display_progress (double file_progress)
{
    char title [40];

    file_progress = (file_index + file_progress) / num_files;
    sprintf (title, "%d%% (WvUnpack)", (int) ((file_progress * 100.0) + 0.5));
    SetConsoleTitle (title);
}
