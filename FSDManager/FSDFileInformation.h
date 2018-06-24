#pragma once

#include "FSDFileExtension.h"
//#include "FSDProcess.h"
#include "FSDCommonDefs.h"
#include "FSDCommonInclude.h"
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

#define DIGEST_SIZE (2*KB)

class CProcess;

class CFileInformation
{
public:
    CFileInformation(LPCWSTR wszFileName)
        : wszFileName(wszFileName)
        , cAccessedForRead(0)
        , cAccessedForWrite(0)
        , fDeleted(false)
        , fCheckForDelete(false)
        , fRecalculateSimilarity(true)
        , dAverageWriteEntropy(0)
        , dAverageReadEntropy(0)
    {}

    void RegisterAccess(FSD_OPERATION_DESCRIPTION* pOperation, CProcess* pProcess);

    void UpdateFileName(wstring wszNewName)
    {
        wszFileName = wszNewName;
    }

    void MoveOut()
    {
        fDeleted = true;
    }

public:
    wstring wszFileName;
    size_t cAccessedForRead;
    size_t cAccessedForWrite;

    double dAverageWriteEntropy;
    double dAverageReadEntropy;

    bool fDeleted;
    bool fCheckForDelete;
    bool fRecalculateSimilarity;

    unordered_map<ULONG, CProcess*> aProcesses;
    vector<int> LZJvalue;
};