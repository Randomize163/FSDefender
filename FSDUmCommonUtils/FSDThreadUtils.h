#pragma once

#include "FSDCommonInclude.h"

HRESULT UtilCreateThreadSimple(
    HANDLE*                    phThread,
    LPTHREAD_START_ROUTINE    pvfThreadMain,
    LPVOID                    pvContext
);

void GetProcessNameByPid(ULONG uPid, LPWSTR wszName, size_t ceSymbols);

LPWSTR GetFileNameFromPath(LPWSTR wszPath);