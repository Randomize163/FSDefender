#pragma once
#include "AutoPtr.h"
#include "FSDStringUtils.h"
#include "FSDCommonInclude.h"


/*
 CAutoFile aFile;
hr = aFile.Initialize(
    wszOpenFileName.Get(),
    cbOpenFileName,
    GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_READ,
    FILE_OPEN
);
*/

class CAutoFile
{
public:
    CAutoFile()
    {}

    ~CAutoFile()
    {}

    NTSTATUS Initialize(LPCWSTR wszFileName, size_t cbFileName, ULONG uDesiredAccess, ULONG uShareAccess, ULONG uFileDisposition)
    {
        NTSTATUS hr = S_OK;

        hr = NewCopyStringW(&m_wszFileName, wszFileName, cbFileName);
        RETURN_IF_FAILED(hr);

        UNICODE_STRING ustrFileName;
        RtlInitUnicodeString(&ustrFileName, wszFileName);

        OBJECT_ATTRIBUTES aAttributes;
        InitializeObjectAttributes(&aAttributes, &ustrFileName, 0, NULL, NULL);

        IO_STATUS_BLOCK aStatusBlock;
        hr = NtCreateFile(&m_hFile, uDesiredAccess, &aAttributes, &aStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, uShareAccess, uFileDisposition, FILE_NON_DIRECTORY_FILE, NULL, 0);
        RETURN_IF_FAILED(hr);
        
        return S_OK;
    }

    NTSTATUS Read(PVOID pvBuffer, size_t* pcbBuffer)
    {
        IO_STATUS_BLOCK aStatusBlock;
        NTSTATUS hr = NtReadFile(
            m_hFile, 
            NULL, NULL, NULL, 
            &aStatusBlock, 
            pvBuffer, 
            numeric_cast<ULONG>(*pcbBuffer), 
            NULL, NULL);
        RETURN_IF_FAILED(hr);

        *pcbBuffer = aStatusBlock.Information;

        return S_OK;
    }

    NTSTATUS ReadAll(PVOID pvBuffer, size_t* pcbBuffer)
    {
        HRESULT hr = S_OK;

        size_t cbData = 0;
        for (;;)
        {
            if (cbData == *pcbBuffer)
            {
                break;
            }

            size_t cbRead = *pcbBuffer - cbData;
            hr = Read((BYTE*)pvBuffer + cbData, &cbRead);
            RETURN_IF_FAILED(hr);

            cbData += cbRead;

            if (cbRead == 0)
            {
                break;
            }
        }

        *pcbBuffer = cbData;

        return S_OK;
    }

private:
    CAutoHandle  m_hFile;
    CAutoStringW m_wszFileName;
};
