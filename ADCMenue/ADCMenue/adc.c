#include "adc.h"
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include "lcd.h"

//! Global variables
uint16_t lastCaptured;

uint16_t* bufferStart;
uint8_t bufferSize;
uint8_t bufferIndex;

/*! \brief This method initializes the necessary registers for using the ADC module. \n
 * Reference voltage:    internal \n
 * Input channel:        PORTA0 \n
 * Conversion frequency: 156kHz
 */
void initAdc(void) {
    // Init DDRA0 as input
    DDRA &= ~(1 << PA0);

    /* 
     * REFS1:0 = 01     Select internal reference voltage
     * ADLAR   = 0      Store the result right adjusted to the ADC register
     * MUX4:0  = 00000  Use ADC0 as input channel (PA0)
     */
    ADMUX = (0 << REFS1) | (1 << REFS0);

    /*
     * ADEN    = 1      Enable ADC
     * ADSC    = 0      Used to start a conversion
     * ADATE   = 0      No continuous conversion
     * ADIF    = 0      Indicates that the conversion has finished
     * ADIE    = 0      Do not use interrupts
     * ADPS2:0 = 111    Prescaler 128 -> 20MHz / 128 = 156kHz ADC frequency
     */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/*! \brief Executes one conversion of the ADC and returns its value.
 *
 * \return The converted voltage (0 = 0V, 1023 = AVCC)
 */
uint16_t getAdcValue() {
    // Start the conversion
    ADCSRA |= (1 << ADSC);

    // Wait until the conversion has finished
    while (ADCSRA & (1 << ADSC)) {};

    // Store the value as last captured
    lastCaptured = ADCL | ((uint16_t)ADCH << 8);

    // Return the result
    return lastCaptured;
}

/*! \brief Returns the size of the buffer which stores voltage values.
 *
 * \return The size of the buffer which stores voltage values.
 */
uint8_t getBufferSize() {
    #warning IMPLEMENT STH. HERE
}

/*! \brief Returns the current index of the buffer which stores voltage values.
 *
 * \return The current index of the buffer which stores voltage values.
 */
uint8_t getBufferIndex() {
    #warning IMPLEMENT STH. HERE
}

/*! \brief Stores the last captured voltage.
 *
 */
void storeVoltage(void) {
    #warning IMPLEMENT STH. HERE
}

/*! \brief Returns the voltage value with the passed index.
 *
 * \param ind   Index of the voltage value.
 * \return      The voltage value with index ind.
 */
uint16_t getStoredVoltage(uint8_t ind) {
    #warning IMPLEMENT STH. HERE
}
