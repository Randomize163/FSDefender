#include "CFSDefender.h"

//#include "FSDUtils.h"
#include "FSDFilterUtils.h"
#include "FSDCommonDefs.h"
#include "FSDRegistrationInfo.h"
#include "stdio.h"
#include "FSDStringUtils.h"
#include "FSDShanonEntropy.h"

extern CFSDefender* g;

CFSDefender::CFSDefender()
    : m_pFilter()
    , m_pPort()
    , m_wszScanPath()
    , m_fSniffer(false)
    , m_fClosed(false)
    , m_pItemsReadyForSend(NULL)
    , m_uManagerPid(0)
{}

CFSDefender::~CFSDefender()
{
    if (!m_fClosed)
    {
        Close();
    }
}

NTSTATUS CFSDefender::Initialize(
    PDRIVER_OBJECT          pDriverObject
){
    NTSTATUS hr = S_OK;

    //
    //  Register with FltMgr to tell it our callback routines
    //

    hr = NewInstanceOf<CFilter>(&m_pFilter, pDriverObject, &FilterRegistration);
    RETURN_IF_FAILED(hr);

    hr = NewInstanceOf<CFSDCommunicationPort>(&m_pPort, g_wszFSDPortName, m_pFilter->Handle(), this, OnConnect, OnDisconnect, OnNewMessage);
    RETURN_IF_FAILED(hr);

    //
    //  Start filtering i/o
    //

    hr = m_pFilter->StartFiltering();
    RETURN_IF_FAILED(hr);

    return S_OK;
}

void CFSDefender::Close()
{
    ASSERT(!m_fClosed);
    
    m_fSniffer = false;
    m_pPort->Close();
    m_pFilter->Close();

    for (;;)
    {
        if (!m_pItemsReadyForSend)
        {
            m_pItemsReadyForSend = m_aIrpOpsQueue.PopAll();
            if (!m_pItemsReadyForSend)
            {
                break;
            }
        }

        IrpOperationItem* pNextOperation = m_aIrpOpsQueue.Next(m_pItemsReadyForSend);
        delete m_pItemsReadyForSend;

        m_pItemsReadyForSend = pNextOperation;
    }
}

NTSTATUS CFSDefender::ConnectClient(PFLT_PORT pClientPort)
{
    TRACE(TL_INFO, "User connected. 0x%p\n", pClientPort);
    m_fSniffer = true;

    return S_OK;
}

void CFSDefender::DisconnectClient(PFLT_PORT pClientPort)
{
    TRACE(TL_INFO, "User disconnected. 0x%p\n", pClientPort);
    m_fSniffer = false;

    return;
}

void CFSDefender::FillOperationDescription(FSD_OPERATION_DESCRIPTION* pOpDescription, IrpOperationItem* pIrpOp)
{
    //
    // Set general information
    //
    pOpDescription->uMajorType = pIrpOp->m_uIrpMajorCode;
    pOpDescription->uMinorType = pIrpOp->m_uIrpMinorCode;
    pOpDescription->uPid       = pIrpOp->m_uPid;
    pOpDescription->SetFileName((LPCWSTR)pIrpOp->m_pFileName.Get(), pIrpOp->m_cbFileName);
    pOpDescription->fCheckForDelete = pIrpOp->m_fCheckForDelete;

    //
    // Set special information
    //
    switch (pOpDescription->uMajorType)
    {
        case IRP_MJ_WRITE:
        {
            FSD_OPERATION_WRITE* pWrite = pOpDescription->WriteDescription();
            pWrite->dWriteEntropy = pIrpOp->m_dDataEntropy;
            pWrite->fWriteEntropyCalculated = pIrpOp->m_fDataEntropyCalculated;
            pWrite->cbWrite = pIrpOp->m_cbData;

            break;
        }

        case IRP_MJ_READ:
        {
            FSD_OPERATION_READ* pRead = pOpDescription->ReadDescription();
            pRead->dReadEntropy = pIrpOp->m_dDataEntropy;
            pRead->fReadEntropyCalculated = pIrpOp->m_fDataEntropyCalculated;
            pRead->cbRead = pIrpOp->m_cbData;
            break;
        }

        case IRP_MJ_SET_INFORMATION:
        {
            FSD_OPERATION_SET_INFORMATION* pSetInfo = pOpDescription->SetInformationDescription();
            pSetInfo->SetNewFileName((LPCWSTR)pIrpOp->m_pFileRenameInfo.Get(), pIrpOp->m_cbFileRenameInfo);

            break;
        }
    }
}

NTSTATUS CFSDefender::HandleNewMessage(IN PVOID pvInputBuffer, IN ULONG uInputBufferLength, OUT PVOID pvOutputBuffer, IN ULONG uOutputBufferLength, OUT PULONG puReturnOutputBufferLength)
{
    UNREFERENCED_PARAMETER(uInputBufferLength);
    NTSTATUS hr = S_OK;

    FSD_MESSAGE_FORMAT* pMessage = static_cast<FSD_MESSAGE_FORMAT*>(pvInputBuffer);
    RETURN_IF_FAILED_ALLOC(pMessage);

    *puReturnOutputBufferLength = 0;

    switch (pMessage->aType)
    {
        case MESSAGE_TYPE_SET_MANAGER_PID:
        {
            m_uManagerPid = pMessage->uPid;
            break;
        }
        case MESSAGE_TYPE_SET_SCAN_DIRECTORY:
        {
            CAutoStringW wszFileName;
            hr = NewCopyStringW(&wszFileName, pMessage->wszFileName, sizeof(pMessage->wszFileName));
            RETURN_IF_FAILED(hr);

            wszFileName.Detach(&m_wszScanPath);

            TRACE(TL_INFO, "New scan directory configured: %ls\n", pMessage->wszFileName);

            if (pvOutputBuffer == NULL || uOutputBufferLength == 0)
            {
                break;
            }

            swprintf_s((WCHAR*)pvOutputBuffer, (size_t)uOutputBufferLength, L"Scan Directory set to: \"%ls\"", pMessage->wszFileName);
            *puReturnOutputBufferLength = (ULONG)wcsnlen_s((WCHAR*)pvOutputBuffer, static_cast<size_t>(uOutputBufferLength)) + 1;

            break;
        }
        case MESSAGE_TYPE_PRINT_STRING:
        {
            TRACE(TL_INFO, "New Message: %ls\n", pMessage->wszFileName);

            if (pvOutputBuffer == NULL || uOutputBufferLength == 0)
            {
                break;
            }

            swprintf_s((WCHAR*)pvOutputBuffer, (size_t)uOutputBufferLength, L"Message \"%ls\" successfully recieved", pMessage->wszFileName);
            *puReturnOutputBufferLength = (ULONG)wcsnlen_s((WCHAR*)pvOutputBuffer, static_cast<size_t>(uOutputBufferLength)) + 1;

            break;
        }
        case MESSAGE_TYPE_QUERY_NEW_OPS:
        {
            FSD_QUERY_NEW_OPS_RESPONSE_FORMAT* pResponse = static_cast<FSD_QUERY_NEW_OPS_RESPONSE_FORMAT*>(pvOutputBuffer);
            FSD_OPERATION_DESCRIPTION* pOpDescription = pResponse->GetFirst();

            if (!m_pItemsReadyForSend)
            {
                m_pItemsReadyForSend = m_aIrpOpsQueue.PopAll();
            }

            size_t cbData = 0;
            for (;;)
            {
                IrpOperationItem* pIrpOp = m_pItemsReadyForSend;
                if (!pIrpOp)
                {
                    break;
                }

                if (uOutputBufferLength < cbData + pIrpOp->PureSize())
                {
                    break;
                }

                FillOperationDescription(pOpDescription, pIrpOp);

                cbData += pOpDescription->PureSize();
                pOpDescription = pOpDescription->GetNext();
                
                m_pItemsReadyForSend = m_aIrpOpsQueue.Next(m_pItemsReadyForSend);

                delete pIrpOp;
            }

            *puReturnOutputBufferLength = numeric_cast<ULONG>(cbData);

            break;
        }
        default:
            TRACE(TL_INFO, "Unknown MESSAGE_TYPE recieved: %u\n", pMessage->aType);
            break;
    }

    return S_OK;
}

bool CFSDefender::SkipScanning(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pRelatedObjects)
{
    UNREFERENCED_PARAMETER(pRelatedObjects);

    if (FltGetRequestorProcessId(pData) == m_uManagerPid)
    {
        return true;
    }

    //
    //  Directory opens don't need to be scanned.
    //

    if (FlagOn(pData->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE)) 
    {
        return true;
    }

    //
    //  Skip pre-rename operations which always open a directory.
    //

    if (FlagOn(pData->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) 
    {
        return true;
    }
    
    //
    //  Skip paging files.
    //

    if (FlagOn(pData->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)) 
    {
        return true;
    }

    //
    //  Skip scanning DASD opens 
    //

    if (FlagOn(pRelatedObjects->FileObject->Flags, FO_VOLUME_OPEN)) 
    {
        return true;
    }
    
    BOOLEAN fIsDir;
    NTSTATUS hr = FltIsDirectory(pData->Iopb->TargetFileObject, pData->Iopb->TargetInstance, &fIsDir);
    if (FAILED(hr) || fIsDir)
    {
        return true;
    }

    return false;
}

NTSTATUS CFSDefender::ProcessIrp(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pRelatedObjects, PVOID* ppCompletionCtx)
{
    NTSTATUS hr = S_NO_CALLBACK;

    if (SkipScanning(pData, pRelatedObjects))
    {
        return S_NO_CALLBACK;
    }

    CAutoNameInformation pNameInfo;
    hr = FltGetFileNameInformation(pData, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &pNameInfo);
    if (pData->Iopb->MajorFunction == IRP_MJ_READ && FAILED(hr))
    {
        TRACE(TL_VERBOSE, "FltGetFileNameInformation FAILED for Read. Status: 0x%x Irql: %u\n", hr, KeGetCurrentIrql());
    }
    RETURN_IF_FAILED(hr);

    if (pData->Iopb->MajorFunction != IRP_MJ_WRITE &&
        pData->Iopb->MajorFunction != IRP_MJ_SET_INFORMATION &&
        pData->Iopb->MajorFunction != IRP_MJ_READ &&
        !IsFilenameForScan(pNameInfo->Name))
    {
        return S_NO_CALLBACK;
    }

    if (m_fSniffer)
    {
        CAutoPtr<IrpOperationItem> pItem = new IrpOperationItem(pData->Iopb->MajorFunction, 
                                                                    pData->Iopb->MinorFunction,
                                                                    FltGetRequestorProcessId(pData));
        RETURN_IF_FAILED_ALLOC(pItem);

        //
        // Special handling of IRPs
        //
        switch (pData->Iopb->MajorFunction)
        { 
            case IRP_MJ_CREATE:
            {
                if (FlagOn(pData->Iopb->Parameters.Create.Options, FILE_DELETE_ON_CLOSE))
                {
                    pItem->m_fCheckForDelete = true;
                }
                break;
            }  

            case IRP_MJ_WRITE:
            {
                if (pData->Iopb->Parameters.Write.Length == 0)
                {
                    return FLT_PREOP_SUCCESS_NO_CALLBACK;
                }

                PVOID pvWriteBuffer = NULL;
                if (pData->Iopb->Parameters.Write.MdlAddress != NULL) 
                {
                    ASSERT(((PMDL)pData->Iopb->Parameters.Write.MdlAddress)->Next == NULL);

                    pvWriteBuffer = MmGetSystemAddressForMdlSafe(pData->Iopb->Parameters.Write.MdlAddress, NormalPagePriority | MdlMappingNoExecute);
                    if (!pvWriteBuffer)
                    {
                        pData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                        pData->IoStatus.Information = 0;
                        return FLT_PREOP_COMPLETE;
                    }
                }
                else
                {
                    pvWriteBuffer = pData->Iopb->Parameters.Write.WriteBuffer;
                }

                ASSERT(pvWriteBuffer != NULL);

                __try
                {
                    pItem->m_dDataEntropy = CalculateShannonEntropy(pData->Iopb->Parameters.Write.WriteBuffer, pData->Iopb->Parameters.Write.Length);
                    pItem->m_cbData = pData->Iopb->Parameters.Write.Length;
                    pItem->m_fDataEntropyCalculated = true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    pData->IoStatus.Status = GetExceptionCode();
                    pData->IoStatus.Information = 0;
                    return FLT_PREOP_COMPLETE;
                }
                
                break;
            }

            case IRP_MJ_READ:
            {
                // Preoperation callback
                if (pData->Iopb->Parameters.Read.Length == 0)
                {
                    return FLT_PREOP_SUCCESS_NO_CALLBACK;
                }

                break;
            }
                  
            case IRP_MJ_SET_INFORMATION:
            {
                switch (pData->Iopb->Parameters.SetFileInformation.FileInformationClass)
                {
                    case FileDispositionInformation:
                    {
                        PFILE_DISPOSITION_INFORMATION pInfo = (PFILE_DISPOSITION_INFORMATION)pData->Iopb->Parameters.SetFileInformation.InfoBuffer;
                        if (pInfo->DeleteFile)
                        {
                            pItem->m_fCheckForDelete = true;
                        }
                        break;
                    }
                    case FileDispositionInformationEx:
                    {
                        PFILE_DISPOSITION_INFORMATION_EX pInfo = (PFILE_DISPOSITION_INFORMATION_EX)pData->Iopb->Parameters.SetFileInformation.InfoBuffer;
                        if (pInfo->Flags & FILE_DISPOSITION_DELETE)
                        {
                            pItem->m_fCheckForDelete = true;
                        }
                        break;
                    }
                    /*
                    case FileDispositionInformation:
                    case FileDispositionInformationEx:
                    {
                        pItem->m_fCheckForDelete = true;
                        break;
                    }
                    */
                    case FileRenameInformation:
                    case FileRenameInformationEx:
                    {
                        PFILE_RENAME_INFORMATION pRenameInfo = (PFILE_RENAME_INFORMATION)pData->Iopb->Parameters.SetFileInformation.InfoBuffer;
                        
                        CAutoNameInformation pNewNameInfo;
                        hr = FltGetDestinationFileNameInformation(
                            pRelatedObjects->Instance,
                            pRelatedObjects->FileObject,
                            pRenameInfo->RootDirectory,
                            pRenameInfo->FileName,
                            pRenameInfo->FileNameLength,
                            FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER | FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
                            &pNewNameInfo);
                        RETURN_IF_FAILED_EX(hr);

                        // Filter only renames that are related to our folder
                        if (!IsFilenameForScan(pNameInfo->Name) && !IsFilenameForScan(pNewNameInfo->Name))
                        {
                            return S_NO_CALLBACK;
                        }

                        hr = FltParseFileNameInformation(pNewNameInfo.Get());
                        RETURN_IF_FAILED_EX(hr); 

                        LPWSTR wszVolumeName;
                        size_t cbVolumeName;
                        hr = GetVolumeName(&wszVolumeName, &cbVolumeName, pRelatedObjects->Volume);
                        RETURN_IF_FAILED(hr);

                        size_t ceVolumeName = cbVolumeName / 2;
                        size_t ceFileName = pNewNameInfo->Name.Length - pNewNameInfo->Volume.Length + cbVolumeName + 1;
                        size_t cbFileName = ceFileName * sizeof(WCHAR);
                        CAutoStringW wszFileName = new WCHAR[ceFileName];
                        RETURN_IF_FAILED_ALLOC(wszFileName);

                        hr = CopyStringW(wszFileName.Get(), wszVolumeName, cbFileName);
                        RETURN_IF_FAILED_EX(hr);

                        hr = CopyStringW(wszFileName.Get() + ceVolumeName, pNewNameInfo->Name.Buffer + pNewNameInfo->Volume.Length / 2, cbFileName - cbVolumeName);
                        RETURN_IF_FAILED_EX(hr);

                        hr = pItem->SetFileRenameInfo(pNewNameInfo->Name.Buffer, pNewNameInfo->Name.Length + sizeof(WCHAR));
                        RETURN_IF_FAILED_EX(hr); 

                        break;
                    }

                    default:
                    {
                        return S_NO_CALLBACK;
                    }
                }

                break;
            }

            default:
            {
                break;
            }
        }

        hr = FltParseFileNameInformation(pNameInfo.Get());
        RETURN_IF_FAILED_EX(hr);

        LPWSTR wszVolumeName;
        size_t cbVolumeName;
        hr = GetVolumeName(&wszVolumeName, &cbVolumeName, pRelatedObjects->Volume);
        RETURN_IF_FAILED(hr);

        size_t ceVolumeName = cbVolumeName / 2;

        size_t ceFileName = pNameInfo->Name.Length - pNameInfo->Volume.Length + cbVolumeName + 1;
        size_t cbFileName = ceFileName * sizeof(WCHAR);
        CAutoStringW wszFileName = new WCHAR[ceFileName];
        RETURN_IF_FAILED_ALLOC(wszFileName);

        hr = CopyStringW(wszFileName.Get(), wszVolumeName, cbFileName);
        RETURN_IF_FAILED_EX(hr);

        hr = CopyStringW(wszFileName.Get() + ceVolumeName, pNameInfo->Name.Buffer + pNameInfo->Volume.Length / 2, cbFileName - cbVolumeName);
        RETURN_IF_FAILED_EX(hr);

        hr = pItem->SetFileName(wszFileName.Get(), cbFileName);
        RETURN_IF_FAILED_EX(hr);

        if (pData->Iopb->MajorFunction == IRP_MJ_READ)
        {
            *ppCompletionCtx = pItem.LetPtr();
            return S_WITH_CALLBACK;
        }

        m_aIrpOpsQueue.Push(pItem.LetPtr());
    }
    else
    {
        CHAR szIrpCode[MAX_STRING_LENGTH] = {};
        PrintIrpCode(pData->Iopb->MajorFunction, pData->Iopb->MinorFunction, szIrpCode, sizeof(szIrpCode));

        TRACE(TL_INFO, "PID: %u File: %.*ls %s\n", 
            FltGetRequestorProcessId(pData), pNameInfo->Name.Length, pNameInfo->Name.Buffer, szIrpCode);
    }
   
    return S_NO_CALLBACK;
}

NTSTATUS
CFSDefender::FSDInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
/*++

Routine Description:

This routine is called whenever a new instance is created on a volume. This
gives us a chance to decide if we need to attach to this volume or not.

If this routine is not defined in the registration structure, automatic
instances are always created.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Flags describing the reason for this attach request.

Return Value:

STATUS_SUCCESS - attach
STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    PAGED_CODE();

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceSetup: Entered\n");

    return S_OK;
}

NTSTATUS
CFSDefender::FSDInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This is called when an instance is being manually deleted by a
call to FltDetachVolume or FilterDetach thereby giving us a
chance to fail that detach request.

If this routine is not defined in the registration structure, explicit
detach requests via FltDetachVolume or FilterDetach will always be
failed.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Indicating where this detach request came from.

Return Value:

Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceQueryTeardown: Entered\n");

    return S_OK;
}

VOID
CFSDefender::FSDInstanceTeardownStart(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This routine is called at the start of instance teardown.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Reason why this instance is being deleted.

Return Value:

None.

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceTeardownStart: Entered\n");
}

VOID
CFSDefender::FSDInstanceTeardownComplete(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

This routine is called at the end of instance teardown.

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance and its associated volume.

Flags - Reason why this instance is being deleted.

Return Value:

None.

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDInstanceTeardownComplete: Entered\n");
}

NTSTATUS
CFSDefender::FSDUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
/*++

Routine Description:

This is the unload routine for this miniFilter driver. This is called
when the minifilter is about to be unloaded. We can fail this unload
request if this is not a mandatory unload indicated by the Flags
parameter.

Arguments:

Flags - Indicating if this is a mandatory unload.

Return Value:

Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDUnload: Entered\n");

    delete g;

    return S_OK;
}

ULONG_PTR CFSDefender::OperationStatusCtx = 1;

/*************************************************************************
MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
CFSDefender::FSDPreOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

This routine is a pre-operation dispatch routine for this miniFilter.

This is non-pageable because it could be called on the paging path

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The context for the completion routine for this
operation.

Return Value:

The return value is the status of the operation.

--*/
{
    NTSTATUS hr;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPreOperation: Entered\n");

    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    if (FSDDoRequestOperationStatus(Data)) {

        hr = FltRequestOperationStatusCallback(Data,
            FSDOperationStatusCallback,
            (PVOID)(++OperationStatusCtx));
        if (FAILED(hr)) {
            TRACE(TL_ERROR, "FSD!FSDPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n", hr);
        }
    }

    if (FltObjects->FileObject == NULL)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // Create and Read will be processed at PostOperation
    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
    {
        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    hr = g->ProcessIrp(Data, FltObjects, CompletionContext);
    if (hr == FLT_PREOP_SUCCESS_WITH_CALLBACK)
    {
        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }
   
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

VOID
CFSDefender::FSDOperationStatusCallback(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
)
/*++

Routine Description:

This routine is called when the given operation returns from the call
to IoCallDriver.  This is useful for operations where STATUS_PENDING
means the operation was successfully queued.  This is useful for OpLocks
and directory change notification operations.

This callback is called in the context of the originating thread and will
never be called at DPC level.  The file object has been correctly
referenced so that you can access it.  It will be automatically
dereferenced upon return.

This is non-pageable because it could be called on the paging path

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

RequesterContext - The context for the completion routine for this
operation.

OperationStatus -

Return Value:

The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(FltObjects);

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDOperationStatusCallback: Entered\n");

    TRACE(TL_VERBOSE, "FSD!FSDOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
        OperationStatus,
        RequesterContext,
        ParameterSnapshot->MajorFunction,
        ParameterSnapshot->MinorFunction,
        FltGetIrpName(ParameterSnapshot->MajorFunction));
}

FLT_POSTOP_CALLBACK_STATUS
CFSDefender::FSDPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
){
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPostOperation: Entered\n");

    if (Data->Iopb->MajorFunction == IRP_MJ_READ)
    {
        if (CompletionContext == NULL)
        {
            TRACE(TL_ERROR, "Context is NULL in post operation\n");
            return FLT_POSTOP_FINISHED_PROCESSING;
        }
        (void)g->ProcessReadPostIrp(Data, FltObjects, CompletionContext, Flags);

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    (void)g->ProcessIrp(Data, FltObjects, NULL);

    return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS CFSDefender::ProcessReadPostIrp(PFLT_CALLBACK_DATA pData, PCFLT_RELATED_OBJECTS pRelatedObjects, PVOID pCompletionCtx, FLT_POST_OPERATION_FLAGS Flags)
{
    //NTSTATUS hr = S_OK;

    CAutoPtr<IrpOperationItem> pItem = (IrpOperationItem*)pCompletionCtx;

    if (FAILED(pData->IoStatus.Status) || pData->IoStatus.Information == 0)
    {
        return S_OK;
    }

    PVOID pvReadBuffer = NULL;
    if (pData->Iopb->Parameters.Read.MdlAddress != NULL)
    {
        ASSERT(((PMDL)pData->Iopb->Parameters.Read.MdlAddress)->Next == NULL);

        pvReadBuffer = MmGetSystemAddressForMdlSafe(pData->Iopb->Parameters.Read.MdlAddress, NormalPagePriority | MdlMappingNoExecute);
        if (!pvReadBuffer)
        {
            pData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            pData->IoStatus.Information = 0;

            return FLT_POSTOP_FINISHED_PROCESSING;
        }
    }
    else
    if (FlagOn(pData->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) || FlagOn(pData->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
    {
        pvReadBuffer = pData->Iopb->Parameters.Read.ReadBuffer;
    }
    else
    {
        FLT_POSTOP_CALLBACK_STATUS status;
        bool fSuccess = FltDoCompletionProcessingWhenSafe(pData, pRelatedObjects, pCompletionCtx, Flags, FSDReadPostReadBuffersWhenSafe, &status);
        if (fSuccess)
        {
            pItem.LetPtr();
            return status;
        }
        else
        {
            pData->IoStatus.Status = STATUS_UNSUCCESSFUL;
            pData->IoStatus.Information = 0;

            return status;
        }
    }

    __try 
    {
        pItem->m_dDataEntropy = CalculateShannonEntropy(pvReadBuffer, pData->IoStatus.Information);
        pItem->m_cbData = pData->IoStatus.Information;
        pItem->m_fDataEntropyCalculated = true;

        g->m_aIrpOpsQueue.Push(pItem.LetPtr());

    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pData->IoStatus.Status = GetExceptionCode();
        pData->IoStatus.Information = 0;
    }

    return S_OK;
}

FLT_POSTOP_CALLBACK_STATUS CFSDefender::FSDReadPostReadBuffersWhenSafe(
    PFLT_CALLBACK_DATA          pData,
    PCFLT_RELATED_OBJECTS       pFltObjects,
    PVOID                       pCompletionCtx,
    FLT_POST_OPERATION_FLAGS    Flags
){
    UNREFERENCED_PARAMETER(pFltObjects);
    UNREFERENCED_PARAMETER(Flags);

    NTSTATUS hr = FLT_POSTOP_FINISHED_PROCESSING;

    CAutoPtr<IrpOperationItem> pItem = (IrpOperationItem*)pCompletionCtx;

    hr = FltLockUserBuffer(pData);
    if (FAILED(hr))
    {
        pData->IoStatus.Status = hr;
        pData->IoStatus.Information = 0;

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    PVOID pvReadBuffer = MmGetSystemAddressForMdlSafe(pData->Iopb->Parameters.Read.MdlAddress, NormalPagePriority | MdlMappingNoExecute);
    if (!pvReadBuffer) 
    {
        pData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        pData->IoStatus.Information = 0;

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    pItem->m_dDataEntropy = CalculateShannonEntropy(pvReadBuffer, pData->IoStatus.Information);
    pItem->m_cbData = pData->IoStatus.Information;
    pItem->m_fDataEntropyCalculated = true;
    
    g->m_aIrpOpsQueue.Push(pItem.LetPtr());

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
CFSDefender::FSDPreOperationNoPostOperation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

This routine is a pre-operation dispatch routine for this miniFilter.

This is non-pageable because it could be called on the paging path

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The context for the completion routine for this
operation.

Return Value:

The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPreOperationNoPostOperation: Entered\n");

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

BOOLEAN
CFSDefender::FSDDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
)
/*++

Routine Description:

This identifies those operations we want the operation status for.  These
are typically operations that return STATUS_PENDING as a normal completion
status.

Arguments:

Return Value:

TRUE - If we want the operation status
FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

        //
        //  Check for oplock operations
        //

        (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
        ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
            (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
            (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
            (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

            ||

            //
            //    Check for directy change notification
            //

            ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
            (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
            );
}

bool CFSDefender::IsFilenameForScan(UNICODE_STRING ustrFileName)
{
    LPCWSTR wszScanDirName = g->GetScanDirectoryName();
    if (!wszScanDirName)
    {
        return false;
    }

    return wcsstr(ustrFileName.Buffer, wszScanDirName) != NULL;
}