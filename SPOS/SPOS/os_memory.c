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

#define isValidNibble(VALUE) (VALUE >= 0x0 && VALUE <= 0xF)

/************************************************************************/
/* Check whether given address is inside the valid heap memory range.   */
/************************************************************************/
bool isValidUseAddressWithError(Heap const *heap, MemAddr addr, bool break_on_error) {
	bool res = (addr >= os_getUseStart(heap) && addr < os_getUseStart(heap) + os_getUseSize(heap));
	if (!res && break_on_error) 
		os_error("addr. outside USE range");
	return res;
}

bool isValidMapAddressWithError(Heap const *heap, MemAddr addr, bool break_on_error) {
	bool res = (addr >= os_getMapStart(heap) && addr < os_getMapStart(heap) + os_getMapSize(heap));
	if (!res && break_on_error) 
		os_error("addr. outside MAP range");
	return res;
}

#define verifyUseAddressWithError(HEAP, ADDR) (isValidUseAddressWithError(HEAP, ADDR, true))
#define verifyMapAddressWithError(HEAP, ADDR) (isValidMapAddressWithError(HEAP, ADDR, true))
#define isValidUseAddress(HEAP, ADDR) (isValidUseAddressWithError(HEAP, ADDR, false))
#define isValidMapAddress(HEAP, ADDR) (isValidMapAddressWithError(HEAP, ADDR, false))

#define isValidAddress(HEAP, ADDR) (isValidUseAddress(HEAP, ADDR) || isValidMapAddress(HEAP,ADDR))

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	heap->alloc_strat = allocStrat;
}

AllocStrategy os_getAllocationStrategy(Heap const *heap){
	return heap->alloc_strat;
}

void setLowNibble (Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidAddress(heap, addr) || !isValidNibble(value))
		return;
	 
	os_enterCriticalSection();
	MemValue new_value = heap->driver->read(addr);
	new_value &= 0b11110000;
	new_value += value;
	heap->driver->write(addr, new_value);
	os_leaveCriticalSection();
}

void setHighNibble (Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidAddress(heap, addr) || !isValidNibble(value))
		return;
	 
	os_enterCriticalSection();
	MemValue new_value = heap->driver->read(addr);
	new_value &= 0b00001111;
	new_value += (value << 4);
	heap->driver->write(addr, new_value);
	os_leaveCriticalSection();
}

MemValue os_getMapEntry(Heap const *heap, MemAddr addr){
	if(!verifyUseAddressWithError(heap, addr))
		return 0;
	
	MemAddr map_entry_byte = os_getMapStart(heap) + (addr - os_getUseStart(heap)) / 2;
	if(addr % 2 == 0) {
		return getHighNibble(heap, map_entry_byte);
	} else 
		return getLowNibble(heap, map_entry_byte);
}

void os_setMapEntry(Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidNibble(value) || !verifyUseAddressWithError(heap, addr))
		return;
	 
	os_enterCriticalSection();
	MemAddr map_entry_byte = os_getMapStart(heap) + (addr - os_getUseStart(heap)) / 2;
	if(addr % 2 == 0)
	    setHighNibble(heap, map_entry_byte, value);
	else
	    setLowNibble(heap, map_entry_byte, value);
	 
	os_leaveCriticalSection();
}

MemAddr os_getFirstByteOfChunk(Heap const *heap, MemAddr addr){
	if(!verifyUseAddressWithError(heap, addr))
		return 0;
	 
	MemValue addr_map_entry = os_getMapEntry(heap, addr);
	MemAddr iter_addr = addr;
	if (addr_map_entry == 0){
		while (isValidUseAddress(heap, iter_addr) && os_getMapEntry(heap, iter_addr) == 0){
			iter_addr--;
		}
		return iter_addr + 1;
	} else {
		while (isValidUseAddress(heap, iter_addr) && os_getMapEntry(heap, iter_addr) == 0xF){
			iter_addr--;
		}
		return iter_addr;
	}
}

uint16_t os_getChunkSize(Heap const *heap, MemAddr addr){
	return os_getChunkSizeUnrestricted(heap, addr, true);
}

uint16_t os_getChunkSizeUnrestricted(Heap const *heap, MemAddr addr, bool is_restricted) {
	if(!verifyUseAddressWithError(heap, addr))
		return 0;
	 
	MemAddr first_chunk_addr = os_getFirstByteOfChunk(heap, addr); 
	MemAddr iter_addr = first_chunk_addr;
	uint16_t chunk_size = 0;
	MemValue look_for = os_getMapEntry(heap, first_chunk_addr) ? 0xF : 0;
	
	if (is_restricted && look_for == 0)
		return 0;
	
	chunk_size++;
	iter_addr++;
	while (isValidUseAddress(heap, iter_addr) && os_getMapEntry(heap, iter_addr) == look_for){
	    chunk_size++;
		iter_addr++;
	}
	return chunk_size;
}

ProcessID os_getOwnerOfChunk(Heap const *heap, MemAddr addr){
	if(!verifyUseAddressWithError(heap, addr)){
		return 0;
	} else {
		return os_getMapEntry(heap, os_getFirstByteOfChunk(heap, addr));
	}
}

void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner){
    if (!verifyUseAddressWithError(heap, addr))
        return; 
         
    os_enterCriticalSection();
    if (os_getOwnerOfChunk(heap, addr) != owner) {
        os_errorPStr(PSTR("free: proc. not own. of heap"));
        return;
    }        
   
    // settings them here, 'cause otherwise os_... would be executed every iteration  
    MemAddr first_byte = os_getFirstByteOfChunk(heap, addr);
    MemAddr end_byte = os_getChunkSize(heap, addr) + first_byte;
    for (MemAddr cur_byte = first_byte; cur_byte < end_byte; cur_byte++) {
        os_setMapEntry(heap, cur_byte, 0);
    }
    os_leaveCriticalSection();
}

MemAddr os_malloc(Heap *heap, size_t size){
	os_enterCriticalSection();
	MemAddr addr = 0;
	switch(heap->alloc_strat){
		case OS_MEM_FIRST: addr = os_Memory_FirstFit(heap, size); break;	
		case OS_MEM_NEXT: addr = os_Memory_NextFit(heap, size); break;	
		case OS_MEM_BEST: addr = os_Memory_BestFit(heap, size); break;	
		case OS_MEM_WORST: addr = os_Memory_WorstFit(heap, size); break;	
		default: return 0; break;
	}
	
	if (addr <= 0) {
		os_errorPStr(PSTR("malloc: No free space found"));
		os_leaveCriticalSection();
		return 0;
	}
	
	for (MemAddr iter = addr; iter < addr + size; iter++) {
		if (os_getMapEntry(heap, iter) != 0)
			os_error("Malloc: overwriten data");
		os_setMapEntry(heap, iter, 0xF);
	}
	// define owner
	os_setMapEntry(heap, addr, os_getCurrentProc());
	os_leaveCriticalSection();
	return addr;
}

void os_free(Heap *heap, MemAddr addr){
    os_freeOwnerRestricted(heap, addr, os_getCurrentProc());
}

void os_freeProcessMemory(Heap *heap, ProcessID pid){
    os_enterCriticalSection();
    for (MemAddr addr = os_getUseStart(heap); addr < os_getUseStart(heap) + os_getUseSize(heap); ) {
        uint16_t size = os_getChunkSizeUnrestricted(heap, addr, false);
        if (os_getOwnerOfChunk(heap, addr) == pid)
            os_freeOwnerRestricted(heap, addr, pid);
        addr += size;
    }
    os_leaveCriticalSection();
}