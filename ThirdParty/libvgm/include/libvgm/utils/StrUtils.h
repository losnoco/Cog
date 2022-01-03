#ifndef __STRUTILS_H__
#define __STRUTILS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"
#include <stddef.h>	// for size_t

typedef struct _codepage_conversion CPCONV;

UINT8 CPConv_Init(CPCONV** retCPC, const char* cpFrom, const char* cpTo);
void CPConv_Deinit(CPCONV* cpc);
// parameters:
//	cpc: codepage conversion object
//	outSize: [input] size of output buffer, [output] size of converted string
//	outStr: [input/output] pointer to output buffer
//	        *outStr == NULL: The routine will allocate a buffer large enough to hold all the data.
//	        *outStr != NULL: place results into the existing buffer (no reallocation when buffer is too small)
//	inSize: [input] length of input string
//	        inSize == 0: automatically determine the string's length, includes the terminating \0 character.
//	inStr: [input] input string
//	Note: For nonzero return codes, the \0-terminator will be missing!
UINT8 CPConv_StrConvert(CPCONV* cpc, size_t* outSize, char** outStr, size_t inSize, const char* inStr);

#ifdef __cplusplus
}
#endif

#endif	// __STRUTILS_H__
