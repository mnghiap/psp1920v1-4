/*
 * os_memory_strategies.h
 *
 * Created: 26.11.2019 11:44:56
 *  Author: iw851247
 */ 


#ifndef OS_MEMORY_STRATEGIES_H_
#define OS_MEMORY_STRATEGIES_H_

#include "os_memheap_drivers.h"

MemAddr os_Memory_FirstFit(Heap *heap, size_t size);

MemAddr os_Memory_NextFit(Heap *heap, size_t size);

MemAddr os_Memory_WorstFit(Heap *heap, size_t size);

MemAddr os_Memory_BestFit(Heap *heap, size_t size);

#endif /* OS_MEMORY_STRATEGIES_H_ */