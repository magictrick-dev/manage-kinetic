#ifndef MANAGE_KINETIC_MEMORY_WIN32_H
#define MANAGE_KINETIC_MEMORY_WIN32_H
#include <managekinetic/primitives.h>

void* virtualHeapAllocate(ui64 size);
void virtualHeapFree(void* heapPtr);

#endif