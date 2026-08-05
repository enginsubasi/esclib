/**
  ******************************************************************************
  *
  * @file      comat.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.5
  * @date      20/02/2020
  *
  * @brief     AT communication framework.
  *
  * @par Device
  * Generic
  *
  * @note      This module carries no checksum on purpose. AT is an ASCII
  *            command protocol and its real peers, from modems to cellular
  *            and BLE modules, do not checksum their frames. Adding one here
  *            would invent a private dialect no peer speaks. comstxetx is
  *            the module for links that need integrity.
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
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 comatInit rejects an rxSize below three. comatReceive @n
  *            stores a byte before it compares the index against @n
  *            rxSize, so a shorter buffer took a write past its end. @n
  * 05/08/2026 Documentation only. A note records that the absence of @n
  *            a checksum here is deliberate, since AT peers do not @n
  *            carry one. No code changed. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "comat.h"

// comatReceive writes rxBuffer[ rxIndex ] and only afterwards checks the
// index against rxSize, so the smallest buffer that never takes a write past
// its end is one that can hold 'A', 'T' and one more byte.
#define COMAT_MIN_RX_SIZE   3

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
 * @return  TRUE on success, FALSE when a pointer is NULL, txSize is zero or
 *          rxSize is below three.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 * @note    Both callbacks are required. comatEvaluate calls them without
 *          checking, so a NULL here would only surface as a crash on the
 *          first complete frame.
 * @note    rxSize must hold at least 'A', 'T' and one further byte.
 *          comatReceive stores a byte before it compares the index against
 *          rxSize, so a shorter buffer would take a write past its end.
 */
uint8_t comatInit ( comat_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                                uint32_t rxSize, uint32_t txSize,
                                uint32_t rxTimeout,
                                void (*packetProcess) ( uint8_t* rxBuf, uint32_t rxInd, uint8_t* txBuf, uint32_t* txInd ),
                                void (*txTransmissionTrigger) ( uint8_t* txBuf, uint32_t txInd ) )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
            ( rxSize >= COMAT_MIN_RX_SIZE ) && ( txSize != 0 ) &&
            ( packetProcess != NULL ) && ( txTransmissionTrigger != NULL ) )
    {
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

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
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
