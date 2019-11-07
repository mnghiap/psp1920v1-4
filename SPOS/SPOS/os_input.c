#include "os_input.h"

#include <avr/io.h>
#include <stdint.h>

/*! \file

Everything that is necessary to get the input from the Buttons in a clean format.

*/

/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 *
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 *
 */
uint8_t os_getInput(void) {
    uint8_t c7c6 = (PINC & 0b11000000) >> 4 ;
    uint8_t c1c0 = (PINC & 0b00000011);
    return (~(c7c6 | c1c0)) & 0b00001111;
}

/*!
 *  Initializes DDR and PORT for input
 */
void os_initInput() {
    DDRC &= 0b00111100; // C0, C1, C6, C7
    PORTC |= 0b11000011; // Pull-Ups
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void os_waitForNoInput() {
    while (os_getInput() != 0b00000000);
}

/*!
 *  Endless loop until at least one button is pressed.
 */
void os_waitForInput() {
    while (os_getInput() == 0b00000000);
}
