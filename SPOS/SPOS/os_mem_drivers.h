/*
 * os_mem_drivers.h
 *
 * Created: 26.11.2019 10:54:06
 *  Author: iw851247
 */ 


#ifndef OS_MEM_DRIVERS_H_
#define OS_MEM_DRIVERS_H_

#include "inttypes.h"

typedef uint16_t MemAddr;

typedef uint8_t MemValue; 

#ifndef intSRAM
#define intSRAM (&intSRAM__)
#endif

typedef struct MemDriver{
	MemAddr const start;
	uint16_t const size;
	void (*init)(void);
	MemValue (*read)(MemAddr addr);
	void (*write)(MemAddr addr, MemValue value);
} MemDriver;

MemDriver intSRAM__;

#endif /* OS_MEM_DRIVERS_H_ */