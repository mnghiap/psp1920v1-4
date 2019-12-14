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
#include "defines.h"

typedef enum{
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;

#define DEFAULT_ALLOCATION_STRATEGY OS_MEM_FIRST

/* About the allocFrameStart and allocFrameEnd arrays:
 * If both are equal 0, this implies that there is no
 * memory allocated to the process. We will make use of 
 * this implication in later interactions with heaps.
 * Convention: allocFrameEnd[pid] is the last valid byte
 * of Process pid in the heap. Functions that make direct 
 * use of these arrays are malloc, freeOwnerRestricted,
 * freeProcessMemory and realloc.
 */
typedef struct Heap {
	MemDriver* driver;
	AllocStrategy alloc_strat;
	const MemAddr map_start;
	const size_t map_size;
	const MemAddr use_start;
	const size_t use_size;
	const char* name;
	MemAddr allocFrameStart[MAX_NUMBER_OF_PROCESSES];
	MemAddr allocFrameEnd[MAX_NUMBER_OF_PROCESSES];
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