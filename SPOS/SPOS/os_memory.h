/*
 * os_memory.h
 *
 * Created: 26.11.2019 17:54:40
 *  Author: iw851247
 */ 


#ifndef OS_MEMORY_H_
#define OS_MEMORY_H_

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"

#define getLowNibble(HEAP, ADDR) (HEAP->driver->read(ADDR) & 0x0F);
#define getHighNibble(HEAP, ADDR) ((HEAP->driver->read(ADDR) & 0xF0) >> 4);

bool isValidUseAddressWithError(Heap const *heap, MemAddr addr, bool break_on_error);
bool isValidMapAddressWithError(Heap const *heap, MemAddr addr, bool break_on_error);

#define verifyUseAddressWithError(HEAP, ADDR) (isValidUseAddressWithError(HEAP, ADDR, true));
#define verifyMapAddressWithError(HEAP, ADDR) (isValidMapAddressWithError(HEAP, ADDR, true));
#define isValidUseAddress(HEAP, ADDR) (isValidUseAddressWithError(HEAP, ADDR, false));
#define isValidMapAddress(HEAP, ADDR) (isValidMapAddressWithError(HEAP, ADDR, false));

MemAddr os_malloc(Heap *heap, size_t size);

void os_free(Heap *heap, MemAddr addr);

MemValue os_getMapEntry(Heap const *heap, MemAddr addr);
void os_setMapEntry(Heap const *heap, MemAddr addr, MemValue value);

void os_freeProcessMemory(Heap *heap, ProcessID pid);

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr);
uint16_t os_getChunkSize(Heap const *heap, MemAddr addr);
uint16_t os_getChunkSizeUnrestricted(Heap const *heap, MemAddr addr, bool do_restricted);


ProcessID os_getOwnerOfChunk(Heap const *heap, MemAddr addr);

inline size_t os_getMapSize(Heap const* heap) { return heap->map_size; }
inline size_t os_getUseSize(Heap const* heap) { return heap->use_size; }
inline MemAddr os_getMapStart(Heap const* heap) { return heap->map_start; }
inline MemAddr os_getUseStart(Heap const* heap) { return heap->use_start; }

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr);

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

AllocStrategy os_getAllocationStrategy(Heap const *heap);
#endif /* OS_MEMORY_H_ */