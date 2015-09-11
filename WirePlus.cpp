/** @ingroup WirePlus
 * @{
 *
 * @brief Replacement library for Arduino Wire library.
 * Every fuction which request a specific bus state (START, RE-START, STOP) is blocking
 * and can therefore be used to sync application with two wire bus.
 *
 * TODO:
 * - Complete error handling
 * - Add lastState to return value for endTransmission, 3 and endReception 
 * - lastStatus (NACK, etc. etc.)
 * - Timeout handling
 * - Think about where to add interrupt locking
 * - Add "whait until STOP was send to endX
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
static WirePlus_Status_t status = WirePlus_Init;
/**
 * Number of bytes requested to be received via two wire interface. In case of NACK received
 * this variable will be set to 0. Thus, #status must always be checked if this variable is used.
 */
static volatile uint8_t bytesToReceive = 0;

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
 * finished. Do not call in interrupt context.
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
 * @pre #beginTransmission was called
 */
void WirePlus::write(const uint8_t data)
{
  /* In case buffer is empty (i.e. first byte to write), copy data directly to TWDR and ask for sent */
  if ( WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) )
  {
    /* need to increment first as the interrupt could happen directly after writing TWDR */
    WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_WRITE;
    WirePlus_incrementIndex(WirePlus_txRingBuffer.head);
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
 * End two wire transmission by requesting to send a stop after buffer was completely
 * transmitted.
 * @note This function will block until stop was really sent. It can therefore be
 * used as a synchronization point
 * @pre #beginTransmission was called
 */
void WirePlus::endTransmission()
{
  /* block until last byte was transferred (or better ACK for last byte was received*/
  while(!WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) ) ;
  /* Then request STOP */
  TWCR = WIREPLUS_TWCR_STOP;
}

/**
 * Function to initiate a read from two wire slave device. Two wire start will be sent followed
 * by address include read bit
 * @param address 7bit slave address. Adress will autmatically be left shifted by one
 * and read bit added.
 * @note This function is blocking! It will wait until all previous communication has
 * finished. Do not call in interrupt context.
 */
void WirePlus::beginReception(uint8_t address)
{
    /* Left shift address and add write bit */
  address = (address << 1) | TW_READ;

  /* wait until all previous communication (rx and tx) has finished */
  while ( ! WirePlus_RingBufferEmpty(WirePlus_txRingBuffer)) ;
  
  /* Unfortunately, we can't use write function here because TWDR register can't be pre-loaded */  
  /* Place data in buffer */
  WirePlus_txRingBuffer.buffer[WirePlus_txRingBuffer.head] = address;
  WirePlus_incrementIndex(WirePlus_txRingBuffer.head);
  WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_WRITE;

  /* Request start signal */
  TWCR = WIREPLUS_TWCR_START;
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
  beginReception(address);
  /* bytesToReceive shall only be increased after call to beginReception to make sure all Tx is completed */
  bytesToReceive += numberOfBytes;
  endReception();
}

/**
 * Requests to receive #numberOfBytes from two wire slave device. This function can be used several
 * times between #beginReception and #endRecepetion to receive data.
 * This function does not directly receive those bytes but forwards the request to the interrupt
 * function. Bytes can be read using #available and #read function.
 * @param numberOfBytes Number of bytes to receive from two wire slave device
 * @pre #beginReception must have been called first
 */
void WirePlus::receiveBytes(uint8_t numberOfBytes)
{
  bytesToReceive += numberOfBytes;
}

bool WirePlus::available()
{
  return !WirePlus_RingBufferEmpty(WirePlus_rxRingBuffer);
}

/**
 * Asks for #length bytes from two wire bus. This function can be called several times
 * to consecutively receive more bytes without sending a stop in between.
 * 
 * @param data Pointer to buffer where to the data received from two wire slave device will
 * be copied. Application must make sure that buffer is big enough to hold data completely
 * @param length Number of bytes to receive from two wire bus
 * @pre #beginReception was called
 */
uint8_t WirePlus::read( )
{
  if(! WirePlus_RingBufferEmpty(WirePlus_rxRingBuffer) )
  {
    uint8_t retVal = WirePlus_rxRingBuffer.buffer[WirePlus_rxRingBuffer.tail];;
    WirePlus_incrementIndex(WirePlus_rxRingBuffer.tail);
    return retVal;
  }
  else {
    return 0x00;
  }
}


void WirePlus::endReception()
{
  /* Wait until data is completely (or NACK) received */
  while (bytesToReceive) ;
  /* Then request STOP */
  TWCR = WIREPLUS_TWCR_STOP;
}

/**
 * Returns number of bytes requested to be received via two wire interface. In case of NACK received
 * return value will be 0 and two wire status must be checked in addition.
 * @return Number of bytes still requested to be received by two wire interface or zero if NACK was
 * received from two wire slave device (status equals to WirePlus_MasterReceiver_NACK).
 */
uint8_t WirePlus::BytesToBeReceived()
{
  return bytesToReceive;
}

/**
 * Provides access to last status of two wire interface
 * @return Last status of two wire interface
 */
WirePlus_Status_t WirePlus::getStatus()
{
  return status;
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
#ifdef WIREPLUS_DEBUG
  PORTB = TW_STATUS>>2;
  digitalWrite(4, HIGH);
#endif
  /* See why exactly interrupt was triggered */
  switch(TW_STATUS)
  {
    /* Slave adress is just one of the bytes which is transefered. Thus, we will just sent one
     * byte after each other after START, RE_START, ACK from the ring buffer. */
    case TW_MT_SLA_ACK:
    case TW_MR_SLA_ACK:
    case TW_MT_SLA_NACK:  /* TODO: remove */
    case TW_MR_SLA_NACK:  /* TODO: remove */
    case TW_MT_DATA_NACK: /* TODO: remove */
    case TW_MT_DATA_ACK:
      /* If ACK was received we've sent something earlier and therefore need to move read pointer */
      WirePlus_incrementIndex(WirePlus_txRingBuffer.tail);
      WirePlus_txRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_READ;
      /* fall through */
    case TW_START:
    case TW_REP_START:
      /* Process next byte in queue if there is one */
      if (! WirePlus_RingBufferEmpty(WirePlus_txRingBuffer) )
      {
        TWDR = WirePlus_txRingBuffer.buffer[WirePlus_txRingBuffer.tail];
        TWCR = WIREPLUS_TWCR_CLEAR;
      }
      else if (bytesToReceive) /* Nothing more to send but something to receive */
      {
        if (bytesToReceive == 1) /* Just one byte to receive so we need to directly send NACK */
        {
          TWCR = WIREPLUS_TWCR_NACK;
        }
        else 
        {
          TWCR = WIREPLUS_TWCR_CLEAR;
        }
      }
      else /* nothing else to do. Just clear interrupt and wait for more data or stop */
      {
        TWCR = WIREPLUS_TWCR_RELEASE;
      }
      break;
    case TW_MR_DATA_NACK:
    case TW_MR_DATA_ACK:
      /* No check for buffer override done here because this would block the complete system */
      if (bytesToReceive)
      {
        /* Place data in buffer */
        WirePlus_rxRingBuffer.buffer[WirePlus_rxRingBuffer.head] = TWDR;
        WirePlus_incrementIndex(WirePlus_rxRingBuffer.head);
        WirePlus_rxRingBuffer.lastOperation = WIREPLUS_LASTOPERATION_WRITE;
        bytesToReceive--;
      }
      /* Is there more than one byte to be received left after this one */
      if (bytesToReceive > 1)
      {
        /* If yes, send ACK */
        TWCR = WIREPLUS_TWCR_ACK;
      }
      else if (bytesToReceive == 1)
      {
        /* Send NACK for last byte (and all following one) to stop reception */
        TWCR = WIREPLUS_TWCR_NACK;
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
#ifdef WIREPLUS_DEBUG
  digitalWrite(4, LOW);
#endif
}

/*******************| Preinstantiate Objects |*************************/
WirePlus Wire = WirePlus();

/** @}*/
