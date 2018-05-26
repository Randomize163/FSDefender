#include "FSDUtils.h"
#include <stdio.h>
#include <ntddk.h>

const WCHAR* SCAN_FOLDER_PATH = L"\\Device\\HarddiskVolume3\\Users\\guran1996\\Documents\\";
const size_t SCAN_FOLDER_PATH_LENGTH = 50;

void *__cdecl operator new(size_t count) {
	return ExAllocatePoolWithTag(NonPagedPool, count, 'TRCm');
}

void __cdecl operator delete(void *object, unsigned __int64 size) {
	UNREFERENCED_PARAMETER(size);
	ExFreePoolWithTag(object, 'TRCm');
}

bool PrintFileName(UNICODE_STRING ustrFileName)
{
	if (ustrFileName.Length < SCAN_FOLDER_PATH_LENGTH)
	{
		return false;
	}

	return wcsncmp(SCAN_FOLDER_PATH, ustrFileName.Buffer, SCAN_FOLDER_PATH_LENGTH) == 0;
}

VOID
PrintIrpCode(
	_In_	UCHAR MajorCode,
	_In_	UCHAR MinorCode,
	_Out_	CHAR* szIrpCodeString,
	_In_	ULONG cbIrpCodeString
)
/*++
Routine Description:
Display the operation code
Arguments:
MajorCode - Major function code of operation
MinorCode - Minor function code of operation
Return Value:
None
--*/
{
	CHAR *irpMajorString, *irpMinorString = NULL;
	CHAR errorBuf[128];

	switch (MajorCode) {
	case IRP_MJ_CREATE:
		irpMajorString = IRP_MJ_CREATE_STRING;
		break;
	case IRP_MJ_CREATE_NAMED_PIPE:
		irpMajorString = IRP_MJ_CREATE_NAMED_PIPE_STRING;
		break;
	case IRP_MJ_CLOSE:
		irpMajorString = IRP_MJ_CLOSE_STRING;
		break;
	case IRP_MJ_READ:
		irpMajorString = IRP_MJ_READ_STRING;
		switch (MinorCode) {
		case IRP_MN_NORMAL:
			irpMinorString = IRP_MN_NORMAL_STRING;
			break;
		case IRP_MN_DPC:
			irpMinorString = IRP_MN_DPC_STRING;
			break;
		case IRP_MN_MDL:
			irpMinorString = IRP_MN_MDL_STRING;
			break;
		case IRP_MN_COMPLETE:
			irpMinorString = IRP_MN_COMPLETE_STRING;
			break;
		case IRP_MN_COMPRESSED:
			irpMinorString = IRP_MN_COMPRESSED_STRING;
			break;
		case IRP_MN_MDL_DPC:
			irpMinorString = IRP_MN_MDL_DPC_STRING;
			break;
		case IRP_MN_COMPLETE_MDL:
			irpMinorString = IRP_MN_COMPLETE_MDL_STRING;
			break;
		case IRP_MN_COMPLETE_MDL_DPC:
			irpMinorString = IRP_MN_COMPLETE_MDL_DPC_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_WRITE:
		irpMajorString = IRP_MJ_WRITE_STRING;
		switch (MinorCode) {
		case IRP_MN_NORMAL:
			irpMinorString = IRP_MN_NORMAL_STRING;
			break;
		case IRP_MN_DPC:
			irpMinorString = IRP_MN_DPC_STRING;
			break;
		case IRP_MN_MDL:
			irpMinorString = IRP_MN_MDL_STRING;
			break;
		case IRP_MN_COMPLETE:
			irpMinorString = IRP_MN_COMPLETE_STRING;
			break;
		case IRP_MN_COMPRESSED:
			irpMinorString = IRP_MN_COMPRESSED_STRING;
			break;
		case IRP_MN_MDL_DPC:
			irpMinorString = IRP_MN_MDL_DPC_STRING;
			break;
		case IRP_MN_COMPLETE_MDL:
			irpMinorString = IRP_MN_COMPLETE_MDL_STRING;
			break;
		case IRP_MN_COMPLETE_MDL_DPC:
			irpMinorString = IRP_MN_COMPLETE_MDL_DPC_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_QUERY_INFORMATION:
		irpMajorString = IRP_MJ_QUERY_INFORMATION_STRING;
		break;
	case IRP_MJ_SET_INFORMATION:
		irpMajorString = IRP_MJ_SET_INFORMATION_STRING;
		break;
	case IRP_MJ_QUERY_EA:
		irpMajorString = IRP_MJ_QUERY_EA_STRING;
		break;
	case IRP_MJ_SET_EA:
		irpMajorString = IRP_MJ_SET_EA_STRING;
		break;
	case IRP_MJ_FLUSH_BUFFERS:
		irpMajorString = IRP_MJ_FLUSH_BUFFERS_STRING;
		break;
	case IRP_MJ_QUERY_VOLUME_INFORMATION:
		irpMajorString = IRP_MJ_QUERY_VOLUME_INFORMATION_STRING;
		break;
	case IRP_MJ_SET_VOLUME_INFORMATION:
		irpMajorString = IRP_MJ_SET_VOLUME_INFORMATION_STRING;
		break;
	case IRP_MJ_DIRECTORY_CONTROL:
		irpMajorString = IRP_MJ_DIRECTORY_CONTROL_STRING;
		switch (MinorCode) {
		case IRP_MN_QUERY_DIRECTORY:
			irpMinorString = IRP_MN_QUERY_DIRECTORY_STRING;
			break;
		case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
			irpMinorString = IRP_MN_NOTIFY_CHANGE_DIRECTORY_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_FILE_SYSTEM_CONTROL:
		irpMajorString = IRP_MJ_FILE_SYSTEM_CONTROL_STRING;
		switch (MinorCode) {
		case IRP_MN_USER_FS_REQUEST:
			irpMinorString = IRP_MN_USER_FS_REQUEST_STRING;
			break;
		case IRP_MN_MOUNT_VOLUME:
			irpMinorString = IRP_MN_MOUNT_VOLUME_STRING;
			break;
		case IRP_MN_VERIFY_VOLUME:
			irpMinorString = IRP_MN_VERIFY_VOLUME_STRING;
			break;
		case IRP_MN_LOAD_FILE_SYSTEM:
			irpMinorString = IRP_MN_LOAD_FILE_SYSTEM_STRING;
			break;
		case IRP_MN_TRACK_LINK:
			irpMinorString = IRP_MN_TRACK_LINK_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_DEVICE_CONTROL:
		irpMajorString = IRP_MJ_DEVICE_CONTROL_STRING;
		switch (MinorCode) {
		case IRP_MN_SCSI_CLASS:
			irpMinorString = IRP_MN_SCSI_CLASS_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		irpMajorString = IRP_MJ_INTERNAL_DEVICE_CONTROL_STRING;
		break;
	case IRP_MJ_SHUTDOWN:
		irpMajorString = IRP_MJ_SHUTDOWN_STRING;
		break;
	case IRP_MJ_LOCK_CONTROL:
		irpMajorString = IRP_MJ_LOCK_CONTROL_STRING;
		switch (MinorCode) {
		case IRP_MN_LOCK:
			irpMinorString = IRP_MN_LOCK_STRING;
			break;
		case IRP_MN_UNLOCK_SINGLE:
			irpMinorString = IRP_MN_UNLOCK_SINGLE_STRING;
			break;
		case IRP_MN_UNLOCK_ALL:
			irpMinorString = IRP_MN_UNLOCK_ALL_STRING;
			break;
		case IRP_MN_UNLOCK_ALL_BY_KEY:
			irpMinorString = IRP_MN_UNLOCK_ALL_BY_KEY_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_CLEANUP:
		irpMajorString = IRP_MJ_CLEANUP_STRING;
		break;
	case IRP_MJ_CREATE_MAILSLOT:
		irpMajorString = IRP_MJ_CREATE_MAILSLOT_STRING;
		break;
	case IRP_MJ_QUERY_SECURITY:
		irpMajorString = IRP_MJ_QUERY_SECURITY_STRING;
		break;
	case IRP_MJ_SET_SECURITY:
		irpMajorString = IRP_MJ_SET_SECURITY_STRING;
		break;
	case IRP_MJ_POWER:
		irpMajorString = IRP_MJ_POWER_STRING;
		switch (MinorCode) {
		case IRP_MN_WAIT_WAKE:
			irpMinorString = IRP_MN_WAIT_WAKE_STRING;
			break;
		case IRP_MN_POWER_SEQUENCE:
			irpMinorString = IRP_MN_POWER_SEQUENCE_STRING;
			break;
		case IRP_MN_SET_POWER:
			irpMinorString = IRP_MN_SET_POWER_STRING;
			break;
		case IRP_MN_QUERY_POWER:
			irpMinorString = IRP_MN_QUERY_POWER_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_SYSTEM_CONTROL:
		irpMajorString = IRP_MJ_SYSTEM_CONTROL_STRING;
		switch (MinorCode) {
		case IRP_MN_QUERY_ALL_DATA:
			irpMinorString = IRP_MN_QUERY_ALL_DATA_STRING;
			break;
		case IRP_MN_QUERY_SINGLE_INSTANCE:
			irpMinorString = IRP_MN_QUERY_SINGLE_INSTANCE_STRING;
			break;
		case IRP_MN_CHANGE_SINGLE_INSTANCE:
			irpMinorString = IRP_MN_CHANGE_SINGLE_INSTANCE_STRING;
			break;
		case IRP_MN_CHANGE_SINGLE_ITEM:
			irpMinorString = IRP_MN_CHANGE_SINGLE_ITEM_STRING;
			break;
		case IRP_MN_ENABLE_EVENTS:
			irpMinorString = IRP_MN_ENABLE_EVENTS_STRING;
			break;
		case IRP_MN_DISABLE_EVENTS:
			irpMinorString = IRP_MN_DISABLE_EVENTS_STRING;
			break;
		case IRP_MN_ENABLE_COLLECTION:
			irpMinorString = IRP_MN_ENABLE_COLLECTION_STRING;
			break;
		case IRP_MN_DISABLE_COLLECTION:
			irpMinorString = IRP_MN_DISABLE_COLLECTION_STRING;
			break;
		case IRP_MN_REGINFO:
			irpMinorString = IRP_MN_REGINFO_STRING;
			break;
		case IRP_MN_EXECUTE_METHOD:
			irpMinorString = IRP_MN_EXECUTE_METHOD_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;

	case IRP_MJ_DEVICE_CHANGE:
		irpMajorString = IRP_MJ_DEVICE_CHANGE_STRING;
		break;
	case IRP_MJ_QUERY_QUOTA:
		irpMajorString = IRP_MJ_QUERY_QUOTA_STRING;
		break;
	case IRP_MJ_SET_QUOTA:
		irpMajorString = IRP_MJ_SET_QUOTA_STRING;
		break;
	case IRP_MJ_PNP:
		irpMajorString = IRP_MJ_PNP_STRING;
		switch (MinorCode) {
		case IRP_MN_START_DEVICE:
			irpMinorString = IRP_MN_START_DEVICE_STRING;
			break;
		case IRP_MN_QUERY_REMOVE_DEVICE:
			irpMinorString = IRP_MN_QUERY_REMOVE_DEVICE_STRING;
			break;
		case IRP_MN_REMOVE_DEVICE:
			irpMinorString = IRP_MN_REMOVE_DEVICE_STRING;
			break;
		case IRP_MN_CANCEL_REMOVE_DEVICE:
			irpMinorString = IRP_MN_CANCEL_REMOVE_DEVICE_STRING;
			break;
		case IRP_MN_STOP_DEVICE:
			irpMinorString = IRP_MN_STOP_DEVICE_STRING;
			break;
		case IRP_MN_QUERY_STOP_DEVICE:
			irpMinorString = IRP_MN_QUERY_STOP_DEVICE_STRING;
			break;
		case IRP_MN_CANCEL_STOP_DEVICE:
			irpMinorString = IRP_MN_CANCEL_STOP_DEVICE_STRING;
			break;
		case IRP_MN_QUERY_DEVICE_RELATIONS:
			irpMinorString = IRP_MN_QUERY_DEVICE_RELATIONS_STRING;
			break;
		case IRP_MN_QUERY_INTERFACE:
			irpMinorString = IRP_MN_QUERY_INTERFACE_STRING;
			break;
		case IRP_MN_QUERY_CAPABILITIES:
			irpMinorString = IRP_MN_QUERY_CAPABILITIES_STRING;
			break;
		case IRP_MN_QUERY_RESOURCES:
			irpMinorString = IRP_MN_QUERY_RESOURCES_STRING;
			break;
		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
			irpMinorString = IRP_MN_QUERY_RESOURCE_REQUIREMENTS_STRING;
			break;
		case IRP_MN_QUERY_DEVICE_TEXT:
			irpMinorString = IRP_MN_QUERY_DEVICE_TEXT_STRING;
			break;
		case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
			irpMinorString = IRP_MN_FILTER_RESOURCE_REQUIREMENTS_STRING;
			break;
		case IRP_MN_READ_CONFIG:
			irpMinorString = IRP_MN_READ_CONFIG_STRING;
			break;
		case IRP_MN_WRITE_CONFIG:
			irpMinorString = IRP_MN_WRITE_CONFIG_STRING;
			break;
		case IRP_MN_EJECT:
			irpMinorString = IRP_MN_EJECT_STRING;
			break;
		case IRP_MN_SET_LOCK:
			irpMinorString = IRP_MN_SET_LOCK_STRING;
			break;
		case IRP_MN_QUERY_ID:
			irpMinorString = IRP_MN_QUERY_ID_STRING;
			break;
		case IRP_MN_QUERY_PNP_DEVICE_STATE:
			irpMinorString = IRP_MN_QUERY_PNP_DEVICE_STATE_STRING;
			break;
		case IRP_MN_QUERY_BUS_INFORMATION:
			irpMinorString = IRP_MN_QUERY_BUS_INFORMATION_STRING;
			break;
		case IRP_MN_DEVICE_USAGE_NOTIFICATION:
			irpMinorString = IRP_MN_DEVICE_USAGE_NOTIFICATION_STRING;
			break;
		case IRP_MN_SURPRISE_REMOVAL:
			irpMinorString = IRP_MN_SURPRISE_REMOVAL_STRING;
			break;
		case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
			irpMinorString = IRP_MN_QUERY_LEGACY_BUS_INFORMATION_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp minor code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;


	case IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION:
		irpMajorString = IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION_STRING;
		break;

	case IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION:
		irpMajorString = IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION_STRING;
		break;

	case IRP_MJ_ACQUIRE_FOR_MOD_WRITE:
		irpMajorString = IRP_MJ_ACQUIRE_FOR_MOD_WRITE_STRING;
		break;

	case IRP_MJ_RELEASE_FOR_MOD_WRITE:
		irpMajorString = IRP_MJ_RELEASE_FOR_MOD_WRITE_STRING;
		break;

	case IRP_MJ_ACQUIRE_FOR_CC_FLUSH:
		irpMajorString = IRP_MJ_ACQUIRE_FOR_CC_FLUSH_STRING;
		break;

	case IRP_MJ_RELEASE_FOR_CC_FLUSH:
		irpMajorString = IRP_MJ_RELEASE_FOR_CC_FLUSH_STRING;
		break;

	case IRP_MJ_NOTIFY_STREAM_FO_CREATION:
		irpMajorString = IRP_MJ_NOTIFY_STREAM_FO_CREATION_STRING;
		break;



	case IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE:
		irpMajorString = IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE_STRING;
		break;

	case IRP_MJ_NETWORK_QUERY_OPEN:
		irpMajorString = IRP_MJ_NETWORK_QUERY_OPEN_STRING;
		break;

	case IRP_MJ_MDL_READ:
		irpMajorString = IRP_MJ_MDL_READ_STRING;
		break;

	case IRP_MJ_MDL_READ_COMPLETE:
		irpMajorString = IRP_MJ_MDL_READ_COMPLETE_STRING;
		break;

	case IRP_MJ_PREPARE_MDL_WRITE:
		irpMajorString = IRP_MJ_PREPARE_MDL_WRITE_STRING;
		break;

	case IRP_MJ_MDL_WRITE_COMPLETE:
		irpMajorString = IRP_MJ_MDL_WRITE_COMPLETE_STRING;
		break;

	case IRP_MJ_VOLUME_MOUNT:
		irpMajorString = IRP_MJ_VOLUME_MOUNT_STRING;
		break;

	case IRP_MJ_VOLUME_DISMOUNT:
		irpMajorString = IRP_MJ_VOLUME_DISMOUNT_STRING;
		break;

	case IRP_MJ_TRANSACTION_NOTIFY:
		irpMajorString = IRP_MJ_TRANSACTION_NOTIFY_STRING;
		switch (MinorCode) {
		case 0:
			irpMinorString = TRANSACTION_BEGIN;
			break;
		case TRANSACTION_NOTIFY_PREPREPARE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_PREPREPARE_STRING;
			break;
		case TRANSACTION_NOTIFY_PREPARE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_PREPARE_STRING;
			break;
		case TRANSACTION_NOTIFY_COMMIT_CODE:
			irpMinorString = TRANSACTION_NOTIFY_COMMIT_STRING;
			break;
		case TRANSACTION_NOTIFY_COMMIT_FINALIZE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_COMMIT_FINALIZE_STRING;
			break;
		case TRANSACTION_NOTIFY_ROLLBACK_CODE:
			irpMinorString = TRANSACTION_NOTIFY_ROLLBACK_STRING;
			break;
		case TRANSACTION_NOTIFY_PREPREPARE_COMPLETE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_PREPREPARE_COMPLETE_STRING;
			break;
		case TRANSACTION_NOTIFY_PREPARE_COMPLETE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_COMMIT_COMPLETE_STRING;
			break;
		case TRANSACTION_NOTIFY_ROLLBACK_COMPLETE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_ROLLBACK_COMPLETE_STRING;
			break;
		case TRANSACTION_NOTIFY_RECOVER_CODE:
			irpMinorString = TRANSACTION_NOTIFY_RECOVER_STRING;
			break;
		case TRANSACTION_NOTIFY_SINGLE_PHASE_COMMIT_CODE:
			irpMinorString = TRANSACTION_NOTIFY_SINGLE_PHASE_COMMIT_STRING;
			break;
		case TRANSACTION_NOTIFY_DELEGATE_COMMIT_CODE:
			irpMinorString = TRANSACTION_NOTIFY_DELEGATE_COMMIT_STRING;
			break;
		case TRANSACTION_NOTIFY_RECOVER_QUERY_CODE:
			irpMinorString = TRANSACTION_NOTIFY_RECOVER_QUERY_STRING;
			break;
		case TRANSACTION_NOTIFY_ENLIST_PREPREPARE_CODE:
			irpMinorString = TRANSACTION_NOTIFY_ENLIST_PREPREPARE_STRING;
			break;
		case TRANSACTION_NOTIFY_LAST_RECOVER_CODE:
			irpMinorString = TRANSACTION_NOTIFY_LAST_RECOVER_STRING;
			break;
		case TRANSACTION_NOTIFY_INDOUBT_CODE:
			irpMinorString = TRANSACTION_NOTIFY_INDOUBT_STRING;
			break;
		case TRANSACTION_NOTIFY_PROPAGATE_PULL_CODE:
			irpMinorString = TRANSACTION_NOTIFY_PROPAGATE_PULL_STRING;
			break;
		case TRANSACTION_NOTIFY_PROPAGATE_PUSH_CODE:
			irpMinorString = TRANSACTION_NOTIFY_PROPAGATE_PUSH_STRING;
			break;
		case TRANSACTION_NOTIFY_MARSHAL_CODE:
			irpMinorString = TRANSACTION_NOTIFY_MARSHAL_STRING;
			break;
		case TRANSACTION_NOTIFY_ENLIST_MASK_CODE:
			irpMinorString = TRANSACTION_NOTIFY_ENLIST_MASK_STRING;
			break;
		default:
			sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Transaction notication code (%u)", MinorCode);
			irpMinorString = errorBuf;
		}
		break;


	default:
		sprintf_s(errorBuf, sizeof(errorBuf), "Unknown Irp major function (%d)", MajorCode);
		irpMajorString = errorBuf;
		break;
	}

	sprintf_s(szIrpCodeString, cbIrpCodeString, "%s %s", irpMajorString, irpMinorString);
}