/** @ingroup TwoWirePlus
 * @{
 *
 * @mainpage
 * @brief Replacement library for Arduino Wire library which should be a bit more
 * flexible
 *
 * Design goal was to implement a library which can be used for a wider range of two
 * wire devices while still being API compatible to the original
 * [Wire Library](https://www.arduino.cc/en/pmwiki.php?n=Reference/Wire).
 *
 * Two main differences compared to the original library
 * 1. Ring buffer are used for rx and tx in order to decouple the application as much as
 * possible from two wire functionality. Only one buffer for rx and tx is used.
 * 2. Sending an receiving data is done asynchronously. Sending bytes will, in contrast to
 * original two wire library, happen in background by ISR whenever a byte is made available by
 * application and *not* when #TwoWirePlus::endTransmission is called. This way the
 * application looses a bit control about when the actual data is sent, the advantage, however,
 * is that data will be send in background already while application is on providing more data.
 * Positive side-effect is that size of buffer can be reduced.
 *
 * Every function which request a specific bus state (START, RE-START, STOP) is blocking
 * and can therefore be used to sync application with two wire bus.
 *
 * @todo
 * - Complete error handling
 * - Timeout handling
 * - Think about where to add interrupt locking
 * - Add "whait until STOP was send to endX
 * - Add two wire slave functionality
 */

/*******************| Inclusions |*************************************/
#include "TwoWirePlus.h"
#include <Arduino.h>
#include <compat/twi.h>

/*******************| Macros |*****************************************/

/*******************| Type definitions |*******************************/

/*******************| Global variables |*******************************/
static TwoWirePlus_RingBuffer_t TwoWirePlus_txRingBuffer;
static TwoWirePlus_RingBuffer_t TwoWirePlus_rxRingBuffer;
static TwoWirePlus_Status_t TwoWirePlus_status = 0x0;

/**
 * Number of bytes requested to be received via two wire interface. In case
 * this variable hits one (1) NACK will be sent to two wire slave device for
 * the last byte.
 */
static volatile uint8_t TwoWirePlus_bytesToReceive = 0;

/*******************| Function Definition |****************************/

/**
 * Initializes WirePlus module.
 * @pre 
 */
TwoWirePlus::TwoWirePlus(void)
{
#ifdef TWOWIREPLUS_DEBUG
  /* make PORTB an output for TWSTATUS output */
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  /* make digital pin 4 output for ISR entry/exit signaling */
  pinMode(4, OUTPUT);
#endif

  /* Initialize ring buffer */
  TwoWirePlus_rxRingBuffer.head = 0;
  TwoWirePlus_rxRingBuffer.tail = 0;
  TwoWirePlus_rxRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ; /* Buffer is empty on start-up */
  TwoWirePlus_txRingBuffer.head = 0;
  TwoWirePlus_txRingBuffer.tail = 0;
  TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ; /* Buffer is empty on start-up */
  
  /* Activate internal pullups for twi lines */
  digitalWrite(SDA, 1);
  digitalWrite(SCL, 1);

  /* Init bitrate for SCL
   * SCL_Frequency = CPU_Freq / (16 + 2 * TWBR * PrescalerValue)
   * Prescaler will always be set to its smallest value to achieve
   * highest possible frequencies */
  TWSR = ~TWOWIREPLUS_TWSR_TWPS_MASK;
  TWSR |= (TWOWIREPLUS_TWSR_TWPS_1 & ~TWOWIREPLUS_TWSR_TWPS_MASK);
  TWBR = ((F_CPU / TWOWIREPLUS_TWI_FREQUENCY) - 16) / 2;

  // enable twi module, acks, and twi interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

/**
 * Initiates communication as I2C master
 * Address byte will be written to ringBuffer and (repeated) start is requested.
 * @param address 7bit slave address. Address will automatically be left shifted by one
 * and write bit added.
 * @note This function is blocking! It will wait until all previous communication has
 * finished. Do not call in interrupt context.
 */
void TwoWirePlus::beginTransmission(uint8_t address)
{
  /* Left shift address and add write bit */
  address = (address << 1) | TW_WRITE;

  /* wait until all previous communication has finished */
  while ( ! TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer) ) ;
  
  /* Unfortunately, we can't use write function here because TWDR register can't be pre-loaded */  
  /* Place data in buffer */
  TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = address;
  TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
  TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;

  /* Request start signal */
  TWCR = TWOWIREPLUS_TWCR_START;
}

/**
 * Write one byte to tx ringbuffer or TWI register. If buffer is empty data will
 * directly be written to twi data register TWDR.
 * @note This function is blocking! If head would move to the same location as the
 * tail the buffer would overflow. Do not call in interrupt context.
 * @param data data to be written
 * @pre #beginTransmission was called
 */
void TwoWirePlus::write(const uint8_t data)
{
  /* In case buffer is empty (i.e. first byte to write), copy data directly to TWDR and ask for sent */
  if ( TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer) )
  {
    /* need to increment first as the interrupt could happen directly after writing TWDR */
    TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
    TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
    TWDR = data;
    TWCR = TWOWIREPLUS_TWCR_SEND;
  }
  /* if not empty use buffer */
  else {
    /* wait in case no space left in buffer */
    while( TwoWirePlus_RingBufferFull(TwoWirePlus_txRingBuffer) ) ;
    /* Place data in buffer */
    TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = data;
    TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
    TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
  }
}

/**
 * End two wire transmission by requesting to send a stop after buffer was completely
 * transmitted.
 * @note This function will block until stop was really sent. It can therefore be
 * used as a synchronization point
 * @pre #beginTransmission was called
 */
TwoWirePlus_Status_t TwoWirePlus::endTransmission()
{
  /* block until last byte was transferred (or better ACK for last byte was received */
  while(!TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer) ) ;
  /* Then request STOP */
  TWCR = TWOWIREPLUS_TWCR_STOP;

  /* Problem: TWINT is not set after a stop condition. Thus, we wait for STOP bit is cleared
   * in TWCR.
   */
  while(TWCR & _BV(TWSTO)) ;

  return TwoWirePlus_status;
}

/**
 * Function to initiate a read from two wire slave device. Two wire start will be sent followed
 * by address include read bit. #bytesToReceive is not reset by this function. Application can
 * use #requestBytes to set bytes to received before calling this function.
 * @param address 7bit slave address. Address will automatically be left shifted by one
 * and read bit added.
 * @note This function is blocking! It will wait until all previous communication has
 * finished. Do not call in interrupt context.
 */
void TwoWirePlus::beginReception(uint8_t address)
{
    /* Left shift address and add write bit */
  address = (address << 1) | TW_READ;

  /* wait until all previous communication (rx and tx) has finished */
  while ( ! TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer)) ;
  
  /* Unfortunately, we can't use write function here because TWDR register can't be pre-loaded */  
  /* Place data in buffer */
  TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.head] = address;
  TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.head);
  TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;

  /* Request start signal */
  TWCR = TWOWIREPLUS_TWCR_START;
}


/**
 * Reads #numberOfBytes from #address
 * @param address Slave device address to read from
 * @param numberOfBytes Number of bytes to read from slave device
 * @note This function mainly exists for compatibility reason with original Wire
 * library. In contrast to the original function, this function will always sent
 * start. 
 * @note This function is blocking. Don't call in interrupt context.
 * @return Number of bytes received
 */
uint8_t TwoWirePlus::requestFrom(uint8_t address, uint8_t numberOfBytes)
{
  beginReception(address);
  /* bytesToReceive shall only be increased after call to beginReception to make sure all Tx is completed */
  TwoWirePlus_bytesToReceive += numberOfBytes;
  endReception();
  return available();
}

/**
 * Requests to receive #numberOfBytes from two wire slave device. This function can be used several
 * times between #beginReception and #endReception to receive data.
 * This function does not directly receive those bytes but forwards the request to the interrupt
 * function. Bytes can be read using #available and #read function. However, after the very last byte
 * to receive a NACK will be sent for the last byte. Therfore make sure that #bytesToReceive always is
 * greater than one.
 * @param numberOfBytes Number of bytes to receive from two wire slave device
 * @pre #beginReception must have been called first
 */
void TwoWirePlus::requestBytes(uint8_t numberOfBytes)
{
  TwoWirePlus_bytesToReceive += numberOfBytes;
}

/**
 * Returns the number of bytes already received from two wire slave device.
 * @return Number of bytes already received from two wire slave device
 */
uint8_t TwoWirePlus::available()
{
  /* Head can be altered in ISR at any time. Therefore we create a local copy */
  uint8_t head = TwoWirePlus_rxRingBuffer.head;
  /* Cast to uint8_t is important here because if not compiler will chose sint8_t */
  return (uint8_t)(head - TwoWirePlus_rxRingBuffer.tail) % TWOWIREPLUS_RINGBUFFER_SIZE;
}

/**
 * Returns one byte, if any, from rx ring buffer. If no bytes is present 0x00 will be 
 * returned. #available shall be used before calling this function to check if a byte
 * was received. 
 * @return Next byte from rx ring buffer or 0x00 if no byte was in buffer
 * @pre #available was called to check if a byte is present in the buffer
 */
uint8_t TwoWirePlus::read( )
{
  uint8_t retVal = 0x00;
  if(! TwoWirePlus_RingBufferEmpty(TwoWirePlus_rxRingBuffer) )
  {
    retVal = TwoWirePlus_rxRingBuffer.buffer[TwoWirePlus_rxRingBuffer.tail];
    TwoWirePlus_incrementIndex(TwoWirePlus_rxRingBuffer.tail);
  }
  return retVal;
}


void TwoWirePlus::endReception()
{
  /* Wait until data is completely (or NACK) received */
  while (TwoWirePlus_bytesToReceive) ;
  /* Then request STOP */
  TWCR = TWOWIREPLUS_TWCR_STOP;
  /* Problem: TWINT is not set after a stop condition. Thus, we wait for STOP bit is cleared
   * in TWCR.
   */
  while(TWCR & _BV(TWSTO)) ;
}

/**
 * Returns number of bytes requested to be received via two wire interface. In case of NACK received
 * return value will be 0 and two wire status must be checked in addition.
 * @return Number of bytes still requested to be received by two wire interface or zero if NACK was
 * received from two wire slave device (status equals to TwoWirePlus_MasterReceiver_NACK).
 */
uint8_t TwoWirePlus::getBytesToReceive()
{
  return TwoWirePlus_bytesToReceive;
}

/**
 * Provides access to last status of two wire interface
 * @return Last status of two wire interface
 */
TwoWirePlus_Status_t TwoWirePlus::getStatus()
{
  return TwoWirePlus_status;
}

/**
 * ISR for two wire interface TWI
 * Only exchange between ISR and the class WirePlus are the two ring buffer.
 * Initial trigger for Tx will be set in beginTransmit or write function. As long as data is
 * available in txRingBuffer it will be written to TWDR.
 * @note Writing a one to TWCR actually clears the corresponding bit
 */
ISR(TWI_vect)
{
#ifdef TWOWIREPLUS_DEBUG
  PORTB = TW_STATUS>>2;
  digitalWrite(4, HIGH);
#endif
  /* remember current status for application */
  TwoWirePlus_status = TW_STATUS;
  /* See why exactly interrupt was triggered */
  switch(TW_STATUS)
  {
    /* Slave adress is just one of the bytes which is transefered. Thus, we will just sent one
     * byte after each other after START, RE_START, ACK from the ring buffer. */
    case TW_MR_SLA_NACK:
      /* In case we sent NACK to two wire slave device there is nothing more to receive */
      TwoWirePlus_bytesToReceive = 0;
      /* fall through */
    case TW_MT_SLA_ACK:
    case TW_MR_SLA_ACK:
    case TW_MT_SLA_NACK:
    case TW_MT_DATA_NACK:
    case TW_MT_DATA_ACK:
      /* If ACK/NACK was received we've sent something earlier and therefore need to move read pointer */
      TwoWirePlus_incrementIndex(TwoWirePlus_txRingBuffer.tail);
      TwoWirePlus_txRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_READ;
      /* fall through */
    case TW_START:
    case TW_REP_START:
      /* Process next byte in queue if there is one */
      if (! TwoWirePlus_RingBufferEmpty(TwoWirePlus_txRingBuffer) )
      {
        TWDR = TwoWirePlus_txRingBuffer.buffer[TwoWirePlus_txRingBuffer.tail];
        TWCR = TWOWIREPLUS_TWCR_CLEAR;
      }
      else if (TwoWirePlus_bytesToReceive) /* Nothing more to send but something to receive */
      {
        if (TwoWirePlus_bytesToReceive == 1) /* Just one byte to receive so we need to directly send NACK */
        {
          TWCR = TWOWIREPLUS_TWCR_NACK;
        }
        else 
        {
          TWCR = TWOWIREPLUS_TWCR_CLEAR;
        }
      }
      else /* nothing else to do. Just clear interrupt and wait for more data or stop */
      {
        TWCR = TWOWIREPLUS_TWCR_RELEASE;
      }
      break;
    case TW_MR_DATA_NACK:
      /* No need to change bytesToReceive here because we are the one who are sending this NACK */
    case TW_MR_DATA_ACK:
      /* No check for buffer override done here because this would block the complete system */
      if (TwoWirePlus_bytesToReceive)
      {
        /* Place data in buffer */
        TwoWirePlus_rxRingBuffer.buffer[TwoWirePlus_rxRingBuffer.head] = TWDR;
        TwoWirePlus_incrementIndex(TwoWirePlus_rxRingBuffer.head);
        TwoWirePlus_rxRingBuffer.lastOperation = TWOWIREPLUS_LASTOPERATION_WRITE;
        TwoWirePlus_bytesToReceive--;
      }
      /* Is there more than one byte to be received left after this one */
      if (TwoWirePlus_bytesToReceive > 1)
      {
        /* If yes, send ACK */
        TWCR = TWOWIREPLUS_TWCR_ACK;
      }
      else if (TwoWirePlus_bytesToReceive == 1)
      {
        /* Send NACK for last byte (and all following one) to stop reception */
        TWCR = TWOWIREPLUS_TWCR_NACK;
      }
      else /* nothing else to do. Just clear interrupt and wait for more data or stop */
      {
        TWCR = TWOWIREPLUS_TWCR_RELEASE;
      }
      break;
    default:
      /* If something is not handled above clear at least INT and go on */
      TWCR = TWOWIREPLUS_TWCR_CLEAR;
    break;
  }
#ifdef TWOWIREPLUS_DEBUG
  digitalWrite(4, LOW);
#endif
}

/**
 * This function exists just for compatibility reasons with original TwoWire library
 */
void TwoWirePlus::begin()
{
}

/*******************| Preinstantiate Objects |*************************/
TwoWirePlus Wire = TwoWirePlus();

/** @}*/
