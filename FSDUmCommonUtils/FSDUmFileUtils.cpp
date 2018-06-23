#include "FSDUmFileUtils.h"

LPCWSTR GetFileExtentionFromFileName(LPWSTR wszFileName)
{
    LPWSTR pLastPoint = NULL;

    size_t cSymbol = 0;
    for (;;)
    {
        if (wszFileName[cSymbol] == L'\0')
        {
            break;
        }

        if (wszFileName[cSymbol] == L'.')
        {
            pLastPoint = wszFileName + cSymbol;
        }

        cSymbol++;
    }

    return pLastPoint;
}