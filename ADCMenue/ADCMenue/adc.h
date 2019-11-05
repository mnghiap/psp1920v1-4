/*! \file
 *  \brief Analog digital converter module.
 *  Contains functionality to access the internal ADC of the micro controller.
 *
 *  \author   Lehrstuhl für Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  1.0
 */

#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>

//! This method initializes the necessary registers for using the ADC module.
void initAdc(void);

//! Executes one conversion of the ADC and returns its value.
uint16_t getAdcValue();

//! Returns the size of the buffer which stores voltage values.
uint8_t getBufferSize();

//! Returns the current index of the buffer which stores voltage values.
uint8_t getBufferIndex();

//! Stores the last captured voltage.
void storeVoltage(void);

//! Returns the voltage value with the passed index.
uint16_t getStoredVoltage(uint8_t ind);

#endif
