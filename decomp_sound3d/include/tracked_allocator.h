// tracked_allocator.h : the tracked allocator of sound3d.dll (sound_init.cpp) and the hook pointers it installs.
//
// A table of 4096 block pointers with a 16 byte label each; the engine's helper code (and the libraries linked into
// the DLL) allocate through g_allocateHook and release through g_freeHook, so a leak report can be written.
#ifndef TRACKED_ALLOCATOR_H
#define TRACKED_ALLOCATOR_H

#include <stddef.h>

const int ALLOC_TABLE_SIZE = 0x1000;       // tracked blocks
const int ALLOC_NAME_SIZE = 0x10;          // label kept per block
const unsigned ALLOC_MAX_BYTES = 100000000;

extern void *g_allocTable[ALLOC_TABLE_SIZE];                       // 0x10283188
extern char g_allocNames[ALLOC_TABLE_SIZE][ALLOC_NAME_SIZE];       // 0x10287290
extern void *(*g_memcopyHook)(void *, const void *, size_t);       // 0x10282d80
extern int g_allocCounters[5];                                      // 0x10282d88
extern void *(*g_allocateHook)(unsigned bytes, const char *label); // 0x10287188, AllocateNamed once installed
extern char g_allocFlag;                                            // 0x10287190
extern void (*g_freeHook)(void *block);                             // 0x10297290, TrackedFree once installed
extern char g_allocUnknownLabel[8];                                 // 0x10297298

void InitTrackedAllocator(void);
void *AllocateNamed(unsigned size, const char *label);
void TrackedFree(void *block);
void DumpTrackedAllocations(const char *path);

#endif
