/**
  ******************************************************************************
  *
  * @file      comat.c
  * @author    Engin Subaşı <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.3
  * @date      20/02/2020
  *
  * @brief     AT communication framework.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 20/02/2020 Created @n
  * 29/07/2026 Bug fix. The driver variable was spelled drive in @n
  *            comatTimeoutCounter. @n
  * 29/07/2026 Bug fix. comatEvaluate assigned instead of compared @n
  *            rxReadyToEvaluate, so it fired on every call. @n
  * 29/07/2026 Bug fix. The packetProcess member took rxInd by @n
  *            pointer while the prototype and the call site passed @n
  *            it by value. The member now takes it by value. @n
  * 29/07/2026 Bug fix. comatInit did not initialize txIndex. @n
  * 01/08/2026 Bug fix. rxTimeoutCounter was cleared only when the @n
  *            timeout fired, so it kept counting across completed @n
  *            frames and progressively shrank the budget left to @n
  *            later frames. It is now cleared everywhere rxIndex @n
  *            returns to zero. @n
  *
  ******************************************************************************
  */

#include "comat.h"

/**
 * @brief   Initializes the AT command framework.
 * @param[out] driver                 Framework state to initialize.
 * @param[in]  rxBuffer               Caller owned receive buffer.
 * @param[in]  txBuffer               Caller owned transmit buffer.
 * @param[in]  rxSize                 Size of rxBuffer in bytes.
 * @param[in]  txSize                 Size of txBuffer in bytes.
 * @param[in]  rxTimeout              Number of comatTimeoutCounter ticks a
 *                                    partial frame may stay pending. It is
 *                                    discarded once the tick count exceeds
 *                                    this value, so the budget is
 *                                    rxTimeout + 1 ticks. This is a whole
 *                                    frame timeout, not an inter-byte one.
 * @param[in]  packetProcess          Called with a complete frame. Receives the
 *                                    byte count by value and writes the reply
 *                                    length through txInd.
 * @param[in]  txTransmissionTrigger  Called to start transmitting the reply.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 */
void comatInit ( comat_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                                uint32_t rxSize, uint32_t txSize,
                                uint32_t rxTimeout,
                                void (*packetProcess) ( uint8_t* rxBuf, uint32_t rxInd, uint8_t* txBuf, uint32_t* txInd ),
                                void (*txTransmissionTrigger) ( uint8_t* txBuf, uint32_t txInd ) )
{
    uint32_t i = 0;
    
    // Function assignment.
    driver->packetProcess = packetProcess;
    driver->txTransmissionTrigger = txTransmissionTrigger;
    
    // Parameter settings.
    driver->rxBuffer = rxBuffer;
    driver->txBuffer = txBuffer;
    
    driver->rxSize = rxSize;
    driver->txSize = txSize;
    
    driver->rxTimeoutCounter = 0;
    driver->rxTimeout = rxTimeout;
    
    // Initialize to zero and FALSE
    driver->rxIndex = 0;
    driver->txIndex = 0;
    driver->rxReadyToEvaluate = FALSE;
    
    // Fill with zero
    for ( i = 0; i < driver->rxSize; ++i )
    {
        driver->rxBuffer[ i ] = 0;
    }
    
    for ( i = 0; i < driver->txSize; ++i )
    {
        driver->txBuffer[ i ] = 0;
    }
}

/**
 * @brief   Called from the receive interrupt, one byte at a time; assembles
 *          a frame that starts with 'A' then 'T' and ends with CR LF.
 * @param[in,out] driver  Framework state.
 * @param[in]     data    Byte received from the interface.
 * @note    If rxBuffer fills before CR LF arrives, the partial frame is
 *          discarded and the driver goes back to looking for a leading 'A'.
 *          Bytes are ignored while a completed frame is still waiting for
 *          comatEvaluate.
 */
void comatReceive ( comat_t* driver, uint8_t data )
{
    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxIndex == 0 )
        {
            if ( data == 'A' )
            {
                driver->rxBuffer [ driver->rxIndex ] = data;
                ++driver->rxIndex;
            }
        }
        else if ( driver->rxIndex == 1 )
        {
            if ( data == 'T' )
            {
                driver->rxBuffer [ driver->rxIndex ] = data;
                ++driver->rxIndex;
            }
            else
            {
                // Terminate buffering
                driver->rxIndex = 0;
                driver->rxTimeoutCounter = 0;
            }
        }
        else
        {
            driver->rxBuffer [ driver->rxIndex ] = data;
            ++driver->rxIndex;

            if ( ( driver->rxBuffer [ driver->rxIndex - 1 ] == '\n' ) &&
                ( driver->rxBuffer [ driver->rxIndex - 2 ] == '\r' ) )
            {
                driver->rxReadyToEvaluate = TRUE;
            }
            else if ( driver->rxIndex >= driver->rxSize )
            {
                // Terminate buffering
                driver->rxIndex = 0;
                driver->rxTimeoutCounter = 0;
            }
        }
    }
}

/**
 * @brief   Called from the main loop; runs the packet callback and starts
 *          the reply transmission when a complete frame is waiting.
 * @param[in,out] driver  Framework state.
 */
void comatEvaluate ( comat_t* driver )
{
    if ( driver->rxReadyToEvaluate == TRUE )
    {
        driver->packetProcess ( driver->rxBuffer, driver->rxIndex, driver->txBuffer, &driver->txIndex );
        driver->txTransmissionTrigger ( driver->txBuffer, driver->txIndex );
        
        driver->rxIndex = 0;
        driver->rxReadyToEvaluate = FALSE;
        driver->txIndex = 0;
        driver->rxTimeoutCounter = 0;
    }
}

/**
 * @brief   Called from a periodic timer tick; increments rxTimeoutCounter
 *          while a partial frame is pending and discards it once the
 *          counter exceeds rxTimeout.
 * @param[in,out] driver  Framework state.
 * @note    rxTimeoutCounter counts ticks for the current partial frame
 *          only. It is cleared wherever rxIndex returns to zero, so every
 *          frame starts with the full rxTimeout budget. Incoming bytes do
 *          not reset it, which makes this a whole frame timeout rather
 *          than an inter-byte one.
 * @note    The counter only advances while a partial frame is pending. It
 *          stands still while no frame is being assembled and while a
 *          completed frame waits for comatEvaluate.
 */
void comatTimeoutCounter ( comat_t* driver )
{
    if ( ( driver->rxIndex != 0 ) && ( driver->rxReadyToEvaluate == FALSE ) )
    {
        if ( driver->rxTimeoutCounter > driver->rxTimeout )
        {
            // Terminate all received bytes.
            driver->rxIndex = 0;
            driver->rxTimeoutCounter = 0;
        }
        else
        {
            ++driver->rxTimeoutCounter;
        }
    }
}
