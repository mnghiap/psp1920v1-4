#include "menu.h"
#include "os_input.h"
#include "bin_clock.h"
#include "lcd.h"
#include "led.h"
#include "adc.h"
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

/*!
 *  Hello world program.
 *  Shows the string 'Hello World!' on the display.
 */

uint8_t get_WaitingESC(){ // Get Zustand von ESC
    if (os_getInput() & 0b1000 != 0) {
        os_waitForNoInput();
        return 1;
    }        
    return 0;
}

void helloWorld(void) {
    // Repeat until ESC gets pressed
    while(get_WaitingESC() == 0){ //ESC nicht gedrueckt
        lcd_clear();
        lcd_line1();
        lcd_writeProgString(PSTR("Hallo Welt!"));
        _delay_ms(500);
        lcd_clear();
        lcd_line1();
        lcd_writeProgString(PSTR("Hallo Welt!"));
        _delay_ms(500);
    }
}
/*!
 *	Zeige Zeiteinheit (Uhr, Minute, Sekunde) in gewuenschter Form %02d
 */

void displayTimeUnit(uint16_t timeUnit) {
    if(timeUnit == 0){
        lcd_writeProgString(PSTR("00:"));
    } else if (timeUnit < 10) {
        lcd_writeProgString(PSTR("0"));
        lcd_writeDec(timeUnit);
        lcd_writeProgString(PSTR(":"));
    } else {
        lcd_writeDec(timeUnit);
        lcd_writeProgString(PSTR(":"));
    }
}

/*!
 *	Zeige Millisekunde in gewuenschter Form %03d
 */
void displayMilliseconds(uint16_t ms) {
    if(ms == 0){
        lcd_writeProgString(PSTR("000"));
    } else if (ms < 10) {
        lcd_writeProgString(PSTR("00"));
        lcd_writeDec(ms);
    } else if (ms < 100) {
        lcd_writeProgString(PSTR("0"));
        lcd_writeDec(ms);
    } else {
        lcd_writeDec(ms);
    }
}


/*!
 *  Shows the clock on the display and a binary clock on the led bar.
 */
void displayClock(void) {
    while(get_WaitingESC() == 0){
        uint16_t clockVal = 0;
        uint16_t hours = getTimeHours(); //uint16_t fuer angenehmes Bitsetzen in clockVal
        uint16_t minutes = getTimeMinutes();
        uint16_t seconds = getTimeSeconds();
        uint16_t milliseconds = getTimeMilliseconds();
        clockVal |= seconds; // Setze Sekunde in clockVal, letzte 6 Bits
        clockVal |= (minutes << 6); // Setze Minute in clockVal, 6 Bits vor Sekunde
        clockVal |= (hours << 12); // Setze Stunde in clockVal, erste 4 Bits 
        setLedBar(clockVal);
        lcd_clear();
        displayTimeUnit(hours);
        displayTimeUnit(minutes);
        displayTimeUnit(seconds);
        displayMilliseconds(milliseconds);
        _delay_ms(10); // Damit die Uhr sichtbar ist. Notwendig wegen hoher Ausfuehrungsgeschwindigkeit
    }
}

/*!
 *  Shows the stored voltage values in the second line of the display.
 */
void displayVoltageBuffer(uint8_t displayIndex) {
    lcd_line2();
    char *string = malloc(3+1+3+2 * sizeof(char));
    sprintf(string, "%03d/100: ", displayIndex);
    lcd_writeString(string);
    free(string);
    lcd_writeVoltage(getStoredVoltage(displayIndex), 1023, 5);
}

/*!
 *  Shows the ADC value on the display and on the led bar.
 */
void displayAdc(void) {
    char index = 0;

    while (1) {
        uint16_t v = getAdcValue();
        // display voltage in first row
        lcd_clear();
        lcd_line1();
        lcd_writeProgString(PSTR("Voltage: "));
        lcd_writeVoltage(v, 1023, 5);

        // display voltage on LED bar graph 
        uint16_t ledValue = 0;
        while (v >= 68) {
            ledValue = (ledValue << 1) | 2;
            v -= 68; 
        }
        setLedBar(ledValue);

        // able to store / display save points
        displayVoltageBuffer(index);

        _delay_ms(100); 
        // handle input here after wait, so user can decide in 100ms, if he wants value.
        uint8_t input = os_getInput();
        if ((input & 0b1000) != 0) { // ESC
            os_waitForNoInput();
            return;
        } if ((input & 0b0001) != 0)  // RETURN
            storeVoltage();
        if ((input & 0b0010) != 0)  // DOWN
            index = (index - 1 + 100) % 100; 
        else if ((input & 0b0100) != 0)  // UP
            index = (index + 1) % 100;
    }
}

/*! \brief Starts the passed program
 *
 * \param programIndex Index of the program to start.
 */
void start(uint8_t programIndex) {
    // Initialize and start the passed 'program'
    switch (programIndex) {
        case 0:
            lcd_clear();
            helloWorld();
            break;
        case 1:
            activateLedMask = 0xFFFF; // Use all LEDs
            initLedBar();
            initClock();
            displayClock();
            break;
        case 2:
            activateLedMask = 0xFFFE; // Don't use led 0
            initLedBar();
            initAdc();
            displayAdc();
            break;
        default:
            break;
    }

    // Do not resume to the menu until all buttons are released
    os_waitForNoInput();
}

/*!
 *  Shows a user menu on the display which allows to start subprograms.
 */
void showMenu(void) {
    uint8_t pageIndex = 0;

    while (1) {
        lcd_clear();
        lcd_writeProgString(PSTR("Select:"));
        lcd_line2();

        switch (pageIndex) {
            case 0:
                lcd_writeProgString(PSTR("1: Hello world"));
                break;
            case 1:
                lcd_writeProgString(PSTR("2: Binary clock"));
                break;
            case 2:
                lcd_writeProgString(PSTR("3: Internal ADC"));
                break;
            default:
                lcd_writeProgString(PSTR("----------------"));
                break;
        }

        os_waitForInput();
        if (os_getInput() == 0b00000001) { // Enter
            os_waitForNoInput();
            start(pageIndex);
        } else if (os_getInput() == 0b00000100) { // Up
            os_waitForNoInput();
            pageIndex = (pageIndex + 1) % 3;
        } else if (os_getInput() == 0b00000010) { // Down
            os_waitForNoInput();
            if (pageIndex == 0) {
                pageIndex = 2;
            } else {
                pageIndex--;
            }
        }
    }
}
