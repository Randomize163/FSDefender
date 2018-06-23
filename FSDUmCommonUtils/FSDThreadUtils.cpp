#include "FSDThreadUtils.h"

#include "FSDCommonInclude.h"
#include "AutoPtr.h"
#include "Psapi.h"
#include "FSDStringUtils.h"

HRESULT UtilCreateThreadSimple(
    HANDLE*                    phThread,
    LPTHREAD_START_ROUTINE    pvfThreadMain,
    LPVOID                    pvContext
){
    *phThread = CreateThread(NULL, 0, pvfThreadMain, pvContext, 0, NULL);
    if (!*phThread)
    {
        return GetLastError();
    }

    return S_OK;
}

LPWSTR GetFileNameFromPath(LPWSTR wszPath)
{
    LPWSTR pLastBackslash = wszPath;

    size_t cSymbol = 0;
    for (;;)
    {
        if (wszPath[cSymbol] == L'\0')
        {
            break;
        }

        if (wszPath[cSymbol] == L'\\')
        {
            pLastBackslash = wszPath + cSymbol + 1;
        }

        cSymbol++;
    }

    return pLastBackslash;
}

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

void GetProcessNameByPid(ULONG uPid, LPWSTR wszName, size_t ceSymbols)
{
    CAutoHandle hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, uPid);
    if (hProcess)
    {
        DWORD dwLength = GetProcessImageFileNameW(hProcess, wszName, numeric_cast<DWORD>(ceSymbols));
        if (dwLength)
        {
            if (CopyStringW(wszName, GetFileNameFromPath(wszName), ceSymbols * sizeof(wszName[0])) == S_OK)
            {
                return;
            }
        }
    }

    wszName[0] = L'\0';

    return;
}