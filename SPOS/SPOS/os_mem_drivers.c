/*
 * os_mem_drivers.c
 *
 * Created: 26.11.2019 10:54:36
 *  Author: iw851247
 */ 

#include "os_mem_drivers.h"
#include "defines.h"
#include "atmega644constants.h"
#include "os_spi.h"

void initSRAM_internal(void){
    // There is nothing to do here.
}

MemValue readSRAM_internal(MemAddr addr){
	MemValue* pointer = (MemValue*) addr;
	return (*pointer);
}

void writeSRAM_internal (MemAddr addr, MemValue value){
	MemValue* pointer = (MemValue*) addr;
	(*pointer) = value;
}

MemDriver intSRAM__ = {
	.init = initSRAM_internal,
	.read = readSRAM_internal,
	.write = writeSRAM_internal,
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM
};