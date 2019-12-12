/*
 * os_spi.h
 *
 * Created: 10.12.2019 14:58:52
 *  Author: Minh Nghia Phan
 */ 


#ifndef OS_SPI_H_
#define OS_SPI_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "util.h"

// This macro would set PORTB4 to output and set the bit B4.
// Because B4 is connected to NOT CS of the external SRAM,
// this is effectively chip deselect
#define CHIP_SRAM_DESELECT (DDRB |= 0b00010000; PORTB |= 0b00010000)

// This macro selects the external SRAM by deleting PORTB4
#define CHIP_SRAM_SELECT (DDRB |= 0b00010000; PORTB &= 0b11101111) 

void os_spi_init(void);

uint8_t os_spi_send(uint8_t data);

uint8_t os_spi_receive();

#endif /* OS_SPI_H_ */