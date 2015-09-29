#ifndef  TWOWIREPLUS_TWI_H
#define  TWOWIREPLUS_TWI_H

/*******************| Inclusions |*************************************/

/*******************| Macros |*****************************************/

/**
 * The lower 3 bits of TWSR are reserved on the ATmega163.
 * The 2 LSB carry the prescaler bits on the newer ATmegas.
 */
#define TW_STATUS_MASK		(_BV(TWS7)|_BV(TWS6)|_BV(TWS5)|_BV(TWS4)|_BV(TWS3))

/**
 * TWSR, masked by TW_STATUS_MASK */
#define TW_STATUS		(TWSR & TW_STATUS_MASK)

/**
 * \name R/~W bit in SLA+R/W address field.
 */
/**
 * SLA+R address */
#define TW_READ		1

/**
 * SLA+W address */
#define TW_WRITE	0

/**
 * start condition transmitted */
#define TW_START		0x08

/**
 * repeated start condition transmitted */
#define TW_REP_START		0x10

/* Master Transmitter */
/**
 * SLA+W transmitted, ACK received */
#define TW_MT_SLA_ACK		0x18

/**
 * SLA+W transmitted, NACK received */
#define TW_MT_SLA_NACK		0x20

/**
 * data transmitted, ACK received */
#define TW_MT_DATA_ACK		0x28

/**
 * data transmitted, NACK received */
#define TW_MT_DATA_NACK		0x30

/**
 * arbitration lost in SLA+W or data */
#define TW_MT_ARB_LOST		0x38

/* Master Receiver */
/**
 * arbitration lost in SLA+R or NACK */
#define TW_MR_ARB_LOST		0x38

/**
 * SLA+R transmitted, ACK received */
#define TW_MR_SLA_ACK		0x40

/**
 * SLA+R transmitted, NACK received */
#define TW_MR_SLA_NACK		0x48

/**
 * data received, ACK returned */
#define TW_MR_DATA_ACK		0x50

/**
 * data received, NACK returned */
#define TW_MR_DATA_NACK		0x58

/* Slave Transmitter */
/**
 * SLA+R received, ACK returned */
#define TW_ST_SLA_ACK		0xA8

/**
 * arbitration lost in SLA+RW, SLA+R received, ACK returned */
#define TW_ST_ARB_LOST_SLA_ACK	0xB0

/**
 * data transmitted, ACK received */
#define TW_ST_DATA_ACK		0xB8

/**
 * data transmitted, NACK received */
#define TW_ST_DATA_NACK		0xC0

/**
 * last data byte transmitted, ACK received */
#define TW_ST_LAST_DATA		0xC8

/* Slave Receiver */
/**
 * SLA+W received, ACK returned */
#define TW_SR_SLA_ACK		0x60

/**
 * arbitration lost in SLA+RW, SLA+W received, ACK returned */
#define TW_SR_ARB_LOST_SLA_ACK	0x68

/**
 * general call received, ACK returned */
#define TW_SR_GCALL_ACK		0x70

/**
 * arbitration lost in SLA+RW, general call received, ACK returned */
#define TW_SR_ARB_LOST_GCALL_ACK 0x78

/**
 * data received, ACK returned */
#define TW_SR_DATA_ACK		0x80

/**
 * data received, NACK returned */
#define TW_SR_DATA_NACK		0x88

/**
 * general call data received, ACK returned */
#define TW_SR_GCALL_DATA_ACK	0x90

/**
 * general call data received, NACK returned */
#define TW_SR_GCALL_DATA_NACK	0x98

/**
 * stop or repeated start condition received while selected */
#define TW_SR_STOP		0xA0

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/

/*******************| Function Definition |****************************/

/*******************| Preinstantiate Objects |*************************/

#endif
