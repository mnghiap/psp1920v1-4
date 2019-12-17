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
#include "lcd.h"
#include <stdbool.h>
#include <stdlib.h>

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
	if(addr % 2 == (os_getUseStart(heap)) % 2) {
	/* We passed Versuch 3 just because getUseStart of 
	 * the internal heap happens to be an even number. 
	 * This is unfortunately not the case for the external heap.
	 * This fix also applies for setMapEntry. */
		return getHighNibble(heap, map_entry_byte);
	} else 
		return getLowNibble(heap, map_entry_byte);
}

void os_setMapEntry(Heap const *heap, MemAddr addr, MemValue value){
	if(!isValidNibble(value) || !verifyUseAddressWithError(heap, addr))
		return;
	 
	os_enterCriticalSection();
	MemAddr map_entry_byte = os_getMapStart(heap) + (addr - os_getUseStart(heap)) / 2;
	if(addr % 2 == (os_getUseStart(heap)) % 2) 
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
    return os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, addr, true, -1);
}

uint16_t os_getChunkSizeUnrestrictedWithZeroMaxSize(Heap const *heap, MemAddr addr, bool is_restricted, uint16_t max_size) {
	if(!verifyUseAddressWithError(heap, addr))
		return 0;
	 
    os_enterCriticalSection();
	MemAddr first_chunk_addr = os_getFirstByteOfChunk(heap, addr); 
	MemAddr iter_addr = first_chunk_addr;
	uint16_t chunk_size = 0;
	MemValue look_for = os_getMapEntry(heap, first_chunk_addr) ? 0xF : 0;
	
	if (is_restricted && look_for == 0)
		return 0;
    if (look_for != 0)
        max_size = -1;

	do {
	    chunk_size++;
		iter_addr++;
	} while (isValidUseAddress(heap, iter_addr) && chunk_size < max_size && os_getMapEntry(heap, iter_addr) == look_for);
    os_leaveCriticalSection();
	return chunk_size;
}

ProcessID os_getOwnerOfChunk(Heap const *heap, MemAddr addr){
	if(!verifyUseAddressWithError(heap, addr)){
		return 0;
	} else {
		return os_getMapEntry(heap, os_getFirstByteOfChunk(heap, addr));
	}
}

void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner, bool handle_frame){
    if (!verifyUseAddressWithError(heap, addr))
        return; 
         
    os_enterCriticalSection();
    if (os_getOwnerOfChunk(heap, addr) != owner) {
        os_errorPStr(PSTR("free: proc. not own. of heap"));
		os_leaveCriticalSection();
        return;
    }        
   
    // settings them here, 'cause otherwise os_... would be executed every iteration  
    MemAddr first_byte = os_getFirstByteOfChunk(heap, addr);
    MemAddr end_byte = os_getChunkSize(heap, addr) + first_byte - 1;
    for (MemAddr cur_byte = first_byte; cur_byte <= end_byte; cur_byte++) {
        os_setMapEntry(heap, cur_byte, 0);
    }
	
	/* The parameter handle_frame specifies whether 
	 * this function should adjust the alloc. frame.
	 *
	 * We'll make use of the fact that
	 * allocFrameStart[owner] <= first_byte and 
	 * allocFrameEnd[owner] >= end_byte. Furthermore
	 * the frame start and frame end have valid USE values.
	 */
	if(handle_frame){
		if(heap->allocFrameStart[owner] == first_byte && heap->allocFrameEnd[owner] == end_byte){
			heap->allocFrameStart[owner] = 0;
			heap->allocFrameEnd[owner] = 0;
		} else if(heap->allocFrameStart[owner] < first_byte && heap->allocFrameEnd[owner] == end_byte){
			// Iterate from the end till we find the frame end. 
			for(MemAddr frame_end = os_getUseStart(heap) + os_getUseSize(heap) - 1;
			frame_end >= heap->allocFrameStart[owner]; ){
				if(os_getOwnerOfChunk(heap, frame_end) == owner){
					heap->allocFrameEnd[owner] = frame_end;
					break;
				}
				frame_end -= os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, frame_end, false, -1);
			}
		} else if(heap->allocFrameStart[owner] == first_byte && heap->allocFrameEnd[owner] > end_byte){
			// Iterate from the start till we find the frame start.
			for(MemAddr frame_start = os_getUseStart(heap); 
					frame_start <= heap->allocFrameEnd[owner]; ){
				if(os_getOwnerOfChunk(heap, frame_start) == owner){
					heap->allocFrameStart[owner] = frame_start;
					break;
				}
				frame_start += os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, frame_start, false, -1);
			}
		}
	}
	// We don't need to handle the last case as nothing changes
    os_leaveCriticalSection();
}

void os_memcpy(Heap *heap_from, MemAddr from, Heap *heap_to, MemAddr to, uint16_t n) {
    ProcessID owner = os_getCurrentProc();
    if (os_getOwnerOfChunk(heap_from, from) != owner 
            || os_getOwnerOfChunk(heap_to, to) != owner) {
        os_errorPStr(PSTR("memcpy: proc. not own."));
        return;
    }

	if (heap_from == heap_to && from == to)
		return;

    os_enterCriticalSection();
    for (uint16_t i = 0; i < n; i++) {
        heap_to->driver->write(
		    to + i, 
            heap_from->driver->read(from)
        );
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
		//os_errorPStr(PSTR("malloc: No free space found"));
		os_leaveCriticalSection();
		return 0;
	}
	
	for (MemAddr iter = addr; iter < addr + size; iter++) {
	//	if (os_getMapEntry(heap, iter) != 0){
	//		os_error("Malloc: overwriten data");
	//	}
		os_setMapEntry(heap, iter, 0xF);
	}
	// define owner
	os_setMapEntry(heap, addr, os_getCurrentProc());
	MemAddr last_addr = addr + size - 1;
	
	// Handling alloc. frame of the process
	ProcessID pid = os_getCurrentProc();
	if(heap->allocFrameStart[pid] == 0 && heap->allocFrameEnd[pid] == 0){ // No allocated memory yet
		heap->allocFrameStart[pid] = addr;
		heap->allocFrameEnd[pid] = last_addr;
	} else { // There's already memory allocated to the process
		heap->allocFrameStart[pid] = min(heap->allocFrameStart[pid], addr);
		heap->allocFrameEnd[pid] = max(heap->allocFrameEnd[pid], last_addr);
	}
	os_leaveCriticalSection();
	return addr;
}

MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size) {
	os_enterCriticalSection();
	
	addr = os_getFirstByteOfChunk(heap, addr);
	MemAddr first_addr = addr;
	ProcessID owner = os_getOwnerOfChunk(heap, addr);
	
	if (owner != os_getCurrentProc()) {
		os_error("realloc: Mem. not from cur. proc.");
		os_leaveCriticalSection();
		return 0;
	}
	
	uint16_t cur_size = os_getChunkSize(heap, addr);
	if (size == cur_size) {
		os_leaveCriticalSection();
		return addr;
	} else if (size < cur_size) {  // shrinking
		volatile uint16_t missing = cur_size - size;
		for (uint16_t i = 1; i <= missing; i++)
			os_setMapEntry(heap, addr + cur_size - i, 0x0);
	}  else {
		volatile uint16_t missing = size - cur_size;
		volatile uint16_t to_left, to_right = to_left = 0;
		volatile bool move = false;
		
		{
			uint16_t r_size = 0;
			if (isValidUseAddress(heap, addr + cur_size) && os_getMapEntry(heap, addr + cur_size) == 0)
				r_size = os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, addr + cur_size, false, missing);
				
			if (r_size >= missing)  // enough room to the right 
				to_right = missing;
			else {
				uint16_t l_size = 0;
				if (isValidUseAddress(heap, addr - 1) && os_getMapEntry(heap, addr - 1) == 0)
					l_size = os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, addr - 1, false, -1);
				
				if (l_size >= missing) {
					to_left = l_size;
				} else if (l_size + r_size >= missing) {
					to_left = l_size;
					to_right = missing - l_size;
				} else  // need to move
					move = true;					
			}
		}
			
		
		if (!move) {
			for (uint16_t i = 0; i < to_left; i++)
				os_setMapEntry(heap, addr - i, 0xF);
			os_setMapEntry(heap, addr - to_left, owner);
			first_addr -= to_left;
			
			os_memcpy(heap, addr, heap, first_addr, cur_size);
		
			if (to_left >= missing)	{ // need to remove smt.
				for (uint16_t i = 0; i < addr + cur_size - (first_addr + size); i++) {
					os_setMapEntry(heap, first_addr + size + i, 0x0);
				}
			} else {
				for (uint16_t i = 0; i < to_right; i++)
					os_setMapEntry(heap, addr + cur_size + i, 0xF);
			}	
		} else {
			// move chunk to new allocation
			first_addr = os_malloc(heap, size);
			os_memcpy(heap, addr, heap, first_addr, cur_size);
			os_free(heap, addr);
		}
	}
	
	// cases:
	//   1. new is in front of old
	//   2. old is in front of new -> search
	if (first_addr <= addr)
		heap->allocFrameStart[owner] = first_addr;
	else {
		MemAddr iter = addr;
		while (isValidUseAddress(iter) && os_getMapEntry(heap, iter) != owner) {iter++;}
		heap->allocFrameStart[owner] = iter;
	}
	
	if (first_addr + size >= addr + cur_size)
		heap->allocFrameEnd[owner] = first_addr + size;
	else {
		MemAddr iter = addr + cur_size;
		while (isValidUseAddress(iter) && os_getMapEntry(heap, iter) != owner) {iter--;}
		heap->allocFrameEnd[owner] = iter;
	}
    
    os_leaveCriticalSection();
    return first_addr;
}

void os_free(Heap *heap, MemAddr addr){
    os_freeOwnerRestricted(heap, addr, os_getCurrentProc(), true);
	// os_freeOwnerRestricted modifies the alloc. frames by itself
}

void os_freeProcessMemory(Heap *heap, ProcessID pid){
	os_enterCriticalSection();
	if(heap->allocFrameStart[pid] != 0 && heap->allocFrameEnd[pid] != 0){	
		for (MemAddr addr = heap->allocFrameStart[pid]; addr <= heap->allocFrameEnd[pid]; addr++) {
			if (os_getMapEntry(heap, addr) == pid)
				os_freeOwnerRestricted(heap, addr, pid, false);	
		}	
	}
	heap->allocFrameStart[pid] = 0;
	heap->allocFrameEnd[pid] = 0;
	os_leaveCriticalSection();
}