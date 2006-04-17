#include "All.h"
#include "CharacterHelper.h"

str_ansi * GetANSIFromUTF8(const str_utf8 * pUTF8)
{
    str_utf16 * pUTF16 = GetUTF16FromUTF8(pUTF8);
    str_ansi * pANSI = GetANSIFromUTF16(pUTF16);
    delete [] pUTF16;
    return pANSI;
}

str_ansi * GetANSIFromUTF16(const str_utf16 * pUTF16)
{
    /* 
     * In UTF-8, there will be at most 4 bytes for one character,
     * so the memory that result string need is at most 4 * len + 1 bytes.
     * That is supposed to be enough.
     */
    const int nCharacters = pUTF16 ? wcslen(pUTF16) : 0;
    int nANSICharacters = (4 * nCharacters);
    str_ansi * pANSI = new str_ansi [nANSICharacters + 1];
    memset(pANSI, 0, (nANSICharacters + 1) * sizeof(str_ansi));
    if (pUTF16)
    #ifdef _WIN32
            WideCharToMultiByte(CP_ACP, 0, pUTF16, -1, pANSI, nANSICharacters, NULL, NULL);
    #else
    {
	setlocale(LC_CTYPE, "");
	wcstombs(pANSI, pUTF16, nANSICharacters);
    }
    #endif

    return (str_ansi *) pANSI;
}

str_utf16 * GetUTF16FromANSI(const str_ansi * pANSI)
{
    const int nCharacters = pANSI ? strlen(pANSI) : 0;
    str_utf16 * pUTF16 = new str_utf16 [nCharacters + 1];
    memset(pUTF16, 0, sizeof(str_utf16) * (nCharacters + 1));
    if (pANSI)
    #ifdef _WIN32
            MultiByteToWideChar(CP_ACP, 0, pANSI, -1, pUTF16, nCharacters);
    #else
    {
	setlocale(LC_CTYPE, "");
	mbstowcs(pUTF16, pANSI, nCharacters);
    }
    #endif

    return pUTF16;
}

str_utf16 * GetUTF16FromUTF8(const str_utf8 * pUTF8)
{
    // get the length
    int nCharacters = 0; int nIndex = 0;
    while (pUTF8[nIndex] != 0)
    {
        if ((pUTF8[nIndex] & 0x80) == 0)
            nIndex += 1;
        else if ((pUTF8[nIndex] & 0xE0) == 0xE0)
            nIndex += 3;
        else
            nIndex += 2;

        nCharacters += 1;
    }

    // make a UTF-16 string
    str_utf16 * pUTF16 = new str_utf16 [nCharacters + 1];
    nIndex = 0; nCharacters = 0;
    while (pUTF8[nIndex] != 0)
    {
        if ((pUTF8[nIndex] & 0x80) == 0)
        {
            pUTF16[nCharacters] = pUTF8[nIndex];
            nIndex += 1;
        }
        else if ((pUTF8[nIndex] & 0xE0) == 0xE0)
        {
            pUTF16[nCharacters] = ((pUTF8[nIndex] & 0x1F) << 12) | ((pUTF8[nIndex + 1] & 0x3F) << 6) | (pUTF8[nIndex + 2] & 0x3F);
            nIndex += 3;
        }
        else
        {
            pUTF16[nCharacters] = ((pUTF8[nIndex] & 0x3F) << 6) | (pUTF8[nIndex + 1] & 0x3F);
            nIndex += 2;
        }

        nCharacters += 1;
    }
    pUTF16[nCharacters] = 0;

    return pUTF16; 
}

str_utf8 * GetUTF8FromANSI(const str_ansi * pANSI)
{
    str_utf16 * pUTF16 = GetUTF16FromANSI(pANSI);
    str_utf8 * pUTF8 = GetUTF8FromUTF16(pUTF16);
    delete [] pUTF16;
    return pUTF8;
}

str_utf8 * GetUTF8FromUTF16(const str_utf16 * pUTF16)
{
    // get the size(s)
    int nCharacters = wcslen(pUTF16);
    int nUTF8Bytes = 0;
    for (int z = 0; z < nCharacters; z++)
    {
        if (pUTF16[z] < 0x0080)
            nUTF8Bytes += 1;
        else if (pUTF16[z] < 0x0800)
            nUTF8Bytes += 2;
        else
            nUTF8Bytes += 3;
    }

    // allocate a UTF-8 string
    str_utf8 * pUTF8 = new str_utf8 [nUTF8Bytes + 1];

    // create the UTF-8 string
    int nUTF8Index = 0;
    for (int z = 0; z < nCharacters; z++)
    {
        if (pUTF16[z] < 0x0080)
        {
            pUTF8[nUTF8Index++] = (str_utf8) pUTF16[z];
        }
        else if (pUTF16[z] < 0x0800)
        {
            pUTF8[nUTF8Index++] = 0xC0 | (pUTF16[z] >> 6);
            pUTF8[nUTF8Index++] = 0x80 | (pUTF16[z] & 0x3F);
        }
        else
        {
            pUTF8[nUTF8Index++] = 0xE0 | (pUTF16[z] >> 12);
            pUTF8[nUTF8Index++] = 0x80 | ((pUTF16[z] >> 6) & 0x3F);
            pUTF8[nUTF8Index++] = 0x80 | (pUTF16[z] & 0x3F);
        }
    }
    pUTF8[nUTF8Index++] = 0;

    // return the UTF-8 string
    return pUTF8;
}
