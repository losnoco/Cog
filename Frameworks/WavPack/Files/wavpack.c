////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2013 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wavpack.c

// This is the main module for the WavPack command-line compressor.

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#if defined(__OS2__)
#define INCL_DOSPROCESS
#include <os2.h>
#include <io.h>
#endif
#include <sys/param.h>
#include <sys/stat.h>
#include <locale.h>
#include <iconv.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include "wavpack.h"
#include "utils.h"
#include "md5.h"

#if defined (__GNUC__) && !defined(WIN32)
#include <unistd.h>
#include <glob.h>
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#ifdef WIN32
#define stricmp(x,y) _stricmp(x,y)
#define strdup(x) _strdup(x)
#define fileno _fileno
#else
#define stricmp strcasecmp
#endif

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
" WAVPACK  Hybrid Lossless Audio Compressor  %s Version %s\n"
" Copyright (c) 1998 - 2013 Conifer Software.  All Rights Reserved.\n\n";

static const char *usage =
#if defined (WIN32)
" Usage:   WAVPACK [-options] infile[.wav]|infile.wv|- [outfile[.wv]|outpath|-]\n"
"             (default is lossless; infile may contain wildcards: ?,*)\n\n"
#else
" Usage:   WAVPACK [-options] infile[.wav]|infile.wv|- [...] [-o outfile[.wv]|outpath|-]\n"
"             (default is lossless; multiple input files allowed)\n\n"
#endif
" Options: -bn = enable hybrid compression, n = 2.0 to 23.9 bits/sample, or\n"
"                                           n = 24-9600 kbits/second (kbps)\n"
"          -c  = create correction file (.wvc) for hybrid mode (=lossless)\n"
"          -f  = fast mode (fast, but some compromise in compression ratio)\n"
"          -h  = high quality (better compression ratio, but slower)\n"
"          -v  = verify output file integrity after write (no pipes)\n"
"          -x  = extra encode processing (no decoding speed penalty)\n"
"          --help = complete help\n\n"
" Web:     Visit www.wavpack.com for latest version and info\n";

static const char *help =
#if defined (WIN32)
" Usage:\n"
"    WAVPACK [-options] infile[.wav]|infile.wv|- [outfile[.wv]|outpath|-]\n\n"
"    The default operation is lossless. Wildcard characters (*,?) may be included\n"
"    in the input file and they may be either WAV or WAVPACK (.wv) files (or raw\n"
"    PCM if --raw-pcm is included). When transcoding, all tags are copied.\n\n"
#else
" Usage:\n"
"    WAVPACK [-options] infile[.wav]|infile.wv|- [...] [-o outfile[.wv]|outpath|-]\n\n"
"    The default operation is lossless. Multiple input files may be specified,\n"
"    and they may be either WAV or WAVPACK (.wv) files (or raw PCM if --raw-pcm\n"
"    is included). When transcoding, all tags are copied.\n\n"
#endif
" Options:\n"
"    -a                      Adobe Audition (CoolEdit) mode for 32-bit floats\n"
"    --allow-huge-tags       allow tag data up to 16 MB (embedding > 1 MB is not\n"
"                             recommended for portable devices and may not work\n"
"                             with some programs including WavPack pre-4.70)\n"
"    -bn                     enable hybrid compression\n"
"                              n = 2.0 to 23.9 bits/sample, or\n"
"                              n = 24-9600 kbits/second (kbps)\n"
"                              add -c to create correction file (.wvc)\n"
"    --blocksize=n           specify block size in samples (max = 131072 and\n"
"                               min = 16 with --merge-blocks, otherwise 128)\n"
"    -c                      hybrid lossless mode (use with -b to create\n"
"                             correction file (.wvc) in hybrid mode)\n"
"    -cc                     maximum hybrid lossless compression (but degrades\n"
"                             decode speed and may result in lower quality)\n"
"    --channel-order=<list>  specify (comma separated) channel order if not\n"
"                             Microsoft standard (which is FL,FR,FC,LFE,BL,BR,\n"
"                             LC,FRC,BC,SL,SR,TC,TFL,TFC,TFR,TBL,TBC,TBR);\n"
"                             specify '...' to indicate that channels are not\n"
"                             assigned to specific speakers, or terminate list\n"
"                             with '...' to indicate that any channels beyond\n"
"                             those specified are unassigned\n"
"    -d                      delete source file if successful (use with caution!)\n"
#if defined (WIN32)
"    -e                      create self-extracting executable with .exe\n"
"                             extension, requires wvself.exe in path\n"
#endif
"    -f                      fast mode (faster encode and decode, but some\n"
"                             compromise in compression ratio)\n"
"    -h                      high quality (better compression ratio, but slower\n"
"                             encode and decode than default mode)\n"
"    -hh                     very high quality (best compression, but slowest\n"
"                             and NOT recommended for portable hardware use)\n"
"    --help                  this extended help display\n"
"    -i                      ignore length in wav header (no pipe output allowed)\n"
"    -jn                     joint-stereo override (0 = left/right, 1 = mid/side)\n"
#if defined (WIN32) || defined (__OS2__)
"    -l                      run at lower priority for smoother multitasking\n"
#endif
"    -m                      compute & store MD5 signature of raw audio data\n"
"    --merge-blocks          merge consecutive blocks with equal redundancy\n"
"                             (used with --blocksize option and is useful for\n"
"                             files generated by the lossyWAV program or\n"
"                             decoded HDCD files)\n"
"    -n                      calculate average and peak quantization noise\n"
"                             (for hybrid mode only, reference fullscale sine)\n"
"    --no-utf8-convert       don't recode passed tags from local encoding to\n"
"                             UTF-8, assume they are in UTF-8 already.\n"
#if !defined (WIN32)
"    -o FILENAME | PATH      specify output filename or path\n"
#endif
"    --optimize-mono         optimization for stereo files that are really mono\n"
"                             (result may be incompatible with older decoders)\n"
"    -p                      practical float storage (also affects 32-bit\n"
"                             integers, no longer technically lossless)\n"
"    --pair-unassigned-chans encode unassigned channels into stereo pairs\n"
"    -q                      quiet (keep console output to a minimum)\n"
"    -r                      generate a new RIFF wav header (removes any\n"
"                             extra chunk info from existing header)\n"
"    --raw-pcm               input data is raw pcm (44100 Hz, 16-bit, 2-ch)\n"
"    --raw-pcm=sr,bps,ch     input data is raw pcm with specified sample rate,\n"
"                             sample bit depth, and number of channels\n"
"                             (specify 32f for 32-bit floating point data)\n"
"    -sn                     override default noise shaping where n is a float\n"
"                             value between -1.0 and 1.0; negative values move noise\n"
"                             lower in freq, positive values move noise higher\n"
"                             in freq, use '0' for no shaping (white noise)\n"
"    -t                      copy input file's time stamp to output file(s)\n"
"    --use-dns               force use of dynamic noise shaping (hybrid mode only)\n"
"    -v                      verify output file integrity after write (no pipes)\n"
"    --version               write the version to stdout\n"
"    -w \"Field=Value\"        write specified text metadata to APEv2 tag\n"
"    -w \"Field=@file.ext\"    write specified text metadata from file to APEv2\n"
"                             tag, normally used for embedded cuesheets and logs\n"
"                             (field names \"Cuesheet\" and \"Log\")\n"
"    --write-binary-tag \"Field=@file.ext\"\n"
"                            write the specified binary metadata file to APEv2\n"
"                             tag, normally used for cover art with the specified\n"
"                             field name \"Cover Art (Front)\"\n"
"    -x[n]                   extra encode processing (optional n = 1 to 6, 1=default)\n"
"                             -x1 to -x3 to choose best of predefined filters\n"
"                             -x4 to -x6 to generate custom filters (very slow!)\n"
"    -y                      yes to all warnings (use with caution!)\n"
"    -z                      don't set console title to indicate progress\n\n"
" Web:\n"
"     Visit www.wavpack.com for latest version and complete information\n";

static const char *speakers [] = {
    "FL", "FR", "FC", "LFE", "BL", "BR", "FLC", "FRC", "BC",
    "SL", "SR", "TC", "TFL", "TFC", "TFR", "TBL", "TBC", "TBR"
};

#define NUM_SPEAKERS (sizeof (speakers) / sizeof (speakers [0]))

// this global is used to indicate the special "debug" mode where extra debug messages
// are displayed and all messages are logged to the file \wavpack.log

int debug_logging_mode;

static int overwrite_all, num_files, file_index, copy_time, quiet_mode, verify_mode, delete_source, store_floats_as_ints,
    adobe_mode, ignore_length, new_riff_header, raw_pcm, no_utf8_convert, no_console_title, allow_huge_tags;

static int num_channels_order;
static unsigned char channel_order [18], channel_order_undefined;
static uint32_t channel_order_mask;
static double encode_time_percent;

// These two statics are used to keep track of tags that the user specifies on the
// command line. The "num_tag_strings" and "tag_strings" fields in the WavpackConfig
// structure are no longer used for anything (they should not have been there in
// the first place).

static int num_tag_items, total_tag_size;

static struct tag_item {
    char *item, *value, *ext;
    int vsize, binary;
} *tag_items;

#if defined (WIN32)
static char *wvselfx_image;
static uint32_t wvselfx_size;
#endif

/////////////////////////// local function declarations ///////////////////////

static FILE *wild_fopen (char *filename, const char *mode);
static int pack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config);
static int pack_audio (WavpackContext *wpc, FILE *infile, unsigned char *new_order, unsigned char *md5_digest_source);
static int repack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config);
static int repack_audio (WavpackContext *wpc, WavpackContext *infile, unsigned char *md5_digest_source);
static int verify_audio (char *infilename, unsigned char *md5_digest_source);
static void display_progress (double file_progress);
static void TextToUTF8 (void *string, int len);

#define NO_ERROR 0L
#define SOFT_ERROR 1
#define HARD_ERROR 2

//////////////////////////////////////////////////////////////////////////////
// The "main" function for the command-line WavPack compressor.             //
//////////////////////////////////////////////////////////////////////////////

int main (argc, argv) int argc; char **argv;
{
#ifdef __EMX__ /* OS/2 */
    _wildcard (&argc, &argv);
#endif
    int error_count = 0, tag_next_arg = 0, output_spec = 0;
    char *outfilename = NULL, *out2filename = NULL;
    char **matches = NULL;
    WavpackConfig config;
    int result, i;

#if defined(WIN32)
    struct _finddata_t _finddata_t;
    char selfname [MAX_PATH];

    if (GetModuleFileName (NULL, selfname, sizeof (selfname)) && filespec_name (selfname) &&
        _strupr (filespec_name (selfname)) && strstr (filespec_name (selfname), "DEBUG")) {
            char **argv_t = argv;
            int argc_t = argc;

            debug_logging_mode = TRUE;

            while (--argc_t)
                error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }

    strcpy (selfname, *argv);
#else
    if (filespec_name (*argv))
        if (strstr (filespec_name (*argv), "ebug") || strstr (filespec_name (*argv), "DEBUG")) {
            char **argv_t = argv;
            int argc_t = argc;

            debug_logging_mode = TRUE;

            while (--argc_t)
                error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }
#endif

    CLEAR (config);

    // loop through command-line arguments

    while (--argc)
        if (**++argv == '-' && (*argv)[1] == '-' && (*argv)[2]) {
            char *long_option = *argv + 2, *long_param = long_option;

            while (*long_param)
                if (*long_param++ == '=')
                    break;

            if (!strcmp (long_option, "help")) {                        // --help
                printf ("%s", help);
                return 1;
            }
            else if (!strcmp (long_option, "version")) {                // --version
                printf ("wavpack %s\n", PACKAGE_VERSION);
                printf ("libwavpack %s\n", WavpackGetLibraryVersionString ());
                return 1;
            }
            else if (!strcmp (long_option, "optimize-mono"))            // --optimize-mono
                config.flags |= CONFIG_OPTIMIZE_MONO;
            else if (!strcmp (long_option, "dns")) {                    // --dns
                error_line ("warning: --dns deprecated, use --use-dns");
                ++error_count;
            }
            else if (!strcmp (long_option, "use-dns"))                  // --use-dns
                config.flags |= CONFIG_DYNAMIC_SHAPING;
            else if (!strcmp (long_option, "merge-blocks"))             // --merge-blocks
                config.flags |= CONFIG_MERGE_BLOCKS;
            else if (!strcmp (long_option, "pair-unassigned-chans"))    // --pair-unassigned-chans
                config.flags |= CONFIG_PAIR_UNDEF_CHANS;
            else if (!strcmp (long_option, "no-utf8-convert"))          // --no-utf8-convert
                no_utf8_convert = 1;
            else if (!strcmp (long_option, "allow-huge-tags"))          // --allow-huge-tags
                allow_huge_tags = 1;
            else if (!strcmp (long_option, "store-floats-as-ints"))     // --store-floats-as-ints
                store_floats_as_ints = 1;
            else if (!strcmp (long_option, "write-binary-tag"))         // --write-binary-tag
                tag_next_arg = 2;
            else if (!strncmp (long_option, "raw-pcm", 7)) {            // --raw-pcm
                int params [] = { 44100, 16, 2 };
                int pi, fp = 0;

                for (pi = 0; *long_param && pi < 3; ++pi) {
                    if (isdigit (*long_param))
                        params [pi] = strtol (long_param, &long_param, 10);

                    if ((*long_param == 'f' || *long_param == 'F') && pi == 1) {
                        long_param++;
                        fp = 1;
                    }

                    if (*long_param == ',')
                        long_param++;
                    else
                        break;
                }

                if (*long_param) {
                    error_line ("syntax error in raw PCM specification!");
                    ++error_count;
                }
                else if (params [0] < 1 || params [0] > 192000 ||
                    params [1] < 1 || params [1] > 32 || (fp && params [1] != 32) ||
                    params [2] < 1 || params [2] > 256) {
                        error_line ("argument range error in raw PCM specification!");
                        ++error_count;
                }
                else {
                    config.sample_rate = params [0];
                    config.bits_per_sample = params [1];
                    config.bytes_per_sample = (params [1] + 7) / 8;
                    config.num_channels = params [2];

                    if (config.num_channels <= 2)
                        config.channel_mask = 0x5 - config.num_channels;
                    else if (config.num_channels <= 18)
                        config.channel_mask = (1 << config.num_channels) - 1;
                    else
                        config.channel_mask = 0x3ffff;

                    config.float_norm_exp = fp ? 127 : 0;
                    raw_pcm = 1;            
                }
            }
            else if (!strncmp (long_option, "blocksize", 9)) {          // --blocksize
                config.block_samples = strtol (long_param, NULL, 10);

                if (config.block_samples < 16 || config.block_samples > 131072) {
                    error_line ("invalid blocksize!");
                    ++error_count;
                }
            }
            else if (!strncmp (long_option, "channel-order", 13)) {      // --channel-order
                char name [6], channel_error = 0;
                uint32_t mask = 0;
                int chan, ci, si;

                for (chan = 0; chan < sizeof (channel_order); ++chan) {

                    if (!*long_param)
                        break;

                    if (*long_param == '.') {
                        if (*++long_param == '.' && *++long_param == '.' && !*++long_param)
                            channel_order_undefined = 1;
                        else
                            channel_error = 1;

                        break;
                    }

                    for (ci = 0; isalpha (*long_param) && ci < sizeof (name) - 1; ci++)
                        name [ci] = *long_param++;

                    if (!ci) {
                        channel_error = 1;
                        break;
                    } 

                    name [ci] = 0;

                    for (si = 0; si < NUM_SPEAKERS; ++si)
                        if (!stricmp (name, speakers [si])) {
                            if (mask & (1L << si))
                                channel_error = 1;

                            channel_order [chan] = si;
                            mask |= (1L << si);
                            break;
                        }

                    if (channel_error || si == NUM_SPEAKERS) {
                        error_line ("unknown or repeated channel spec: %s!", name);
                        channel_error = 1;
                        break;
                    } 

                    if (*long_param && *long_param++ != ',') {
                        channel_error = 1;
                        break;
                    } 
                }

                if (channel_error) {
                    error_line ("syntax error in channel order specification!");
                    ++error_count;
                }
                else if (*long_param) {
                    error_line ("too many channels specified!");
                    ++error_count;
                }
                else {
                    channel_order_mask = mask;
                    num_channels_order = chan;
                }
            }
            else {
                error_line ("unknown option: %s !", long_option);
                ++error_count;
            }
        }
#if defined (WIN32)
        else if ((**argv == '-' || **argv == '/') && (*argv)[1])
#else
        else if ((**argv == '-') && (*argv)[1])
#endif
            while (*++*argv)
                switch (**argv) {

                    case 'Y': case 'y':
                        overwrite_all = 1;
                        break;

                    case 'D': case 'd':
                        delete_source = 1;
                        break;

                    case 'C': case 'c':
                        if (config.flags & CONFIG_CREATE_WVC)
                            config.flags |= CONFIG_OPTIMIZE_WVC;
                        else
                            config.flags |= CONFIG_CREATE_WVC;

                        break;

                    case 'X': case 'x':
                        config.xmode = strtol (++*argv, argv, 10);

                        if (config.xmode < 0 || config.xmode > 6) {
                            error_line ("extra mode only goes from 1 to 6!");
                            ++error_count;
                        }
                        else
                            config.flags |= CONFIG_EXTRA_MODE;

                        --*argv;
                        break;

                    case 'F': case 'f':
                        config.flags |= CONFIG_FAST_FLAG;
                        break;

                    case 'H': case 'h':
                        if (config.flags & CONFIG_HIGH_FLAG)
                            config.flags |= CONFIG_VERY_HIGH_FLAG;
                        else
                            config.flags |= CONFIG_HIGH_FLAG;

                        break;

                    case 'N': case 'n':
                        config.flags |= CONFIG_CALC_NOISE;
                        break;

                    case 'A': case 'a':
                        adobe_mode = 1;
                        break;
#if defined (WIN32)
                    case 'E': case 'e':
                        config.flags |= CONFIG_CREATE_EXE;
                        break;
#endif
#if defined (WIN32)
                    case 'L': case 'l':
                        SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
                        break;
#elif defined (__OS2__)
                    case 'L': case 'l':
                        DosSetPriority (0, PRTYC_IDLETIME, 0, 0);
                        break;
#endif
#if defined (WIN32)
                    case 'O': case 'o':  // ignore -o in Windows to be Linux compatible
                        break;
#else
                    case 'O': case 'o':
                        output_spec = 1;
                        break;
#endif
                    case 'T': case 't':
                        copy_time = 1;
                        break;

                    case 'P': case 'p':
                        config.flags |= CONFIG_SKIP_WVX;
                        break;

                    case 'Q': case 'q':
                        quiet_mode = 1;
                        break;

                    case 'Z': case 'z':
                        no_console_title = 1;
                        break;

                    case 'M': case 'm':
                        config.flags |= CONFIG_MD5_CHECKSUM;
                        break;

                    case 'I': case 'i':
                        ignore_length = 1;
                        break;

                    case 'R': case 'r':
                        new_riff_header = 1;
                        break;

                    case 'V': case 'v':
                        verify_mode = 1;
                        break;

                    case 'B': case 'b':
                        config.flags |= CONFIG_HYBRID_FLAG;
                        config.bitrate = (float) strtod (++*argv, argv);
                        --*argv;

                        if (config.bitrate < 2.0 || config.bitrate > 9600.0) {
                            error_line ("hybrid spec must be 2.0 to 9600!");
                            ++error_count;
                        }

                        if (config.bitrate >= 24.0)
                            config.flags |= CONFIG_BITRATE_KBPS;

                        break;

                    case 'J': case 'j':
                        switch (strtol (++*argv, argv, 10)) {

                            case 0:
                                config.flags |= CONFIG_JOINT_OVERRIDE;
                                config.flags &= ~CONFIG_JOINT_STEREO;
                                break;

                            case 1:
                                config.flags |= (CONFIG_JOINT_OVERRIDE | CONFIG_JOINT_STEREO);
                                break;

                            default:
                                error_line ("-j0 or -j1 only!");
                                ++error_count;
                        }

                        --*argv;
                        break;

                    case 'S': case 's':
                        config.shaping_weight = (float) strtod (++*argv, argv);

                        if (!config.shaping_weight) {
                            config.flags |= CONFIG_SHAPE_OVERRIDE;
                            config.flags &= ~CONFIG_HYBRID_SHAPE;
                        }
                        else if (config.shaping_weight >= -1.0 && config.shaping_weight <= 1.0)
                            config.flags |= (CONFIG_HYBRID_SHAPE | CONFIG_SHAPE_OVERRIDE);
                        else {
                            error_line ("-s-1.00 to -s1.00 only!");
                            ++error_count;
                        }

                        --*argv;
                        break;

                    case 'W': case 'w':
                        if (++tag_next_arg == 2) {
                            error_line ("warning: -ww deprecated, use --write-binary-tag");
                            ++error_count;
                        }

                        break;

                    default:
                        error_line ("illegal option: %c !", **argv);
                        ++error_count;
                }
        else if (tag_next_arg) {
            char *cp = strchr (*argv, '=');

            if (cp && cp > *argv) {
                int i = num_tag_items;

                tag_items = realloc (tag_items, ++num_tag_items * sizeof (*tag_items));
                tag_items [i].item = malloc (cp - *argv + 1);
                memcpy (tag_items [i].item, *argv, cp - *argv);
                tag_items [i].item [cp - *argv] = 0;
                tag_items [i].vsize = (int) strlen (cp + 1);
                tag_items [i].value = malloc (tag_items [i].vsize + 1);
                strcpy (tag_items [i].value, cp + 1);
                tag_items [i].binary = (tag_next_arg == 2);
                tag_items [i].ext = NULL;
            }
            else {
                error_line ("error in tag spec: %s !", *argv);
                ++error_count;
            }

            tag_next_arg = 0;
        }
#if defined (WIN32)
        else if (!num_files) {
            matches = realloc (matches, (num_files + 1) * sizeof (*matches));
            matches [num_files] = malloc (strlen (*argv) + 10);
            strcpy (matches [num_files], *argv);

            if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                !filespec_ext (matches [num_files]))
                    strcat (matches [num_files], raw_pcm ? ".raw" : ".wav");

            num_files++;
        }
        else if (!outfilename) {
            outfilename = malloc (strlen (*argv) + PATH_MAX);
            strcpy (outfilename, *argv);
        }
        else if (!out2filename) {
            out2filename = malloc (strlen (*argv) + PATH_MAX);
            strcpy (out2filename, *argv);
        }
        else {
            error_line ("extra unknown argument: %s !", *argv);
            ++error_count;
        }
#else
        else if (output_spec) {
            outfilename = malloc (strlen (*argv) + PATH_MAX);
            strcpy (outfilename, *argv);
            output_spec = 0;
        }
        else {
            matches = realloc (matches, (num_files + 1) * sizeof (*matches));
            matches [num_files] = malloc (strlen (*argv) + 10);
            strcpy (matches [num_files], *argv);

            if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                !filespec_ext (matches [num_files]))
                    strcat (matches [num_files], raw_pcm ? ".raw" : ".wav");

            num_files++;
        }
#endif

    setup_break ();     // set up console and detect ^C and ^Break

    // check for various command-line argument problems

    if (!(~config.flags & (CONFIG_HIGH_FLAG | CONFIG_FAST_FLAG))) {
        error_line ("high and fast modes are mutually exclusive!");
        ++error_count;
    }

    if (ignore_length && outfilename && *outfilename == '-') {
        error_line ("can't ignore length in header when using stdout!");
        ++error_count;
    }

    if (verify_mode && outfilename && *outfilename == '-') {
        error_line ("can't verify output file when using stdout!");
        ++error_count;
    }

    if (config.flags & CONFIG_HYBRID_FLAG) {
        if ((config.flags & CONFIG_CREATE_WVC) && outfilename && *outfilename == '-') {
            error_line ("can't create correction file when using stdout!");
            ++error_count;
        }
        if (config.flags & CONFIG_MERGE_BLOCKS) {
            error_line ("--merge-blocks option is for lossless mode only!");
            ++error_count;
        }
        if ((config.flags & CONFIG_SHAPE_OVERRIDE) && (config.flags & CONFIG_DYNAMIC_SHAPING)) {
            error_line ("-s and --use-dns options are mutually exclusive!");
            ++error_count;
        }
    }
    else {
        if (config.flags & (CONFIG_CALC_NOISE | CONFIG_SHAPE_OVERRIDE | CONFIG_CREATE_WVC | CONFIG_DYNAMIC_SHAPING)) {
            error_line ("-c, -n, -s, and --use-dns options are for hybrid mode (-b) only!");
            ++error_count;
        }
    }

    if (config.flags & CONFIG_MERGE_BLOCKS) {
        if (!config.block_samples) {
            error_line ("--merge-blocks only makes sense when --blocksize is specified!");
            ++error_count;
        }
    }
    else if (config.block_samples && config.block_samples < 128) {
        error_line ("minimum blocksize is 128 when --merge-blocks is not specified!");
        ++error_count;
    }

    if (!quiet_mode && !error_count)
        fprintf (stderr, sign_on, VERSION_OS, WavpackGetLibraryVersionString ());

    // Loop through any tag specification strings and check for file access, convert text
    // strings to UTF-8, and otherwise prepare for writing to APE tags. This is done here
    // rather than after encoding so that any errors can be reported to the user now.

    for (i = 0; i < num_tag_items; ++i) {
        if (*tag_items [i].value == '@') {
            char *fn = tag_items [i].value + 1, *new_value = NULL;
            FILE *file = wild_fopen (fn, "rb");

            // if the file is not found, try using any input and output directories that the
            // user may have specified on the command line

            if (!file && num_files && filespec_name (matches [0]) && *matches [0] != '-') {
                char *temp = malloc (strlen (matches [0]) + PATH_MAX);

                strcpy (temp, matches [0]);
                strcpy (filespec_name (temp), fn);
                file = wild_fopen (temp, "rb");
                free (temp);
            }

            if (!file && outfilename && filespec_name (outfilename) && *outfilename != '-') {
                char *temp = malloc (strlen (outfilename) + PATH_MAX);

                strcpy (temp, outfilename);
                strcpy (filespec_name (temp), fn);
                file = wild_fopen (temp, "rb");
                free (temp);
            }

            if (file) {
                uint32_t bcount;

                tag_items [i].vsize = (int) DoGetFileSize (file);

                if (filespec_ext (fn))
                    tag_items [i].ext = strdup (filespec_ext (fn));

                if (tag_items [i].vsize < 1048576 * (allow_huge_tags ? 16 : 1)) {
                    new_value = malloc (tag_items [i].vsize + 2);
                    memset (new_value, 0, tag_items [i].vsize + 2);

                    if (!DoReadFile (file, new_value, tag_items [i].vsize, &bcount) ||
                        bcount != tag_items [i].vsize) {
                            free (new_value);
                            new_value = NULL;
                        }
                }

                DoCloseHandle (file);
            }

            if (!new_value) {
                error_line ("error in tag spec: %s !", tag_items [i].value);
                ++error_count;
            }
            else {
                free (tag_items [i].value);
                tag_items [i].value = new_value;
            }
        }
        else if (tag_items [i].binary) {
            error_line ("binary tags must be from files: %s !", tag_items [i].value);
            ++error_count;
        }

        if (tag_items [i].binary) {
            int isize = (int) strlen (tag_items [i].item);
            int esize = tag_items [i].ext ? (int) strlen (tag_items [i].ext) : 0;

            tag_items [i].value = realloc (tag_items [i].value, isize + esize + 1 + tag_items [i].vsize);
            memmove (tag_items [i].value + isize + esize + 1, tag_items [i].value, tag_items [i].vsize);
            strcpy (tag_items [i].value, tag_items [i].item);

            if (tag_items [i].ext)
                strcat (tag_items [i].value, tag_items [i].ext);

            tag_items [i].vsize += isize + esize + 1;
        }
        else if (tag_items [i].vsize) {
            tag_items [i].value = realloc (tag_items [i].value, tag_items [i].vsize * 2 + 1);

            if (!no_utf8_convert)
                TextToUTF8 (tag_items [i].value, (int) tag_items [i].vsize * 2 + 1);

            tag_items [i].vsize = (int) strlen (tag_items [i].value);
        }

        if ((total_tag_size += tag_items [i].vsize) > 1048576 * (allow_huge_tags ? 16 : 1)) {
            error_line ("total APEv2 tag size exceeds %d MB !", allow_huge_tags ? 16 : 1);
            ++error_count;
            break;
        }
    }

    if (error_count) {
        fprintf (stderr, "\ntype 'wavpack' for short help or 'wavpack --help' for full help\n");
        return 1;
    }

    if (!num_files) {
        printf ("%s", usage);
        return 1;
    }

    // If we are trying to create self-extracting .exe files, this is where
    // we read the wvselfx.exe file into memory in preparation for pre-pending
    // it to the WavPack files.

#if defined (WIN32)
    if (config.flags & CONFIG_CREATE_EXE) {
        FILE *wvselfx_file;
        uint32_t bcount;

        strcpy (filespec_name (selfname), "wvselfx.exe");

        wvselfx_file = fopen (selfname, "rb");

        if (!wvselfx_file) {
            _searchenv ("wvselfx.exe", "PATH", selfname);
            wvselfx_file = fopen (selfname, "rb");
        }

        if (wvselfx_file) {
            wvselfx_size = (uint32_t) DoGetFileSize (wvselfx_file);

            if (wvselfx_size && wvselfx_size != 26624 && wvselfx_size != 30720 && wvselfx_size < 49152) {
                wvselfx_image = malloc (wvselfx_size);

                if (!DoReadFile (wvselfx_file, wvselfx_image, wvselfx_size, &bcount) || bcount != wvselfx_size) {
                    free (wvselfx_image);
                    wvselfx_image = NULL;
                }
            }

            DoCloseHandle (wvselfx_file);
        }

        if (!wvselfx_image) {
            error_line ("wvselfx.exe file is not readable or is outdated!");
            free (wvselfx_image);
            exit (1);
        }
    }
#endif

    for (file_index = 0; file_index < num_files; ++file_index) {
        char *infilename = matches [file_index];

        // If the single infile specification begins with a '@', then it
        // actually points to a file that contains the names of the files
        // to be converted. This was included for use by Wim Speekenbrink's
        // frontends, but could be used for other purposes.

        if (*infilename == '@') {
            FILE *list = fopen (infilename+1, "rt");
            int di, c;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if (list == NULL) {
                error_line ("file %s not found!", infilename+1);
                free (infilename);
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
                    fname = realloc (fname, ci);
                    matches = realloc (matches, ++num_files * sizeof (*matches));

                    for (di = num_files - 1; di > file_index + 1; di--)
                        matches [di] = matches [di - 1];

                    matches [++file_index] = fname;
                }
            }

            fclose (list);
            free (infilename);
        }
#if defined (WIN32)
        else if (filespec_wild (infilename)) {
            FILE *list = fopen (infilename+1, "rt");
            intptr_t file;
            int di;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if ((file = _findfirst (infilename, &_finddata_t)) != (intptr_t) -1) {
                do {
                    if (!(_finddata_t.attrib & _A_SUBDIR)) {
                        matches = realloc (matches, ++num_files * sizeof (*matches));

                        for (di = num_files - 1; di > file_index + 1; di--)
                            matches [di] = matches [di - 1];

                        matches [++file_index] = malloc (strlen (infilename) + strlen (_finddata_t.name) + 10);
                        strcpy (matches [file_index], infilename);
                        *filespec_name (matches [file_index]) = '\0';
                        strcat (matches [file_index], _finddata_t.name);
                    }
                } while (_findnext (file, &_finddata_t) == 0);

                _findclose (file);
            }

            free (infilename);
        }
#endif
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

    if (out2filename && (num_files > 1 || !(config.flags & CONFIG_CREATE_WVC))) {
        error_line ("extra unknown argument: %s !", out2filename);
        return 1;
    }

    // if we found any files to process, this is where we start

    if (num_files) {
        char outpath, addext;

        // calculate an estimate for the percentage of the time that will be used for the encoding (as opposed
        // to the optional verification step) based on the "extra" mode processing; this is only used for
        // displaying the progress and so is not very critical

        if (verify_mode) {
            if (config.flags & CONFIG_EXTRA_MODE) {
                if (config.xmode)
                    encode_time_percent = 100.0 * (1.0 - (1.0 / ((1 << config.xmode) + 1)));
                else
                    encode_time_percent = 66.7;
            }
            else
                encode_time_percent = 50.0;
        }
        else
            encode_time_percent = 100.0;

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

        addext = !outfilename || outpath || !filespec_ext (outfilename);

        // loop through and process files in list

        for (file_index = 0; file_index < num_files; ++file_index) {
            if (check_break ())
                break;

            // generate output filename

            if (outpath) {
                strcat (outfilename, filespec_name (matches [file_index]));

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }
            else if (!outfilename) {
                outfilename = malloc (strlen (matches [file_index]) + 10);
                strcpy (outfilename, matches [file_index]);

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }

            if (addext && *outfilename != '-')
                strcat (outfilename, (config.flags & CONFIG_CREATE_EXE) ? ".exe" : ".wv");

            // if "correction" file is desired, generate name for that

            if (config.flags & CONFIG_CREATE_WVC) {
                if (!out2filename) {
                    out2filename = malloc (strlen (outfilename) + 10);
                    strcpy (out2filename, outfilename);
                }
                else {
                    char *temp = malloc (strlen (outfilename) + PATH_MAX);

                    strcpy (temp, outfilename);
                    strcpy (filespec_name (temp), filespec_name (out2filename));
                    strcpy (out2filename, temp);
                    free (temp);
                }

                if (filespec_ext (out2filename))
                    *filespec_ext (out2filename) = '\0';

                strcat (out2filename, ".wvc");
            }
            else
                out2filename = NULL;

            if (num_files > 1 && !quiet_mode)
                fprintf (stderr, "\n%s:\n", matches [file_index]);

            if (filespec_ext (matches [file_index]) && !stricmp (filespec_ext (matches [file_index]), ".wv"))
                result = repack_file (matches [file_index], outfilename, out2filename, &config);
            else
                result = pack_file (matches [file_index], outfilename, out2filename, &config);

            if (result != NO_ERROR)
                ++error_count;

            if (result == HARD_ERROR)
                break;

            // clean up in preparation for potentially another file

            if (outpath)
                *filespec_name (outfilename) = '\0';
            else if (*outfilename != '-') {
                free (outfilename);
                outfilename = NULL;
            }

            if (out2filename) {
                free (out2filename);
                out2filename = NULL;
            }

            free (matches [file_index]);
        }

        if (num_files > 1) {
            if (error_count)
                fprintf (stderr, "\n **** warning: errors occurred in %d of %d files! ****\n", error_count, num_files);
            else if (!quiet_mode)
                fprintf (stderr, "\n **** %d files successfully processed ****\n", num_files);
        }

        free (matches);
    }
    else {
        error_line ("nothing to do!");
        ++error_count;
    }

    if (outfilename)
        free (outfilename);

#ifdef DEBUG_ALLOC
    error_line ("malloc_count = %d", dump_alloc ());
#endif

    if (!no_console_title)
        DoSetConsoleTitle ("WavPack Completed");

    return error_count ? 1 : 0;
}

// This structure and function are used to write completed WavPack blocks in
// a device independent way.

typedef struct {
    uint32_t bytes_written, first_block_size;
    FILE *file;
    int error;
} write_id;

static int write_block (void *id, void *data, int32_t length)
{
    write_id *wid = (write_id *) id;
    uint32_t bcount;

    if (wid->error)
        return FALSE;

    if (wid && wid->file && data && length) {
        if (!DoWriteFile (wid->file, data, length, &bcount) || bcount != length) {
            DoTruncateFile (wid->file);
            DoCloseHandle (wid->file);
            wid->file = NULL;
            wid->error = 1;
            return FALSE;
        }
        else {
            wid->bytes_written += length;

            if (!wid->first_block_size)
                wid->first_block_size = bcount;
        }
    }

    return TRUE;
}

// Special version of fopen() that allows a wildcard specification for the
// filename. If a wildcard is specified, then it must match 1 and only 1
// file to be acceptable (i.e. it won't match just the "first" file).

#if defined (WIN32)

static FILE *wild_fopen (char *filename, const char *mode)
{
    struct _finddata_t _finddata_t;
    char *matchname = NULL;
    FILE *res = NULL;
    intptr_t file;

    if (!filespec_wild (filename) || !filespec_name (filename))
        return fopen (filename, mode);

    if ((file = _findfirst (filename, &_finddata_t)) != (intptr_t) -1) {
        do {
            if (!(_finddata_t.attrib & _A_SUBDIR)) {
                if (matchname) {
                    free (matchname);
                    matchname = NULL;
                    break;
                }
                else {
                    matchname = malloc (strlen (filename) + strlen (_finddata_t.name));
                    strcpy (matchname, filename);
                    strcpy (filespec_name (matchname), _finddata_t.name);
                }
            }
        } while (_findnext (file, &_finddata_t) == 0);

        _findclose (file);
    }

    if (matchname) {
        res = fopen (matchname, mode);
        free (matchname);
    }

    return res;
}

#else

static FILE *wild_fopen (char *filename, const char *mode)
{
    char *matchname = NULL;
    struct stat statbuf;
    FILE *res = NULL;
    glob_t globbuf;
    int i;

    glob (filename, 0, NULL, &globbuf);

    for (i = 0; i < globbuf.gl_pathc; ++i) {
        if (stat (globbuf.gl_pathv [i], &statbuf) == -1 || S_ISDIR (statbuf.st_mode))
            continue;

        if (matchname) {
            free (matchname);
            matchname = NULL;
            break;
        }
        else {
            matchname = malloc (strlen (globbuf.gl_pathv [i]) + 10);
            strcpy (matchname, globbuf.gl_pathv [i]);
        }
    }

    globfree (&globbuf);

    if (matchname) {
        res = fopen (matchname, mode);
        free (matchname);
    }

    return res;
}

#endif


// This function packs a single file "infilename" and stores the result at
// "outfilename". If "out2filename" is specified, then the "correction"
// file would go there. The files are opened and closed in this function
// and the "config" structure specifies the mode of compression.

static int pack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config)
{
    char *outfilename_temp, *out2filename_temp, dummy;
    int use_tempfiles = (out2filename != NULL);
    uint32_t total_samples = 0, bcount;
    WavpackConfig loc_config = *config;
    RiffChunkHeader riff_chunk_header;
    unsigned char *new_channel_order = NULL;
    unsigned char md5_digest [16];
    write_id wv_file, wvc_file;
    ChunkHeader chunk_header;
    WaveHeader WaveHeader;
    WavpackContext *wpc;
    int64_t infilesize;
    double dtime;
    FILE *infile;
    int result;

#if defined(WIN32)
    struct _timeb time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    CLEAR (WaveHeader);

    CLEAR (wv_file);
    CLEAR (wvc_file);
    wpc = WavpackOpenFileOutput (write_block, &wv_file, out2filename ? &wvc_file : NULL);

    // open the source file for reading

    if (*infilename == '-') {
        infile = stdin;
#if defined(WIN32)
        _setmode (fileno (stdin), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdin), O_BINARY);
#endif
    }
    else if ((infile = fopen (infilename, "rb")) == NULL) {
        error_line ("can't open file %s!", infilename);
        WavpackCloseFile (wpc);
        return SOFT_ERROR;
    }

    infilesize = DoGetFileSize (infile);

    if (raw_pcm) {
        if (infilesize) {
            int sample_size = loc_config.bytes_per_sample * loc_config.num_channels;

            total_samples = (int) (infilesize / sample_size);

            if (infilesize % sample_size)
                error_line ("warning: raw pcm infile length does not divide evenly, %d bytes will be discarded",
                    infilesize % sample_size);
        }
        else
            total_samples = -1;
    }
    else if (infilesize >= 4294967296LL && !ignore_length) {
        error_line ("can't handle .WAV files larger than 4 GB (non-standard)!");
        WavpackCloseFile (wpc);
        return SOFT_ERROR;
    }

    // check both output files for overwrite warning required

    // note that for a file to be considered "overwritable", it must both be openable for reading
    // and have at least 1 readable byte - this prevents us getting stuck on "nul" (Windows) 

    if (*outfilename != '-' && (wv_file.file = fopen (outfilename, "rb")) != NULL) {
        size_t res = fread (&dummy, 1, 1, wv_file.file);

        DoCloseHandle (wv_file.file);

        if (res == 1) {
            use_tempfiles = 1;

            if (!overwrite_all) {
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (outfilename));

                if (!no_console_title)
                    DoSetConsoleTitle ("overwrite?");

                switch (yna ()) {
                    case 'n':
                        DoCloseHandle (infile);
                        WavpackCloseFile (wpc);
                        return SOFT_ERROR;

                    case 'a':
                        overwrite_all = 1;
                }
            }
        }
    }

    if (out2filename && !overwrite_all && (wvc_file.file = fopen (out2filename, "rb")) != NULL) {
        size_t res = fread (&dummy, 1, 1, wvc_file.file);

        DoCloseHandle (wvc_file.file);

        if (res == 1) {
            fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (out2filename));

            if (!no_console_title)
                DoSetConsoleTitle ("overwrite?");

            switch (yna ()) {

                case 'n':
                    DoCloseHandle (infile);
                    WavpackCloseFile (wpc);
                    return SOFT_ERROR;

                case 'a':
                    overwrite_all = 1;
            }
        }
    }

    // if we are using temp files, either because the output filename already exists or we are creating a
    // "correction" file, search for and generate the corresponding names here

    if (use_tempfiles) {
        FILE *testfile;
        int count = 0;

        outfilename_temp = malloc (strlen (outfilename) + 16);

        if (out2filename)
            out2filename_temp = malloc (strlen (outfilename) + 16);

        while (1) {
            strcpy (outfilename_temp, outfilename);

            if (filespec_ext (outfilename_temp)) {
                if (count++)
                    sprintf (filespec_ext (outfilename_temp), ".tmp%d", count-1);
                else
                    strcpy (filespec_ext (outfilename_temp), ".tmp");

                strcat (outfilename_temp, filespec_ext (outfilename));
            }
            else {
                if (count++)
                    sprintf (outfilename_temp + strlen (outfilename_temp), ".tmp%d", count-1);
                else
                    strcat (outfilename_temp, ".tmp");
            }

            testfile = fopen (outfilename_temp, "rb");

            if (testfile) {
                int res = (int) fread (&dummy, 1, 1, testfile);

                fclose (testfile);

                if (res == 1)
                    continue;
            }

            if (out2filename) {
                strcpy (out2filename_temp, outfilename_temp);
                strcat (out2filename_temp, "c");

                testfile = fopen (out2filename_temp, "rb");

                if (testfile) {
                    int res = (int) fread (&dummy, 1, 1, testfile);

                    fclose (testfile);

                    if (res == 1)
                        continue;
                }
            }   

            break;
        }
    }

#if defined(WIN32)
    _ftime (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    // open output file for writing

    if (*outfilename == '-') {
        wv_file.file = stdout;
#if defined(WIN32)
        _setmode (fileno (stdout), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdout), O_BINARY);
#endif
    }
    else if ((wv_file.file = fopen (use_tempfiles ? outfilename_temp : outfilename, "w+b")) == NULL) {
        error_line ("can't create file %s!", use_tempfiles ? outfilename_temp : outfilename);
        DoCloseHandle (infile);
        WavpackCloseFile (wpc);
        return SOFT_ERROR;
    }

    if (!quiet_mode) {
        if (*outfilename == '-')
            fprintf (stderr, "packing %s to stdout,", *infilename == '-' ? "stdin" : FN_FIT (infilename));
        else if (out2filename)
            fprintf (stderr, "creating %s (+%s),", FN_FIT (outfilename), filespec_ext (out2filename));
        else
            fprintf (stderr, "creating %s,", FN_FIT (outfilename));
    }

#if defined (WIN32)
    if (loc_config.flags & CONFIG_CREATE_EXE)
        if (!DoWriteFile (wv_file.file, wvselfx_image, wvselfx_size, &bcount) || bcount != wvselfx_size) {
            error_line ("can't write WavPack data, disk probably full!");
            DoCloseHandle (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (wpc);
            return SOFT_ERROR;
        }
#endif

    // if not in "raw" mode, read (and copy to output) initial RIFF form header

    if (!raw_pcm) {
        if ((!DoReadFile (infile, &riff_chunk_header, sizeof (RiffChunkHeader), &bcount) ||
            bcount != sizeof (RiffChunkHeader) || strncmp (riff_chunk_header.ckID, "RIFF", 4) ||
            strncmp (riff_chunk_header.formType, "WAVE", 4))) {
                error_line ("%s is not a valid .WAV file!", infilename);
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
        }
        else if (!new_riff_header &&
            !WavpackAddWrapper (wpc, &riff_chunk_header, sizeof (RiffChunkHeader))) {
                error_line ("%s", WavpackGetErrorMessage (wpc));
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
        }
    }

    // if not in "raw" mode, loop through all elements of the RIFF wav header
    // (until the data chuck) and copy them to the output file

    while (!raw_pcm) {

        if (!DoReadFile (infile, &chunk_header, sizeof (ChunkHeader), &bcount) ||
            bcount != sizeof (ChunkHeader)) {
                error_line ("%s is not a valid .WAV file!", infilename);
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
        }
        else if (!new_riff_header &&
            !WavpackAddWrapper (wpc, &chunk_header, sizeof (ChunkHeader))) {
                error_line ("%s", WavpackGetErrorMessage (wpc));
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
        }

        WavpackLittleEndianToNative (&chunk_header, ChunkHeaderFormat);

        // if it's the format chunk, we want to get some info out of there and
        // make sure it's a .wav file we can handle

        if (!strncmp (chunk_header.ckID, "fmt ", 4)) {
            int supported = TRUE, format;

            if (chunk_header.ckSize < 16 || chunk_header.ckSize > sizeof (WaveHeader) ||
                !DoReadFile (infile, &WaveHeader, chunk_header.ckSize, &bcount) ||
                bcount != chunk_header.ckSize) {
                    error_line ("%s is not a valid .WAV file!", infilename);
                    DoCloseHandle (infile);
                    DoCloseHandle (wv_file.file);
                    DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                    WavpackCloseFile (wpc);
                    return SOFT_ERROR;
            }
            else if (!new_riff_header &&
                !WavpackAddWrapper (wpc, &WaveHeader, chunk_header.ckSize)) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    DoCloseHandle (infile);
                    DoCloseHandle (wv_file.file);
                    DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                    WavpackCloseFile (wpc);
                    return SOFT_ERROR;
            }

            WavpackLittleEndianToNative (&WaveHeader, WaveHeaderFormat);

            if (debug_logging_mode) {
                error_line ("format tag size = %d", chunk_header.ckSize);
                error_line ("FormatTag = %x, NumChannels = %d, BitsPerSample = %d",
                    WaveHeader.FormatTag, WaveHeader.NumChannels, WaveHeader.BitsPerSample);
                error_line ("BlockAlign = %d, SampleRate = %d, BytesPerSecond = %d",
                    WaveHeader.BlockAlign, WaveHeader.SampleRate, WaveHeader.BytesPerSecond);

                if (chunk_header.ckSize > 16)
                    error_line ("cbSize = %d, ValidBitsPerSample = %d", WaveHeader.cbSize,
                        WaveHeader.ValidBitsPerSample);

                if (chunk_header.ckSize > 20)
                    error_line ("ChannelMask = %x, SubFormat = %d",
                        WaveHeader.ChannelMask, WaveHeader.SubFormat);
            }

            if (chunk_header.ckSize > 16 && WaveHeader.cbSize == 2)
                adobe_mode = 1;

            format = (WaveHeader.FormatTag == 0xfffe && chunk_header.ckSize == 40) ?
                WaveHeader.SubFormat : WaveHeader.FormatTag;

            loc_config.bits_per_sample = (chunk_header.ckSize == 40 && WaveHeader.ValidBitsPerSample) ?
                WaveHeader.ValidBitsPerSample : WaveHeader.BitsPerSample;

            if (format != 1 && format != 3)
                supported = FALSE;

            if (format == 3 && loc_config.bits_per_sample != 32 && !store_floats_as_ints)
                supported = FALSE;

            if (!WaveHeader.NumChannels || WaveHeader.NumChannels > 256 ||
                WaveHeader.BlockAlign / WaveHeader.NumChannels < (loc_config.bits_per_sample + 7) / 8 ||
                WaveHeader.BlockAlign / WaveHeader.NumChannels > 4 ||
                WaveHeader.BlockAlign % WaveHeader.NumChannels)
                    supported = FALSE;

            if (loc_config.bits_per_sample < 1 || loc_config.bits_per_sample > 32)
                supported = FALSE;

            if (!supported) {
                error_line ("%s is an unsupported .WAV format!", infilename);
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }

            if (chunk_header.ckSize < 40) {
                if (WaveHeader.NumChannels <= 2)
                    loc_config.channel_mask = 0x5 - WaveHeader.NumChannels;
                else if (WaveHeader.NumChannels <= 18)
                    loc_config.channel_mask = (1 << WaveHeader.NumChannels) - 1;
                else
                    loc_config.channel_mask = 0x3ffff;
            }
            else if (WaveHeader.ChannelMask && (num_channels_order || channel_order_undefined)) {
                error_line ("this WAV file already has channel order information!");
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }
            else
                loc_config.channel_mask = WaveHeader.ChannelMask;

            if (format == 3 && !store_floats_as_ints)
                loc_config.float_norm_exp = 127;
            else if (adobe_mode &&
                WaveHeader.BlockAlign / WaveHeader.NumChannels == 4) {
                    if (WaveHeader.BitsPerSample == 24)
                        loc_config.float_norm_exp = 127 + 23;
                    else if (WaveHeader.BitsPerSample == 32)
                        loc_config.float_norm_exp = 127 + 15;
            }

            if (debug_logging_mode) {
                if (loc_config.float_norm_exp == 127)
                    error_line ("data format: normalized 32-bit floating point");
                else if (loc_config.float_norm_exp)
                    error_line ("data format: 32-bit floating point (Audition %d:%d float type 1)",
                        loc_config.float_norm_exp - 126, 150 - loc_config.float_norm_exp);
                else
                    error_line ("data format: %d-bit integers stored in %d byte(s)",
                        loc_config.bits_per_sample, WaveHeader.BlockAlign / WaveHeader.NumChannels);
            }
        }
        else if (!strncmp (chunk_header.ckID, "data", 4)) {

            // on the data chunk, get size and exit loop

            if (!WaveHeader.NumChannels) {      // make sure we saw a "fmt" chunk...
                error_line ("%s is not a valid .WAV file!", infilename);
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }

            if (infilesize && !ignore_length && infilesize - chunk_header.ckSize > 16777216) {
                error_line ("this .WAV file has over 16 MB of extra RIFF data, probably is corrupt!");
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }

            total_samples = chunk_header.ckSize / WaveHeader.BlockAlign;

            if (!total_samples && !ignore_length) {
                error_line ("this .WAV file has no audio samples, probably is corrupt!");
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }

            loc_config.bytes_per_sample = WaveHeader.BlockAlign / WaveHeader.NumChannels;
            loc_config.num_channels = WaveHeader.NumChannels;
            loc_config.sample_rate = WaveHeader.SampleRate;
            break;
        }
        else {          // just copy unknown chunks to output file

            int bytes_to_copy = (chunk_header.ckSize + 1) & ~1L;
            char *buff = malloc (bytes_to_copy);

            if (debug_logging_mode)
                error_line ("extra unknown chunk \"%c%c%c%c\" of %d bytes",
                    chunk_header.ckID [0], chunk_header.ckID [1], chunk_header.ckID [2],
                    chunk_header.ckID [3], chunk_header.ckSize);

            if (!DoReadFile (infile, buff, bytes_to_copy, &bcount) ||
                bcount != bytes_to_copy ||
                (!new_riff_header &&
                !WavpackAddWrapper (wpc, buff, bytes_to_copy))) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    DoCloseHandle (infile);
                    DoCloseHandle (wv_file.file);
                    DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                    free (buff);
                    WavpackCloseFile (wpc);
                    return SOFT_ERROR;
            }

            free (buff);
        }
    }

    if (num_channels_order || channel_order_undefined) {
        int i, j;

        if (loc_config.num_channels < num_channels_order ||
            (loc_config.num_channels > num_channels_order && !channel_order_undefined)) {
                error_line ("file does not have %d channel(s)!", num_channels_order);
                DoCloseHandle (infile);
                DoCloseHandle (wv_file.file);
                DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
                WavpackCloseFile (wpc);
                return SOFT_ERROR;
            }

        loc_config.channel_mask = channel_order_mask;

        if (num_channels_order) {
            new_channel_order = malloc (loc_config.num_channels);

            for (i = 0; i < loc_config.num_channels; ++i)
                new_channel_order [i] = i;

            memcpy (new_channel_order, channel_order, num_channels_order);

            for (i = 0; i < num_channels_order;) {
                for (j = 0; j < num_channels_order; ++j)
                    if (new_channel_order [j] == i) {
                        i++;
                        break;
                    }

                if (j == num_channels_order)
                    for (j = 0; j < num_channels_order; ++j)
                        if (new_channel_order [j] > i)
                            new_channel_order [j]--;
            }
        }
    }

    if (!WavpackSetConfiguration (wpc, &loc_config, total_samples)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        DoCloseHandle (infile);
        DoCloseHandle (wv_file.file);
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
        WavpackCloseFile (wpc);
        return SOFT_ERROR;
    }

    // if we are creating a "correction" file, open it now for writing

    if (out2filename) {
        if ((wvc_file.file = fopen (use_tempfiles ? out2filename_temp : out2filename, "w+b")) == NULL) {
            error_line ("can't create correction file!");
            DoCloseHandle (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (wpc);
            return SOFT_ERROR;
        }
    }

    // pack the audio portion of the file now; calculate md5 if we're writing it to the file or verify mode is active

    result = pack_audio (wpc, infile, new_channel_order, ((loc_config.flags & CONFIG_MD5_CHECKSUM) || verify_mode) ? md5_digest : NULL);

    if (new_channel_order)
        free (new_channel_order);

    // write the md5 sum if the user asked for it to be included

    if (result == NO_ERROR && (loc_config.flags & CONFIG_MD5_CHECKSUM))
        WavpackStoreMD5Sum (wpc, md5_digest);

    // if everything went well (and we're not ignoring length) try to read
    // anything else that might be appended to the audio data and write that
    // to the WavPack metadata as "wrapper"

    if (result == NO_ERROR && !ignore_length && !raw_pcm) {
        unsigned char buff [16];

        while (DoReadFile (infile, buff, sizeof (buff), &bcount) && bcount)
            if (!new_riff_header &&
                !WavpackAddWrapper (wpc, buff, bcount)) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    result = HARD_ERROR;
                    break;
            }
    }

    DoCloseHandle (infile);     // we're now done with input file, so close

    // we're now done with any WavPack blocks, so flush any remaining data

    if (result == NO_ERROR && !WavpackFlushSamples (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        result = HARD_ERROR;
    }

    // if still no errors, check to see if we need to create & write a tag
    // (which is NOT stored in regular WavPack blocks)

    if (result == NO_ERROR && num_tag_items) {
        int i, res = TRUE;

        for (i = 0; i < num_tag_items && res; ++i)
            if (tag_items [i].vsize) {
                if (tag_items [i].binary) 
                    res = WavpackAppendBinaryTagItem (wpc, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
                else
                    res = WavpackAppendTagItem (wpc, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
            }

        if (!res || !WavpackWriteTag (wpc)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            result = HARD_ERROR;
        }
    }

    // At this point we're done writing to the output files. However, in some
    // situations we might have to back up and re-write the initial blocks.
    // Currently the only case is if we're ignoring length or inputting raw pcm data.

    if (result == NO_ERROR && WavpackGetNumSamples (wpc) != WavpackGetSampleIndex (wpc)) {
        if (raw_pcm || ignore_length) {
            char *block_buff = malloc (wv_file.first_block_size);
            uint32_t wrapper_size;

            if (block_buff && !DoSetFilePositionAbsolute (wv_file.file, 0) &&
                DoReadFile (wv_file.file, block_buff, wv_file.first_block_size, &bcount) &&
                bcount == wv_file.first_block_size && !strncmp (block_buff, "wvpk", 4)) {

                    WavpackUpdateNumSamples (wpc, block_buff);

                    if (WavpackGetWrapperLocation (block_buff, &wrapper_size)) {
                        unsigned char *wrapper_location = WavpackGetWrapperLocation (block_buff, NULL);
                        unsigned char *chunk_header = malloc (sizeof (ChunkHeader));
                        uint32_t data_size = WavpackGetSampleIndex (wpc) * WavpackGetNumChannels (wpc) *
                            WavpackGetBytesPerSample (wpc);

                        memcpy (chunk_header, wrapper_location, sizeof (ChunkHeader));

                        if (!strncmp ((char *) chunk_header, "RIFF", 4)) {
                            ((ChunkHeader *)chunk_header)->ckSize = wrapper_size + data_size - 8;
                            WavpackNativeToLittleEndian (chunk_header, ChunkHeaderFormat);
                        }

                        memcpy (wrapper_location, chunk_header, sizeof (ChunkHeader));
                        memcpy (chunk_header, wrapper_location + wrapper_size - sizeof (ChunkHeader), sizeof (ChunkHeader));

                        if (!strncmp ((char *) chunk_header, "data", 4)) {
                            ((ChunkHeader *)chunk_header)->ckSize = data_size;
                            WavpackNativeToLittleEndian (chunk_header, ChunkHeaderFormat);
                        }

                        memcpy (wrapper_location + wrapper_size - sizeof (ChunkHeader), chunk_header, sizeof (ChunkHeader));
                        free (chunk_header);
                    }

                    if (DoSetFilePositionAbsolute (wv_file.file, 0) ||
                        !DoWriteFile (wv_file.file, block_buff, wv_file.first_block_size, &bcount) ||
                        bcount != wv_file.first_block_size) {
                            error_line ("couldn't update WavPack header with actual length!!");
                            result = SOFT_ERROR;
                    }

                    free (block_buff);
            }
            else {
                error_line ("couldn't update WavPack header with actual length!!");
                result = SOFT_ERROR;
            }

            if (result == NO_ERROR && wvc_file.file) {
                block_buff = malloc (wvc_file.first_block_size);

                if (block_buff && !DoSetFilePositionAbsolute (wvc_file.file, 0) &&
                    DoReadFile (wvc_file.file, block_buff, wvc_file.first_block_size, &bcount) &&
                    bcount == wvc_file.first_block_size && !strncmp (block_buff, "wvpk", 4)) {

                        WavpackUpdateNumSamples (wpc, block_buff);

                        if (DoSetFilePositionAbsolute (wvc_file.file, 0) ||
                            !DoWriteFile (wvc_file.file, block_buff, wvc_file.first_block_size, &bcount) ||
                            bcount != wvc_file.first_block_size) {
                                error_line ("couldn't update WavPack header with actual length!!");
                                result = SOFT_ERROR;
                        }
                }
                else {
                    error_line ("couldn't update WavPack header with actual length!!");
                    result = SOFT_ERROR;
                }

                free (block_buff);
            }
        }
        else {
            error_line ("couldn't read all samples, file may be corrupt!!");
            result = SOFT_ERROR;
        }
    }

    // at this point we're completely done with the files, so close 'em whether there
    // were any other errors or not

    if (!DoCloseHandle (wv_file.file)) {
        error_line ("can't close WavPack file!");

        if (result == NO_ERROR)
            result = SOFT_ERROR;
    }

    if (out2filename && !DoCloseHandle (wvc_file.file)) {
        error_line ("can't close correction file!");

        if (result == NO_ERROR)
            result = SOFT_ERROR;
    }

    // if there have been no errors up to now, and verify mode is enabled, do that now; only pass in the md5 if this
    // was a lossless operation (either explicitly or because a high lossy bitrate resulted in lossless)

    if (result == NO_ERROR && verify_mode)
        result = verify_audio (use_tempfiles ? outfilename_temp : outfilename, !WavpackLossyBlocks (wpc) ? md5_digest : NULL);

    // if there were any errors, delete the output files, close the context, and return the error

    if (result != NO_ERROR) {
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);

        if (out2filename)
            DoDeleteFile (use_tempfiles ? out2filename_temp : out2filename);

        WavpackCloseFile (wpc);
        return result;
    }

    // if we were writing to a temp file because the target file already existed,
    // do the rename / overwrite now (and if that fails, return the error)

    if (use_tempfiles) {
#if defined(WIN32)
        FILE *temp;

        if (remove (outfilename) && (temp = fopen (outfilename, "rb"))) {
            error_line ("can not remove file %s, result saved in %s!", outfilename, outfilename_temp);
            result = SOFT_ERROR;
            fclose (temp);
        }
        else
#endif
        if (rename (outfilename_temp, outfilename)) {
            error_line ("can not rename temp file %s to %s!", outfilename_temp, outfilename);
            result = SOFT_ERROR;
        }

        if (out2filename) {
#if defined(WIN32)
            FILE *temp;

            if (remove (out2filename) && (temp = fopen (out2filename, "rb"))) {
                error_line ("can not remove file %s, result saved in %s!", out2filename, out2filename_temp);
                result = SOFT_ERROR;
                fclose (temp);
            }
            else
#endif
            if (rename (out2filename_temp, out2filename)) {
                error_line ("can not rename temp file %s to %s!", out2filename_temp, out2filename);
                result = SOFT_ERROR;
            }
        }

        free (outfilename_temp);
        if (out2filename) free (out2filename_temp);

        if (result != NO_ERROR) {
            WavpackCloseFile (wpc);
            return result;
        }
    }

    if (result == NO_ERROR && copy_time)
        if (!copy_timestamp (infilename, outfilename) ||
            (out2filename && !copy_timestamp (infilename, out2filename)))
                error_line ("failure copying time stamp!");

    // delete source file if that option is enabled

    if (result == NO_ERROR && delete_source) {
        int res = DoDeleteFile (infilename);

        if (!quiet_mode || !res)
            error_line ("%s source file %s", res ?
                "deleted" : "can't delete", infilename);
    }

    // compute and display the time consumed along with some other details of
    // the packing operation, and then return NO_ERROR

#if defined(WIN32)
    _ftime (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if ((loc_config.flags & CONFIG_CALC_NOISE) && WavpackGetEncodedNoise (wpc, NULL) > 0.0) {
        int full_scale_bits = WavpackGetBitsPerSample (wpc);
        double full_scale_rms = 0.5, sum, peak;

        while (full_scale_bits--)
            full_scale_rms *= 2.0;

        full_scale_rms = full_scale_rms * (full_scale_rms - 1.0) * 0.5;
        sum = WavpackGetEncodedNoise (wpc, &peak);

        error_line ("ave noise = %.2f dB, peak noise = %.2f dB",
            log10 (sum / WavpackGetNumSamples (wpc) / full_scale_rms) * 10,
            log10 (peak / full_scale_rms) * 10);
    }

    if (!quiet_mode) {
        char *file, *fext, *oper, *cmode, cratio [16] = "";

        if (loc_config.flags & CONFIG_MD5_CHECKSUM) {
            char md5_string [] = "original md5 signature: 00000000000000000000000000000000";
            int i;

            for (i = 0; i < 16; ++i)
                sprintf (md5_string + 24 + (i * 2), "%02x", md5_digest [i]);

            error_line (md5_string);
        }

        if (outfilename && *outfilename != '-') {
            file = FN_FIT (outfilename);
            fext = wvc_file.bytes_written ? " (+.wvc)" : "";
            oper = verify_mode ? "created (and verified)" : "created";
        }
        else {
            file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
            fext = "";
            oper = "packed";
        }

        if (WavpackLossyBlocks (wpc)) {
            cmode = "lossy";

            if (WavpackGetAverageBitrate (wpc, TRUE) != 0.0)
                sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (wpc, TRUE) / 1000.0));
        }
        else {
            cmode = "lossless";

            if (WavpackGetRatio (wpc) != 0.0)
                sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (wpc) * 100.0);
        }

        error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (wpc);
    return NO_ERROR;
}

// This function handles the actual audio data compression. It assumes that the
// input file is positioned at the beginning of the audio data and that the
// WavPack configuration has been set. This is where the conversion from RIFF
// little-endian standard the executing processor's format is done and where
// (if selected) the MD5 sum is calculated and displayed.

static void reorder_channels (void *data, unsigned char *new_order, int num_chans,
    int num_samples, int bytes_per_sample);

#define INPUT_SAMPLES 65536

static int pack_audio (WavpackContext *wpc, FILE *infile, unsigned char *new_order, unsigned char *md5_digest_source)
{
    uint32_t samples_remaining, input_samples = INPUT_SAMPLES, samples_read = 0;
    double progress = -1.0;
    int bytes_per_sample;
    int32_t *sample_buffer;
    unsigned char *input_buffer;
    MD5_CTX md5_context;

    // don't use an absurd amount of memory just because we have an absurd number of channels

    while (input_samples * sizeof (int32_t) * WavpackGetNumChannels (wpc) > 2048*1024)
        input_samples >>= 1;

    if (md5_digest_source)
        MD5Init (&md5_context);

    WavpackPackInit (wpc);
    bytes_per_sample = WavpackGetBytesPerSample (wpc) * WavpackGetNumChannels (wpc);
    input_buffer = malloc (input_samples * bytes_per_sample);
    sample_buffer = malloc (input_samples * sizeof (int32_t) * WavpackGetNumChannels (wpc));
    samples_remaining = WavpackGetNumSamples (wpc);

    while (1) {
        uint32_t bytes_to_read, bytes_read = 0;
        unsigned int sample_count;

        if (raw_pcm || ignore_length || samples_remaining > input_samples)
            bytes_to_read = input_samples * bytes_per_sample;
        else
            bytes_to_read = samples_remaining * bytes_per_sample;

        samples_remaining -= bytes_to_read / bytes_per_sample;
        DoReadFile (infile, input_buffer, bytes_to_read, &bytes_read);
        samples_read += sample_count = bytes_read / bytes_per_sample;

        if (new_order)
            reorder_channels (input_buffer, new_order, WavpackGetNumChannels (wpc),
                sample_count, WavpackGetBytesPerSample (wpc));

        if (md5_digest_source)
            MD5Update (&md5_context, input_buffer, sample_count * bytes_per_sample);

        if (!sample_count)
            break;

        if (sample_count) {
            unsigned int cnt = sample_count * WavpackGetNumChannels (wpc);
            unsigned char *sptr = input_buffer;
            int32_t *dptr = sample_buffer;

            switch (WavpackGetBytesPerSample (wpc)) {

                case 1:
                    while (cnt--)
                        *dptr++ = *sptr++ - 128;

                    break;

                case 2:
                    while (cnt--) {
                        *dptr++ = sptr [0] | ((int32_t)(signed char) sptr [1] << 8);
                        sptr += 2;
                    }

                    break;

                case 3:
                    while (cnt--) {
                        *dptr++ = sptr [0] | ((int32_t) sptr [1] << 8) | ((int32_t)(signed char) sptr [2] << 16);
                        sptr += 3;
                    }

                    break;

                case 4:
                    while (cnt--) {
                        *dptr++ = sptr [0] | ((int32_t) sptr [1] << 8) | ((int32_t) sptr [2] << 16) | ((int32_t)(signed char) sptr [3] << 24);
                        sptr += 4;
                    }

                    break;
            }
        }

        if (!WavpackPackSamples (wpc, sample_buffer, sample_count)) {
            error_line ("%s", WavpackGetErrorMessage (wpc));
            free (sample_buffer);
            free (input_buffer);
            return HARD_ERROR;
        }

        if (check_break ()) {
#if defined(WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            free (sample_buffer);
            free (input_buffer);
            return SOFT_ERROR;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * encode_time_percent + 0.5)) {
                int nobs = progress == -1.0;

                progress = floor (WavpackGetProgress (wpc) * encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode)
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
        }
    }

    free (sample_buffer);
    free (input_buffer);

    if (!WavpackFlushSamples (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        return HARD_ERROR;
    }

    if (md5_digest_source)
        MD5Final (md5_digest_source, &md5_context);

    return NO_ERROR;
}

// This function transcodes a single WavPack file "infilename" and stores the resulting
// WavPack file at "outfilename". If "out2filename" is specified, then the "correction"
// file would go there. The files are opened and closed in this function and the "config"
// structure specifies the mode of compression. Note that lossy to lossless transcoding
// is not allowed (no technical reason, it's just dumb, and could result in files that
// fail their MD5 verification test).

static int repack_file (char *infilename, char *outfilename, char *out2filename, const WavpackConfig *config)
{
    int output_lossless = !(config->flags & CONFIG_HYBRID_FLAG) || (config->flags & CONFIG_CREATE_WVC);
    int use_tempfiles = (out2filename != NULL), input_mode;
    unsigned char md5_verify [16], md5_display [16];
    char *outfilename_temp, *out2filename_temp;
    uint32_t total_samples = 0, bcount;
    WavpackConfig loc_config = *config;
    WavpackContext *infile, *outfile;
    write_id wv_file, wvc_file;
    char error [80];
    double dtime;
    int result;

#if defined(WIN32)
    struct _timeb time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    // use library to open input WavPack file

    infile = WavpackOpenFileInput (infilename, error, OPEN_WVC | OPEN_TAGS | OPEN_WRAPPER, 0);

    if (!infile) {
        error_line (error);
        return SOFT_ERROR;
    }

    input_mode = WavpackGetMode (infile);

    if (!(input_mode & MODE_LOSSLESS) && output_lossless) {
        error_line ("can't transcode lossy file %s to lossless...not allowed!", infilename);
        WavpackCloseFile (infile);
        return SOFT_ERROR;
    }

    total_samples = WavpackGetNumSamples (infile);

    if (total_samples == (uint32_t) -1) {
        error_line ("can't transcode file %s of unknown length!", infilename);
        WavpackCloseFile (infile);
        return SOFT_ERROR;
    }

    // open an output context

    CLEAR (wv_file);
    CLEAR (wvc_file);
    outfile = WavpackOpenFileOutput (write_block, &wv_file, out2filename ? &wvc_file : NULL);

    // check both output files for overwrite warning required

    if (*outfilename != '-' && (wv_file.file = fopen (outfilename, "rb")) != NULL) {
        DoCloseHandle (wv_file.file);
        use_tempfiles = 1;

        if (!overwrite_all) {
            if (output_lossless)
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (outfilename));
            else
                fprintf (stderr, "overwrite %s with lossy transcode (yes/no/all)? ", FN_FIT (outfilename));

            if (!no_console_title)
                DoSetConsoleTitle ("overwrite?");

            switch (yna ()) {
                case 'n':
                    WavpackCloseFile (infile);
                    WavpackCloseFile (outfile);
                    return SOFT_ERROR;

                case 'a':
                    overwrite_all = 1;
            }
        }
    }

    if (out2filename && !overwrite_all && (wvc_file.file = fopen (out2filename, "rb")) != NULL) {
        DoCloseHandle (wvc_file.file);
        fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (out2filename));

        if (!no_console_title)
            DoSetConsoleTitle ("overwrite?");

        switch (yna ()) {

            case 'n':
                WavpackCloseFile (infile);
                WavpackCloseFile (outfile);
                return SOFT_ERROR;

            case 'a':
                overwrite_all = 1;
        }
    }

    // if we are using temp files, either because the output filename already exists or we are creating a
    // "correction" file, search for and generate the corresponding names here

    if (use_tempfiles) {
        FILE *testfile;
        int count = 0;

        outfilename_temp = malloc (strlen (outfilename) + 16);

        if (out2filename)
            out2filename_temp = malloc (strlen (outfilename) + 16);

        while (1) {
            strcpy (outfilename_temp, outfilename);

            if (filespec_ext (outfilename_temp)) {
                if (count++)
                    sprintf (filespec_ext (outfilename_temp), ".tmp%d", count-1);
                else
                    strcpy (filespec_ext (outfilename_temp), ".tmp");

                strcat (outfilename_temp, filespec_ext (outfilename));
            }
            else {
                if (count++)
                    sprintf (outfilename_temp + strlen (outfilename_temp), ".tmp%d", count-1);
                else
                    strcat (outfilename_temp, ".tmp");
            }

            testfile = fopen (outfilename_temp, "rb");

            if (testfile) {
                fclose (testfile);
                continue;
            }

            if (out2filename) {
                strcpy (out2filename_temp, outfilename_temp);
                strcat (out2filename_temp, "c");

                testfile = fopen (out2filename_temp, "rb");

                if (testfile) {
                    fclose (testfile);
                    continue;
                }
            }   

            break;
        }
    }

#if defined(WIN32)
    _ftime (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    // open output file for writing

    if (*outfilename == '-') {
        wv_file.file = stdout;
#if defined(WIN32)
        _setmode (fileno (stdout), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdout), O_BINARY);
#endif
    }
    else if ((wv_file.file = fopen (use_tempfiles ? outfilename_temp : outfilename, "w+b")) == NULL) {
        error_line ("can't create file %s!", use_tempfiles ? outfilename_temp : outfilename);
        WavpackCloseFile (infile);
        WavpackCloseFile (outfile);
        return SOFT_ERROR;
    }

    if (!quiet_mode) {
        if (*outfilename == '-')
            fprintf (stderr, "packing %s to stdout,", *infilename == '-' ? "stdin" : FN_FIT (infilename));
        else if (out2filename)
            fprintf (stderr, "creating %s (+%s),", FN_FIT (outfilename), filespec_ext (out2filename));
        else
            fprintf (stderr, "creating %s,", FN_FIT (outfilename));
    }

#if defined (WIN32)
    if (loc_config.flags & CONFIG_CREATE_EXE)
        if (!DoWriteFile (wv_file.file, wvselfx_image, wvselfx_size, &bcount) || bcount != wvselfx_size) {
            error_line ("can't write WavPack data, disk probably full!");
            WavpackCloseFile (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (outfile);
            return SOFT_ERROR;
        }
#endif

    // unless we've been specifically told not to, copy RIFF header

    if (!new_riff_header && WavpackGetWrapperBytes (infile)) {
        if (!WavpackAddWrapper (outfile, WavpackGetWrapperData (infile), WavpackGetWrapperBytes (infile))) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            WavpackCloseFile (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (outfile);
            return SOFT_ERROR;
        }

        WavpackFreeWrapper (infile);
    }

    loc_config.bytes_per_sample = WavpackGetBytesPerSample (infile);
    loc_config.bits_per_sample = WavpackGetBitsPerSample (infile);
    loc_config.channel_mask = WavpackGetChannelMask (infile);
    loc_config.num_channels = WavpackGetNumChannels (infile);
    loc_config.sample_rate = WavpackGetSampleRate (infile);

    if (input_mode & MODE_FLOAT)
        loc_config.float_norm_exp = WavpackGetFloatNormExp (infile);

    if (input_mode & MODE_MD5)
        loc_config.flags |= CONFIG_MD5_CHECKSUM;

    if (!WavpackSetConfiguration (outfile, &loc_config, total_samples)) {
        error_line ("%s", WavpackGetErrorMessage (outfile));
        WavpackCloseFile (infile);
        DoCloseHandle (wv_file.file);
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
        WavpackCloseFile (outfile);
        return SOFT_ERROR;
    }

    // if we are creating a "correction" file, open it now for writing

    if (out2filename) {
        if ((wvc_file.file = fopen (use_tempfiles ? out2filename_temp : out2filename, "w+b")) == NULL) {
            error_line ("can't create correction file!");
            WavpackCloseFile (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (outfile);
            return SOFT_ERROR;
        }
    }

    // pack the audio portion of the file now; calculate md5 if we're writing it to the file or verify mode is active

    result = repack_audio (outfile, infile, md5_verify);

    // before anything else, make sure the source file was read without errors

    if (result == NO_ERROR) {
        if (WavpackGetNumErrors (infile)) {
            error_line ("missing data or crc errors detected in %d block(s)!", WavpackGetNumErrors (infile));
            result = SOFT_ERROR;
        }

        if (WavpackGetNumSamples (outfile) != total_samples) {
            error_line ("incorrect number of samples read from source file!");
            result = SOFT_ERROR;
        }

        if (input_mode & MODE_LOSSLESS) {
            unsigned char md5_source [16];

            if (WavpackGetMD5Sum (infile, md5_source) && memcmp (md5_source, md5_verify, sizeof (md5_source))) {
                error_line ("MD5 signature in source should match, but does not!");
                result = SOFT_ERROR;
            }
        }
    }

    // copy the md5 sum if present in source; if there's not one there and the user asked to add it,
    // store the one we just calculated

    if (result == NO_ERROR) {
        if (WavpackGetMD5Sum (infile, md5_display)) {
            if (input_mode & MODE_LOSSLESS)
                memcpy (md5_verify, md5_display, sizeof (md5_verify));
                
            WavpackStoreMD5Sum (outfile, md5_display);
        }
        else if (loc_config.flags & CONFIG_MD5_CHECKSUM) {
            memcpy (md5_display, md5_verify, sizeof (md5_display));
            WavpackStoreMD5Sum (outfile, md5_verify);
        }
    }

    // unless we've been specifically told not to, copy RIFF trailer

    if (result == NO_ERROR && !new_riff_header && WavpackGetWrapperBytes (infile)) {
        if (!WavpackAddWrapper (outfile, WavpackGetWrapperData (infile), WavpackGetWrapperBytes (infile))) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            WavpackCloseFile (infile);
            DoCloseHandle (wv_file.file);
            DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);
            WavpackCloseFile (outfile);
            return SOFT_ERROR;
        }

        WavpackFreeWrapper (infile);
    }

    // we're now done with any WavPack blocks, so flush any remaining data

    if (result == NO_ERROR && !WavpackFlushSamples (outfile)) {
        error_line ("%s", WavpackGetErrorMessage (outfile));
        result = HARD_ERROR;
    }

    // if still no errors, check to see if we need to create & write a tag
    // (which is NOT stored in regular WavPack blocks)

    if (result == NO_ERROR && ((input_mode & MODE_VALID_TAG) || num_tag_items)) {
        int num_binary_items = WavpackGetNumBinaryTagItems (infile);
        int num_items = WavpackGetNumTagItems (infile), i;
        int item_len, value_len;
        char *item, *value;
        int res = TRUE;

        for (i = 0; i < num_items && res; ++i) {
            item_len = WavpackGetTagItemIndexed (infile, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetTagItemIndexed (infile, i, item, item_len + 1);
            value_len = WavpackGetTagItem (infile, item, NULL, 0);
            value = malloc (value_len * 2 + 1);
            WavpackGetTagItem (infile, item, value, value_len + 1);
            res = WavpackAppendTagItem (outfile, item, value, value_len);
            free (value);
            free (item);
        }

        for (i = 0; i < num_binary_items && res; ++i) {
            item_len = WavpackGetBinaryTagItemIndexed (infile, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetBinaryTagItemIndexed (infile, i, item, item_len + 1);
            value_len = WavpackGetBinaryTagItem (infile, item, NULL, 0);
            value = malloc (value_len);
            value_len = WavpackGetBinaryTagItem (infile, item, value, value_len);
            res = WavpackAppendBinaryTagItem (outfile, item, value, value_len);
            free (value);
            free (item);
        }

        for (i = 0; i < num_tag_items && res; ++i)
            if (tag_items [i].vsize) {
                if (tag_items [i].binary) 
                    res = WavpackAppendBinaryTagItem (outfile, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
                else
                    res = WavpackAppendTagItem (outfile, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
            }
            else
                WavpackDeleteTagItem (outfile, tag_items [i].item);

        if (!res || !WavpackWriteTag (outfile)) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            result = HARD_ERROR;
        }
    }

    WavpackCloseFile (infile);     // we're now done with input file, so close

    // at this point we're completely done with the files, so close 'em whether there
    // were any other errors or not

    if (!DoCloseHandle (wv_file.file)) {
        error_line ("can't close WavPack file!");

        if (result == NO_ERROR)
            result = SOFT_ERROR;
    }

    if (out2filename && !DoCloseHandle (wvc_file.file)) {
        error_line ("can't close correction file!");

        if (result == NO_ERROR)
            result = SOFT_ERROR;
    }

    // if there have been no errors up to now, and verify mode is enabled, do that now; only pass in the md5 if this
    // was a lossless operation (either explicitly or because a high lossy bitrate resulted in lossless)

    if (result == NO_ERROR && verify_mode)
        result = verify_audio (use_tempfiles ? outfilename_temp : outfilename, !WavpackLossyBlocks (outfile) ? md5_verify : NULL);

    // if there were any errors, delete the output files, close the context, and return the error

    if (result != NO_ERROR) {
        DoDeleteFile (use_tempfiles ? outfilename_temp : outfilename);

        if (out2filename)
            DoDeleteFile (use_tempfiles ? out2filename_temp : out2filename);

        WavpackCloseFile (outfile);
        return result;
    }

    if (result == NO_ERROR && copy_time)
        if (!copy_timestamp (infilename, use_tempfiles ? outfilename_temp : outfilename) ||
            (out2filename && !copy_timestamp (infilename, use_tempfiles ? out2filename_temp : out2filename)))
                error_line ("failure copying time stamp!");

    // delete source file(s) if that option is enabled (this is done before temp file rename to make sure
    // we don't delete the file(s) we just created)

    if (result == NO_ERROR && delete_source) {
        int res;

        if (stricmp (infilename, outfilename)) {
            res = DoDeleteFile (infilename);

            if (!quiet_mode || !res)
                error_line ("%s source file %s", res ?
                    "deleted" : "can't delete", infilename);
        }
    
        if (input_mode & MODE_WVC) {
            char in2filename [PATH_MAX];

            strcpy (in2filename, infilename);
            strcat (in2filename, "c");

            if (!out2filename || stricmp (in2filename, out2filename)) {
                res = DoDeleteFile (in2filename);

                if (!quiet_mode || !res)
                    error_line ("%s source file %s", res ?
                        "deleted" : "can't delete", in2filename);
            }
        }
    }

    // if we were writing to a temp file because the target file already existed,
    // do the rename / overwrite now (and if that fails, return the error)

    if (use_tempfiles) {
#if defined(WIN32)
        FILE *temp;

        if (remove (outfilename) && (temp = fopen (outfilename, "rb"))) {
            error_line ("can not remove file %s, result saved in %s!", outfilename, outfilename_temp);
            result = SOFT_ERROR;
            fclose (temp);
        }
        else
#endif
        if (rename (outfilename_temp, outfilename)) {
            error_line ("can not rename temp file %s to %s!", outfilename_temp, outfilename);
            result = SOFT_ERROR;
        }

        if (out2filename) {
#if defined(WIN32)
            FILE *temp;

            if (remove (out2filename) && (temp = fopen (out2filename, "rb"))) {
                error_line ("can not remove file %s, result saved in %s!", out2filename, out2filename_temp);
                result = SOFT_ERROR;
                fclose (temp);
            }
            else
#endif
            if (rename (out2filename_temp, out2filename)) {
                error_line ("can not rename temp file %s to %s!", out2filename_temp, out2filename);
                result = SOFT_ERROR;
            }
        }

        free (outfilename_temp);
        if (out2filename) free (out2filename_temp);

        if (result != NO_ERROR) {
            WavpackCloseFile (outfile);
            return result;
        }
    }

    // compute and display the time consumed along with some other details of
    // the packing operation, and then return NO_ERROR

#if defined(WIN32)
    _ftime (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if ((loc_config.flags & CONFIG_CALC_NOISE) && WavpackGetEncodedNoise (outfile, NULL) > 0.0) {
        int full_scale_bits = WavpackGetBitsPerSample (outfile);
        double full_scale_rms = 0.5, sum, peak;

        while (full_scale_bits--)
            full_scale_rms *= 2.0;

        full_scale_rms = full_scale_rms * (full_scale_rms - 1.0) * 0.5;
        sum = WavpackGetEncodedNoise (outfile, &peak);

        error_line ("ave noise = %.2f dB, peak noise = %.2f dB",
            log10 (sum / WavpackGetNumSamples (outfile) / full_scale_rms) * 10,
            log10 (peak / full_scale_rms) * 10);
    }

    if (!quiet_mode) {
        char *file, *fext, *oper, *cmode, cratio [16] = "";

        if (config->flags & CONFIG_MD5_CHECKSUM) {
            char md5_string [] = "original md5 signature: 00000000000000000000000000000000";
            int i;

            for (i = 0; i < 16; ++i)
                sprintf (md5_string + 24 + (i * 2), "%02x", md5_display [i]);

            error_line (md5_string);
        }

        if (outfilename && *outfilename != '-') {
            file = FN_FIT (outfilename);
            fext = wvc_file.bytes_written ? " (+.wvc)" : "";
            oper = verify_mode ? "created (and verified)" : "created";
        }
        else {
            file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
            fext = "";
            oper = "packed";
        }

        if (WavpackLossyBlocks (outfile)) {
            cmode = "lossy";

            if (WavpackGetAverageBitrate (outfile, TRUE) != 0.0)
                sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (outfile, TRUE) / 1000.0));
        }
        else {
            cmode = "lossless";

            if (WavpackGetRatio (outfile) != 0.0)
                sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (outfile) * 100.0);
        }

        error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (outfile);
    return NO_ERROR;
}

// This function handles the actual audio data transcoding. It assumes that the
// input file is positioned at the beginning of the audio data and that the
// WavPack configuration has been set. If the "md5_digest_source" pointer is not
// NULL, then a MD5 sum is calculated on the audio data during the transcoding
// and stored there at the completion. Note that the md5 requires a conversion
// to the native data format (endianness and bytes per sample) that is not
// required overwise.

static unsigned char *format_samples (int bps, unsigned char *dst, int32_t *src, uint32_t samcnt);

static int repack_audio (WavpackContext *outfile, WavpackContext *infile, unsigned char *md5_digest_source)
{
    int bps = WavpackGetBytesPerSample (infile), num_channels = WavpackGetNumChannels (infile);
    uint32_t input_samples = INPUT_SAMPLES, samples_read = 0;
    unsigned char *format_buffer;
    int32_t *sample_buffer;
    double progress = -1.0;
    MD5_CTX md5_context;

    // don't use an absurd amount of memory just because we have an absurd number of channels

    while (input_samples * sizeof (int32_t) * WavpackGetNumChannels (outfile) > 2048*1024)
        input_samples >>= 1;

    if (md5_digest_source) {
        format_buffer = malloc (input_samples * bps * WavpackGetNumChannels (outfile));
        MD5Init (&md5_context);
    }

    WavpackPackInit (outfile);
    sample_buffer = malloc (input_samples * sizeof (int32_t) * WavpackGetNumChannels (outfile));

    while (1) {
        unsigned int sample_count = WavpackUnpackSamples (infile, sample_buffer, input_samples);

        if (!sample_count)
            break;

        if (md5_digest_source) {
            format_samples (bps, format_buffer, sample_buffer, sample_count * num_channels);
            MD5Update (&md5_context, format_buffer, bps * sample_count * num_channels);
        }

        if (!WavpackPackSamples (outfile, sample_buffer, sample_count)) {
            error_line ("%s", WavpackGetErrorMessage (outfile));
            free (sample_buffer);
            return HARD_ERROR;
        }

        if (check_break ()) {
#if defined(WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            free (sample_buffer);
            return SOFT_ERROR;
        }

        if (WavpackGetProgress (outfile) != -1.0 &&
            progress != floor (WavpackGetProgress (outfile) * encode_time_percent + 0.5)) {
                int nobs = progress == -1.0;

                progress = floor (WavpackGetProgress (outfile) * encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode)
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
        }
    }

    free (sample_buffer);

    if (!WavpackFlushSamples (outfile)) {
        error_line ("%s", WavpackGetErrorMessage (outfile));
        return HARD_ERROR;
    }

    if (md5_digest_source) {
        MD5Final (md5_digest_source, &md5_context);
        free (format_buffer);
    }

    return NO_ERROR;
}

static void reorder_channels (void *data, unsigned char *order, int num_chans,
    int num_samples, int bytes_per_sample)
{
    char *temp = malloc (num_chans * bytes_per_sample);
    char *src = data;

    while (num_samples--) {
        char *start = src;
        int chan;

        for (chan = 0; chan < num_chans; ++chan) {
            char *dst = temp + (order [chan] * bytes_per_sample);
            int bc = bytes_per_sample;

            while (bc--)
                *dst++ = *src++;
        }

        memcpy (start, temp, num_chans * bytes_per_sample);
    }

    free (temp);
}

// Verify the specified WavPack input file. This function uses the library
// routines provided in wputils.c to do all unpacking. If an MD5 sum is provided
// by the caller, then this function will take care of reformatting the data
// (which is returned in native-endian longs) to the standard little-endian
// for a proper MD5 verification. Otherwise a lossy verification is assumed,
// and we only verify the exact number of samples and whether the decoding
// library detected CRC errors in any WavPack blocks.

static int verify_audio (char *infilename, unsigned char *md5_digest_source)
{
    int bytes_per_sample, num_channels, wvc_mode, bps;
    uint32_t total_unpacked_samples = 0;
    unsigned char md5_digest_result [16];
    double progress = -1.0;
    int result = NO_ERROR;
    int32_t *temp_buffer;
    MD5_CTX md5_context;
    WavpackContext *wpc;
    char error [80];

    // use library to open WavPack file

    wpc = WavpackOpenFileInput (infilename, error, OPEN_WVC, 0);

    if (!wpc) {
        error_line (error);
        return SOFT_ERROR;
    }

    if (md5_digest_source)
        MD5Init (&md5_context);

    wvc_mode = WavpackGetMode (wpc) & MODE_WVC;
    num_channels = WavpackGetNumChannels (wpc);
    bps = WavpackGetBytesPerSample (wpc);
    bytes_per_sample = num_channels * bps;
    temp_buffer = malloc (4096L * num_channels * 4);

    while (result == NO_ERROR) {
        uint32_t samples_unpacked;

        samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, 4096);
        total_unpacked_samples += samples_unpacked;

        if (md5_digest_source && samples_unpacked) {
            format_samples (bps, (unsigned char *) temp_buffer, temp_buffer, samples_unpacked * num_channels);
            MD5Update (&md5_context, (unsigned char *) temp_buffer, bps * samples_unpacked * num_channels);
        }

        if (!samples_unpacked)
            break;

        if (check_break ()) {
#if defined(WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            result = SOFT_ERROR;
            break;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * (100.0 - encode_time_percent) + encode_time_percent + 0.5)) {

                progress = floor (WavpackGetProgress (wpc) * (100.0 - encode_time_percent) + encode_time_percent + 0.5);
                display_progress (progress / 100.0);

                if (!quiet_mode)
                    fprintf (stderr, "%s%3d%% done...",
                        "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
        }
    }

    free (temp_buffer);

    // If we have been provided an MD5 sum, then the assumption is that we are doing lossless compression (either explicitly
    // with lossless mode or having a high enough bitrate that the result is lossless) and we can use the MD5 sum as a pretty
    // definitive verification.

    if (result == NO_ERROR && md5_digest_source) {
        MD5Final (md5_digest_result, &md5_context);

        if (memcmp (md5_digest_result, md5_digest_source, 16)) {
            char md5_string1 [] = "00000000000000000000000000000000";
            char md5_string2 [] = "00000000000000000000000000000000";
            int i;

            for (i = 0; i < 16; ++i) {
                sprintf (md5_string1 + (i * 2), "%02x", md5_digest_source [i]);
                sprintf (md5_string2 + (i * 2), "%02x", md5_digest_result [i]);
            }

            error_line ("original md5: %s", md5_string1);
            error_line ("verified md5: %s", md5_string2);
            error_line ("MD5 signatures should match, but do not!");
            result = SOFT_ERROR;
        }
    }

    // If we have not been provided an MD5 sum, then the assumption is that we are doing lossy compression and cannot rely
    // (obviously) on that for verification. For these cases we make sure that the number of samples generated was exactly
    // correct and that the WavPack decoding library did not detect an error. There is a simple CRC on every WavPack block
    // that should catch any random corruption, although it's possible that this might miss some decoder bug that occurs
    // late in the decoding process (e.g., after the CRC).

    if (result == NO_ERROR) {
        if (WavpackGetNumSamples (wpc) != (uint32_t) -1) {
            if (total_unpacked_samples < WavpackGetNumSamples (wpc)) {
                error_line ("file is missing %u samples!",
                    WavpackGetNumSamples (wpc) - total_unpacked_samples);
                result = SOFT_ERROR;
            }
            else if (total_unpacked_samples > WavpackGetNumSamples (wpc)) {
                error_line ("file has %u extra samples!",
                    total_unpacked_samples - WavpackGetNumSamples (wpc));
                result = SOFT_ERROR;
            }
        }

        if (WavpackGetNumErrors (wpc)) {
            error_line ("missing data or crc errors detected in %d block(s)!", WavpackGetNumErrors (wpc));
            result = SOFT_ERROR;
        }
    }

    WavpackCloseFile (wpc);
    return result;
}

// Reformat samples from longs in processor's native endian mode to
// little-endian data with (possibly) less than 4 bytes / sample.

static unsigned char *format_samples (int bps, unsigned char *dst, int32_t *src, uint32_t samcnt)
{
    int32_t temp;

    switch (bps) {

        case 1:
            while (samcnt--)
                *dst++ = *src++ + 128;

            break;

        case 2:
            while (samcnt--) {
                *dst++ = (unsigned char) (temp = *src++);
                *dst++ = (unsigned char) (temp >> 8);
            }

            break;

        case 3:
            while (samcnt--) {
                *dst++ = (unsigned char) (temp = *src++);
                *dst++ = (unsigned char) (temp >> 8);
                *dst++ = (unsigned char) (temp >> 16);
            }

            break;

        case 4:
            while (samcnt--) {
                *dst++ = (unsigned char) (temp = *src++);
                *dst++ = (unsigned char) (temp >> 8);
                *dst++ = (unsigned char) (temp >> 16);
                *dst++ = (unsigned char) (temp >> 24);
            }

            break;
    }

    return dst;
}
#if defined(WIN32)

// Convert the Unicode wide-format string into a UTF-8 string using no more
// than the specified buffer length. The wide-format string must be NULL
// terminated and the resulting string will be NULL terminated. The actual
// number of characters converted (not counting terminator) is returned, which
// may be less than the number of characters in the wide string if the buffer
// length is exceeded.

static int WideCharToUTF8 (const wchar_t *Wide, unsigned char *pUTF8, int len)
{
    const wchar_t *pWide = Wide;
    int outndx = 0;

    while (*pWide) {
        if (*pWide < 0x80 && outndx + 1 < len)
            pUTF8 [outndx++] = (unsigned char) *pWide++;
        else if (*pWide < 0x800 && outndx + 2 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xc0 | ((*pWide >> 6) & 0x1f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else if (outndx + 3 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xe0 | ((*pWide >> 12) & 0xf));
            pUTF8 [outndx++] = (unsigned char) (0x80 | ((*pWide >> 6) & 0x3f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else
            break;
    }

    pUTF8 [outndx] = 0;
    return (int)(pWide - Wide);
}

// Convert a text string into its Unicode UTF-8 format equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

static void TextToUTF8 (void *string, int len)
{
    if (* (wchar_t *) string == 0xFEFF) {
        wchar_t *temp = _wcsdup (string);

        WideCharToUTF8 (temp + 1, (unsigned char *) string, len);
        free (temp);
    }
    else {
        int max_chars = (int) strlen (string);
        wchar_t *temp = (wchar_t *) malloc ((max_chars + 1) * 2);

        MultiByteToWideChar (CP_ACP, 0, string, -1, temp, max_chars + 1);
        WideCharToUTF8 (temp, (unsigned char *) string, len);
        free (temp);
    }
}

#else

static void TextToUTF8 (void *string, int len)
{
    char *temp = malloc (len);
    char *outp = temp;
    char *inp = string;
    size_t insize = 0;
    size_t outsize = len - 1;
    int err = 0;
    char *old_locale;
    iconv_t converter;

    memset(temp, 0, len);
    old_locale = setlocale (LC_CTYPE, "");

    if ((unsigned char) inp [0] == 0xFF && (unsigned char) inp [1] == 0xFE) {
        uint16_t *utf16p = (uint16_t *) (inp += 2);

        while (*utf16p++)
            insize += 2;

        converter = iconv_open ("UTF-8", "UTF-16LE");
    }
    else {
        insize = strlen (string);
        converter = iconv_open ("UTF-8", "");
    }

    if (converter != (iconv_t) -1) {
        err = iconv (converter, &inp, &insize, &outp, &outsize);
        iconv_close (converter);
    }
    else
        err = -1;

    setlocale (LC_CTYPE, old_locale);

    if (err == -1) {
        free(temp);
        return;
    }

    memmove (string, temp, len);
    free (temp);
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function displays the progress status on the title bar of the DOS   //
// window that WavPack is running in. The "file_progress" argument is for   //
// the current file only and ranges from 0 - 1; this function takes into    //
// account the total number of files to generate a batch progress number.   //
//////////////////////////////////////////////////////////////////////////////

static void display_progress (double file_progress)
{
    char title [40];

    if (!no_console_title) {
        file_progress = (file_index + file_progress) / num_files;
        sprintf (title, "%d%% (WavPack)", (int) ((file_progress * 100.0) + 0.5));
        DoSetConsoleTitle (title);
    }
}
