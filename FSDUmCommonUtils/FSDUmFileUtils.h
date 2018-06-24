#pragma once

#include "FSDCommonInclude.h"
#include "AutoPtr.h"
#include "FSDStringUtils.h"

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

class CAutoFile
{
public:
    
    CAutoFile()
    {}

    // dwShareMode FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
    // dwAccess GENERIC_READ, GENERIC_WRITE
    HRESULT Initialize(LPCWSTR wszFileName, size_t cbFileName/*, DWORD dwAccess, DWORD dwShareMode, DWORD dwCreationDisposition*/)
    {
        HRESULT hr = S_OK;

        hr = NewCopyStringW(&m_wszName, wszFileName, cbFileName);
        RETURN_IF_FAILED(hr);
                   
        m_hFile = CreateFileW(wszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }

    HRESULT Read(LPVOID pvBuffer, DWORD* pdwBuffer)
    {
        HRESULT hr = S_OK;

        HANDLE hFile = m_hFile.Get();

        DWORD dwReadSize = 0;
        bool fRead = ReadFile(hFile, pvBuffer, *pdwBuffer, &dwReadSize, NULL);
        if (!fRead)
        {
            *pdwBuffer = 0;
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (hr == E_HANDLE)
            {
                m_hFile = INVALID_HANDLE_VALUE;
                ASSERT(m_hFile == INVALID_HANDLE_VALUE);
            }
            RETURN_IF_FAILED(hr);
        }

        *pdwBuffer = dwReadSize;

        return S_OK;
    }

    HRESULT ReadAll(LPVOID pvBuffer, DWORD* pdwBuffer)
    {
        HRESULT hr = S_OK;

        ULONG cbData = 0;
        for (;;)
        {
            if (cbData == *pdwBuffer)
            {
                break;
            }

            ULONG cbRead = *pdwBuffer - cbData;
            hr = Read((BYTE*)pvBuffer + cbData, &cbRead);
            RETURN_IF_FAILED(hr);

            cbData += cbRead;

            if (cbRead == 0)
            {
                break;
            }
        }

        *pdwBuffer = cbData;

        return S_OK;
    }

private:
    CAutoStringW m_wszName;
    CAutoHandle  m_hFile;
};

LPCWSTR GetFileExtentionFromFileName(LPWSTR wszFileName);

HRESULT UtilCreateFileW(HANDLE* phFile, LPCWSTR wszFileName);

HRESULT UtilReadFile(HANDLE hFile, LPVOID pvBuffer, DWORD* pdwRead);

HRESULT UtilTryToOpenFileW(HANDLE* phFile, LPCWSTR wszFileName, size_t cRetries);