/*
 * os_memory_strategies.c
 *
 * Created: 26.11.2019 11:44:23
 *  Author: iw851247
 */ 

#include "os_memory_strategies.h"
#include "os_memory.h"
#include <stdint-gcc.h>

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
static volatile MemAddr next_fit_start = 0;

MemAddr os_Memory_FirstFit(Heap *heap, size_t size){
    os_enterCriticalSection();
    MemAddr start = os_getUseStart(heap);
    MemAddr end = os_getUseSize(heap) + os_getUseStart(heap);
    
    for (volatile MemAddr addr = start; addr < end; ) {
		uint16_t chunk_size = os_getChunkSizeUnrestricted(heap, addr, false);
        if (os_getOwnerOfChunk(heap, addr) == 0 && chunk_size >= size) {
            MemAddr free_space = os_getFirstByteOfChunk(heap, addr); 
            os_leaveCriticalSection();
            return free_space;
        }
		addr += chunk_size;
    } 
    os_leaveCriticalSection();
    return 0;
}

void setNextFitStart(Heap* heap, MemAddr last_time, uint16_t size){
	next_fit_start = last_time + size;
	if(!isValidUseAddress(heap, next_fit_start)){
		next_fit_start = 0;
	}
}

MemAddr os_Memory_NextFit(Heap *heap, size_t size){
	os_enterCriticalSection();
	if(next_fit_start == 0){ // Next Fit has never been called or we left off at the last chunk
		MemAddr addr = os_Memory_FirstFit(heap, size); // Next Fit degenerates to First Fit in this case
		if(addr != 0){
			/* setNextFitStart should only be called if there really is allocable memory
			 * to avoid inconsistency.
             */
		    setNextFitStart(heap, addr, os_getChunkSizeUnrestricted(heap, addr, false));
		}
		os_leaveCriticalSection();
		return addr;
	} else {
		MemAddr iter = next_fit_start; // Pick up where it left off
		while(isValidUseAddress(heap, iter)){
			uint16_t iter_chunk_size = os_getChunkSizeUnrestricted(heap, iter, false);
			if(os_getOwnerOfChunk(heap, iter) == 0 && iter_chunk_size >= size){ // We can choose this
				setNextFitStart(heap, iter, iter_chunk_size);
				os_leaveCriticalSection();
				return iter;
			} else {
				iter += iter_chunk_size; // Try the next chunk
			}
		}
	}
	// If the code ever reaches here, it means that there's no free space
	// after the current next_fit_start can be found. The strategy degenerates
	// to First Fit.
	MemAddr addr = os_Memory_FirstFit(heap, size);
	uint16_t addr_chunk_size = os_getChunkSizeUnrestricted(heap, addr, false);
	if(addr != 0){
	    setNextFitStart(heap, addr, addr_chunk_size);
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
	while(isValidUseAddress(heap, iter)){
		uint16_t iter_chunk_size = os_getChunkSizeUnrestricted(heap, iter, false);
		if(os_getOwnerOfChunk(heap, iter) == 0 && iter_chunk_size >= size && iter_chunk_size > addr_chunk_size){ // Looking for the biggest possible chunk
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
		uint16_t iter_chunk_size = os_getChunkSizeUnrestricted(heap, iter, false);
		if(os_getOwnerOfChunk(heap, iter) == 0 && iter_chunk_size >= size && iter_chunk_size < addr_chunk_size){ // Looking for the smallest possible chunk
			addr = iter;
			addr_chunk_size = iter_chunk_size;
		} 
		iter += iter_chunk_size; // try the next chunk
	}
	os_leaveCriticalSection();
	return addr;
}