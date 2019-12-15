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
		os_leaveCriticalSection();
        return;
    }        
   
    // settings them here, 'cause otherwise os_... would be executed every iteration  
    MemAddr first_byte = os_getFirstByteOfChunk(heap, addr);
    MemAddr end_byte = os_getChunkSizeUnrestricted(heap, addr, false) + first_byte - 1;
    for (MemAddr cur_byte = first_byte; cur_byte <= end_byte; cur_byte++) {
        os_setMapEntry(heap, cur_byte, 0);
    }
	
	/* We'll make use of the fact that
	 * allocFrameStart[owner] <= first_byte and 
	 * allocFrameEnd[owner] >= end_byte. Furthermore
	 * the frame start and frame end have valid USE values.
	 */
	if(heap->allocFrameStart[owner] == first_byte && heap->allocFrameEnd[owner] == end_byte){
		heap->allocFrameStart[owner] = 0;
		heap->allocFrameEnd[owner] = 0;
	}
	if(heap->allocFrameStart[owner] < first_byte && heap->allocFrameEnd[owner] == end_byte){
		// Iterate from the end till we find the frame end. 
		for(MemAddr frame_end = first_byte - 1; frame_end >= heap->allocFrameStart[owner]; frame_end--){
			if(os_getOwnerOfChunk(heap, frame_end) == owner){
				heap->allocFrameEnd[owner] = frame_end;
				break;
			}
		}
	}
	if(heap->allocFrameStart[owner] == first_byte && heap->allocFrameEnd[owner] > end_byte){
		// Iterate from the start till we find the frame start.
		for(MemAddr frame_start = end_byte + 1; frame_start <= heap->allocFrameEnd[owner]; frame_start++){
			if(os_getOwnerOfChunk(heap, frame_start) == owner){
				heap->allocFrameStart[owner] = frame_start;
				break;
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
		if (os_getMapEntry(heap, iter) != 0){
			os_error("Malloc: overwriten data");
		}
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
    uint16_t cur_size = os_getChunkSize(heap, addr);
    if (cur_size == size || addr <= 0)
        return 0;

    ProcessID owner = os_getCurrentProc();
    if (owner != os_getOwnerOfChunk(heap, addr)) {
        os_errorPStr("realloc: proc. not own.");
        return 0;
    }

    os_enterCriticalSection();
    MemAddr first_addr = os_getFirstByteOfChunk(heap, addr);
    if (size < cur_size) {  // shrinking
        uint16_t missing = cur_size - size;
        MemAddr last = first_addr + size;
        for (uint16_t i =0; i<missing; i++)
            os_setMapEntry(heap, last + i, 0x0);
    } else {  // enlarging
        uint16_t missing = size - cur_size;
        MemAddr start_a = first_addr;
        MemAddr end_a = first_addr + size; 
        uint16_t to_left, to_right = to_left = 0;
        bool move = false;

        {
            uint16_t l_size = os_getChunkSizeUnrestricted(heap, first_addr - 1, false);
            uint16_t r_size = os_getChunkSizeUnrestricted(heap, first_addr + cur_size, false);
            if (r_size >= missing)  // enough room to right
                to_right = missing;
            else if (l_size >= missing)  // enough room to left
                to_left = missing;
            else if (l_size + r_size >= missing) {  // enough room to left+right
                to_left = l_size;
                to_right = missing - l_size;
            } else  // need to move
                move = true;
        }

        if (!move) {
            // expand left
            for (uint16_t i = 0; i < to_left; i++)
                os_setMapEntry(heap, start_a - i, 0xF);
            os_setMapEntry(heap, start_a-to_left, owner);
            first_addr = start_a-to_left;

            // expand right
            for (uint16_t i = 0; i < to_right; i++)
                os_setMapEntry(heap, end_a + i, 0xF);
        } else {
            MemAddr addr = os_malloc(heap, size);
            os_memcpy(heap, first_addr, heap, addr, cur_size);
            first_addr = addr;
        }
    }
	/* There are just too many (edge) cases here to consider.
	 * Iterating from both ends of the heap works, although it's
	 * not the most efficient solution. */
	for(MemAddr frame_start = heap->use_start; frame_start <= first_addr; ){
		if(os_getOwnerOfChunk(heap, frame_start) == owner){ 
			heap->allocFrameStart[owner] = frame_start;
			break;
		}
		frame_start += os_getChunkSizeUnrestricted(heap, frame_start, false);
	}
	
	for(MemAddr frame_end = heap->use_start + heap->use_size - 1; frame_end >= first_addr; ){
		if(os_getOwnerOfChunk(heap, frame_end) == owner){ 
			heap->allocFrameEnd[owner] = frame_end;
			break;
		}
		frame_end -= os_getChunkSizeUnrestricted(heap, frame_end, false);
	}
    
    os_leaveCriticalSection();
    return first_addr;
}

void os_free(Heap *heap, MemAddr addr){
    os_freeOwnerRestricted(heap, addr, os_getCurrentProc());
	// os_freeOwnerRestricted modifies the alloc. frames by itself
}

void os_freeProcessMemory(Heap *heap, ProcessID pid){
	os_enterCriticalSection();
	if(heap->allocFrameStart[pid] != 0 && heap->allocFrameEnd[pid] != 0){	
		for (MemAddr addr = heap->allocFrameStart[pid]; addr <= heap->allocFrameEnd[pid]; addr++) {
			if (os_getMapEntry(heap, addr) == pid)
				os_freeOwnerRestricted(heap, addr, pid);	
		}	
	}
	// No need to manually handle allocFrameStart or -End here
	// freeOwnerRestricted does that in every loop iteration
	os_leaveCriticalSection();
}