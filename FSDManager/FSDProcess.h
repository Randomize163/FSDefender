#pragma once

#include "FSDFileExtension.h"
#include "FSDFileInformation.h"
#include "FSDThreadUtils.h"
#include "FSDUmFileUtils.h"
#include <iostream>
#include <unordered_map>
#include <string>
#include "FSDCommonInclude.h"
#include "FSDCommonDefs.h"

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
        , cFilesMovedOut(0)
        , cFilesMovedIn(0)
        , cPrint(0)
        , cChangedExtensions(0)
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
            auto fileExtension = aReadExtensions.insert({ wszFileExtension, CFileExtension(wszFileExtension) });
            fileExtension.first->second.RegisterAccess(pOperation);
        }

        case IRP_WRITE:
        {
            auto fileExtension = aWriteExtensions.insert({ wszFileExtension, CFileExtension(wszFileExtension) });
            fileExtension.first->second.RegisterAccess(pOperation);
        }

        default:
        {
            ASSERT(false);
        }
        }
    }

    bool IsMalicious()
    {
        PrintInfo();

        ULONG uTrigger = 0;

        uTrigger += EntropyTrigger();
        uTrigger += FileDistanceTrigger();
        //uTrigger += FileExtensionsTrigger();
        uTrigger += DeletionTrigger();
        uTrigger += RenameTrigger();
        //uTrigger += AccessTypeTrigger();
        uTrigger += MoveInTrigger();
        //uTrigger += RemoveFromFolderTrigger();
        uTrigger += ChangeExtensionTrigger();

        return uTrigger >= 3;
    }

    void DeleteFile()
    {
        cFilesDeleted++;
    }

    void LZJDistanceExceed()
    {
        cLZJDistanceExceed++;
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
        // TODO: Complete function

        cFilesMovedIn++;

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
        if (cPrint % 1000 == 0 || fUnconditionally)
        {
            printf("Process: %ls PID: %u\n", wszProcessName, uPid);
            cout << "Read: " << cbFilesRead << " Bytes, Write: " << cbFilesWrite << " Bytes" << endl
                << "Files Deleted: " << cFilesDeleted << endl
                << "Files Renamed: " << cFilesRenamed << endl
                << "Write entropy: " << ((dSumOfWeightedWriteEntropies > 0) ? dSumOfWeightedWriteEntropies / dSumOfWeightsForWriteEntropy : 0) << endl
                << "Read entropy: " << ((dSumOfWeightedReadEntropies > 0) ? dSumOfWeightedReadEntropies / dSumOfWeightsForReadEntropy : 0) << endl
                << "Removed From folder: " << cFilesMovedOut << endl
                << "Moved to folder: " << cFilesMovedIn << endl
                << "LZJ distance exceeded: " << cLZJDistanceExceed << endl
                << "File extensions changed: " << cChangedExtensions << endl;
            printf("----------------------------------------------------------------------\n");
        }

        cPrint++;
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

private:
    static bool IsFileFromSafeDir(wstring wszFileName, wstring wsdDirName)
    {
        return wcsstr(wszFileName.c_str(), wsdDirName.c_str()) != NULL;
    }

    bool EntropyTrigger()
    {
        if (dSumOfWeightedWriteEntropies > 0 && dSumOfWeightedReadEntropies > 0)
        {
            return dSumOfWeightedWriteEntropies / dSumOfWeightsForWriteEntropy -
                dSumOfWeightedReadEntropies / dSumOfWeightsForReadEntropy > ENTROPY_THRESHOLD;
        }
        else if (dSumOfWeightedWriteEntropies > 0)
        {
            return dSumOfWeightedWriteEntropies / dSumOfWeightsForWriteEntropy > WRITE_ENTROPY_TRIGGER;
        }

        return false;
    }

    bool FileDistanceTrigger()
    {
        return cLZJDistanceExceed > 0;
    }

    bool RenameTrigger()
    {
        return cFilesRenamed > 0;
    }

    bool DeletionTrigger()
    {
        return cFilesDeleted + cFilesMovedOut > 0;
    }

    bool MoveInTrigger()
    {
        return cFilesMovedIn > 0;
    }

    bool ChangeExtensionTrigger()
    {
        return cChangedExtensions > 0;
    }

    ULONG uPid;

    WCHAR wszProcessName[MAX_FILE_NAME_LENGTH];

    size_t cPrint;

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

    size_t cbFilesRead;
    size_t cbFilesWrite;

    unordered_map<wstring, CFileExtension> aReadExtensions;
    unordered_map<wstring, CFileExtension> aWriteExtensions;
};
