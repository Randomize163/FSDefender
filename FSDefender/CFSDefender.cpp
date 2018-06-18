#include "CFSDefender.h"

#include "FSDUtils.h"
#include "FSDCommonDefs.h"
#include "FSDRegistrationInfo.h"
#include "stdio.h"
#include "FSDStringUtils.h"

extern CFSDefender* g;

CFSDefender::CFSDefender()
    : m_pFilter()
    , m_pPort()
    , m_wszScanPath()
    , m_fSniffer(false)
    , m_fClosed(false)
    , m_pItemsReadyForSend(NULL)
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

NTSTATUS CFSDefender::HandleNewMessage(IN PVOID pvInputBuffer, IN ULONG uInputBufferLength, OUT PVOID pvOutputBuffer, IN ULONG uOutputBufferLength, OUT PULONG puReturnOutputBufferLength)
{
    UNREFERENCED_PARAMETER(uInputBufferLength);
    NTSTATUS hr = S_OK;

    FSD_MESSAGE_FORMAT* pMessage = static_cast<FSD_MESSAGE_FORMAT*>(pvInputBuffer);
    RETURN_IF_FAILED_ALLOC(pMessage);

    *puReturnOutputBufferLength = 0;

    switch (pMessage->aType)
    {
        case MESSAGE_TYPE_SET_SCAN_DIRECTORY:
        {
            CAutoStringW wszFileName;
            hr = NewCopyStringW(&wszFileName, pMessage->wszFileName, uInputBufferLength - sizeof(MESSAGE_TYPE));
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

                pOpDescription->uMajorType = pIrpOp->m_uIrpMajorCode;
                pOpDescription->uMinorType = pIrpOp->m_uIrpMinorCode;
                pOpDescription->uPid       = pIrpOp->m_uPid;
                pOpDescription->cbData     = pIrpOp->m_cbBuffer;

                memcpy(
                    pOpDescription->pData,
                    pIrpOp->m_pBuffer.LetPtr(), 
                    pIrpOp->m_cbBuffer
                );

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

NTSTATUS CFSDefender::ProcessPreIrp(PFLT_CALLBACK_DATA pData)
{
    NTSTATUS hr = S_OK;

    CAutoNameInformation pNameInfo;
    hr = FltGetFileNameInformation(pData, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &pNameInfo);
    if (hr == STATUS_FLT_NAME_CACHE_MISS)
    {
        hr = FltGetFileNameInformation(pData, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY, &pNameInfo);
        if (FAILED(hr))
        {
            //TRACE(TL_VERBOSE, "FSD!FSDPreOperation: FltGetFileNameInformation Failed, status=%08x\n", hr);
            return FLT_PREOP_SUCCESS_WITH_CALLBACK;
        }
    }
    if (FAILED(hr))
    {
        //TRACE(TL_VERBOSE, "FSD!FSDPreOperation: FltGetFileNameInformation Failed, status=%08x\n", hr);
        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    if (IsFilenameForScan(pNameInfo->Name))
    {
        if (m_fSniffer)
        {
            IrpOperationItem* pItem = new IrpOperationItem(pData->Iopb->MajorFunction, 
                                                           pData->Iopb->MinorFunction,
                                                           FltGetRequestorProcessId(pData));
            RETURN_IF_FAILED_ALLOC(pItem);

            m_aIrpOpsQueue.Push(pItem);
        }
        else
        {
            CHAR szIrpCode[MAX_STRING_LENGTH] = {};
            PrintIrpCode(pData->Iopb->MajorFunction, pData->Iopb->MinorFunction, szIrpCode, sizeof(szIrpCode));

            TRACE(TL_VERBOSE, "PID: %u File: %.*ls %s\n", 
                FltGetRequestorProcessId(pData), pNameInfo->Name.Length, pNameInfo->Name.Buffer, szIrpCode);
        }
    }

    return S_OK;
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
        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    (void)g->ProcessPreIrp(Data);   

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
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
)
/*++

Routine Description:

This routine is the post-operation completion routine for this
miniFilter.

This is non-pageable because it may be called at DPC level.

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The completion context set in the pre-operation routine.

Flags - Denotes whether the completion is successful or is being drained.

Return Value:

The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);

    TRACE(TL_FUNCTION_ENTRY, "FSD!FSDPostOperation: Entered\n");

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