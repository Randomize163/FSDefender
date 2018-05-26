#pragma once

#ifdef _KERNEL_MODE  
	#define FAILED(hr) (!NT_SUCCESS(hr))
	#define S_OK	STATUS_SUCCESS
	#define	E_FAIL  STATUS_INTERNAL_ERROR
#else
	#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#define RETURN_IF_FAILED(hr) { if(FAILED(hr)) return hr; }
#define RETURN_IF_FAILED_ALLOC(ptr) { if(!ptr) return STATUS_NO_MEMORY;}
#define RETURN_IF_FAILED_ERRNO(err) { if (err != 0) return E_FAIL;}

//
//  Name of port used to communicate
//
const LPCWSTR g_wszFSDPortName = L"\\FSDCommunicationPort";

#define MAX_STRING_LENGTH 256
#define MAX_FILE_NAME_LENGTH MAX_STRING_LENGTH

enum MESSAGE_TYPE {
	MESSAGE_TYPE_SET_SCAN_DIRECTORY,
	MESSAGE_TYPE_PRINT_STRING
};

struct FSD_MESSAGE_FORMAT
{
	MESSAGE_TYPE aType;
	union
	{
		WCHAR wszFileName[MAX_FILE_NAME_LENGTH];
	};

};