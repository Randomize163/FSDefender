#include "FSDStringUtils.h"
#include "AutoPtr.h"
#include <string.h>

NTSTATUS NewCopyStringW(LPWSTR* pwszDest, LPCWSTR wszSource, size_t cbSource)
{
	CAutoPtr<WCHAR> wszDest = new WCHAR[cbSource];
	RETURN_IF_FAILED_ALLOC(wszDest);

	INT err = wcscpy_s(wszDest.LetPtr(), cbSource, wszSource);
	RETURN_IF_FAILED_ERRNO(err);
	
	wszDest.Detach(pwszDest);
	
	return S_OK;
}