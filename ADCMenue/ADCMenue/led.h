/*! \file
 *  \brief LED bar module.
 *  Contains functionality to access the LEDs of the evaluation board.
 *
 *  \author   Lehrstuhl für Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  1.0
 */

#ifndef _LED_H
#define _LED_H

#include <stdint.h>

//! Mask for activating/deactivating LEDs on the bar
extern uint16_t activateLedMask;

//! Initializes the led bar. Note: All PORTs will be set to output.
void initLedBar(void);

//! Sets the passed value as states of the led bar (1 = on, 0 = off).
void setLedBar(uint16_t value);

#endif
