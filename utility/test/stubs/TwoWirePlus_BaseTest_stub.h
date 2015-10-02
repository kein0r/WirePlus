#ifndef  TWOWIREPLUS_BASETEST_STUB_H
#define  TWOWIREPLUS_BASETEST_STUB_H

/*******************| Inclusions |*************************************/
#include <stdint.h>
#include <stdbool.h>
/*******************| Macros |*****************************************/
#define ISR(a)		void a (void)

#define pinMode(a,b)

#define _BV(bit) (1 << (bit))

#define HEX			0x01

#define SDA			1
#define SCL			2


/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/
extern uint8_t PORTA;
extern uint8_t PORTB;
extern uint8_t PORTC;
extern uint8_t PORTD;

extern uint8_t SDA_reg;
extern uint8_t SCL_reg;

extern uint8_t TWSR;
extern uint8_t TWBR;
extern uint8_t TWCR;
extern uint8_t TWDR;

/*******************| Function Definition |****************************/

void digitalWrite(int, uint8_t);

class Serial_t {

public:
	void Serial(void) {};
	void print(const char[]) {};
	void print(uint8_t) {};
	void print(const char[], char) {};
	void print(uint8_t, char) {};
	void println(const char[]) {};
	void println(uint8_t) {};
	void println(const char[], char) {};
	void println(uint8_t, char) {};
};

/*******************| Preinstantiate Objects |*************************/
extern Serial_t Serial;

#endif
