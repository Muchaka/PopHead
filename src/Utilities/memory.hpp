namespace ph {

#define Kilobytes(n) n * 1024
#define Megabytes(n) Kilobytes(n * 1024)

struct BumpMemoryArena
{
	char* base;
	size_t size;
	size_t used;
};

void allocateAndInitArena(BumpMemoryArena*, size_t size);
void initArena(BumpMemoryArena*, size_t size, char* base);
BumpMemoryArena subArena(BumpMemoryArena*, size_t size);

#define pushStruct(arena, type) pushSize(arena, sizeof(type))
#define pushArray(arena, count, type) pushSize(arena, count * sizeof(type))
char* pushSize(BumpMemoryArena* arena, size_t size); 

void* allocateVirtualMemory(size_t size);
void freeVirtualMemory(void* memory);

}
