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

void os_spi_init(void);

uint8_t os_spi_send(uint8_t data);

uint8_t os_spi_receive();

#endif /* OS_SPI_H_ */