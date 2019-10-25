#include "led.h"

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

void led_init() {
	DDRA |= 0b11111111;
	PORTA |= 0b11111111;
	
	initInput();
}

void led_fun() {
	while(1){
		waitForInput();
		uint8_t p = getInput();
		char last_led;
		switch(p){
			case 0b001:
				PORTA ^= 0b1;
				break;
			
			case 0b10:
				last_led = PORTA >> 7;
				PORTA = PORTA << 1;
				PORTA |= last_led;
				break;
			
			case 0b100:
				PORTA = ~PORTA;
				break;
			
			default: break;
		}
		waitForNoInput();
	}
}

void led_test() {
	led_init();
	led_fun();
}