/** @ingroup WirePlus
 * @{
 */
#ifndef  WIREPLUS_H
#define  WIREPLUS_H

/*******************| Inclusions |*************************************/
#include <stdint.h>

/*******************| Macros |*****************************************/
#ifndef WIREPLUS_TWI_FREQUENCY
#define WIREPLUS_TWI_FREQUENCY        100000L
#endif

#define WIREPLUS_RINGBUFFER_SIZE      16

#define WIREPLUS_TWSR_TWPS_MASK       (_BV(TWPS1)|_BV(TWPS0))
#define WIREPLUS_TWSR_TWPS_1          0x00
#define WIREPLUS_TWSR_TWPS_4          0x01
#define WIREPLUS_TWSR_TWPS_16         0x02
#define WIREPLUS_TWSR_TWPS_64         0x03

/*
 * TWINT: TWI Interrupt Flag
 * TWEA: TWI Enable Acknowledge Bit
 * TWSTA: TWI START Condition Bit
 * TWSTO: TWI STOP Condition Bit
 * TWWC: TWI Write Collision Flag
 * TWEN: TWI Enable Bit
 * TWIE: TWI Interrupt Enable
 */
#define WIREPLUS_TWCR_START           _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE)
#define WIREPLUS_TWCR_CLEAR           _BV(TWINT) | _BV(TWEN) | _BV(TWIE)
#define WIREPLUS_TWCR_SEND            _BV(TWINT) | _BV(TWEN) | _BV(TWIE)
#define WIREPLUS_TWCR_STOP            _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTO);
#define WIREPLUS_TWCR_RELEASE         _BV(TWEN)
/*******************| Type definitions |*******************************/

typedef uint8_t WirePlus_BufferIndex_t;

/**
 * Keeps track of last operation to ring buffer. Zero (or false) if last operation was 
 * a read, otherwise last operation was a write to buffer 
 */
typedef bool WirePlus_lastOperation_t; 
#define WIREPLUS_LASTOPERATION_READ         false
#define WIREPLUS_LASTOPERATION_WRITE        true

/**
 * \brief Data struct for ring buffer.
 * Index pointer for head and tail will always point to element which is next to be read/written.
 * This ring buffer will use "Record last operation" for full/empty buffer distinction see
 * https://en.wikipedia.org/wiki/Circular_buffer "Record last operation" for details
 */
typedef struct
{
  unsigned char buffer[WIREPLUS_RINGBUFFER_SIZE];     /*!< Content of ring buffer */
  volatile WirePlus_BufferIndex_t head;               /*!< Index for writing to the ring buffer. Index is increased after writing (i.e. producing) an element */
  volatile WirePlus_BufferIndex_t tail;               /*!< Index for reading from ring buffer. Index is increased after reading (i.e consuming) an element. */
  volatile WirePlus_lastOperation_t lastOperation;    /*!< False if last operation was reading the buffer, true if last operation was writting into the buffer */
} WirePlus_RingBuffer_t;

/* Some function like macro to make code more readable */
#define WirePlus_incrementIndex(a)          a = (a + 1) % WIREPLUS_RINGBUFFER_SIZE
#define WirePlus_RingBufferFull(x)          (x.lastOperation == WIREPLUS_LASTOPERATION_WRITE && (x.head == x.tail))
#define WirePlus_RingBufferEmpty(x)         (x.lastOperation == WIREPLUS_LASTOPERATION_READ && (x.head == x.tail))


/*******************| Global variables |*******************************/

/*******************| Function prototypes |****************************/

void printStatus();

class WirePlus
{
private:
  
public:
  WirePlus();
  void beginTransmission(uint8_t address);
  void write(uint8_t data);
  void endTransmission();
  void beginReception();
  void read(uint8_t *data);
  void endReception();
  uint8_t requestFrom(uint8_t address, uint8_t numberOfBytes);

};

#endif

/** @}*/
