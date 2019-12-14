/*
 * os_memheap_drivers.h
 *
 * Created: 26.11.2019 11:43:27
 *  Author: iw851247
 */ 


#ifndef OS_MEMHEAP_DRIVERS_H_
#define OS_MEMHEAP_DRIVERS_H_

#include "os_mem_drivers.h"
#include <stddef.h>

typedef enum{
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;

#define DEFAULT_ALLOCATION_STRATEGY OS_MEM_FIRST

typedef struct Heap {
	MemDriver* driver;
	AllocStrategy alloc_strat;
	const MemAddr map_start;
	const size_t map_size;
	const MemAddr use_start;
	const size_t use_size;
	const char* name;
} Heap;

void os_initHeaps(void);

Heap* os_lookupHeap(uint8_t index);

size_t os_getHeapListLength(void);

Heap intHeap__;
Heap extHeap__;

#ifndef intHeap
#define intHeap (&intHeap__)
#endif

#ifndef extHeap
#define extHeap (&extHeap__)
#endif

#endif /* OS_MEMHEAP_DRIVERS_H_ */