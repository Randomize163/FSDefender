#include "FSDThreadUtils.h"

#include "FSDCommonInclude.h"
#include "AutoPtr.h"

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
