#pragma once

void* __cdecl operator new(size_t size);
void* __cdecl operator new[](size_t size);

void __cdecl operator delete(void *object, unsigned __int64);
void __cdecl operator delete[](void *p);