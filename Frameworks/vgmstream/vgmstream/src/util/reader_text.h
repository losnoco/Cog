#ifndef _READER_TEXT_H
#define _READER_TEXT_H
#include "../streamfile.h"


/* Read into dst a line delimited by CRLF (Windows) / LF (Unux) / CR (Mac) / EOF, null-terminated
 * and without line feeds. Returns bytes read (including CR/LF), *not* the same as string length.
 * p_line_ok is set to 1 if the complete line was read; pass NULL to ignore. */
size_t read_line(char* buf, int buf_size, off_t offset, STREAMFILE* sf, int* p_line_ok);

/* skip BOM if needed */
size_t read_bom(STREAMFILE* sf);

/* reads a c-string (ANSI only), up to bufsize or NULL, returning size. buf is optional (works as get_string_size). */
size_t read_string(char* buf, size_t buf_size, off_t offset, STREAMFILE* sf);
/* reads a UTF16 string... but actually only as ANSI (discards the upper byte) */
size_t read_string_utf16(char* buf, size_t buf_size, off_t offset, STREAMFILE* sf, int big_endian);
size_t read_string_utf16le(char* buf, size_t buf_size, off_t offset, STREAMFILE* sf);
size_t read_string_utf16be(char* buf, size_t buf_size, off_t offset, STREAMFILE* sf);

#endif
