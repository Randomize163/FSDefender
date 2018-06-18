#pragma once

#include "FSDCommonInclude.h"

NTSTATUS NewCopyStringW(LPWSTR* pwszDest, LPCWSTR wszSource, size_t cbSource);
NTSTATUS CopyStringW(LPWSTR wszDest, LPCWSTR wszSource, size_t cbSource);