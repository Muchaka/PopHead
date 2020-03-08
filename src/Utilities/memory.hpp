namespace ph {

#define Kilobytes(n) n * 1024
#define Megabytes(n) Kilobytes(n * 1024)

struct LinkedMemoryHeader
{
	LinkedMemoryHeader* next;
	LinkedMemoryHeader* prev;
	size_t size;
};

struct LinkedMemoryArena
{
	LinkedMemoryHeader* base;
	size_t size;
	size_t used;
};

struct BumpMemoryArena
{
	char* base;
	size_t size;
	size_t used;
};

void allocateAndInitArena(BumpMemoryArena*, size_t size);
void allocateAndInitArena(LinkedMemoryArena*, size_t size);

void initArena(BumpMemoryArena*, size_t size, char* base);
void initArena(LinkedMemoryArena*, size_t size, char* base);

BumpMemoryArena subBumpArena(BumpMemoryArena*, size_t size);
BumpMemoryArena subBumpArena(LinkedMemoryArena*, size_t size);
LinkedMemoryArena subLinkedArena(BumpMemoryArena*, size_t size);
LinkedMemoryArena subLinkedArena(LinkedMemoryArena*, size_t size);

#define pushStruct(arena, type) pushSize(arena, sizeof(type))
#define pushArray(arena, count, type) pushSize(arena, count * sizeof(type))
#define rePushStruct(arena, memory, type) rePushSize(arena, memory, sizeof(type))
#define rePushArray(arena, memory, count, type) rePushSize(arena, memory, count * sizeof(type))
char* pushSize(BumpMemoryArena* arena, size_t size); 
char* pushSize(LinkedMemoryArena* arena, size_t size); 
void freeLinkedListNode(LinkedMemoryArena* arena, void* memory);
char* rePushSize(LinkedMemoryArena* arena, void* memory, size_t newSize); 

void* allocateVirtualMemory(size_t size);
void freeVirtualMemory(void* memory);

}
