/** @ingroup TwoWirePlus
 * @{
 */
#ifndef  TWOWIREPLUS_H
#define  TWOWIREPLUS_H

/*******************| Inclusions |*************************************/
#include <stdint.h>

/*******************| Macros |*****************************************/
#define TWOWIREPLUS_DEBUG

#ifndef TWOWIREPLUS_TWI_FREQUENCY
#define TWOWIREPLUS_TWI_FREQUENCY        100000L
#endif

#define TWOWIREPLUS_RINGBUFFER_SIZE      16

#define TWOWIREPLUS_TWSR_TWPS_MASK       (_BV(TWPS1)|_BV(TWPS0))
#define TWOWIREPLUS_TWSR_TWPS_1          0x00
#define TWOWIREPLUS_TWSR_TWPS_4          0x01
#define TWOWIREPLUS_TWSR_TWPS_16         0x02
#define TWOWIREPLUS_TWSR_TWPS_64         0x03

/*
 * TWINT: TWI Interrupt Flag
 * TWEA: TWI Enable Acknowledge Bit
 * TWSTA: TWI START Condition Bit
 * TWSTO: TWI STOP Condition Bit
 * TWWC: TWI Write Collision Flag
 * TWEN: TWI Enable Bit
 * TWIE: TWI Interrupt Enable
 */
#define TWOWIREPLUS_TWCR_START           _BV(TWINT) | _BV(TWEA) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE)
#define TWOWIREPLUS_TWCR_CLEAR           _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE)
#define TWOWIREPLUS_TWCR_SEND            _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE)
#define TWOWIREPLUS_TWCR_STOP            _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO);
#define TWOWIREPLUS_TWCR_ACK             _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE)
#define TWOWIREPLUS_TWCR_NACK            _BV(TWINT) | _BV(TWEN) | _BV(TWIE)
#define TWOWIREPLUS_TWCR_RELEASE         _BV(TWEA) | _BV(TWEN)
/*******************| Type definitions |*******************************/

/**
 * Typedef to make access of buffer easier
 */
typedef uint8_t TwoWirePlus_BufferIndex_t;

/**
 * Keeps track of last operation to ring buffer. Zero (or false) if last operation was 
 * a read, otherwise last operation was a write to buffer 
 */
typedef bool TwoWirePlus_lastOperation_t; 
#define TWOWIREPLUS_LASTOPERATION_READ         false
#define TWOWIREPLUS_LASTOPERATION_WRITE        true

/**
 * Data structure for ring buffer.
 * Index pointer for head and tail will always point to element which is next to be read/written.
 * This ring buffer will use "Record last operation" for full/empty buffer distinction see
 * https://en.wikipedia.org/wiki/Circular_buffer "Record last operation" for details
 */
typedef struct
{
  unsigned char buffer[TWOWIREPLUS_RINGBUFFER_SIZE];     /*!< Content of ring buffer */
  volatile TwoWirePlus_BufferIndex_t head;               /*!< Index for writing to the ring buffer. Index is increased after writing (i.e. producing) an element */
  volatile TwoWirePlus_BufferIndex_t tail;               /*!< Index for reading from ring buffer. Index is increased after reading (i.e consuming) an element. */
  volatile TwoWirePlus_lastOperation_t lastOperation;    /*!< False if last operation was reading the buffer, true if last operation was writting into the buffer */
} TwoWirePlus_RingBuffer_t;

/* Some function like macro to make code more readable */
#define TwoWirePlus_incrementIndex(a)          a = (a + 1) % TWOWIREPLUS_RINGBUFFER_SIZE
#define TwoWirePlus_RingBufferFull(x)          (x.lastOperation == TWOWIREPLUS_LASTOPERATION_WRITE && (x.head == x.tail))
#define TwoWirePlus_RingBufferEmpty(x)         (x.lastOperation == TWOWIREPLUS_LASTOPERATION_READ && (x.head == x.tail))

/**
 * Last status of two wire bus. This variable will reflect the content of TWSR and therefore
 */
typedef uint8_t TwoWirePlus_Status_t;


/*******************| Global variables |*******************************/

/*******************| Function prototypes |****************************/

void printStatus();

class TwoWirePlus
{
private:
  
public:
  TwoWirePlus();
  void begin();
  void beginTransmission(uint8_t address);
  void write(uint8_t data);
  TwoWirePlus_Status_t endTransmission();
  void beginReception(uint8_t address);
  uint8_t requestFrom(uint8_t address, uint8_t numberOfBytes);
  void requestBytes(uint8_t numberOfBytes);
  uint8_t available();
  uint8_t read();
  uint8_t BytesToBeReceived();
  void endReception();
  TwoWirePlus_Status_t getStatus();
};

/*******************| Preinstantiate Objects |*************************/
extern TwoWirePlus Wire;

#endif

/** @}*/
