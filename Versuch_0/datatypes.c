/*
 * CFile1.c
 *
 * Created: 24.10.2019 11:50:07
 *  Author: iw851247
 */ 

#include <datatypes.h>

#include <avr/io.h>
#include <lcd.h>

#define init_disp lcd_init();lcd_clear();

void loop() {
	uint8_t result = 0;
	for (int8_t i = 5; i >= 0; i--) {
		result += i * 2 + 2;
	}
	init_disp
	lcd_writeProgString(PSTR("Loop finished:"));
	lcd_writeDec(result);
	while(1);
}

void convert() {
	uint8_t target = 200;
	uint16_t source = target + 98;
	target = source;
}

void shift() {
	init_disp
	
	uint32_t result = (uint32_t)1 << 31;
	lcd_write32bitHex(result);
}