#include <windows.h>
#include <platform/memory_win32.h>

/**
 * Allocates n-bytes up to the nearest page from the operating system.
 * 
 * @param size The size, in bytes, to allocate.
 * 
 * @returns A pointer of the starting address of the heap.
 */
void*
virtualHeapAllocate(ui64 size)
{

	ui64 pageGranularitySize = KILOBYTES(64);
	ui64 numberPagesRequired = (size / pageGranularitySize) + 1;
	ui64 minimumAllocationSize = numberPagesRequired * pageGranularitySize;

	LPVOID heapPointer = VirtualAlloc(0, minimumAllocationSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return heapPointer;

}

/**
 * Deallocates a region of heap-allocated memory.
 * 
 * @param heapPtr The pointer to the starting address of the heap buffer returned
 * by virtualHeapAllocate().
 */
void
virtualHeapFree(void* heapPtr)
{
	VirtualFree((LPVOID)heapPtr, NULL, MEM_RELEASE);
}