#pragma once

#include "FSDCommonInclude.h"
//
//  Name of port used to communicate
//
const LPCWSTR g_wszFSDPortName = L"\\FSDCommunicationPort";

#define KB 1024
#define MB (KB*KB)

#define MAX_STRING_LENGTH 256
#define MAX_FILE_NAME_LENGTH MAX_STRING_LENGTH
#define MAX_FILE_NAME_SIZE (MAX_FILE_NAME_LENGTH * sizeof(WCHAR))

#define MAX_COMMAND_LENGTH 10
#define MAX_PARAMETER_LENGTH 256
#define MAX_BUFFER_SIZE (2*MB)
#define LZJDISTANCE_THRESHOLD 40 // 0: two blobs of random data; 100: high likelihood that two files are related
#define ENTROPY_THRESHOLD 0.5
#define WRITE_ENTROPY_TRIGGER 7.9



enum MESSAGE_TYPE 
{
    MESSAGE_TYPE_SET_SCAN_DIRECTORY,
    MESSAGE_TYPE_PRINT_STRING,
    MESSAGE_TYPE_SET_MANAGER_PID,
    MESSAGE_TYPE_QUERY_NEW_OPS,
    MESSAGE_TYPE_KILL_PROCESS,
};

struct FSD_MESSAGE_FORMAT
{
    MESSAGE_TYPE aType;
    union
    {
        WCHAR wszFileName[MAX_FILE_NAME_LENGTH];
        WCHAR wszPrintString[MAX_STRING_LENGTH];
        ULONG uPid;
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
    IRP_CLEANUP             = 0x12,
};

#pragma warning(disable : 4200)  // zero sized array
struct FSD_OPERATION_GENERAL
{
    size_t cbFileName;
    BYTE   pFileName[];

    size_t PureSize() const
    {
        return sizeof(FSD_OPERATION_GENERAL) + cbFileName;
    }
};

struct FSD_OPERATION_WRITE
{
    size_t cbWrite;
    double dWriteEntropy;
    bool   fWriteEntropyCalculated;

    size_t cbFileName;
    BYTE   pFileName[];

    size_t PureSize() const
    {
        return sizeof(FSD_OPERATION_WRITE) + cbFileName;
    }
};

struct FSD_OPERATION_READ
{
    size_t cbRead;
    double dReadEntropy;
    bool   fReadEntropyCalculated;

    size_t cbFileName;
    BYTE   pFileName[];

    size_t PureSize() const
    {
        return sizeof(FSD_OPERATION_READ) + cbFileName;
    }
};

struct FSD_OPERATION_SET_INFORMATION
{
    size_t cbInitialFileName;
    size_t cbNewFileName;

    // Begins with null terminated Initaial File Name and then null terminated New File Name 
    BYTE   pFileNames[]; 

    size_t PureSize() const
    {
        return sizeof(FSD_OPERATION_WRITE) + cbInitialFileName + cbNewFileName;
    }

    LPWSTR GetInitialFileName()
    {
        return (LPWSTR)pFileNames;
    }

    LPWSTR GetNewFileName()
    {
        ASSERT(cbNewFileName != 0);
        return (LPWSTR)(pFileNames + cbInitialFileName);
    }

    void SetInitialFileName(LPCWSTR wszFileName, size_t cbFileName)
    {
        memcpy(GetInitialFileName(), wszFileName, cbFileName);
        cbInitialFileName = cbFileName;
    }

    void SetNewFileName(LPCWSTR wszFileName, size_t cbFileName)
    {
        memcpy(GetNewFileName(), wszFileName, cbFileName);
        cbNewFileName = cbFileName;
    }
};

struct FSD_OPERATION_DESCRIPTION
{
    ULONG          uPid;
    ULONG          uMajorType;
    ULONG          uMinorType;
    bool           fCheckForDelete;
    BYTE           pData[];

    FSD_OPERATION_WRITE* WriteDescription()
    {
        return (FSD_OPERATION_WRITE*)pData;
    }

    FSD_OPERATION_READ* ReadDescription()
    {
        return (FSD_OPERATION_READ*)pData;
    }

    FSD_OPERATION_SET_INFORMATION* SetInformationDescription()
    {
        return (FSD_OPERATION_SET_INFORMATION*)pData;
    }

    size_t PureSize() const
    {
        return DataPureSize() + sizeof(FSD_OPERATION_DESCRIPTION);
    }

    size_t DataPureSize() const
    {
        switch (uMajorType)
        {
            case IRP_WRITE:
            {
                return ((FSD_OPERATION_WRITE*)pData)->PureSize();
            }

            case IRP_READ:
            {
                return ((FSD_OPERATION_READ*)pData)->PureSize();
            }

            case IRP_SET_INFORMATION:
            {
                return ((FSD_OPERATION_SET_INFORMATION*)pData)->PureSize();
            }

            default:
            {
                return ((FSD_OPERATION_GENERAL*)pData)->PureSize();
            }
        }
    }

    void SetFileName(LPCWSTR wszFileName, size_t cbFileName)
    {
        memcpy(GetFileName(), wszFileName, cbFileName);
        
        switch (uMajorType)
        {
            case IRP_WRITE:
            {
                ((FSD_OPERATION_WRITE*)pData)->cbFileName = cbFileName;
            }

            case IRP_READ:
            {
                ((FSD_OPERATION_READ*)pData)->cbFileName = cbFileName;
            }

            case IRP_SET_INFORMATION:
            {
                ((FSD_OPERATION_SET_INFORMATION*)pData)->cbInitialFileName = cbFileName;
            }

            default:
            {
                ((FSD_OPERATION_GENERAL*)pData)->cbFileName = cbFileName;
            }
        }
    }

    LPWSTR GetFileName()
    {
        switch (uMajorType)
        {
            case IRP_WRITE:
            {
                return (LPWSTR)((FSD_OPERATION_WRITE*)pData)->pFileName;
            }

            case IRP_READ:
            {
                return (LPWSTR)((FSD_OPERATION_READ*)pData)->pFileName;
            }

            case IRP_SET_INFORMATION:
            {
                return ((FSD_OPERATION_SET_INFORMATION*)pData)->GetInitialFileName();
            }

            default:
                return (LPWSTR)((FSD_OPERATION_GENERAL*)pData)->pFileName;
        }
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