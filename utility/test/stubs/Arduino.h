#ifndef ARDUINO_H
#define ARDUINO_H

/*******************| Inclusions |*************************************/

/*******************| Macros |*****************************************/

/* All microcontroller defines shall go here */
#define HIGH		0x1
#define LOW			0x0

#define INPUT		0x0
#define OUTPUT		0x1

#define F_CPU		1600000UL  // 16 MHz

/* TWSR */
#define TWPS0 0
#define TWPS1 1
#define TWS3 3
#define TWS4 4
#define TWS5 5
#define TWS6 6
#define TWS7 7

/* TWCR */
#define TWIE 0
#define TWEN 2
#define TWWC 3
#define TWSTO 4
#define TWSTA 5
#define TWEA 6
#define TWINT 7

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/

/*******************| Function Definition |****************************/

/*******************| Preinstantiate Objects |*************************/

#endif
