#include "FSDFileInformation.h"
#include "FSDProcess.h"
#include <unordered_map>
#include "LZJD.h"
#include "MurmurHash3.h"

void CFileInformation::RegisterAccess(FSD_OPERATION_DESCRIPTION* pOperation, CProcess* pProcess)
{
    // add exstension to process
    pProcess->AddFileExstension(pOperation);

    // add process to hash
    aProcesses.insert({ pProcess->GetPid() , pProcess });

    switch (pOperation->uMajorType)
    {
    case IRP_READ:
    {
        cAccessedForRead++;

        FSD_OPERATION_READ* pReadOp = pOperation->ReadDescription();

        if (pReadOp->fReadEntropyCalculated)
        {
            pProcess->UpdateReadEntropy(pReadOp->dReadEntropy, pReadOp->cbRead);
        }

        break;
    }

    case IRP_WRITE:
    {
        cAccessedForWrite++;

        FSD_OPERATION_WRITE* pWriteOp = pOperation->WriteDescription();

        if (pWriteOp->fWriteEntropyCalculated)
        {
            pProcess->UpdateWriteEntropy(pWriteOp->dWriteEntropy, pWriteOp->cbWrite);
            fRecalculateSimilarity = true;
        }

        break;
    }

    case IRP_CREATE:
    {
        fCheckForDelete = pOperation->fCheckForDelete;

        // LZJvalue already computed
        if (!LZJvalue.empty())
        {
            break;
        }

        // calculate file initial LZJ
        CAutoHandle hFile;
        HRESULT hr = UtilTryToOpenFileW(&hFile, wszFileName.c_str(), 10);
        if (hr == E_FILE_NOT_FOUND)
        {
            fCheckForDelete = true;
            break;
        }
        VOID_IF_FAILED_EX(hr);

        CAutoArrayPtr<BYTE> pBuffer = new BYTE[DIGEST_SIZE];
        VOID_IF_FAILED_ALLOC(pBuffer);

        DWORD dwRead = DIGEST_SIZE;
        hr = UtilReadFile(hFile, pBuffer.Get(), &dwRead);
        VOID_IF_FAILED(hr);

        LZJvalue = digest(DIGEST_SIZE, pBuffer.Get(), dwRead);
        break;
    }
    case IRP_CLEANUP:
    case IRP_CLOSE:
    {
        if (fCheckForDelete && !fDeleted)
        {
            HRESULT hr = S_OK;

            CAutoHandle hFile;
            hr = UtilTryToOpenFileW(&hFile, wszFileName.c_str(), 10);
            if (hr == E_FILE_NOT_FOUND)
            {
                fCheckForDelete = false;
                fDeleted = true;
                pProcess->DeleteFile();

                break;
            }
            VOID_IF_FAILED_EX(hr);
        }

        if (!LZJvalue.empty() && fRecalculateSimilarity)
        {
            // calculate file final ZLJ and ZLJDistance
            CAutoHandle hFile;
            HRESULT hr = UtilTryToOpenFileW(&hFile, wszFileName.c_str(), 10);
            if (hr == E_FILE_NOT_FOUND)
            {
                fCheckForDelete = false;
                fDeleted = true;
                pProcess->DeleteFile();
                break;
            }
            VOID_IF_FAILED_EX(hr);

            CAutoArrayPtr<BYTE> pBuffer = new BYTE[DIGEST_SIZE];
            VOID_IF_FAILED_ALLOC(pBuffer);

            DWORD dwRead = DIGEST_SIZE;
            hr = UtilReadFile(hFile, pBuffer.Get(), &dwRead);
            VOID_IF_FAILED(hr);

            vector<int> LZJnewVaue = digest(DIGEST_SIZE, pBuffer.Get(), dwRead);

            ULONG uSimilarity = similarity(LZJvalue, LZJnewVaue);
            if (uSimilarity < LZJDISTANCE_THRESHOLD) // TODO: modify threshold
            {
                printf("Similarity of %ls: %u\n", wszFileName.c_str(), uSimilarity);
                pProcess->LZJDistanceExceed();
            }

            // update LZJ value
            LZJvalue = LZJnewVaue;
            fRecalculateSimilarity = false;
        }

        break;
    }

    case IRP_SET_INFORMATION:
    {
        fCheckForDelete = pOperation->fCheckForDelete;
        break;
    }

    default:
    {
        ASSERT(false);
    }
    }
}