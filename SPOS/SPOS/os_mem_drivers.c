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
#include "os_scheduler.h"

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

void spi_sendAddr(MemAddr addr) {
    os_spi_send(0x00);  // …at address (as only 16bit, first 8 must be empty)
    os_spi_send(addr >> 8);
    os_spi_send(addr & 0xFF);
}

void initSRAM_external(void) {
    os_enterCriticalSection();
    // must be done here, as otherwise write would be interrupted everytime  
    CHIP_SRAM_SELECT;  
    os_spi_init();
    os_spi_send(0x01);  // set MODE register…
    os_spi_send(0x00);  // …to byte-read/write
    CHIP_SRAM_DESELECT;
    os_leaveCriticalSection();
}

MemValue readSRAM_external(MemAddr addr){
    os_enterCriticalSection();
    CHIP_SRAM_SELECT;
    os_spi_send(0x03);  // cmd: "read"…
    spi_sendAddr(addr);  // …at addr
    MemValue ret = os_spi_receive();
    CHIP_SRAM_DESELECT; 
    os_leaveCriticalSection();
    return ret;
}

void writeSRAM_external(MemAddr addr, MemValue value){
    os_enterCriticalSection();
    CHIP_SRAM_SELECT;
    os_spi_send(0x02);  // cmd "write"…
    spi_sendAddr(addr);  // …at addr…
    os_spi_send(value);  // …with val
    CHIP_SRAM_DESELECT;
    os_leaveCriticalSection();
}

MemDriver intSRAM__ = {
	.init = initSRAM_internal,
	.read = readSRAM_internal,
	.write = writeSRAM_internal,
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM
};

MemDriver extSRAM__ = {
	.init = initSRAM_external,
	.read = readSRAM_external,
	.write = writeSRAM_external,
	.start = EXT_SRAM_START,
	.size = EXT_MEMORY_SRAM
};