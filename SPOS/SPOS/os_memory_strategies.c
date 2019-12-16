/*
 * os_memory_strategies.c
 *
 * Created: 26.11.2019 11:44:23
 *  Author: iw851247
 */ 

#include "os_memory_strategies.h"
#include "os_memory.h"
#include <stdint-gcc.h>
#include <util.h>

#define verifyUseAddressWithError(HEAP, ADDR) (isValidUseAddressWithError(HEAP, ADDR, true))
#define verifyMapAddressWithError(HEAP, ADDR) (isValidMapAddressWithError(HEAP, ADDR, true))
#define isValidUseAddress(HEAP, ADDR) (isValidUseAddressWithError(HEAP, ADDR, false))
#define isValidMapAddress(HEAP, ADDR) (isValidMapAddressWithError(HEAP, ADDR, false))

/* The address that Next Fit should starts next time it is called.
 * For the first time Next First is called this variable is 0.
 * Every time Next Fit finds a valid address it will modify this variable.
 * If the modification were to make this variable outside of the valid use range,
 * it would instead be reset to 0. 
 * For implementation see function setNextFitStart.
 * I'm considering whether we should add another variable for the external heap.
 */
//static volatile MemAddr next_fit_start = 0;

MemAddr get_next_fit(Heap *heap, size_t size, MemAddr start) {
	os_enterCriticalSection();
	
    MemAddr current_candidate = 0;

	for (MemAddr addr = start; isValidUseAddress(heap, addr); addr++) {		
        if (os_getMapEntry(heap, addr) == 0) {
            if (current_candidate == 0)
                current_candidate = addr;
        } else
            current_candidate = 0;

        if (addr - current_candidate + 1 >= size && current_candidate != 0) {
            os_leaveCriticalSection();
            return current_candidate;
        }
    } 
    os_leaveCriticalSection();
    return 0;
}

void setNextFitStart(Heap* heap, MemAddr last_time, uint16_t size){
	heap->last_next_fit = last_time + size;
	if(!isValidUseAddress(heap, heap->last_next_fit)){
		heap->last_next_fit = 0;
	}
}

MemAddr os_Memory_FirstFit(Heap *heap, size_t size) {
	return get_next_fit(heap, size, os_getUseStart(heap));
}

MemAddr os_Memory_NextFit(Heap *heap, size_t size) {
	os_enterCriticalSection();
	if(heap->last_next_fit == 0){ // Next Fit has never been called or we left off at the last chunk
		MemAddr addr = os_Memory_FirstFit(heap, size); // Next Fit degenerates to First Fit in this case
		if(addr != 0){
			/* setNextFitStart should only be called if there really is allocable memory
			 * to avoid inconsistency.
             */
		    setNextFitStart(heap, addr, size);
		}
		os_leaveCriticalSection();
		return addr;
	} else {
		MemAddr addr = get_next_fit(heap, size, heap->last_next_fit);
		if (addr > 0) {
			setNextFitStart(heap, addr, size);
			os_leaveCriticalSection();
			return addr;
		}
	}
	// If the code ever reaches here, it means that there's no free space
	// after the current next_fit_start can be found. The strategy degenerates
	// to First Fit.
	MemAddr addr = os_Memory_FirstFit(heap, size);
	if(addr != 0) {
	    setNextFitStart(heap, addr, size);
	}
	os_leaveCriticalSection();
	return addr;
}


MemAddr os_Memory_WorstFit(Heap *heap, size_t size){
	os_enterCriticalSection();
	volatile MemAddr addr = 0; // Where we store the current biggest chunk
	volatile uint16_t addr_chunk_size = 0; // Size of the current biggest chunk
	volatile MemAddr iter = os_getUseStart(heap);

	// Iterate with iter from heap use start
	// Store the temporary result in addr and its chunk size in addr_chunk_size
	while(isValidUseAddress(heap, iter)) {
		uint16_t iter_chunk_size = os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, iter, false, max(size, os_getUseSize(heap) / 2 + 1));
		// Looking for the biggest possible chunk
		if(os_getOwnerOfChunk(heap, iter) == 0 && iter_chunk_size >= size && iter_chunk_size > addr_chunk_size){ 
			addr = iter;
			addr_chunk_size = iter_chunk_size;
		} 
		iter += iter_chunk_size; // try the next chunk
	}
	os_leaveCriticalSection();
	return addr;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size){
	/* This is literally a copy and paste of Worst Fit.
	 * The only difference is a change from ">" to "<" 
	 * in the boolean expression inside the while loop
	 * and the default chunk size is set to maximum.
	 */
	os_enterCriticalSection();
	volatile MemAddr addr = 0; // Where we store the current smallest chunk
	volatile uint16_t addr_chunk_size = 0xFFFF; // Size of the current smallest chunk
	volatile MemAddr iter = os_getUseStart(heap);
	// Iterate with iter from heap use start
	// Store the temporary result in addr and its chunk size in addr_chunk_size
	while(isValidUseAddress(heap, iter)){
		uint16_t iter_chunk_size = os_getChunkSizeUnrestrictedWithZeroMaxSize(heap, iter, false, max(size, os_getUseSize(heap) / 2 + 1));
		if(os_getOwnerOfChunk(heap, iter) == 0 && iter_chunk_size >= size && iter_chunk_size < addr_chunk_size){ // Looking for the smallest possible chunk
			addr = iter;
			addr_chunk_size = iter_chunk_size;
		} 
		iter += iter_chunk_size; // try the next chunk
	}
	os_leaveCriticalSection();
	return addr;
}