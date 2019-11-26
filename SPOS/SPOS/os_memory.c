/*
 * os_memory.c
 *
 * Created: 26.11.2019 17:53:04
 *  Author: iw851247
 */ 

#include "os_memory.h"
#include "os_memory_strategies.h"
#include "util.h"
#include "os_core.h"

void setLowNibble (Heap const *heap, MemAddr addr, MemValue value){
	MemValue new_value = heap->driver->read(addr);
	new_value &= 0b11110000;
	new_value += value;
	heap->driver->write(addr, new_value);
}

void setHighNibble (Heap const *heap, MemAddr addr, MemValue value){
	MemValue new_value = heap->driver->read(addr);
	new_value &= 0b00001111;
	new_value += (value << 4);
	heap->driver->write(addr, new_value);
}	

MemValue getLowNibble (Heap const *heap, MemAddr addr){
	return (heap->driver->read(addr) & 0b00001111);
}

MemValue getHighNibble (Heap const *heap, MemAddr addr){
	return ((heap->driver->read(addr) & 0b11110000) >> 4);
}

MemAddr os_malloc(Heap *heap, size_t size);

void os_free(Heap *heap, MemAddr addr);

void setMapEntry(Heap const *heap, MemAddr addr, MemValue value){
	setHighNibble(heap, addr, value >> 4);
	setLowNibble(heap, addr, value & 0b00001111);
}

MemValue os_getMapEntry(Heap const *heap, MemAddr addr){
	return heap->driver->read(addr);
}

void os_freeProcessMemory(Heap *heap, ProcessID pid);

ProcessID getOwnerOfChunk(Heap const *heap, MemAddr addr);

size_t os_getMapSize(Heap const *heap){
	return heap->map_size;
}

size_t os_getUseSize(Heap const *heap){
	return heap->use_size;
}

MemAddr os_getMapStart(Heap const *heap){
	return heap->map_start;
}

MemAddr os_getUseStart(Heap const *heap){
	return heap->use_start;
}

uint16_t os_getChunkSize(Heap const *heap, MemAddr addr);

void os_freeOwnerRestricted (Heap *heap, MemAddr addr, ProcessID owner);

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr);

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	heap->mem_strat = allocStrat;
}

void os_freeProcessMemory (Heap *heap, ProcessID pid);
