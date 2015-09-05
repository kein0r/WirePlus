/** @ingroup WirePlus
 * @{
 *
 * @brief Replacement library for Arduino Wire library.
 * Every fuction which request a specific bus state (START, RE-START, STOP) is blocking
 * and can therefore used to sync application with two wire bus.
 *
 * TODO:
 * - Complete error handling
 * - lastStatus (NACK, etc. etc.)
 */

/*******************| Inclusions |*************************************/
#include "WirePlus.h"
#include <Arduino.h>
#include <compat/twi.h>



/*******************| Macros |*****************************************/

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/
static WirePlus_RingBuffer_t WirePlus_txRingBuffer;
static WirePlus_RingBuffer_t WirePlus_rxRingBuffer;
static bool WirePlus_sendStop = false;

/*******************| Function Definition |****************************/

/**
 * Initializes WirePlus module.
 * @pre 
 */
WirePlus::WirePlus(void)
{
  
  /* Initialize ring buffer */
  WirePlus_rxRingBuffer.head = 0;
  WirePlus_rxRingBuffer.tail = 0;
  WirePlus_rxRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_READ; /* Buffer is empty on start-up */
  WirePlus_txRingBuffer.head = 0;
  WirePlus_txRingBuffer.tail = 0;
  WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_READ; /* Buffer is empty on start-up */
  
  /* Activate internal pullups for twi lines */
  digitalWrite(SDA, 1);
  digitalWrite(SCL, 1);

  /* Init bitrate for SCL
   * SCL_Frequency = CPU_Freq / (16 + 2 * TWBR * PrescalerValue)
   * Prescaler will always be set to its smallest value to achieve
   * highest possible frequencies */
  TWSR = ~WIREPLUS_TWSR_TWPS_MASK;
  TWSR |= (WIREPLUS_TWSR_TWPS_1 & ~WIREPLUS_TWSR_TWPS_MASK);
  TWBR = ((F_CPU / WIREPLUS_TWI_FREQUENCY) - 16) / 2;

  // enable twi module, acks, and twi interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

/**
 * Initiates communication as I2C master
 * Adress byte will be writen to ringBuffer and (repeated) start is requested.
 * @param address 7bit slave address. Adress will autmatically be left shifted by one
 * and write bit added.
 * @note This function is blocking! It will wait until all previous communication has
 * finished. The reason is because this function is also used to generate a restart for
 * reading from two wire slave device. Do not call in interrupt context.
 */
void WirePlus::beginTransmission(uint8_t address)
{
  /* Left shift address and add write bit */
  address = (address << 1) | TW_WRITE;

  /* wait until all previous communication has finished */
  while ( ! WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) ) ;
  
  /* Unfortunately, we can't use write function here because TWDR register can't be pre-loaded */  
  /* Place data in buffer */
  WirePlus_txRingBuffer.buffer[WirePlus_txRingBuffer.head] = address;
  WirePlus_incrementIndex(WirePlus_txRingBuffer.head);
  WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_WRITE;

  /* Request start signal */
  TWCR = WIREPLUS_TWCR_START;
}

/**
 * Write one byte to tx ringbuffer or TWI register. If buffer is empty data will
 * directly be written to twi data register TWDR.
 * @note This function is blocking! If head would move to the same location as the
 * tail the buffer would overflow. Do not call in interrupt context.
 * @param data data to be written
 */
void WirePlus::write(const uint8_t data)
{
  /* In case buffer is empty (i.e. first byte to write), copy data directly to TWDR and ask for sent */
  if ( WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) )
  {
    /* Buffer is empty so last one must be a read */
    WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_READ;
    /* need to increment first as the interrupt could happen directly after writing TWDR */
    TWDR = data;
    TWCR = WIREPLUS_TWCR_SEND;
  }
  /* if not empty use buffer */
  else {
    /* wait in case no space left in buffer */
    while( WirePlus_RingBufferFull(WirePlus_txRingBuffer) ) ;
    /* Place data in buffer */
    WirePlus_txRingBuffer.buffer[WirePlus_txRingBuffer.head] = data;
    WirePlus_incrementIndex(WirePlus_txRingBuffer.head);
    WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_WRITE;
  }
}

/**
 * End two wire transmission bei either requesting to send a stop when buffer
 * was completely transmitted or, if buffer is already empty, requesting a stop
 * @note This function will block until stop was really sent. It can therefore be
 * used as a synchronization point
 */
void WirePlus::endTransmission()
{
  /* if tx buffer is empty, all bytes are transmitted, request stop */
  if (WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) )
  {
    TWCR = WIREPLUS_TWCR_STOP;
  }
  else {   /* if still transmitting just remember to sent stop later */
    WirePlus_sendStop = true;
  }
  /* Block until STOP was transmitted successfully */
  while(TWCR & _BV(TWSTO)) ;
}

void WirePlus::beginReception()
{
}

void WirePlus::read(uint8_t *data)
{
}

void WirePlus::endReception()
{
}

/**
 * Reads #numberOfBytes from #address
 * @param address Slave device address to read from
 * @param numberOfBytes Number of bytes to read from slave device
 * @note This function mainly exists for compatibility reason with original Wire
 * library. In contrast to the original function, this function will always sent
 * start. 
 * @note This function is blocking. Don't call in interrupt context.
 */
uint8_t WirePlus::requestFrom(uint8_t address, uint8_t numberOfBytes)
{

}

uint8_t twiStatus = 0x0;
uint8_t numBytesSend = 0x00;
uint8_t lastByteTransmitted = 0x00;

/**
 * ISR for two wire interface TWI
 * Only exchange between ISR and the class WirePlus are the two ring buffer.
 * Initial trigger for Tx will be set in beginTransmit or write function. As long as data is
 * available in txRingBuffer it will be written to TWDR.
 * @note Writing a one to TWCR actually clears the corresponding bit
 */
ISR(TWI_vect)
{
  PORTB = TW_STATUS>>2;
  digitalWrite(4, HIGH);
  /* See why exactly interrupt was triggered */
  switch(TW_STATUS)
  {
    /* Slave adress is just one of the bytes which is transefered. Thus, we will just sent one
     * byte after each other after START, RE_START, ACK from the ring buffer. */
    case TW_START:
    case TW_REP_START:
    case TW_MT_SLA_ACK:
    case TW_MT_SLA_NACK:  /* TODO: remove */
    case TW_MT_DATA_NACK: /* TODO: remove */
    case TW_MT_DATA_ACK:
      /* Process next byte in queue if there is one */
      if (! WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) )
      {
        lastByteTransmitted = WirePlus_txRingBuffer.buffer[WirePlus_txRingBuffer.tail];
        numBytesSend++;
        TWDR = WirePlus_txRingBuffer.buffer[WirePlus_txRingBuffer.tail];
        WirePlus_incrementIndex(WirePlus_txRingBuffer.tail);
        WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_READ;
        TWCR = WIREPLUS_TWCR_CLEAR;
        twiStatus = TW_STATUS;
      }
      else /* nothing else to do. Just clear interrupt and wait for more data or stop */
      {
        TWCR = WIREPLUS_TWCR_RELEASE;
      }
      break;
    default:
      /* If something is not handled above clear at least INT and go on */
      TWCR = WIREPLUS_TWCR_CLEAR;
    break;
  }
  digitalWrite(4, LOW);
}

void printStatus()
{
  Serial.print("TW Status 0x"); Serial.print(twiStatus, HEX);
  Serial.print("  # Bytes: "); Serial.print(numBytesSend);
  Serial.print("  Tx head 0x"); Serial.print(WirePlus_txRingBuffer.head, HEX);
  Serial.print("  Tx tail 0x"); Serial.print(WirePlus_txRingBuffer.tail, HEX); 
  Serial.println();
}
/** @}*/
