#include "led.h"
#include <avr/io.h>

uint16_t activateLedMask = 0xFFFF;

/*!
 *  Initializes the led bar. Note: All PORTs will be set to output.
 */
void initLedBar(void) { // PORTA und PORTD als Ausgang
    DDRA = 0b11111111;
	DDRD = 0b11111111;
	PORTA = 0b11111111;
	PORTD = 0b11111111;
}

/*!
 *  Sets the passed value as states of the led bar (1 = on, 0 = off).
 */
void setLedBar(uint16_t value) {
    PORTA = ~(value & 0b0000000011111111); // Letzte 8 Bits
	PORTD = ~(value >> 8); // Erste 8 Bits
}
