#pragma once

#include "FSDCommonDefs.h"
#include <string>

using namespace std;

class CFileExtension
{
public:
    CFileExtension() = default;

    CFileExtension(wstring wszExtension)
        : wszExtension(wszExtension)
        , cAccessedForRead(0)
        , cAccessedForWrite(0)
    {}

    void RegisterAccess(FSD_OPERATION_DESCRIPTION* pOperation)
    {
        if (pOperation->uMajorType == IRP_READ)
        {
            cAccessedForRead++;
        }

        if (pOperation->uMajorType == IRP_WRITE)
        {
            cAccessedForWrite++;
        }
    }

    size_t readAccess()
    {
        return cAccessedForRead;
    }

    size_t writeAccess()
    {
        return cAccessedForWrite;
    }

private:
    size_t  cAccessedForRead;
    size_t  cAccessedForWrite;

    wstring wszExtension;
};
