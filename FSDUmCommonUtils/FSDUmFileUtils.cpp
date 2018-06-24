#include "FSDUmFileUtils.h"

LPCWSTR GetFileExtensionFromFileName(LPWSTR wszFileName)
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

HRESULT UtilCreateFileW(HANDLE* phFile, LPCWSTR wszFileName)
{
    HANDLE hFile = CreateFileW(wszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *phFile = hFile;

    return S_OK;
}

HRESULT UtilTryToOpenFileW(HANDLE* phFile, LPCWSTR wszFileName, size_t cRetries)
{
    HRESULT hr = S_OK;

    size_t cCount = 0;
    for (;;)
    {
        if (cCount == cRetries)
        {
            break;
        }

        hr = UtilCreateFileW(phFile, wszFileName);
        if (hr == S_OK)
        {
            return S_OK;
        }
        if (hr == E_FILE_NOT_FOUND || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
        {
            return E_FILE_NOT_FOUND;
        }

        Sleep(10);

        cCount++;
    }

    return hr;
}


HRESULT UtilReadFile(HANDLE hFile, LPVOID pvBuffer, DWORD* pdwRead)
{
    HRESULT hr = S_OK;

    DWORD dwRead = *pdwRead;
    DWORD dwActualRead = 0;
    bool fRead = ReadFile(hFile, pvBuffer, dwRead, &dwActualRead, NULL);
    if (!fRead)
    {
        *pdwRead = 0;
        hr = HRESULT_FROM_WIN32(GetLastError());
        RETURN_IF_FAILED(hr);
    }

    *pdwRead = dwActualRead;

    return S_OK;
}