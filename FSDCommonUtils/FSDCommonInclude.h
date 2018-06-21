#pragma once

#ifdef _KERNEL_MODE  
    #include <fltKernel.h>
    #include "FSDKmCommonInclude.h"
    #define CloseHandle NtClose
    #define FAILED(hr) (!NT_SUCCESS(hr))
    #define S_OK    STATUS_SUCCESS
    #define E_FAIL  STATUS_INTERNAL_ERROR
    #define HRESULT NTSTATUS
    #define BYTE char
#else
    #include <windows.h>
    #include <fltUser.h>
    #include <assert.h>
    #define FAILED(hr) (((HRESULT)(hr)) < 0)
    #define HR_IGNORE_ERROR(error) {if (hr == error) hr = S_OK;}
    #define NTSTATUS HRESULT
    #define ASSERT assert
    #define E_FILE_NOT_FOUND HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
    #define STATUS_IO_PENDING HRESULT_FROM_WIN32(ERROR_IO_PENDING)
    #define BYTE char
#endif

#define RETURN_IF_FAILED(hr) { if(FAILED(hr)) return hr; }
#define RETURN_IF_FAILED_ALLOC(ptr) { if(!ptr) return STATUS_NO_MEMORY;}
#define RETURN_IF_FAILED_ERRNO(err) { if (err != 0) return E_FAIL;}
#define CATCH_ALL_AND_RETURN_FAILED_HR catch(...) { return E_FAIL;}

#define VOID_IF_FAILED(hr) { if(FAILED(hr)) return; }
#define VOID_IF_FAILED_ALLOC(ptr) { if(!ptr) return; }

template<class T, class M>
T numeric_cast(M number)
{
    T result = static_cast<T>(number);
    ASSERT(result == number);
    return result;
}