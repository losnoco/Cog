#ifndef APE_APELINK_H
#define APE_APELINK_H

#include "IO.h"
#include "APEInfo.h"

class CAPELink
{
public:

    CAPELink(const char * pFilename);
    CAPELink(const char * pData, const char * pFilename);
    ~CAPELink();

    BOOL GetIsLinkFile();
    int GetStartBlock();
    int GetFinishBlock();
    const wchar_t * GetImageFilename();

protected:

    BOOL m_bIsLinkFile;
    int m_nStartBlock;
    int m_nFinishBlock;
    str_utf16 m_cImageFilename[MAX_PATH];

    void ParseData(const char * pData, const char * pFilename);
};

#endif // #ifndef APE_APELINK_H
