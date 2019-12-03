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

#define os_getMapSize(HEAP) (HEAP->map_size)
#define os_getUseSize(HEAP) (HEAP->use_size)
#define os_getMapStart(HEAP) (HEAP->map_start)
#define os_getUseStart(HEAP) (HEAP->use_start)

MemAddr os_malloc(Heap *heap, size_t size);

void os_free(Heap *heap, MemAddr addr);

MemValue os_getMapEntry(Heap const *heap, MemAddr addr);

void os_freeProcessMemory(Heap *heap, ProcessID pid);

uint16_t os_getChunkSize(Heap const *heap, MemAddr addr);

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr);

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

AllocStrategy os_getAllocationStrategy(Heap const *heap);
#endif /* OS_MEMORY_H_ */