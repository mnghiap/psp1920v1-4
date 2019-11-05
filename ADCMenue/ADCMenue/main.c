/*! \file
 *  \brief Analog digital converter
 *
 *  \author   Lehrstuhl für Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  1.0
 */

#include "lcd.h"
#include "os_input.h"
#include "menu.h"

int main(void) {
    // 1. Initialize the buttons
    os_initInput();

    // 2. Initialize LCD
    lcd_init();

    // 3. Show menu
    showMenu();
	
}
