#include "adc.h"
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include "lcd.h"
#include <stdlib.h>

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
	uint16_t sum = 0;
	uint16_t tmp;
	
	for (uint8_t i = 0; i < 16; i++){ 
		// Start the conversion
		ADCSRA |= (1 << ADSC);

		// Wait until the conversion has finished
		while (ADCSRA & (1 << ADSC)) {};

		// Store the value as last captured
		tmp = ADCL | ((uint16_t)ADCH << 8);

		sum += tmp;
	}
	lastCaptured = sum / 16;
	return lastCaptured;
	
}

/*! \brief Returns the size of the buffer which stores voltage values.
 *
 * \return The size of the buffer which stores voltage values.
 */
uint8_t getBufferSize() {
    return bufferSize;
}

/*! \brief Returns the current index of the buffer which stores voltage values.
 *
 * \return The current index of the buffer which stores voltage values.
 */
uint8_t getBufferIndex() {
    return bufferIndex;
}

/*! \brief Stores the last captured voltage.
 *
 */
void storeVoltage(void) {
    // init buffer if buffer_size = 0
    if (bufferSize == 0) {
        bufferStart = malloc(100 * sizeof(uint16_t));
        bufferSize = 100;
        bufferIndex = 0;
    }

    // write value to bufferIndex
    if (bufferIndex < bufferSize) {
        *(bufferStart + bufferIndex) = lastCaptured;
        bufferIndex++;
    }
   
}

/*! \brief Returns the voltage value with the passed index.
 *
 * \param ind   Index of the voltage value.
 * \return      The voltage value with index ind.
 */
uint16_t getStoredVoltage(uint8_t ind) {
    return (ind < bufferIndex)?*(bufferStart + ind):0;
}
