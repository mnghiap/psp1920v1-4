#include <avr/io.h>
#include <util/delay.h>
#include <os_input.h>

void init_wandler(){
	DDRA = 0b11111111; // PORTA als Ausgang an LED 
	DDRB = 0b11111111; // PORTB als Signal an R-2R-Netzwerk
	PORTB = 0b00000000; // initialisere ref (als PORTB)
	_delay_ms(50);
	PORTA = 0b11111111;
}

void manuell(){
	DDRA = 0b11111111; // PORTA als Ausgang an LED 
	DDRB = 0b11111111; // PORTB als Signal an R-2R-Netzwerk
	DDRD = 0b00000000; // PORTD als Eingang fuer DIP
	PORTD = 0b11111111; // Pull-Ups
	PORTA = PIND; // DIP-Kombination ausgeben
	PORTB = ~PIND; // DIP-Kombination an R-2R-Netz uebertragen
}

void an_LED_ausgeben(){ // Ergebnis des AD-Verfahren als LED ausgeben
	PORTA = ~PORTB;
}

uint8_t get_pinc0(){
	return (PINC & 0b00000001);
};

void tracking(){
    init_wandler();
    do { 
		if (PORTB == 0xFF) {
			return;	
		}
		
        PORTB += 1; //Inkrementiert
        _delay_ms(50);
        an_LED_ausgeben();	
    } while (get_pinc0() == 0); //Solange U_ref < U_mess
    PORTB -= 1; // U_mess einmal ueberschritten
    _delay_ms(50);
    an_LED_ausgeben();
}

void sar(){
	init_wandler();
	for (uint8_t i = 0; i < 8; i++){ // Interpretiere PORTB als Bitfolge b[0],b[1],...,b[7]
		PORTB |= (0b10000000 >> i); // Setze Bit b[i] 
		_delay_ms(50);
		an_LED_ausgeben();
		if(get_pinc0() == 1){ //U_ref > U_mess
			PORTB &= ~(0b10000000 >> i); // Loesche Bit b[i]
			_delay_ms(50);
			an_LED_ausgeben();
		}
	}
}

int main(void)
{
    uint8_t wandlung = 2; // Auswahl von Wandlungsart

	while(1){
		os_initInput();
		if (wandlung != 0)
			os_waitForInput();
		switch(wandlung){
			case 0:
				manuell();
				break;
				
			case 1:
				tracking();
				break;
				
			case 2:
				sar();
				break;
				
			default:
				break;
		};
		if (wandlung != 0)
			os_waitForNoInput();
		//wandlung = (wandlung + 1) % 3;
	}
}

