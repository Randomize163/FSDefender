#pragma once

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
#define FILE_EXTENSIONS_THRESHOLD 0.25
#define DELETION_THRESHOLD 0.3
#define RENAME_THRESHOLD 0.3
#define ACCESS_TYPE_THRESHOLD 0.9
#define MOVE_IN_THRESHOLD 0.01

using namespace std;

extern unordered_map<wstring, CFileInformation> gFiles;
extern bool g_fKillMode;

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

    void SetPrintFrequency(size_t cFrequency)
    {
        cPrintFrequency = cFrequency;
    }

    void RegisterAccess(FSD_OPERATION_DESCRIPTION* pOperation, CFileInformation* pInfo, LPCWSTR wszScanDir)
    {
        if (!IsFileFromSafeDir(pInfo->wszFileName.c_str(), wszScanDir))
        {
            return;
        }

        switch (pOperation->uMajorType)
        {
        case IRP_READ:
        {
            aFileNamesRead.insert(pInfo);
            aFileNames.insert(pInfo);
            break;
        }
        case IRP_WRITE:
        {
            aFileNamesWrite.insert(pInfo);
            aFileNames.insert(pInfo);
            break;
        }
        }
    }

    bool IsMalicious()
    {
        ULONG uTrigger = 0;
        double ET, ETT, FDT, FDTT, FET, FETT, DT, DTT, RT, RTT, ATT, ATTT, MIT, MITT, CET, CETT, HERT, HERTT;
        bool bET = EntropyTrigger(ET, ETT);
        bool  bFDT = FileDistanceTrigger(FDT, FDTT);
        bool bFET = FileExtensionsTrigger(FET, FETT);
        bool bDT = DeletionTrigger(DT, DTT);
        bool bRT = RenameTrigger(RT, RTT);
        bool bATT = AccessTypeTrigger(ATT, ATTT);
        bool bMIT = MoveInTrigger(MIT, MITT);
        bool bCET = ChangeExtensionTrigger(CET, CETT);
        bool bHRT = HighEntropyReplacesTrigger(HERT, HERTT);
        uTrigger = bET + bFDT + bFET + bDT + bRT + bATT + bMIT + bCET + bHRT;

        if (cPrint % cPrintFrequency == 0 || (uTrigger >= 4 && g_fKillMode))
        {
            printf("                         Process <%u>                       \n", uPid);
            printf("-----Trigger------Result----Calc.-------Threshold--------------\n");
            printf("EntropyTrigger:      %u |   %f  |   %f  \n", bET, ET, ETT);
            printf("FileDistanceTrigger: %u |   %f  |   %f  \n", bFDT, FDT, FDTT);
            printf("FileExtTrigger:      %u |   %f  |   %f  \n", bFET, FET, FETT);
            printf("DeletionTrigger:     %u |   %f  |   %f  \n", bDT, DT, DTT);
            printf("RenameTrigger:       %u |   %f  |   %f  \n", bRT, RT, RTT);
            printf("AccessTypeTrigger    %u |   %f  |   %f  \n", bATT, ATT, ATTT);
            printf("MoveInTrigger:       %u |   %f  |   %f  \n", bMIT, MIT, MITT);
            printf("ChangeExtTrigger:    %u |   %f  |   %f  \n", bCET, CET, CETT);
            printf("HighEntropyReplaces: %u |   %f  |   %f  \n", bHRT, HERT, HERTT);
            if (uTrigger >= 4)
            {
                printf("<<<<<<<<<< Process %u is malicious >>>>>>>>>\n", uPid);
            }
        }

        PrintInfo();

        if (uTrigger >= 4)
        {
            return true;
        }

        return false;
    }

    bool AccessTypeTrigger(double& res, double& threshold)
    {
        threshold = ACCESS_TYPE_THRESHOLD;
        if (cbFilesRead + cbFilesWrite == 0)
        {
            res = 0;
            return false;
        }
        res = (double)cbFilesWrite / (double)(cbFilesRead + cbFilesWrite);
        return res > threshold;
    }

    bool FileExtensionsTrigger(double& res, double& threshold)
    {
        threshold = FILE_EXTENSIONS_THRESHOLD;
  
        vector<wstring> aDiff;
        auto it = set_symmetric_difference(aReadExtensions.begin(), aReadExtensions.end(), aWriteExtensions.begin(), aWriteExtensions.end(), back_inserter(aDiff));
        size_t diff = aDiff.size();

        vector<wstring> aUnion;
        it = set_union(aReadExtensions.begin(), aReadExtensions.end(), aWriteExtensions.begin(), aWriteExtensions.end(), back_inserter(aUnion));
        size_t uni = aUnion.size();

        res = (double)diff / (double)uni;

        return  res > threshold;
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
        cFilesDeleted++;

        aOldFile.MoveOut();
    }

    void MoveFileIn()
    {
        cFilesMovedIn++;
    }

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

    void ChangeExtension(LPCWSTR wszOldFileExtension, LPCWSTR wszNewFileExtension)
    {
        UNREFERENCED_PARAMETER(wszOldFileExtension);
        aWriteExtensions.insert(wszNewFileExtension);

        cChangedExtensions++;
    }

    void PrintInfo(bool fUnconditionally = false)
    {
        if (cPrint % cPrintFrequency == 0 || fUnconditionally)
        {
            printf("\nProcess: %ls PID: %u\n", wszProcessName, uPid);
            cout << "Read: " << cbFilesRead / KB << " KBytes, Write: " << cbFilesWrite / KB << " KBytes" << endl
                << "Files Accessed: " << GetFileAccessedCount() << " Read: " << aFileNamesRead.size() << " Write: " << aFileNamesWrite.size() << endl
                << "Files Deleted: " << cFilesDeleted << endl
                << "Files Renamed: " << cFilesRenamed << endl
                << "Write entropy: " << ((dSumOfWeightedWriteEntropies > 0) ? dSumOfWeightedWriteEntropies / dSumOfWeightsForWriteEntropy : 0) << endl
                << "Read entropy: " << ((dSumOfWeightedReadEntropies > 0) ? dSumOfWeightedReadEntropies / dSumOfWeightsForReadEntropy : 0) << endl
                << "Removed From folder: " << cFilesMovedOut << endl
                << "Moved to folder: " << cFilesMovedIn << endl
                << "LZJ distance exceeded: " << cLZJDistanceExceed << endl
                << "LZJ distance calculated: " << cLZJDistanceCalculated << endl
                << "File extensions changed: " << cChangedExtensions << " Read: " << aReadExtensions.size() << " Write: " << aWriteExtensions.size() << endl
                << "High entropy replaces: " << cHighEntropyReplaces << endl
                << "Read extensions: ";
            int extCount = 0;
            int maxExtCount = 10;
            for (auto readExtension : aReadExtensions)
            {
                printf("%ls ", readExtension.c_str());
                extCount++;
                if (extCount == maxExtCount)
                {
                    cout << ". . . ";
                    break;
                }
            }
            extCount = 0;
            cout << endl;
            cout << "Write extensions : ";
            for (auto writeExtension : aWriteExtensions)
            {
                printf("%ls ", writeExtension.c_str());
                extCount++;
                if (extCount == maxExtCount)
                {
                    cout << ". . . ";
                    break;
                }
            }
            cout << endl;
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
        
        if (ExtensionChanged(wszOldFileExtension, wszNewFileExtension))
        {
            ChangeExtension(wszOldFileExtension, wszNewFileExtension);
        }

        if (newfile != gFiles.end())
        {
            // File was replaced or returned back
            ReplaceFile(newfile, oldfile);
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
            // Replaces are counted in ReplaceFile(newfile, oldfile);
            MoveFileIn();
        }
        else
        if (fOldFileFromSafeZone && !fNewFileFromSafeZone)
        {
            // Operation is done on file from safe folder, but its not in our base. Did we miss CreateFile()?
            ASSERT(oldfile != gFiles.end());
            
            MoveFileOut(oldfile->second);
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
                CFileInformation aNewFileInfo(oldfile->second);
                aNewFileInfo.UpdateFileName(pSetInformation->GetNewFileName());

                gFiles.erase(oldfile);
                gFiles.erase(newfile);

                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), aNewFileInfo });
                ASSERT(file.second == true);
            }
            else
            {
                // Absolutely unknown file replacing file from the safe zone
                gFiles.erase(newfile);
                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), CFileInformation(pSetInformation->GetNewFileName()) });
                ASSERT(file.second == true);
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
                ASSERT(file.second == true);
            }
            else
            {
                auto file = gFiles.insert({ pSetInformation->GetNewFileName(), CFileInformation(pSetInformation->GetNewFileName()) });
                ASSERT(file.second == true);
            }
        }
    }

    void ReplaceFile(unordered_map<wstring, CFileInformation>::iterator& oldfile, unordered_map<wstring, CFileInformation>::iterator& newfile)
    {
        CFileInformation& aOldFile = oldfile->second;
        CFileInformation& aNewFile = newfile->second;

        double res, threshold;
        if (EntropyExceeded(aOldFile.AverageWriteEntropy(), aNewFile.AverageReadEntropy(), res, threshold))
        {
            cHighEntropyReplaces++;
        }

        if (newfile->second.fDeleted)
        {
            cFilesDeleted--;
            newfile->second.fDeleted = false;
        }
        else
        {
            cFilesDeleted++;
        }
    }

    bool IsKilled()
    {
        return fIsKilled;
    }

private:
    static bool ExtensionChanged(LPCWSTR wszOldFileExtension, LPCWSTR wszNewFileExtension)
    {
        if (wszOldFileExtension == NULL && wszOldFileExtension == NULL)
        {
            return false;
        }

        if (wszOldFileExtension == NULL && wszNewFileExtension != NULL)
        {
            return true;
        }

        if (wszOldFileExtension != NULL && wszNewFileExtension == NULL)
        {
            return true;
        }

        return wcscmp(wszOldFileExtension, wszNewFileExtension);
    }

    static bool IsFileFromSafeDir(LPCWSTR wszFileName, LPCWSTR wsdDirName)
    {
        if (!wszFileName || !wsdDirName)
        {
            return false;
        }

        return wcsstr(wszFileName, wsdDirName) != NULL;
    }

    static bool EntropyExceeded(double dAverageWriteEntropy, double dAverageReadEntropy, double& res, double& threshold)
    {
        if (dAverageWriteEntropy > 0 && dAverageReadEntropy > 0)
        {
            res = dAverageWriteEntropy - dAverageReadEntropy;
            threshold = ENTROPY_THRESHOLD(dAverageReadEntropy);

            return dAverageWriteEntropy - dAverageReadEntropy > ENTROPY_THRESHOLD(dAverageReadEntropy);
        }

        res = dAverageWriteEntropy;
        threshold = WRITE_ENTROPY_TRIGGER;

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

    bool HighEntropyReplacesTrigger(double& res, double& threshold)
    {
        threshold = HIGH_ENTROPY_REPLACES_THRESHOLD;
        if (cFilesMovedIn == 0)
        {
            res = 0;
            return false;
        }

        ASSERT(cFilesMovedIn >= cHighEntropyReplaces);
        res = (double)cHighEntropyReplaces / (double)cFilesMovedIn;
        return res > threshold;
    }

    bool EntropyTrigger(double& res, double& threshold)
    {
        return EntropyExceeded(AverageWriteEntropy(), AverageReadEntropy(), res, threshold);
    }

    bool FileDistanceTrigger(double& res, double& threshold)
    {
        threshold = FILE_DISTANCE_RATIO_THRESHOLD;
        ASSERT(cLZJDistanceCalculated >= cLZJDistanceExceed);
        if (cLZJDistanceCalculated == 0)
        {
            res = 0;
            return false;
        }
        res = (double)cLZJDistanceExceed / (double)cLZJDistanceCalculated;
        threshold = FILE_DISTANCE_RATIO_THRESHOLD;
        if (res > threshold)
        {
            return true;
        }
        else if (cLZJDistanceExceed > FILE_DISTANCE_COUNT_THRESHOLD)
        {
            res = (double)cLZJDistanceExceed;
            threshold = FILE_DISTANCE_COUNT_THRESHOLD;
            return true;
        }
        return false;
    }

    bool RenameTrigger(double& res, double& threshold)
    {
        threshold = RENAME_THRESHOLD;
        if (GetFileAccessedCount() == 0)
        {
            res = 0;
            return false;
        }
        res = (double)cFilesRenamed / (double)GetFileAccessedCount();
        return res > threshold;
    }

    bool DeletionTrigger(double& res, double& threshold)
    {
        threshold = DELETION_THRESHOLD;
        if (GetFileAccessedCount() == 0)
        {
            res = 0;
            return false;
        }
        res = (double)(cFilesDeleted + cFilesMovedOut) / (double)GetFileAccessedCount();
        return res > threshold;
    }

    size_t GetFileAccessedCount()
    {
        return aFileNames.size();
    }

    bool MoveInTrigger(double& res, double& threshold)
    {
        threshold = MOVE_IN_THRESHOLD;
        if (cFilesDeleted + cFilesMovedOut == 0 || cFilesMovedIn == 0)
        {
            res = 0;
            return false;
        }

        if (cFilesMovedIn > cFilesDeleted + cFilesMovedOut)
        {
            res = (double)cFilesMovedIn / cFilesDeleted + cFilesMovedOut;
            return res < threshold;
        }
        res = (double)(cFilesDeleted + cFilesMovedOut) / cFilesMovedIn;
        return res < threshold;
    }

    bool ChangeExtensionTrigger(double& res, double& threshold)
    {
        threshold = CHANGE_EXTENSION_THRESHOLD;
        size_t cAccessedFiles = GetFileAccessedCount();
        if (cAccessedFiles == 0)
        {
            res = 0;
            return false;
        }

        ASSERT(cAccessedFiles >= cChangedExtensions);
        res = (double)cChangedExtensions / (double)cAccessedFiles;
        return res > threshold;
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
    set<CFileInformation*> aFileNames;

    set<wstring> aReadExtensions;
    set<wstring> aWriteExtensions;
};
