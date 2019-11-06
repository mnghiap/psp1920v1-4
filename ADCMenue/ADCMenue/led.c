#include "led.h"
#include <avr/io.h>

uint16_t activateLedMask = 0xFFFF;

/*!
 *  Initializes the led bar. Note: All PORTs will be set to output.
 */
void initLedBar(void) { // PORTA und PORTD als Ausgang

    DDRA = activateLedMask & 0xFF;
	DDRD = activateLedMask >> 8;
	PORTA = activateLedMask & 0xFF;
	PORTD = activateLedMask >> 8;
}

/*!
 *  Sets the passed value as states of the led bar (1 = on, 0 = off).
 */
void setLedBar(uint16_t value) {
    value &= activateLedMask;
    PORTA = ~(value & 0xFF); // Letzte 8 Bits
	PORTD = ~(value >> 8); // Erste 8 Bits
}
