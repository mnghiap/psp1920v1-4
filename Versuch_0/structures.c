/*
 * structures.c
 *
 * Created: 24.10.2019 12:24:01
 *  Author: iw851247
 */ 

#include <structures.h>
#include <lcd.h>
#include <button.h>

const char* decide(DistributionStatus ds) {
	switch(ds){
		case AVAILABLE:
			return PSTR("AVAILABLE");
			
		case BACKORDER:
			return PSTR("BACKORDER");
			
		case SOLD_OUT:
			return PSTR("SOLD_OUT");
			
		default:
			return PSTR("ERROR!");
	}
}

void init() {
	lcd_init();
	lcd_clear();
	initInput();
}

void displayArticles() {
	init();
	
	volatile struct Article artArray[] = {
		{{0x1101}, AVAILABLE},
		{{0x1110}, BACKORDER},
		{{0x0101}, SOLD_OUT},	
	};
	uint8_t i = 0;
	while(1){
		waitForInput();
	
		lcd_clear();
		volatile struct Article *product = (artArray + (i%3));
		lcd_line1();
		lcd_writeProgString(PSTR("#M:"));
		lcd_writeDec(product->articleNumber.singleNumbers.manufactureId);
		lcd_writeProgString(PSTR(" / #P:"));
		lcd_writeDec(product->articleNumber.singleNumbers.productID);
		
		lcd_line2();
		lcd_writeProgString(decide(product->status));
		i++;
		waitForNoInput();	
	}
}