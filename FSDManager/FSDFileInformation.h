#pragma once

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
        , cbAccessedForRead(0)
        , cbAccessedForWrite(0)
        , fDeleted(false)
        , fCheckForDelete(false)
        , fRecalculateSimilarity(true)
        , dSumOfWeightedReadEntropies(0)
        , dSumOfWeightsForReadEntropy(0)
        , dSumOfWeightedWriteEntropies(0)
        , dSumOfWeightsForWriteEntropy(0)
    {}

    void RegisterAccess(FSD_OPERATION_DESCRIPTION* pOperation, CProcess* pProcess, LPCWSTR wszScanDir);

    void UpdateFileName(wstring wszNewName)
    {
        wszFileName = wszNewName;
    }

    void UpdateWriteEntropy(double dWriteEntropy, size_t cbWrite)
    {
        double w = 0.125 * round(dWriteEntropy) * cbWrite;
        dSumOfWeightedWriteEntropies += w * dWriteEntropy;
        dSumOfWeightsForWriteEntropy += w;

        cbAccessedForWrite += cbWrite;
    }

    void UpdateReadEntropy(double dReadEntropy, size_t cbRead)
    {
        double w = 0.125 * round(dReadEntropy) * cbRead;
        dSumOfWeightedReadEntropies += w * dReadEntropy;
        dSumOfWeightsForReadEntropy += w;

        cbAccessedForRead += cbRead;
    }

    double AverageReadEntropy()
    {
        if (dSumOfWeightsForReadEntropy == 0)
        {
            return 0;
        }
        
        return dSumOfWeightedReadEntropies / dSumOfWeightsForReadEntropy;
    }

    double AverageWriteEntropy()
    {
        if (dSumOfWeightsForWriteEntropy == 0)
        {
            return 0;
        }

        return dSumOfWeightedWriteEntropies / dSumOfWeightsForWriteEntropy;
    }

    void MoveOut()
    {
        fDeleted = true;
    }

public:
    wstring wszFileName;
    size_t cbAccessedForRead;
    size_t cbAccessedForWrite;

    double dSumOfWeightedWriteEntropies;
    double dSumOfWeightedReadEntropies;

    double dSumOfWeightsForWriteEntropy;
    double dSumOfWeightsForReadEntropy;

    bool fDeleted;
    bool fCheckForDelete;
    bool fRecalculateSimilarity;

    unordered_map<ULONG, CProcess*> aProcesses;
    vector<int> LZJvalue;
};