#include "FSDMemoryUtils.h"
#include <ntddk.h>

void *__cdecl operator new(size_t size) {
	return ExAllocatePoolWithTag(NonPagedPool, size, 'TRCm');
}

void* __cdecl operator new[](size_t size) {
	return ExAllocatePoolWithTag(NonPagedPool, size, 'TRCm');
}

void __cdecl operator delete(void *object, unsigned __int64 size) {
	UNREFERENCED_PARAMETER(size);
	ExFreePoolWithTag(object, 'TRCm');
}

void __cdecl operator delete[](void *object) {
	ExFreePoolWithTag(object, 'TRCm');
}