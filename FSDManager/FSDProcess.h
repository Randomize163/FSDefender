#pragma once

#include "FSDFileExtension.h"
#include "FSDFileInformation.h"
#include "FSDThreadUtils.h"
#include "FSDUmFileUtils.h"
#include <iostream>
#include <unordered_map>
#include <set>
#include <string>
#include <iterator>
#include <algorithm>
#include "FSDCommonInclude.h"
#include "FSDCommonDefs.h"

#define FILE_DISTANCE_RATIO_THRESHOLD 0.10
#define FILE_DISTANCE_COUNT_THRESHOLD 15
#define CHANGE_EXTENSION_THRESHOLD 0.25
#define FILE_EXTENSIONS_THRESHOLD 5
#define DELETION_THRESHOLD 0.3
#define RENAME_THRESHOLD 0.3
#define ACCESS_TYPE_THRESHOLD 0.9
#define MOVE_IN_THRESHOLD 0.01

using namespace std;

extern unordered_map<wstring, CFileInformation> gFiles;

class CProcess
{
public:
    CProcess(ULONG uPid)
        : uPid(uPid)
        , dSumOfWeightedReadEntropies(0)
        , dSumOfWeightsForReadEntropy(0)
        , dSumOfWeightedWriteEntropies(0)
        , dSumOfWeightsForWriteEntropy(0)
        , cFilesDeleted(0)
        , cFilesRenamed(0)
        , cbFilesRead(0)
        , cbFilesWrite(0)
        , cLZJDistanceExceed(0)
        , cLZJDistanceCalculated(0)
        , cFilesMovedOut(0)
        , cFilesMovedIn(0)
        , cHighEntropyReplaces(0)
        , cPrint(0)
        , cMaliciousPrint(0)
        , cChangedExtensions(0)
        , cPrintFrequency(1000)
        , fIsKilled(false)
    {
        GetProcessNameByPid(uPid, wszProcessName, MAX_FILE_NAME_LENGTH);
    }

    void AddFileExstension(FSD_OPERATION_DESCRIPTION* pOperation)
    {
        LPCWSTR wszFileExtension = GetFileExtensionFromFileName(pOperation->GetFileName());
        if (!wszFileExtension)
        {
            wszFileExtension = L"no file extension";
        }

        switch (pOperation->uMajorType)
        {
        case IRP_READ:
        {
            aReadExtensions.insert({ wszFileExtension});
        }

        case IRP_WRITE:
        {
            aWriteExtensions.insert({ wszFileExtension});
        }

        default:
        {
            break;
        }
        }
    }

    void RegisterAccess(FSD_OPERATION_DESCRIPTION* pOperation, CFileInformation* pInfo)
    {
        switch (pOperation->uMajorType)
        {
        case IRP_READ:
        {
            aFileNamesRead.insert(pInfo);
            break;
        }
        case IRP_WRITE:
        {
            aFileNamesWrite.insert(pInfo);
            break;
        }
        }
    }

    bool IsMalicious()
    {
        PrintInfo();

        ULONG uTrigger = 0;

        uTrigger += EntropyTrigger();
        uTrigger += FileDistanceTrigger();
        uTrigger += FileExtensionsTrigger();
        uTrigger += DeletionTrigger();
        uTrigger += RenameTrigger();
        uTrigger += AccessTypeTrigger();
        uTrigger += MoveInTrigger();
        uTrigger += ChangeExtensionTrigger();
        uTrigger += HighEntropyReplacesTrigger();

        if (uTrigger >= 5)
        {
            if (cMaliciousPrint % 1000 == 0)
            {
                printf("Process %u is malicious:\n", uPid);
                printf("EntropyTrigger:      %u\n", EntropyTrigger() ? 1 : 0);
                printf("FileDistanceTrigger: %u\n", FileDistanceTrigger() ? 1 : 0);
                printf("FileExtTrigger:      %u\n", FileExtensionsTrigger() ? 1 : 0);
                printf("DeletionTrigger:     %u\n", DeletionTrigger() ? 1 : 0);
                printf("RenameTrigger:       %u\n", RenameTrigger() ? 1 : 0);
                printf("AccessTypeTrigger    %u\n", AccessTypeTrigger() ? 1 : 0);
                printf("MoveInTrigger:       %u\n", MoveInTrigger() ? 1 : 0);
                printf("ChangeExtTrigger:    %u\n", ChangeExtensionTrigger() ? 1 : 0);
                printf("HighEntropyReplaces: %u\n", HighEntropyReplacesTrigger() ? 1 : 0);

                cMaliciousPrint++;
            }

            return true;
        }

        return false;
    }

    bool AccessTypeTrigger()
    {
        if (cbFilesRead + cbFilesWrite == 0)
        {
            return false;
        }

        return (double)cbFilesWrite / (double)(cbFilesRead + cbFilesWrite) > ACCESS_TYPE_THRESHOLD;
    }

    bool FileExtensionsTrigger()
    {
        if (aReadExtensions.size() < aWriteExtensions.size())
        {
            return false;
        }

        vector<wstring> aDiff;
        auto it = set_difference(aReadExtensions.begin(), aReadExtensions.end(), aWriteExtensions.begin(), aWriteExtensions.end(), aDiff.begin());
        
        return (it - aDiff.begin()) > FILE_EXTENSIONS_THRESHOLD;
    }

    void Kill()
    {
        ASSERT(fIsKilled == false);
        fIsKilled = true;
    }

    void DeleteFile()
    {
        cFilesDeleted++;
    }

    void LZJDistanceCalculated(ULONG uSimilarity)
    {
        cLZJDistanceCalculated++;
        if (uSimilarity < LZJDISTANCE_THRESHOLD)
        {
            cLZJDistanceExceed++;
        }
    }

    void RenameFile()
    {
        cFilesRenamed++;
    }

    void MoveFileOut(CFileInformation& aOldFile)
    {
        cFilesMovedOut++;

        aOldFile.MoveOut();
    }

    void MoveFileIn(CFileInformation& aOldFile, CFileInformation& aNewFile)
    {
        UNREFERENCED_PARAMETER(aOldFile);
        UNREFERENCED_PARAMETER(aNewFile);
       
        cFilesMovedIn++;

        if (EntropyExceeded(aOldFile.AverageWriteEntropy(), aNewFile.AverageReadEntropy()))
        {
            cHighEntropyReplaces++;
        }
    }

    void ReplaceFiles(CFileInformation& aOldFile, CFileInformation& aNewFile);

    ULONG GetPid()
    {
        return uPid;
    }

    void UpdateWriteEntropy(double dWriteEntropy, size_t cbWrite)
    {
        double w = 0.125 * round(dWriteEntropy) * cbWrite;
        dSumOfWeightedWriteEntropies += w * dWriteEntropy;
        dSumOfWeightsForWriteEntropy += w;

        cbFilesWrite += cbWrite;
    }

    void UpdateReadEntropy(double dReadEntropy, size_t cbRead)
    {
        double w = 0.125 * round(dReadEntropy) * cbRead;
        dSumOfWeightedReadEntropies += w * dReadEntropy;
        dSumOfWeightsForReadEntropy += w;

        cbFilesRead += cbRead;
    }

    void ChangeExtension()
    {
        cChangedExtensions++;
    }

    void PrintInfo(bool fUnconditionally = false)
    {
        if (cPrint % cPrintFrequency == 0 || fUnconditionally)
        {
            printf("\nProcess: %ls PID: %u\n", wszProcessName, uPid);
            cout << "Read: " << cbFilesRead << " Bytes, Write: " << cbFilesWrite << " Bytes" << endl
                << "Files Deleted: " << cFilesDeleted << endl
                << "Files Renamed: " << cFilesRenamed << endl
                << "Write entropy: " << ((dSumOfWeightedWriteEntropies > 0) ? dSumOfWeightedWriteEntropies / dSumOfWeightsForWriteEntropy : 0) << endl
                << "Read entropy: " << ((dSumOfWeightedReadEntropies > 0) ? dSumOfWeightedReadEntropies / dSumOfWeightsForReadEntropy : 0) << endl
                << "Removed From folder: " << cFilesMovedOut << endl
                << "Moved to folder: " << cFilesMovedIn << endl
                << "LZJ distance exceeded: " << cLZJDistanceExceed << endl
                << "File extensions changed: " << cChangedExtensions << " Read: " << aReadExtensions.size() << " Write: " << aWriteExtensions.size() << endl;
            printf("----------------------------------------------------------------------\n");
        }

        cPrint++;

        /*if (cPrint >= cPrintFrequency)
        {
            cPrintFrequency = cPrint * 2;
        }*/
    }

    void SetFileInfo(FSD_OPERATION_DESCRIPTION* pOperation, LPCWSTR wszScanDir)
    {
        FSD_OPERATION_SET_INFORMATION* pSetInformation = pOperation->SetInformationDescription();
        bool fOldFileFromSafeZone = IsFileFromSafeDir(pSetInformation->GetInitialFileName(), wszScanDir);
        bool fNewFileFromSafeZone = IsFileFromSafeDir(pSetInformation->GetNewFileName(), wszScanDir);

        auto newfile = gFiles.find(pSetInformation->GetNewFileName());
        auto oldfile = gFiles.find(pSetInformation->GetInitialFileName());

        ASSERT((newfile == gFiles.end() && oldfile == gFiles.end()) || newfile != oldfile);

        //
        // Global analysis
        //
        LPCWSTR wszOldFileExtension = GetFileExtensionFromFileName(pSetInformation->GetInitialFileName());
        LPCWSTR wszNewFileExtension = GetFileExtensionFromFileName(pSetInformation->GetNewFileName());
        if (wszOldFileExtension || wszNewFileExtension)
        {
            ChangeExtension();
        }

        //
        // Special analysis
        //
        if (fOldFileFromSafeZone && fNewFileFromSafeZone)
        {
            RenameFile();
        }
        else
            if (!fOldFileFromSafeZone && fNewFileFromSafeZone)
            {
                if (newfile != gFiles.end() && oldfile != gFiles.end())
                {
                    MoveFileIn(oldfile->second, newfile->second);
                }
            }
            else
                if (fOldFileFromSafeZone && !fNewFileFromSafeZone)
                {
                    if (oldfile != gFiles.end())
                    {
                        MoveFileOut(oldfile->second);
                    }
                }
                else
                    if (!fOldFileFromSafeZone && !fNewFileFromSafeZone)
                    {
                        // TODO: support this case
                        ASSERT(false);
                    }
                    else
                    {
                        ASSERT(false);
                    }

        //
        // UpdateFilenamesGlobals
        //
        if (newfile != gFiles.end())
        {
            // New file is replaced or is returned back
            if (oldfile != gFiles.end())
            {

                CFileInformation newFileInfo(oldfile->second);
                newFileInfo.UpdateFileName(pSetInformation->GetNewFileName());

                gFiles.erase(oldfile);

                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), newFileInfo });
                file.first->second.RegisterAccess(pOperation, this);
            }
            else
            {
                // Absolutely unknown file replacing file from the safe zone
                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), CFileInformation(pSetInformation->GetNewFileName()) });
                file.first->second.RegisterAccess(pOperation, this);
            }
        }
        else
        {
            // File is new in safe folder
            if (oldfile != gFiles.end())
            {
                CFileInformation newFileInfo(oldfile->second);
                newFileInfo.UpdateFileName(pSetInformation->GetNewFileName());

                gFiles.erase(oldfile);

                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), newFileInfo });
                file.first->second.RegisterAccess(pOperation, this);
            }
            else
            {
                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), CFileInformation(pSetInformation->GetNewFileName()) });
                file.first->second.RegisterAccess(pOperation, this);
            }
        }
    }

    bool IsKilled()
    {
        return fIsKilled;
    }

private:
    static bool IsFileFromSafeDir(wstring wszFileName, wstring wsdDirName)
    {
        return wcsstr(wszFileName.c_str(), wsdDirName.c_str()) != NULL;
    }

    static bool EntropyExceeded(double dAverageWriteEntropy, double dAverageReadEntropy)
    {
        if (dAverageWriteEntropy > 0 && dAverageReadEntropy > 0)
        {
            return dAverageWriteEntropy - dAverageReadEntropy > ENTROPY_THRESHOLD(dAverageReadEntropy);
        }
        
        return dAverageWriteEntropy > WRITE_ENTROPY_TRIGGER;
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

    bool HighEntropyReplacesTrigger()
    {
        if (cFilesMovedIn == 0)
        {
            return false;
        }

        ASSERT(cFilesMovedIn > cHighEntropyReplaces);

        return (double)cHighEntropyReplaces / (double)cFilesMovedIn > HIGH_ENTROPY_REPLACES_THRESHOLD;
    }

    bool EntropyTrigger()
    {
        return EntropyExceeded(AverageWriteEntropy(), AverageReadEntropy());
    }

    bool FileDistanceTrigger()
    {
        ASSERT(cLZJDistanceCalculated >= cLZJDistanceExceed);
        if (cLZJDistanceCalculated == 0)
        {
            return false;
        }

        return ((double)cLZJDistanceExceed / (double)cLZJDistanceCalculated > FILE_DISTANCE_RATIO_THRESHOLD) 
            || (cLZJDistanceExceed > FILE_DISTANCE_COUNT_THRESHOLD);
    }

    bool RenameTrigger()
    {
        if (GetFileAccessedCount() == 0)
        {
            return false;
        }

        return (double)cFilesRenamed / (double)GetFileAccessedCount() > RENAME_THRESHOLD;
    }

    bool DeletionTrigger()
    {
        if (GetFileAccessedCount() == 0)
        {
            return false;
        }

        return (double)(cFilesDeleted + cFilesMovedOut) / (double)GetFileAccessedCount() > DELETION_THRESHOLD;
    }

    size_t GetFileAccessedCount()
    {
        return (aFileNamesRead.size() + aFileNamesWrite.size())/2;
    }

    bool MoveInTrigger()
    {
        if (cFilesDeleted + cFilesMovedOut == 0 || cFilesMovedIn == 0)
        {
            return false;
        }

        if (cFilesMovedIn > cFilesDeleted + cFilesMovedOut)
        {
            return (double)cFilesMovedIn / cFilesDeleted + cFilesMovedOut < MOVE_IN_THRESHOLD;
        }

        return (double)(cFilesDeleted + cFilesMovedOut) / cFilesMovedIn < MOVE_IN_THRESHOLD;
    }

    bool ChangeExtensionTrigger()
    {
        size_t cAccessedFiles = GetFileAccessedCount();
        if (cAccessedFiles == 0)
        {
            return false;
        }

        ASSERT(cAccessedFiles - cChangedExtensions > 0);

        return (double)cChangedExtensions / (double)cAccessedFiles > CHANGE_EXTENSION_THRESHOLD;
    }

    ULONG uPid;

    WCHAR wszProcessName[MAX_FILE_NAME_LENGTH];

    size_t cPrint;
    size_t cPrintFrequency;

    size_t cMaliciousPrint;

    double dSumOfWeightedWriteEntropies;
    double dSumOfWeightedReadEntropies;

    double dSumOfWeightsForWriteEntropy;
    double dSumOfWeightsForReadEntropy;

    size_t cFilesDeleted;                   // Counts files that were deleted from safe zone
    size_t cFilesRenamed;                   // Counts files that were renamed/moved inside safe zone
    size_t cFilesMovedOut;                  // Counts files that were moved out of safe zone
    size_t cFilesMovedIn;                   // Counts files that were moved to safe zone from outside
    size_t cChangedExtensions;              // Counts files that changed their Extension after rename operation
    size_t cLZJDistanceExceed;              // Counts similarity violation
    size_t cLZJDistanceCalculated;
    size_t cHighEntropyReplaces;            // Counts files that were replaced with high entropy files

    size_t cbFilesRead;
    size_t cbFilesWrite;

    

    bool   fIsKilled;

    set<CFileInformation*> aFileNamesRead;
    set<CFileInformation*> aFileNamesWrite;

    set<wstring> aReadExtensions;
    set<wstring> aWriteExtensions;
};
