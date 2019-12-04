/*
 * os_memory_strategies.c
 *
 * Created: 26.11.2019 11:44:23
 *  Author: iw851247
 */ 

#include "os_memory_strategies.h"
#include "os_memory.h"

MemAddr os_Memory_FirstFit(Heap *heap, size_t size){
    os_enterCriticalSection();
    MemAddr start = os_getUseStart(heap);
    MemAddr end = os_getUseSize(heap) + os_getUseStart(heap);
    
    for (volatile MemAddr addr = start; addr < end; ) {
		MemAddr chunk_size = os_getChunkSizeUnrestricted(heap, addr, false);
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
	//#warning implement something here
    return -1;
}

MemAddr os_Memory_WorstFit(Heap *heap, size_t size){
	//#warning implement something here
    return -1;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size){
	//#warning implement something here
    return -1;
}