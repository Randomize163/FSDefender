#include "FSDStringUtils.h"
//#include "FSDCommonInclude.h"
#include "AutoPtr.h"
//#include <string.h>
//#include <stdio.h>

NTSTATUS NewCopyStringW(LPWSTR* pwszDest, LPCWSTR wszSource, size_t cbSource)
{
    CAutoStringW wszDest = new WCHAR[cbSource / sizeof(WCHAR)];
    RETURN_IF_FAILED_ALLOC(wszDest);

    INT err = wcscpy_s(wszDest.LetPtr(), cbSource / sizeof(WCHAR), wszSource);
    RETURN_IF_FAILED_ERRNO(err);

    wszDest.Detach(pwszDest);

    return S_OK;
}

NTSTATUS CopyStringW(LPWSTR wszDest, LPCWSTR wszSource, size_t cbSource)
{
    INT err = wcscpy_s(wszDest, cbSource / sizeof(WCHAR), wszSource);
    RETURN_IF_FAILED_ERRNO(err);

    return S_OK;
}