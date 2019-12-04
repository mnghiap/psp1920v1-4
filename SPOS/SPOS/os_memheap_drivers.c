/*
 * os_memheap_drivers.c
 *
 * Created: 26.11.2019 11:43:11
 *  Author: iw851247
 */ 

#include "os_memheap_drivers.h"
#include "os_mem_drivers.h"
#include <avr/pgmspace.h>
#include "atmega644constants.h"
#include "defines.h"

const PROGMEM char intStr[] = "internal";

Heap* heap_list[] = {intHeap}; // We put the heaps in an array for the functions in this file

#define HEAP_MEMORY (AVR_MEMORY_SRAM / 2 - HEAPOFFSET)
#define HEAP_START (AVR_SRAM_START + HEAPOFFSET)

Heap intHeap__ = {
	.driver = intSRAM,
	.alloc_strat = OS_MEM_FIRST, // default strategy
	.map_start = HEAP_START,
	.map_size = HEAP_MEMORY/ 3,
	.use_start = HEAP_START + HEAP_MEMORY / 3, // practically .map_start + .map_size
	.use_size = HEAP_MEMORY - HEAP_MEMORY / 3,
	.name = intStr, // as per the Doxygen documentation
};

void os_initHeaps(void){
	for(uint8_t i = 0; i < os_getHeapListLength() - 1; i++){
		Heap current_heap = *heap_list[i];
		for(MemAddr addr = current_heap.map_start; 
		    addr < current_heap.map_start + current_heap.map_size;
			addr++){
			current_heap.driver->write(addr, 0);
		}
	}
}

Heap* os_lookupHeap(uint8_t index){
	return heap_list[index];
}

size_t os_getHeapListLength (void){
	return sizeof(heap_list) / sizeof(Heap*);
}