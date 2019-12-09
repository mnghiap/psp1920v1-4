/*
 * os_memory_strategies.c
 *
 * Created: 26.11.2019 11:44:23
 *  Author: iw851247
 */ 

#include "os_memory_strategies.h"
#include "os_memory.h"
#include <stdint-gcc.h>

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

MemAddr os_Memory_NextFit(Heap *heap, size_t size){
	os_enterCriticalSection();
	MemAddr first_fit = os_Memory_FirstFit(heap, size); // What we would become if using First Fit instead
	if(!isValidUseAddress(first_fit)){ // This means there is no allocable memory
		os_leaveCriticalSection();
		return 0;
	}
	volatile MemAddr iter = first_fit + os_getChunkSize(first_fit); // Here begins the memory search
	if(!isValidUseAddress(iter)){ // First Fit returns the last memory chunk, thus the only fitting one for the process
		os_leaveCriticalSection();
		return first_fit;
	}
	// The search begins here. We iterate with variable iter, as long as it is in the valid use region.
	while (isValidUseAddress(iter)){
		uint16_t iter_chunk_size = os_getChunkSize(iter);
		if(os_getOwnerOfChunk(iter) == 0 && iter_chunk_size >= size){
		// start and its block can be chosen
		    os_leaveCriticalSection();
			return iter;
		} else {
		// try the next chunk
			iter += iter_chunk_size;
		}
	}
	// If the while loop ever exits, it means there is no allocable chunk beyond First Fit,
	// leaving First Fit as the only option
	os_leaveCriticalSection();
	return first_fit;
}

MemAddr os_Memory_WorstFit(Heap *heap, size_t size){
	os_enterCriticalSection();
	MemAddr addr = os_Memory_FirstFit(heap, size);
	if(addr == 0){ // No allocable memory
		os_leaveCriticalSection();
		return 0;
	}
	MemAddr iter = addr;
	uint16_t addr_chunk_size = os_getChunkSize(addr);
	// Iterate with iter from addr as it is the first available address anyway
	// Save the temporary result in addr and its chunk size in cur_chunk_size
	while(isValidUseAddress(iter)){
		uint16_t iter_chunk_size = os_getChunkSize(iter);
		if(os_getOwnerOfChunk(iter) == 0 && iter_chunk_size > addr_chunk_size){ // Looking for the biggest possible chunk
			addr = iter;
			addr_chunk_size = iter_chunk_size;
		} else {
			iter += iter_chunk_size; // try the next chunk
		}
	}
	os_leaveCriticalSection();
	return addr;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size){
	/* This is literally a copy and paste of Worst Fit.
	 * The only difference is a change from ">" to "<" 
	 * in the boolean expression inside the while loop.
	 */
	os_enterCriticalSection();
	MemAddr addr = os_Memory_FirstFit(heap, size);
	if(addr == 0){ // No allocable memory
		os_leaveCriticalSection();
		return 0;
	}
	MemAddr iter = addr;
	uint16_t addr_chunk_size = os_getChunkSize(addr);
	// Iterate with iter from addr as it is the first available address anyway
	// Save the temporary result in addr and its chunk size in cur_chunk_size
	while(isValidUseAddress(iter)){
		uint16_t iter_chunk_size = os_getChunkSize(iter);
		if(os_getOwnerOfChunk(iter) == 0 && iter_chunk_size < addr_chunk_size){ // Looking for the smallest possible chunk
			addr = iter;
			addr_chunk_size = iter_chunk_size;
		} else {
			iter += iter_chunk_size; // try the next chunk
		}
	}
	os_leaveCriticalSection();
	return addr;
}