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
#include <stdbool.h>

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

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	heap->alloc_strat = allocStrat;
}

AllocStrategy os_getAllocationStrategy(Heap const *heap){
	return heap->alloc_strat;
}

bool isValidNibble(MemValue value){
	return (value >= 0x0 && value <= 0xF);
}

bool isValidUseAddress(Heap const *heap, MemAddr addr){
	return (addr >= os_getUseStart(heap) && addr < os_getUseStart(heap) + os_getUseSize(heap));
}

bool isValidMapAdress(Heap const *heap, MemAddr addr){
	return (addr >= os_getMapStart(heap) && addr < os_getUseStart(heap));
}

bool isValidAddress(Heap const *heap, MemAddr addr){
	return (isValidUseAddress(heap, addr) && isValidMapAdress(heap,addr));
}

void setLowNibble (Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidAddress(heap, addr) || !isValidNibble(value)){
		return;
	}
	os_enterCriticalSection();
	MemValue new_value = heap->driver->read(addr);
	new_value &= 0b11110000;
	new_value += value;
	heap->driver->write(addr, new_value);
	os_leaveCriticalSection();
}

void setHighNibble (Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidAddress(heap, addr) || !isValidNibble(value)){
		return;
	}
	os_enterCriticalSection();
	MemValue new_value = heap->driver->read(addr);
	new_value &= 0b00001111;
	new_value += (value << 4);
	heap->driver->write(addr, new_value);
	os_leaveCriticalSection();
}	

MemValue getLowNibble (Heap const *heap, MemAddr addr){
	if (isValidAddress(heap, addr)){
	    return (heap->driver->read(addr) & 0b00001111);
	} else return 0; // 0 is not a good idea as it doesn't indicate an invalid address
}

MemValue getHighNibble (Heap const *heap, MemAddr addr){
	if (isValidAddress(heap, addr)){
	    return ((heap->driver->read(addr) & 0b11110000) >> 4);
	} else return 0;
}

void setMapEntry(Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidNibble(value) || !isValidMapAdress(heap, addr)){
		return;
	}
	os_enterCriticalSection();
	if(addr % 2 == 0){
	    setHighNibble(heap, addr, value);
	}
	else {
	    setLowNibble(heap, addr, value);
	}
	os_leaveCriticalSection();
}

MemValue os_getMapEntry(Heap const *heap, MemAddr addr){
	if(!isValidMapAdress(heap, addr)){
		return 0;
	}
	MemAddr map_entry_byte = os_getMapStart(heap) + (addr - os_getUseStart(heap)) / 2;
	if(addr % 2 == 0) return getHighNibble(heap, map_entry_byte);
	else return getLowNibble(heap, map_entry_byte);
}

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr){
	if(!isValidUseAddress()){
		return 0;
	}
	MemValue addr_map_entry = os_getMapEntry(heap, addr);
	MemAddr iter_addr = addr;
	if (addr_map_entry == 0){
		while (os_getMapEntry(iter_addr) == 0 && isValidUseAddress(iter_addr)){
			iter_addr--;
		}
		return iter_addr + 1;
	} else {
		while (os_getMapEntry(iter_addr) == 0xF && isValidUseAddress(iter_addr)){
			iter_addr--;
		}
		return iter_addr;
	}
}

uint16_t os_getChunkSize(Heap const *heap, MemAddr addr){
	if(!isValidUseAddress(heap, addr)){
		return 0;
	}
	MemAddr first_chunk_addr = os_getFirstByteOfChunk(heap, addr); 
	MemAddr iter_addr = first_chunk_addr;
	uint16_t chunk_size = 0;
	MemValue addr_map_entry = os_getMapEntry(heap, addr);
	if(addr_map_entry != 0){ 
	    while ((os_getMapEntry(iter_addr) == 0xF || os_getMapEntry(iter_addr) == os_getMapEntry(first_chunk_addr))
		       && isValidUseAddress(iter_addr)){
	        chunk_size++;
		    iter_addr++;
	    }
	    return chunk_size;
	} else {
		while (isValidUseAddress(iter_addr) && os_getMapEntry(iter_addr) == 0){ 
			chunk_size++;
			iter_addr++;
		}
		return chunk_size;
	}
}

void os_freeOwnerRestricted (Heap *heap, MemAddr addr, ProcessID owner);

MemAddr os_malloc(Heap *heap, size_t size);

void os_free(Heap *heap, MemAddr addr);

void os_freeProcessMemory(Heap *heap, ProcessID pid);

ProcessID getOwnerOfChunk(Heap const *heap, MemAddr addr);