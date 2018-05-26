#pragma once

#ifdef _KERNEL_MODE  
	#define FAILED(hr) (!NT_SUCCESS(hr))
	#define S_OK	STATUS_SUCCESS
#else
	#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#define RETURN_IF_FAILED(hr) { if(FAILED(hr)) return hr; }
#define RETURN_IF_FAILED_ALLOC(ptr) { if(!ptr) return STATUS_NO_MEMORY;}

//
//  Name of port used to communicate
//
const LPCWSTR g_wszFSDPortName = L"\\FSDCommunicationPort";
