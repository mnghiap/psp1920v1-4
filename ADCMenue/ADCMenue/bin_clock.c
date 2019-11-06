#include "bin_clock.h"
#include <avr/io.h>
#include <avr/interrupt.h>

//! Global variables
volatile uint8_t hours = 12;
volatile uint8_t minutes = 0;
volatile uint8_t seconds = 0;
volatile uint16_t milliseconds = 0;

/*!
 * \return The milliseconds counter of the current time.
 */
uint16_t getTimeMilliseconds() {
    return milliseconds;
}

/*!
 * \return The seconds counter of the current time.
 */
uint8_t getTimeSeconds() {
    return seconds;
}

/*!
 * \return The minutes counter of the current time.
 */
uint8_t getTimeMinutes() {
    return minutes;
}

/*!
 * \return The hour counter of the current time.
 */
uint8_t getTimeHours() {
    return hours;
}

/*!
 *  Initializes the binary clock (ISR and global variables)
 */
void initClock(void) {
    // Set timer mode to CTC
    TCCR0A &= ~(1 << WGM00);
    TCCR0A |= (1 << WGM01);
    TCCR0B &= ~(1 << WGM02);

    // Set prescaler to 1024
    TCCR0B |= (1 << CS02) | (1 << CS00);
    TCCR0B &= ~(1 << CS01);

    // Set compare register to 195 -> match every 10ms
    OCR0A = 195;

    // Init variables
    hours = 12;
	minutes = 0;
	seconds = 0;
	milliseconds = 0;

    // Enable timer and global interrupts
    TIMSK0 |= (1 << OCIE0A);
    sei();
}

/*!
 *  Updates the global variables to get a valid 12h-time
 */
void updateClock(void) {
    if (milliseconds == 1000){
		milliseconds = 0;
		seconds += 1;
	}
	if (seconds == 60){
		seconds = 0;
		minutes += 1;
	}
	if (minutes == 60){
		minutes = 0;
        hours = (hours % 12) + 1;
	}
}

/*!
 *  ISR to increase second-counter of the clock
 */
ISR(TIMER0_COMPA_vect) {
    milliseconds += 10; // Intervall ist 10ms
	updateClock();
}
