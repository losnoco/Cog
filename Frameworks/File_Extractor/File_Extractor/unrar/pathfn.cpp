#include "rar.hpp"

wchar* GetWideName(const char *Name,const wchar *NameW,wchar *DestW,size_t DestSize)
{
    if (NameW!=NULL && *NameW!=0)
    {
        if (DestW!=NameW)
            my_wcsncpy(DestW,NameW,DestSize);
    }
    else
        if (Name!=NULL)
            CharToWide(Name,DestW,DestSize);
        else
            *DestW=0;
    
    // Ensure that we return a zero terminate string for security reasons.
    if (DestSize>0)
        DestW[DestSize-1]=0;
    
    return(DestW);
}
