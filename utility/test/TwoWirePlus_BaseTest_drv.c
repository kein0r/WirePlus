/** \brief TwoWirePlus Basic Test
 *
 * This file contains the implementation of the
 * module test for TwoWirePlus library
 *
 */


/*******************| Inclusions |*************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "TwoWirePlus_BaseTest.h"
#include "TwoWirePlus_BaseTest_stub.h"

/* module under test has to be the last include */
#include "TwoWirePlus.cpp"

/*******************| Macros |*****************************************/

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/

/*******************| Function Definition |****************************/
static void setUp(void);
static void tearDown(void);

/**
 * A member of TwoWirePlus named Wire is already created in TwoWirePlus.cpp.
 * Therefore we assume that the constructor is already called. Check if twi
 * register TWSR, TWBR are set-up correctly.
 */
static void TwoWirePlus_BaseTest_Constructor_TC1(void)
{
	/* Check if two wire pre-scaler Bits 1..0 – TWPS: TWI Prescaler Bits - part of
	 * TWSR are set to 0, that it pre-scaler equals 1.
	 */
	TEST_ASSERT_EQUAL_INT(0x00, ((uint8_t)TWSR & 0x03));
	/* Check if baudrate in TWBR is set according to TWOWIREPLUS_TWI_FREQUENCY
	 * SCL_Frequency = CPU_Freq / (16 + 2 * TWBR * PrescalerValue). With pre-scaler
	 * equals to one, TWBR should be ((F_CPU / TWOWIREPLUS_TWI_FREQUENCY) - 16)/2
	 */
	TEST_ASSERT_EQUAL_INT( ((F_CPU / TWOWIREPLUS_TWI_FREQUENCY) - 16)/2, ((uint8_t)TWBR));
	/* Check if TWCR was correctly set to TWEN (bit 2, 0x04), TWIE (bit 0, 0x01),
	 * TWEA (bit 6, 0x40).
	 */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA, TWCR);
	/* Check if SDA and SCL pins are set to HIGH (one) to enable internal pull-up
	 */
	TEST_ASSERT_EQUAL_INT(0x01, SDA_reg);
	TEST_ASSERT_EQUAL_INT(0x01, SCL_reg);
}

/**
 * A member of TwoWirePlus named Wire is already created in TwoWirePlus.cpp.
 * Therefore we assume that the constructor is already called. Check if both
 * ring buffers are initialized correctly.
 */
static void TwoWirePlus_BaseTest_Constructor_TC2(void)
{
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_rxRingBuffer.head);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_rxRingBuffer.tail);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_READ, TwoWirePlus_rxRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_txRingBuffer.head);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_txRingBuffer.tail);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_READ, TwoWirePlus_txRingBuffer.lastOperation);
}

/**
 * Request a few bytes from two wire slave device. Make slave device
 * ACK his address and all bytes.
 */
static void TwoWirePlus_BaseTest_MasterReceiver_TC1(void)
{
	uint8_t oldNumberOfBytes = bytesToReceive;
	/* Start reading from address 0x42 four bytes */
	Wire.beginReception(0x42);
	Wire.requestBytes(4);
	/* Test if address SLA+R was written to txRingBuffer and that START was requested in TWCR */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT((0x42 << 1) | 0x01, TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_BaseTest_previousElement(TwoWirePlus_txRingBuffer.head)]);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_TWCR_START, TWCR);
	/* Preset status register to SLA ACK received and call ISR */
	TWSR = TW_MR_SLA_ACK;
	TWI_vect();
	/* Check if global status was set correctly and address was writte to tw data register */
	TEST_ASSERT_EQUAL_INT(status, TW_MR_SLA_ACK);
	TEST_ASSERT_EQUAL_INT(TWDR, (0x42 << 1) | 0x01);
	/* Test if txRingBuffer is empty now because address was sent. */
	TEST_ASSERT_EQUAL_INT(TRUE, TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));
	/* Check if  TWIE, TWEN, TWEA and TWINT were set in ISR. */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT, TWCR);
	/* Now signal three bytes to be received from two wire slave device. Test if TWCR is set
	 * correctly to make sure ACK and not NACK was sent. */
	TWSR = TW_MR_DATA_ACK;
	TWDR = 0xa1;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT, TWCR);
	TWDR = 0xa2;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT, TWCR);
	TWDR = 0xa3;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT, TWCR);
	/* Signal last byte and test if NACH (TWEA not set) is returned to two wire slave device */
	TWDR = 0xa4;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT, TWCR);
	/* Test if we really received four bytes and if the values match */
	TEST_ASSERT_EQUAL_INT(4, Wire.available());
	TEST_ASSERT_EQUAL_INT(0xa1, Wire.read());
	TEST_ASSERT_EQUAL_INT(3, Wire.available());
	TEST_ASSERT_EQUAL_INT(0xa2, Wire.read());
	TEST_ASSERT_EQUAL_INT(2, Wire.available());
	TEST_ASSERT_EQUAL_INT(0xa3, Wire.read());
	TEST_ASSERT_EQUAL_INT(1, Wire.available());
	TEST_ASSERT_EQUAL_INT(0xa4, Wire.read());
	TEST_ASSERT_EQUAL_INT(0, Wire.available());
	/* Wire.endReception can't be tested. A bit is set in this function and function will wait until bit is cleared in ISR */
}

/* Test
 *  - SLA+R NACK (with and without data)
 *  - No bytes requested but bytes received
 *  - Read more bytes the requested
 *  - Check if status is set correctly for all values
 */
static void TwoWirePlus_BaseTest_TC1(void)
{
	TEST_FAIL("Not my fault");
}

/**
 * Test Setup function which is called for all test cases
 */
static void setUp(void)
{
	/* Rest two wire status register for every test */
	TWSR = 0;
}



/**
 * Test Teardown function which is called for all test cases
 */
static void tearDown(void)
{
}

TestRef TwoWirePlus_BaseTest_RunTests(void)
{
   EMB_UNIT_TESTFIXTURES(fixtures) {
	new_TestFixture("Constructor: Register initialization", TwoWirePlus_BaseTest_Constructor_TC1),
	new_TestFixture("Constructor: Ring buffer initialization", TwoWirePlus_BaseTest_Constructor_TC2),
	new_TestFixture("Master Receiver: ",TwoWirePlus_BaseTest_MasterReceiver_TC1),
    new_TestFixture("TwoWirePlus BaseTest TC1", TwoWirePlus_BaseTest_TC1)
  };
   EMB_UNIT_TESTCALLER(TwoWirePlus_BaseTest,"TwoWirePlus_BaseTest",setUp,tearDown, fixtures);
   return (TestRef)&TwoWirePlus_BaseTest;
}

int main(void)
{
   unsigned int head, tail, diff;
   head = 6; tail = 0x0c;
   diff = (head - tail) % 16;
   printf("head: 0x%x tail: 0x%x -> 0x%x", head, tail, diff);
   TestRunner_start();
   TestRunner_runTest(TwoWirePlus_BaseTest_RunTests());
   TestRunner_end();
   return 0;
}

/*******************| Preinstantiate Objects |*************************/
