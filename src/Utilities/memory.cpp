#ifdef PH_WINDOWS

#include "memory.hpp"
#include "Logs/logs.hpp"
#include <windows.h>

namespace ph {

void allocateAndInitArena(BumpMemoryArena* arena, size_t size)
{
	char* base = (char*)allocateVirtualMemory(size);
	initArena(arena, size, base); 
}

void initArena(BumpMemoryArena* arena, size_t size, char* base)
{
	arena->base = base;
	arena->size = size;
	arena->used = 0;
}

BumpMemoryArena subArena(BumpMemoryArena* arena, size_t size)
{
	char* base = pushSize(arena, size);	
	BumpMemoryArena subArena;
	initArena(&subArena, size, base);
	return subArena;
}

char* pushSize(BumpMemoryArena* arena, size_t size)
{
	PH_ASSERT_CRITICAL(arena->used + size <= arena->size, "arena didn't have enough space to allocate from it");	
	char* result = arena->base + arena->used;
	arena->used += size;
	return result;
}

void* allocateVirtualMemory(size_t size)
{
	return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void freeVirtualMemory(void* memory)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}

}

#else
	#error You have to implement memory allocation functions for your platform
#endif
