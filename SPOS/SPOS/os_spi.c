/*
 * os_spi.c
 *
 * Created: 10.12.2019 14:58:38
 *  Author: Minh Nghia Phan
 */ 

#include "os_spi.h"
#include "os_scheduler.h"

/* Some details before implementation:
 * 
 * SPCR = SPIE | SPE | DORD | MSTR | CPOL | CPHA | SPR1 | SPR0
 * SPIE = 0 because we don't use interrupts
 *  SPE = 1 to activate SPI mode
 * DORD = 0 for MSB first. This should be more intuitive than LSB first.
 * Like if we send or receive a byte, the slave gets/means exactly 
 * that byte, not the reversed one like the diagram in Versuch 4 document.
 * MSTR = 1, ofc we are the master
 * CPOL = 0, active high idle low per the timing diagram 
 * CPHA = 0, leading edge
 * SPR1 = 0, for maximal clock speed
 * SPR0 = 0, same as SPR1
 *
 * SPSR = SPIF | WCOL | - | - | - | - | - | SPI2X
 *  SPIF = 1, we don't transmit anything by initialization
 *  WCOL = Don't care
 * SPI2X = 1 for maximal clock speed
 */
void os_spi_init(void){
	// Configure DDRB for CK, SI, SO, NOT CS
	DDRB   = 0b10110000; // SO is input, all else are output
	PORTB |= 0b01000000; // Pull-up for SO
	
	SPCR = 0b01010000;
	SPSR |= 0b10000001;
}

// This macro deletes the SPIF bit, thus starts the transmission
#define START_TRANSMISSION (SPSR &= 0b01111111)

// This macro actually gets the SPIF bit
#define TRANSMISSION_COMPLETE ((SPSR & 0b10000000) >> 7)

/* Both send and receive work like this:
 * Put the byte you want to send in SPDR.
 * Delete the SPIF bit to start the transmission.
 * Busy wait for it to become 1 and see
 * what we got in SPDR. It's what the slave sent us.
 * What is meaningful is based on whether we want to
 * send or receive data.
 *
 * Fix 1: send and receive shouldn't do the chip select
 * and deselect by themselves, as during the whole read or
 * write sequence the ext. SRAM should be selected. The init, 
 * read and write functions of the ext. SRAM driver will do that.
 */
uint8_t os_spi_send(uint8_t data){ 
	os_enterCriticalSection();
	SPDR = data;
	START_TRANSMISSION;
	while(TRANSMISSION_COMPLETE == 0);  // Busy waiting
	os_leaveCriticalSection();
	return SPDR; // So we can reuse it for receive
}

/* This function would send a dummy byte 0x00
 * and only cares about the received byte in SPDR
 */
uint8_t os_spi_receive(){
	return os_spi_send(0x00);
}