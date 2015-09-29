#include"TwoWirePlus_BaseTest_stub.h"

/** \brief TwoWirePlus Basic Test
 *
 * This file contains the implementation of the
 * module test for TwoWirePlus library
 *
 */


/*******************| Inclusions |*************************************/
#include <stdint.h>

/*******************| Macros |*****************************************/

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/
uint8_t PORTA;
uint8_t PORTB;
uint8_t PORTC;
uint8_t PORTD;

/* For those pins that are access with digitalWrite/digitalRead we need two
 * things: #define for the pin number and a _reg variable;
 */
uint8_t SDA_reg = 0;
uint8_t SCL_reg = 0;

/* Arduino twi register */
uint8_t TWSR;
uint8_t TWBR;
uint8_t TWCR;
uint8_t TWDR;

/*******************| Function Definition |****************************/

void digitalWrite(int pinNumber, uint8_t value)
{
	switch(pinNumber)
	{
		case (SDA):
			/* add port register here */
			SDA_reg = value;
			break;
		case (SCL):
			/* add port register here */
			SCL_reg = value;
			break;
		default:
			break;
	}
}

/*******************| Preinstantiate Objects |*************************/
Serial_t Serial;


/*==================[external function definitions]==========================*/

/*==================[internal function definitions]==========================*/


/*==================[end of file]============================================*/

