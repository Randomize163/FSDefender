/*++

Module Name:

    FSD.c

Abstract:

    This is the main module of the FSD miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "FSDUtils.h"
#include "FSDCommon.h"
#include "AutoPtr.h"
#include "CFSDefender.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

ULONG gTraceFlags = 15;

CFSDefender* g;

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

EXTERN_C_END

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS hr;

    UNREFERENCED_PARAMETER( RegistryPath );

    TRACE(TL_FUNCTION_ENTRY, "FSD!DriverEntry: Entered\n");

	CAutoPtr<CFSDefender> pDefender;
	hr = NewInstanceOf<CFSDefender>(&pDefender, DriverObject);
	RETURN_IF_FAILED(hr);

	pDefender.Detach(&g);

    return S_OK;
}