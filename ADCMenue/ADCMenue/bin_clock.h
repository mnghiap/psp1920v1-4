/*! \file
 *  \brief Binary clock module.
 *  Contains timer functionality. The clock works in 12h mode.
 *
 *  \author   Lehrstuhl für Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  1.0
 */

#ifndef _BIN_CLOCK_H
#define _BIN_CLOCK_H

#include <stdint.h>

//! Initializes the binary clock (ISR and global variables)
void initClock(void);

//! Returns the milliseconds counter of the current time.
uint16_t getTimeMilliseconds();

//! Returns the seconds counter of the current time.
uint8_t getTimeSeconds();

//! Returns the minutes counter of the current time.
uint8_t getTimeMinutes();

//! Returns the hour counter of the current time.
uint8_t getTimeHours();

#endif
