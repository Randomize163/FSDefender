#pragma once

#include "FSDCommonInclude.h"
//
//  Name of port used to communicate
//
const LPCWSTR g_wszFSDPortName = L"\\FSDCommunicationPort";

#define MAX_STRING_LENGTH 256
#define MAX_FILE_NAME_LENGTH MAX_STRING_LENGTH
#define MAX_FILE_NAME_SIZE (MAX_FILE_NAME_LENGTH * sizeof(WCHAR))
#define MAX_FILE_EXTENTION_LENGTH 10

enum MESSAGE_TYPE 
{
    MESSAGE_TYPE_SET_SCAN_DIRECTORY,
    MESSAGE_TYPE_PRINT_STRING,
    MESSAGE_TYPE_QUERY_NEW_OPS,
};

struct FSD_MESSAGE_FORMAT
{
    MESSAGE_TYPE aType;
    union
    {
        WCHAR wszFileName[MAX_FILE_NAME_LENGTH];
        WCHAR wszPrintString[MAX_STRING_LENGTH];
    };
};

enum IRP_MAJOR_TYPE
{
    IRP_CREATE              = 0x00,
    IRP_CLOSE               = 0x02,
    IRP_READ                = 0x03,
    IRP_WRITE               = 0x04,
    IRP_QUERY_INFORMATION   = 0x05,
    IRP_SET_INFORMATION     = 0x06,
};

#pragma warning(disable : 4200)  // zero sized array
struct FSD_OPERATION_GENERAL
{
    WCHAR  wszFileExtention[MAX_FILE_EXTENTION_LENGTH];

    size_t cbFileName;
    BYTE   pFileName[];

    size_t PureSize() const
    {
        return sizeof(FSD_OPERATION_GENERAL) + cbFileName;
    }
};

struct FSD_OPERATION_WRITE
{
    ULONG  uWriteEntropy;
    WCHAR  wszFileExtention[MAX_FILE_EXTENTION_LENGTH];

    size_t cbFileName;
    BYTE   pFileName[];

    size_t PureSize() const
    {
        return sizeof(FSD_OPERATION_WRITE) + cbFileName;
    }
};

struct FSD_OPERATION_DESCRIPTION
{
    ULONG          uPid;
    ULONG          uMajorType;
    ULONG          uMinorType;
    size_t         cbData;
    BYTE           pData[];

    size_t PureSize() const
    {
        return DataPureSize() + sizeof(FSD_OPERATION_DESCRIPTION);
    }

    size_t DataPureSize() const
    {
        if (uMajorType == IRP_WRITE)
        {
            return ((FSD_OPERATION_WRITE*)pData)->PureSize();
        }

        return ((FSD_OPERATION_GENERAL*)pData)->PureSize();
    }

    void SetFileExtention(LPCWSTR wszFileExtention, size_t cbFileExtention)
    {
        memcpy(GetFileExtention(), wszFileExtention, cbFileExtention);
    }

    void SetFileName(LPCWSTR wszFileName, size_t cbFileName)
    {
        memcpy(GetFileName(), wszFileName, cbFileName);
        
        if (uMajorType == IRP_WRITE)
        {
            ((FSD_OPERATION_WRITE*)pData)->cbFileName = cbFileName;
        }

        ((FSD_OPERATION_GENERAL*)pData)->cbFileName = cbFileName;
    }

    LPWSTR GetFileExtention()
    {
        if (uMajorType == IRP_WRITE)
        {
            return ((FSD_OPERATION_WRITE*)pData)->wszFileExtention;
        }

        return ((FSD_OPERATION_GENERAL*)pData)->wszFileExtention;
    }

    LPWSTR GetFileName()
    {
        if (uMajorType == IRP_WRITE)
        {
            return (LPWSTR)((FSD_OPERATION_WRITE*)pData)->pFileName;
        }

        return (LPWSTR)((FSD_OPERATION_GENERAL*)pData)->pFileName;
    }

    FSD_OPERATION_DESCRIPTION* GetNext()
    {
        return (FSD_OPERATION_DESCRIPTION*)((BYTE*)this + PureSize());
    }
};

struct FSD_QUERY_NEW_OPS_RESPONSE_FORMAT
{
    size_t cbData;
    BYTE   pData[];

    size_t PureSize() const
    {
        return sizeof(FSD_QUERY_NEW_OPS_RESPONSE_FORMAT) + cbData;
    }

    FSD_OPERATION_DESCRIPTION* GetFirst()
    {
        return (FSD_OPERATION_DESCRIPTION*)pData;
    }
};