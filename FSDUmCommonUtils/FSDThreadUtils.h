#pragma once

#include "FSDCommonInclude.h"

HRESULT UtilCreateThreadSimple(
    HANDLE*                    phThread,
    LPTHREAD_START_ROUTINE    pvfThreadMain,
    LPVOID                    pvContext
);