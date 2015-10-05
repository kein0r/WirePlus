/** @ingroup TwoWirePlus_BasicTest
 * @{
 */
#ifndef  TWOWIREPLUS_BASETEST_H
#define  TWOWIREPLUS_BASETEST_H

/*******************| Inclusions |*************************************/

#include "stdint.h"
#include <embUnit/embUnit.h>

/*******************| Macros |*****************************************/
#define FALSE		0
#define TRUE		!FALSE

#define TWOWIREPLUS_BASETEST_BUFFERINITVALUE	0xff

/* Some function like macro to make code more readable */
#define TwoWirePlus_BaseTest_previousElement(a)            (a - 1) % TWOWIREPLUS_RINGBUFFER_SIZE
#define TwoWirePlus_BaseTest_RingBufferBytesAvailable(x)   ((uint8_t)(x.head - x.tail) % TWOWIREPLUS_RINGBUFFER_SIZE)

/* Bits for TWCR register */
#define TWOWIREPLUS_BASETEST_TWCR_TWIE		0x01
#define TWOWIREPLUS_BASETEST_TWCR_TWEN		0x04
#define TWOWIREPLUS_BASETEST_TWCR_TWWC		0x08
#define TWOWIREPLUS_BASETEST_TWCR_TWSTO		0x10
#define TWOWIREPLUS_BASETEST_TWCR_TWSTA		0x20
#define TWOWIREPLUS_BASETEST_TWCR_TWEA		0x40
#define TWOWIREPLUS_BASETEST_TWCR_TWINT		0x80

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/

/*******************| Function Definition |****************************/
void TwoWirePlus_BaseTest_resetBuffer();

/*******************| Preinstantiate Objects |*************************/
/** @} */
#endif
