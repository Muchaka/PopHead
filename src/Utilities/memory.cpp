#include "memory.hpp"
#include "Logs/logs.hpp"

#ifdef PH_WINDOWS
#include <windows.h>
#endif

namespace ph {

static size_t bumpToMemoryPageSize(size_t size)
{
	size_t temp = size;
	while(temp > 4096)
		temp -= 4096;
	return size + 4096 - temp;	
}

void allocateAndInitArena(BumpMemoryArena* arena, size_t size)
{
	size = bumpToMemoryPageSize(size);
	char* base = (char*)allocateVirtualMemory(size);
	initArena(arena, size, base); 
}

void allocateAndInitArena(LinkedMemoryArena* arena, size_t size)
{
	size = bumpToMemoryPageSize(size);
	char* base = (char*)allocateVirtualMemory(size);
	initArena(arena, size, base); 
}

void initArena(BumpMemoryArena* arena, size_t size, char* base)
{
	arena->base = base;
	arena->size = size;
	arena->used = 0;
}

void initArena(LinkedMemoryArena* arena, size_t size, char* base)
{
	arena->base = (LinkedMemoryHeader*)base;
	arena->size = size;
	arena->used = 0;
}

BumpMemoryArena subBumpArena(BumpMemoryArena* arena, size_t size)
{
	char* base = pushSize(arena, size);	
	BumpMemoryArena subArena;
	initArena(&subArena, size, base);
	return subArena;
}

BumpMemoryArena subBumpArena(LinkedMemoryArena* arena, size_t size)
{
	char* base = pushSize(arena, size);
	BumpMemoryArena subArena;
	initArena(&subArena, size, base);
	return subArena;
}

LinkedMemoryArena subLinkedArena(BumpMemoryArena* arena, size_t size)
{
	char* base = pushSize(arena, size);
	LinkedMemoryArena subArena;
	initArena(&subArena, size, base);
	return subArena;
}

LinkedMemoryArena subLinkedArena(LinkedMemoryArena* arena, size_t size)
{
	char* base = pushSize(arena, size);
	LinkedMemoryArena subArena;
	initArena(&subArena, size, base);
	return subArena;
}

static size_t align(size_t size, size_t aligment = sizeof(void*))
{
	return (size + aligment - 1) & ~(aligment - 1);	
}

char* pushSize(BumpMemoryArena* arena, size_t size)
{
	size = align(size);
	PH_ASSERT_CRITICAL(arena->used + size <= arena->size, "arena didn't have enough space to allocate from it");	
	char* result = arena->base + arena->used;
	arena->used += size;
	return result;
}

char* pushSize(LinkedMemoryArena* arena, size_t size)
{
	size += sizeof(LinkedMemoryHeader);
	size = align(size);

	LinkedMemoryHeader* header = arena->base;

	bool firstNodeInLinkedList = arena->used == 0;
	if(firstNodeInLinkedList)
	{
		PH_ASSERT_CRITICAL(size <= arena->size, "LinkedMemoryArena didn't have enough space to allocate from it");
		LinkedMemoryHeader* newHeader = arena->base;
		newHeader->next = nullptr;
		newHeader->prev = nullptr;
		newHeader->size = size;
		arena->used += size;	
		return (char*)(newHeader + sizeof(LinkedMemoryHeader));
	}
	else
	{
		for(;;)
		{
			bool isItEndOfLinkedList = header->next == nullptr;
			if(isItEndOfLinkedList)
			{
				// allocate on the end of linked list
				LinkedMemoryHeader* newHeader = header + sizeof(LinkedMemoryHeader) + header->size;
				PH_ASSERT_CRITICAL(newHeader <= arena->base + arena->size, "LinkedMemoryArena didn't have enough space to allocate from it");
				header->next = newHeader;
				newHeader->next = nullptr;
				newHeader->prev = header;
				newHeader->size = size;
				arena->used += size;
				return (char*)(newHeader + sizeof(LinkedMemoryHeader));
			}

			size_t freeSpaceBetweenHeaders = header->next - header - header->size;
			bool newMemoryBlockFits = freeSpaceBetweenHeaders >= size;
			if(newMemoryBlockFits)
			{
				// allocate in free space in the middle of linked list
				LinkedMemoryHeader* newHeader = header + sizeof(LinkedMemoryHeader) + header->size;
				header->next->prev = newHeader;
				newHeader->next = header->next;
				newHeader->prev = header;
				newHeader->size = size;
				header->next = newHeader;
				return (char*)(newHeader + sizeof(LinkedMemoryHeader));
			}

			header += header->size + sizeof(LinkedMemoryHeader);
		}
	}
}

void freeLinkedListNode(LinkedMemoryArena* arena, void* memory)
{
	LinkedMemoryHeader* header = (LinkedMemoryHeader*)memory - sizeof(LinkedMemoryHeader);
	header->prev->next = header->next;
	header->next->prev = header->prev;
}

char* rePushSize(LinkedMemoryArena* arena, void* memory, size_t newSize)
{
	char* newMemory = pushSize(arena, newSize);
	LinkedMemoryHeader* header = (LinkedMemoryHeader*)((char*)memory - sizeof(LinkedMemoryHeader));
	memcpy(newMemory, memory, header->size);
	freeLinkedListNode(arena, memory);
	return newMemory; 
}

#ifdef PH_WINDOWS
void* allocateVirtualMemory(size_t size)
{
	return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void freeVirtualMemory(void* memory)
{
	VirtualFree(memory, 0, MEM_RELEASE);
}
#else
	#error You have to implement memory allocation functions for your platform
#endif
}
