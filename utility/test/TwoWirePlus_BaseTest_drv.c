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
 * Helper function to reset buffer during a test
 */
void TwoWirePlus_BaseTest_resetBuffer()
{
	TwoWirePlus_rxRingBuffer.head = 0;
	TwoWirePlus_rxRingBuffer.tail = 0;
	TwoWirePlus_rxRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ; /* Buffer is empty on start-up */
	TwoWirePlus_txRingBuffer.head = 0;
	TwoWirePlus_txRingBuffer.tail = 0;
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ; /* Buffer is empty on start-up */
	for (int i=0; i< TWOWIREPLUS_RINGBUFFER_SIZE; i++)
	{
		TwoWirePlus_txRingBuffer.buffer[i] = TWOWIREPLUS_BASETEST_BUFFERINITVALUE;
		TwoWirePlus_rxRingBuffer.buffer[i] = TWOWIREPLUS_BASETEST_BUFFERINITVALUE;
	}
	TwoWirePlus_bytesToReceive = 0;

	TWDR = 0;
	TWCR = 0;
	TWBR = 0;
}

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
	TEST_ASSERT_EQUAL_INT(0x00, (((uint8_t)TWSR & 0x03)));
	/* Check if baudrate in TWBR is set according to TWOWIREPLUS_TWI_FREQUENCY
	 * SCL_Frequency = CPU_Freq / (16 + 2 * TWBR * PrescalerValue). With pre-scaler
	 * equals to one, TWBR should be ((F_CPU / TWOWIREPLUS_TWI_FREQUENCY) - 16)/2
	 */
	TEST_ASSERT_EQUAL_INT( (((F_CPU / TWOWIREPLUS_TWI_FREQUENCY) - 16)/2), ((uint8_t)TWBR));
	/* Check if TWCR was correctly set to TWEN (bit 2, 0x04), TWIE (bit 0, 0x01),
	 * TWEA (bit 6, 0x40).
	 */
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA), TWCR);
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
 * When calling beginTransmission address shall be shifted by one to the left
 * and read bit (bit 0) shall be set. After this START shall be requested in TWCR
 * Test if address << 1 | 0x01
 */
static void TwoWirePlus_BaseTest_beginTransmission_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();

	/* Start reading from address 0x42 four bytes */
	Wire.beginTransmission(0x42);
	/* Test if address SLA+R was written to txRingBuffer and that START was requested in TWCR */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT((0x42 << 1), TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_BaseTest_previousElement(TwoWirePlus_txRingBuffer.head)]);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_TWCR_START), TWCR);
}

/**
 * When calling beginTransmission TWDR shall not be changed (TWDR can't be pre-loaded)
 */
static void TwoWirePlus_BaseTest_beginTransmission_TC2(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWDR = 0xaa;
	Wire.beginTransmission(0x42);
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
}

/**
 * When txRingBuffer is empty write shall put directly into TWDR, nonetheless txRingBuffer
 * head shall be increased because it will later be decreased in ISR.
 * After data was written tw sent shall be requested
 */
static void TwoWirePlus_BaseTest_write_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWDR = 0xaa;
	Wire.write(0x55);
	/* Test if byte was written to TWDR */
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	/* Test if head of ring-buffer was increased */
	TEST_ASSERT_EQUAL_INT(0x01, TwoWirePlus_BaseTest_RingBufferBytesAvailable(TwoWirePlus_txRingBuffer));
	/* Test if tw sent was requested */
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	/* Test again, just at the end of buffer */
	TwoWirePlus_BaseTest_resetBuffer();
	/* Move head and tail to buffer end and see if this still works */
	TwoWirePlus_txRingBuffer.head = TWOWIREPLUS_RINGBUFFER_SIZE-1;
	TwoWirePlus_txRingBuffer.tail = TWOWIREPLUS_RINGBUFFER_SIZE-1;
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ;
	TWDR = 0xaa;
	Wire.write(0x55);
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(0x01, TwoWirePlus_BaseTest_RingBufferBytesAvailable(TwoWirePlus_txRingBuffer));
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
}

/**
 * When txRingBuffer is not empty write shall put data into txRingBuffer TWDR shall not be changed.
 * When txRingBuffer is not empty, communication is on-going and therefore TWCR shall not be changed.
 */
static void TwoWirePlus_BaseTest_write_TC2(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWDR = 0xaa;
	TWCR = 0xaa;
	/* put one byte in buffer to make it non-empty */
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	Wire.write(0x55);
	/* Test if TWDR was not changed */
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	/* Test if two bytes are buffer now. The one we added above and the one we added during write */
	TEST_ASSERT_EQUAL_INT(0x02, TwoWirePlus_BaseTest_RingBufferBytesAvailable(TwoWirePlus_txRingBuffer));
	/* Test if values was written to txRingBuffer */
	TEST_ASSERT_EQUAL_INT(0x55, TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_BaseTest_previousElement(TwoWirePlus_txRingBuffer.head)]);
	/* Test if TWCR was not changed */
	TEST_ASSERT_EQUAL_INT(0xaa, TWCR);
}

/**
 * When calling beginTransmission address shall be shifted by one to the left
 * and read bit (bit 0) shall be set. After this START shall be requested in TWCR
 * Test if address << 1 | 0x01
 */
static void TwoWirePlus_BaseTest_beginReception_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();

	/* Start reading from address 0x42 four bytes */
	Wire.beginReception(0x42);
	/* Test if address SLA+R was written to txRingBuffer and that START was requested in TWCR */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(((0x42 << 1) | 0x01), TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_BaseTest_previousElement(TwoWirePlus_txRingBuffer.head)]);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_TWCR_START), TWCR);
}

/**
 * When calling beginTransmission TWDR shall not be changed (TWDR can't be pre-loaded)
 */
static void TwoWirePlus_BaseTest_beginReception_TC2(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWDR = 0xaa;
	Wire.beginReception(0x42);
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
}

/**
 * Function requestBytes shall just increase the TwoWirePlus_bytesToReceive. More bytes than
 * TWOWIREPLUS_RINGBUFFER_SIZE are allowed to be requested
 */
static void TwoWirePlus_BaseTest_requestBytes_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWDR = 0xaa;
	TWCR = 0x55;
	Wire.requestBytes(5);
	TEST_ASSERT_EQUAL_INT(5, TwoWirePlus_bytesToReceive);
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
	TEST_ASSERT_EQUAL_INT(0x55, TWCR);
	Wire.requestBytes(TWOWIREPLUS_RINGBUFFER_SIZE);
	TEST_ASSERT_EQUAL_INT(5 + TWOWIREPLUS_RINGBUFFER_SIZE, TwoWirePlus_bytesToReceive);
}

/**
 * Function available shall return the number of bytes available in rxRingBuffer
 */
static void TwoWirePlus_BaseTest_available_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWDR = 0xaa;
	TWCR = 0x55;
	/* Sime test without module operation */
	TwoWirePlus_rxRingBuffer.head = 5;
	TEST_ASSERT_EQUAL_INT(5, Wire.available());
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
	TEST_ASSERT_EQUAL_INT(0x55, TWCR);
	/* Move tail to end of buffer to test module operation */
	TwoWirePlus_rxRingBuffer.head = 0x5;
	TwoWirePlus_rxRingBuffer.tail = TWOWIREPLUS_RINGBUFFER_SIZE - 1;
	TEST_ASSERT_EQUAL_INT(6, Wire.available());
}

/**
 * Function getBytesToBeReceived shall return content TwoWirePlus_bytesToReceive.
 * Preload TwoWirePlus_bytesToReceive and check if correct value is returned
 */
static void TwoWirePlus_BaseTest_getBytesToBeReceived_TC1(void)
{
	TwoWirePlus_bytesToReceive = 0xaa;
	TEST_ASSERT_EQUAL_INT(0xaa, Wire.getBytesToReceive());
	TwoWirePlus_bytesToReceive = 0x55;
	TEST_ASSERT_EQUAL_INT(0x55, Wire.getBytesToReceive());
}

/**
 * Function getBytesToBeReceived shall return content TwoWirePlus_bytesToReceive.
 * Preload TwoWirePlus_bytesToReceive and check if correct value is returned
 */
static void TwoWirePlus_BaseTest_getStatus_TC1(void)
{
	TwoWirePlus_status = 0xaa;
	TEST_ASSERT_EQUAL_INT(0xaa, Wire.getStatus());
	TwoWirePlus_status = 0x55;
	TEST_ASSERT_EQUAL_INT(0x55, Wire.getStatus());
}

/**
 * When ISR is called TwoWirePlus_status shall reflect the latest status of two
 * wire bus. Status is stored in bit 3 to 7 of TWSR.
 * Set different status values and test if status is copied to TwoWirePlus_status
 * and if only bit 3 to 7 are used.
 */
static void TwoWirePlus_BaseTest_ISR_TC1(void)
{
	TWSR = 0xaa;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((0xaa & 0xf8), TwoWirePlus_status);
	TWSR = 0x55;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((0x55 & 0xf8), TwoWirePlus_status);
}

/**
 * In case TW_MR_SLA_NACK was received fro m two wire slave device for SLA+R
 * no more bytes shall be received and therefore TwoWirePlus_bytesToReceive shall
 * be set to 0.
 * Set TwoWirePlus_bytesToReceive to some value and test if set to 0 in case of
 * TW_MR_SLA_NACK
 */
static void TwoWirePlus_BaseTest_ISR_TC2(void)
{
	TWSR = TW_MR_SLA_NACK;
	TwoWirePlus_bytesToReceive = 0xaa;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_bytesToReceive);
}

/**
 * In case of TW_MR_SLA_NACK, TW_MT_SLA_ACK, TW_MR_SLA_ACK, TW_MT_SLA_NACK, TW_MT_DATA_NACK
 * or TW_MT_DATA_ACK a byte was sent to two wire slave device earlier and therefore ring
 * buffer tail must be increased accordingly.
 * Set above mentioned two wire status, put one byte in txRingBuffer, call ISR and test if
 * byte was consumed in ISR.
 */
static void TwoWirePlus_BaseTest_ISR_TC3(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MR_SLA_NACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MT_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MR_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MT_SLA_NACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MT_DATA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));
}

/**
 * In case of TW_MR_SLA_NACK, TW_MT_SLA_ACK, TW_MR_SLA_ACK, TW_MT_SLA_NACK, TW_MT_DATA_NACK,
 * TW_MT_DATA_ACK, TW_START or TW_REP_START next data byte from txRingBuffer, if available,
 * shall be sent and interrupt cleared (TWINT).
 * Set above mentioned two wire status, put one byte in txRingBuffer, call ISR and test if
 * byte was consumed in ISR and TW_INT was set. For TW_MR_SLA_NACK, TW_MT_SLA_ACK,
 * TW_MR_SLA_ACK, TW_MT_SLA_NACK, TW_MT_DATA_NACK and TW_MT_DATA_ACK two bytes need to be
 * put into buffer (see previous test)
 */
static void TwoWirePlus_BaseTest_ISR_TC4(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MR_SLA_NACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MT_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MR_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MT_SLA_NACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_MT_DATA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0x55, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_START;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TWSR = TW_REP_START;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0xaa;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT(0xaa, TWDR);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
}

/**
 * In case of TW_MT_SLA_ACK, TW_MR_SLA_ACK, TW_MT_SLA_NACK, TW_MT_DATA_NACK,
 * TW_MT_DATA_ACK, TW_START or TW_REP_START and no data available in txRingBuffer but
 * TwoWirePlus_bytesToReceive bigger than 1 TW_INT shall be cleared to be able to receive
 * next byte. For TW_MR_SLA_NACK TwoWirePlus_bytesToReceive will be set to 0 (see following
 * tests).
 * Set above mentioned two wire status, set TwoWirePlus_bytesToReceive to 2, call ISR and
 * test if TW_INT is set correctly in TWSR.For TW_MR_SLA_NACK, TW_MT_SLA_ACK, TW_MR_SLA_ACK,
 * TW_MT_SLA_NACK, TW_MT_DATA_NACK and TW_MT_DATA_ACK one byte must be present in txRingBuffer
 * (see previous test)
 */
static void TwoWirePlus_BaseTest_ISR_TC5(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 2;
	TWSR = TW_MT_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 2;
	TWSR = TW_MR_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 2;
	TWSR = TW_MT_SLA_NACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 2;
	TWSR = TW_MT_DATA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 2;
	TWSR = TW_START;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 2;
	TWSR = TW_REP_START;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
}

/**
 * In case of TW_MT_SLA_ACK, TW_MR_SLA_ACK, TW_MT_SLA_NACK, TW_MT_DATA_NACK,
 * TW_MT_DATA_ACK, TW_START or TW_REP_START and no data available in txRingBuffer but
 * TwoWirePlus_bytesToReceive is 1, so only one more byte to sent, TW_INT shall be cleared and NACH
 * requested to signal that last byte is now about to be received. For TW_MR_SLA_NACK
 * TwoWirePlus_bytesToReceive will be set to 0 in ISR (see following tests).
 * Set above mentioned two wire status, set TwoWirePlus_bytesToReceive to 2, call ISR and
 * test if TW_INT is set correctly in TWSR.For TW_MR_SLA_NACK, TW_MT_SLA_ACK, TW_MR_SLA_ACK,
 * TW_MT_SLA_NACK, TW_MT_DATA_NACK and TW_MT_DATA_ACK one byte must be present in txRingBuffer
 * (see previous test)
 */
static void TwoWirePlus_BaseTest_ISR_TC6(void)
{
	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 1;
	TWSR = TW_MT_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 1;
	TWSR = TW_MR_SLA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 1;
	TWSR = TW_MT_SLA_NACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 1;
	TWSR = TW_MT_DATA_ACK;
	TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = 0x55;
	TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
	TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 1;
	TWSR = TW_START;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);

	TwoWirePlus_BaseTest_resetBuffer();
	TwoWirePlus_bytesToReceive = 1;
	TWSR = TW_REP_START;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
}

/**
 * Function begin is only for API compatibility to original TwoWire library and empty.
 * Test if no register where changed after this function is called.
 */
static void TwoWirePlus_BaseTest_begin_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();

	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_rxRingBuffer.head);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_rxRingBuffer.tail);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_READ, TwoWirePlus_rxRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_txRingBuffer.head);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_txRingBuffer.tail);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_READ, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(0, TwoWirePlus_bytesToReceive);

	TEST_ASSERT_EQUAL_INT(0, TWDR);
	TEST_ASSERT_EQUAL_INT(0, TWCR);
	TEST_ASSERT_EQUAL_INT(0, TWBR);
}

/**
 * Test if TwoWirePlus_incrementIndex will increment correctly and always stays
 * between 0 and TWOWIREPLUS_RINGBUFFER_SIZE-1
 */
static void TwoWirePlus_BaseTest_RingBuffer_TC1(void)
{
	uint8_t testVar = 0;
	for (int i=0; i< 100; i++)
	{
		TEST_ASSERT(testVar >= 0);
		TEST_ASSERT(testVar < TWOWIREPLUS_RINGBUFFER_SIZE);
		TEST_ASSERT_EQUAL_INT((i % TWOWIREPLUS_RINGBUFFER_SIZE), testVar);
		TwoWirePlus_incrementIndex(testVar);
	}
}

/**
 * Test if TwoWirePlus_RingBufferFull and TwoWirePlus_RingBufferEmpty work in various
 * situations.
 */
static void TwoWirePlus_BaseTest_RingBuffer_TC2(void)
{
	TwoWirePlus_RingBuffer_t testBuffer;
	testBuffer.head = 0;
	testBuffer.tail = 0;
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ;
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(!TwoWirePlus_RingBufferFull(testBuffer));
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TEST_ASSERT(!TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(TwoWirePlus_RingBufferFull(testBuffer));

	testBuffer.head = 8;
	testBuffer.tail = 8;
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ;
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(!TwoWirePlus_RingBufferFull(testBuffer));
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TEST_ASSERT(!TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(TwoWirePlus_RingBufferFull(testBuffer));

	testBuffer.head = 8;
	testBuffer.tail = 4;
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ;
	TEST_ASSERT(!TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(TwoWirePlus_RingBufferFull(testBuffer));
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TEST_ASSERT(!TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(TwoWirePlus_RingBufferFull(testBuffer));

	testBuffer.head = 4;
	testBuffer.tail = 8;
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ;
	TEST_ASSERT(!TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(TwoWirePlus_RingBufferFull(testBuffer));
	testBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
	TEST_ASSERT(!TwoWirePlus_RingBufferEmpty(testBuffer));
	TEST_ASSERT(TwoWirePlus_RingBufferFull(testBuffer));
}

/**
 * Request a few bytes from two wire slave device. Make slave device
 * ACK his address. This test tests a typical two wire slave device
 * read.
 */
static void TwoWirePlus_BaseTest_MasterReceiver_TC1(void)
{
	TwoWirePlus_BaseTest_resetBuffer();

	/* Start reading from address 0x42 four bytes */
	Wire.beginReception(0x42);
	/* Test if address SLA+R was written to txRingBuffer and that START was requested in TWCR */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(((0x42 << 1) | 0x01), TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_BaseTest_previousElement(TwoWirePlus_txRingBuffer.head)]);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_TWCR_START), TWCR);
	/* Request four bytes and see if they are requested */
	Wire.requestBytes(4);
	TEST_ASSERT_EQUAL_INT(0x4, TwoWirePlus_bytesToReceive);
	/* Emulate START was generated */
	TWSR = TW_START;
	TWI_vect();
	/* Check if global TwoWirePlus_status was set correctly to START */
	TEST_ASSERT_EQUAL_INT((TW_START), TwoWirePlus_status);
	/* Preset TwoWirePlus_status register to emulate ACK for address from two wire slave device */
	TWSR = TW_MR_SLA_ACK;
	TWI_vect();
	/* Test if TwoWirePlus_status is set correctly and SLA+R was read from txRingBuffer and written to tw data register */
	TEST_ASSERT_EQUAL_INT((TW_MR_SLA_ACK), TwoWirePlus_status);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_READ, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(((0x42 << 1) | 0x01), TWDR);
	/* Test if txRingBuffer is empty now because address was sent. */
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));
	/* Check if  TWIE, TWEN, TWEA and TWINT were set in ISR. */
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
	/* Now signal three bytes to be received from two wire slave device. Test if TWCR is set
	 * correctly to make sure ACK and not NACK was sent. */
	TWSR = TW_MR_DATA_ACK;
	TWDR = 0xa1;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TW_MR_DATA_ACK), TwoWirePlus_status);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
	TWDR = 0xa2;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
	TWDR = 0xa3;
	TWI_vect();
	/* For the very last by a NACK shall be sent TWEA = 0 */
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWIE | TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWINT), TWCR);
	/* Signal last byte and test if NACH (TWEA not set) is returned to two wire slave device */
	TWDR = 0xa4;
	TWI_vect();
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA ), TWCR);
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

/**
 * Request a few bytes from two wire slave device. Make slave device
 * NACK his address. Test if TwoWirePlus_bytesToReceive is set to 0 and no bytes are
 * received.
 */
static void TwoWirePlus_BaseTest_MasterReceiver_TC2(void)
{
	TwoWirePlus_BaseTest_resetBuffer();

	/* Start reading from address 0x42 four bytes */
	Wire.beginReception(0x42);
	Wire.requestBytes(4);
	/* Test if address SLA+R was written to txRingBuffer and that START was requested in TWCR */
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_WRITE, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(((0x42 << 1) | 0x01), TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_BaseTest_previousElement(TwoWirePlus_txRingBuffer.head)]);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_TWCR_START), TWCR);
	/* Emulate START was generated */
	TWSR = TW_START;
	TWI_vect();
	/* Check if global TwoWirePlus_status was set correctly to START */
	TEST_ASSERT_EQUAL_INT((TW_START), TwoWirePlus_status);
	/* Preset TwoWirePlus_status register to emulate ACK for address from two wire slave device */
	TWSR = TW_MR_SLA_NACK;
	TWI_vect();
	/* Test if TwoWirePlus_status is set correctly and SLA+R was read from txRingBuffer and written to tw data register */
	TEST_ASSERT_EQUAL_INT((TW_MR_SLA_NACK), TwoWirePlus_status);
	TEST_ASSERT_EQUAL_INT(TWOWIREPLUS_LASTOPERATION_READ, TwoWirePlus_txRingBuffer.lastOperation);
	TEST_ASSERT_EQUAL_INT(((0x42 << 1) | 0x01), TWDR);
	/* Test if txRingBuffer is empty now because address was sent. */
	TEST_ASSERT(TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer));
	/* NACK was sent for address by two wire slave device, so no data to be received */
	TEST_ASSERT_EQUAL_INT(0x0, TwoWirePlus_bytesToReceive);
	TEST_ASSERT_EQUAL_INT((TWOWIREPLUS_BASETEST_TWCR_TWEN | TWOWIREPLUS_BASETEST_TWCR_TWEA), TWCR);
	/* Wire.endReception can't be tested. A bit is set in this function and function will wait until bit is cleared in ISR */
}


/* Possible further test to be implemented
 *  - No bytes requested but bytes received
 *  - Read more bytes the requested
 *  - Read, Restart, Read
 */

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
	new_TestFixture("beginTransmission: Check correct address is sent", TwoWirePlus_BaseTest_beginTransmission_TC1),
	new_TestFixture("beginTransmission: No changes to TWDR", TwoWirePlus_BaseTest_beginTransmission_TC2),
	new_TestFixture("write: Check data is written to TWDR", TwoWirePlus_BaseTest_write_TC1),
	new_TestFixture("write: Check data is written to ring-buffer", TwoWirePlus_BaseTest_write_TC2),
	new_TestFixture("beginReception: Check correct address is sent", TwoWirePlus_BaseTest_beginReception_TC1),
	new_TestFixture("beginReception: No changes to TWDR", TwoWirePlus_BaseTest_beginReception_TC2),
	new_TestFixture("requestBytes: Check TwoWirePlus_bytesToReceive", TwoWirePlus_BaseTest_requestBytes_TC1),
	new_TestFixture("available: Check if available bytes are correct", TwoWirePlus_BaseTest_available_TC1),
	new_TestFixture("getBytesToBeReceived: Check return value",TwoWirePlus_BaseTest_getBytesToBeReceived_TC1),
	new_TestFixture("getStatus: Check return value", TwoWirePlus_BaseTest_getStatus_TC1),
	new_TestFixture("ISR: Check if status is reported", TwoWirePlus_BaseTest_ISR_TC1),
	new_TestFixture("ISR: Check TW_MR_SLA_NACK", TwoWirePlus_BaseTest_ISR_TC2),
	new_TestFixture("ISR: Check TW_MR_SLA_NACK, TW_MT_SLA_ACK, TW_MR_SLA_ACK, TW_MT_SLA_NACK, TW_MT_DATA_NACK, TW_MT_DATA_ACK", TwoWirePlus_BaseTest_ISR_TC3),
	new_TestFixture("ISR: Check data bytes are sent", TwoWirePlus_BaseTest_ISR_TC4),
	new_TestFixture("ISR: Check TW_INT is cleared", TwoWirePlus_BaseTest_ISR_TC5),
	new_TestFixture("ISR: Check master receiver NACK for last byte", TwoWirePlus_BaseTest_ISR_TC6),
	new_TestFixture("begin: Check if begin does nothing", TwoWirePlus_BaseTest_begin_TC1),
	new_TestFixture("RingBuffer: Increment index test", TwoWirePlus_BaseTest_RingBuffer_TC1),
	new_TestFixture("RingBuffer: Full/Empty test", TwoWirePlus_BaseTest_RingBuffer_TC2),
	new_TestFixture("Master Receiver: ",TwoWirePlus_BaseTest_MasterReceiver_TC1),
	new_TestFixture("Master Receiver: ",TwoWirePlus_BaseTest_MasterReceiver_TC2),
  };
   EMB_UNIT_TESTCALLER(TwoWirePlus_BaseTest,"TwoWirePlus_BaseTest",setUp,tearDown, fixtures);
   return (TestRef)&TwoWirePlus_BaseTest;
}

int main(void)
{
   TestRunner_start();
   TestRunner_runTest(TwoWirePlus_BaseTest_RunTests());
   TestRunner_end();
   return 0;
}

/*******************| Preinstantiate Objects |*************************/
