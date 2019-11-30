#include "os_core.h"
#include "defines.h"
#include "util.h"
#include "lcd.h"
#include "os_input.h"
#include "os_memheap_drivers.h"

#include <avr/interrupt.h>
#include <avr/io.h>

void os_initScheduler(void);

/*! \file
 *
 * The main system core with initialization functions and error handling.
 *
 */

/* 
 * Certain key-functionalities of SPOS do not work properly if optimization is
 * disabled (O0). It will still compile with O0, but you may encounter
 * run-time errors. This check is supposed to remind you on that.
 */
#ifndef __OPTIMIZE__
    #warning "Compiler optimizations disabled; SPOS and Testtasks may not work properly."
#endif

/*!
 *  Initializes the used timers.
 */
void os_init_timer(void) {
    // Init timer 2 (Scheduler)
    sbi(TCCR2A, WGM21); // Clear on timer compare match

    sbi(TCCR2B, CS22); // Prescaler 1024  1
    sbi(TCCR2B, CS21); // Prescaler 1024  1
    sbi(TCCR2B, CS20); // Prescaler 1024  1
    sbi(TIMSK2, OCIE2A); // Enable interrupt
    OCR2A = 60;

    // Init timer 0 with prescaler 256
    cbi(TCCR0B, CS00);
    cbi(TCCR0B, CS01);
    sbi(TCCR0B, CS02);

    sbi(TIMSK0, TOIE0);
}

/*!
 *  Readies stack, scheduler and heap for first use. Additionally, the LCD is initialized. In order to do those tasks,
 *  it calls the sub function os_initScheduler().
 */
void os_init(void) {
    // Init timer 0 and 2
    os_init_timer();

    // Init buttons
    os_initInput();

    // Init LCD display
    lcd_init();

    lcd_writeProgString(PSTR("Booting SPOS ..."));
	
	os_initHeaps();
	
	// Check whether heap offset is correctly initialized
	
	if(os_lookupHeap(0)->map_start != (MemAddr)&(__heap_start)){
		os_error("Heap start init incorrect");
	}
	
    os_initScheduler();

    os_coarseSystemTime = 0;
}

/*!
 *  Terminates the OS and displays a corresponding error on the LCD.
 *
 *  \param str  The error to be displayed
 */
void os_errorPStr(char const* str) {
    uint8_t SREGbak = SREG;
    SREG &= ~(0b1 << 7);  // stop all action
    
    // display error
    lcd_clear();
    lcd_line1();
    
    lcd_writeProgString(str);
    
    // wait for Enter + ESC input 
    uint8_t input;
    do {
        input = os_getInput();
    } while(input != 0b1001);
    os_waitForNoInput();
    SREG = SREGbak;  // return action
}
