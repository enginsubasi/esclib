/**
  ******************************************************************************
  *
  * @file      comstxetx.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.4
  * @date      26/08/2020
  *
  * @brief     Basic STX, ETX communication framework.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 26/08/2020 Created. @n
  * 29/07/2026 Bug fix. The driver variable was spelled drive in @n
  *            comstxetxTimeoutCounter. @n
  * 29/07/2026 Bug fix. comstxetxEvaluate assigned instead of @n
  *            compared rxReadyToEvaluate, so it fired on every call. @n
  * 29/07/2026 Bug fix. The receive bound checked a rxMaxLength @n
  *            member that does not exist. It now checks rxSize. @n
  * 01/08/2026 Bug fix. rxTimeoutCounter was cleared only when the @n
  *            timeout fired, so it kept counting across completed @n
  *            frames and progressively shrank the budget left to @n
  *            later frames. It is now cleared everywhere rxIndex @n
  *            returns to zero. @n
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 comstxetxInit rejects an rxSize below two, for the same @n
  *            write past the end that comatInit now rejects, and an @n
  *            stx equal to etx, which can never frame anything. @n
  *
  ******************************************************************************
  */
  
#include <stddef.h>

#include "comstxetx.h"

// comstxetxReceive writes rxBuffer[ rxIndex ] and only afterwards checks the
// index against rxSize, so the smallest buffer that never takes a write past
// its end is one that can hold the STX byte and one payload byte.
#define COMSTXETX_MIN_RX_SIZE   2

/**
 * @brief   Initializes the STX, ETX communication framework.
 * @param[out] driver         Framework state to initialize.
 * @param[in]  rxBuffer       Caller owned receive buffer.
 * @param[in]  txBuffer       Caller owned transmit buffer.
 * @param[in]  rxSize         Size of rxBuffer in bytes.
 * @param[in]  txSize         Size of txBuffer in bytes.
 * @param[in]  stx            Byte that marks the start of a frame.
 * @param[in]  etx            Byte that marks the end of a frame.
 * @param[in]  rxTimeout      Number of comstxetxTimeoutCounter ticks a
 *                            partial frame may stay pending. It is discarded
 *                            once the tick count exceeds this value, so the
 *                            budget is rxTimeout + 1 ticks. This is a whole
 *                            frame timeout, not an inter-byte one.
 * @param[in]  packetProcess  Called with the completed frame and its length.
 *                            The frame holds the STX byte at index 0
 *                            followed by the payload bytes; ETX itself is
 *                            not stored, unlike comatReceive, which stores
 *                            every byte of its frame including the leading
 *                            'A', 'T' and the trailing CR LF.
 * @return  TRUE on success, FALSE when a pointer is NULL, txSize is zero,
 *          rxSize is below two or stx equals etx.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 * @note    packetProcess is required. comstxetxEvaluate calls it without
 *          checking, so a NULL here would only surface as a crash on the
 *          first complete frame.
 * @note    rxSize must hold at least the STX byte and one payload byte.
 *          comstxetxReceive stores a byte before it compares the index
 *          against rxSize, so a shorter buffer would take a write past its
 *          end.
 * @note    stx equal to etx is rejected. The same byte cannot both open a
 *          frame and close it, since the receive state machine tests for
 *          etx only once a frame is already open, which would make every
 *          frame end empty.
 */
uint8_t comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer, uint32_t rxSize, uint32_t txSize, uint8_t stx, uint8_t etx, uint32_t rxTimeout, void (*packetProcess) ( uint8_t* buffer, uint32_t index ) )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
            ( rxSize >= COMSTXETX_MIN_RX_SIZE ) && ( txSize != 0 ) &&
            ( stx != etx ) && ( packetProcess != NULL ) )
    {
        // Function assignment.
        driver->packetProcess = packetProcess;

        // Parameter settings.
        driver->rxBuffer = rxBuffer;
        driver->txBuffer = txBuffer;

        driver->rxSize = rxSize;
        driver->txSize = txSize;

        driver->stx = stx;
        driver->etx = etx;

        driver->rxTimeoutCounter = 0;
        driver->rxTimeout = rxTimeout;

        // Initialize to zero and FALSE
        driver->rxIndex = 0;
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
 *          a frame that starts at the STX byte and completes at the ETX byte.
 * @param[in,out] driver  Framework state.
 * @param[in]     data    Byte received from the interface.
 * @note    The assembled buffer holds STX at index 0 followed by the
 *          payload bytes; ETX is not stored, it only sets the ready flag,
 *          so rxIndex is not incremented for it. This differs from
 *          comatReceive, which stores every byte of the frame including
 *          the leading 'A', 'T' and the trailing CR LF.
 * @note    If rxBuffer fills before ETX arrives, the partial frame is
 *          discarded and the driver goes back to looking for STX. Bytes are
 *          ignored while a completed frame is still waiting for
 *          comstxetxEvaluate.
 */
void comstxetxReceive ( comstxetx_t* driver, uint8_t data )
{
    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxIndex == 0 )
        {
            if ( data == driver->stx )
            {
                driver->rxBuffer[ driver->rxIndex ] = data;
                ++driver->rxIndex;
            }
        }
        else
        {
            if ( data == driver->etx )
            {
                driver->rxReadyToEvaluate = TRUE;
            }
            else
            {
                driver->rxBuffer[ driver->rxIndex ] = data;
                ++driver->rxIndex;
                
                if ( driver->rxIndex >= driver->rxSize )
                {
                    // Terminate all received bytes.
                    driver->rxIndex = 0;
                    driver->rxTimeoutCounter = 0;
                }
            }
        }
    }
}

/**
 * @brief   Called from the main loop; runs the packet callback when a
 *          complete frame is waiting.
 * @param[in,out] driver  Framework state.
 */
void comstxetxEvaluate ( comstxetx_t* driver )
{
    if ( driver->rxReadyToEvaluate == TRUE )
    {
        driver->packetProcess ( driver->rxBuffer, driver->rxIndex );
        
        driver->rxIndex = 0;
        driver->rxReadyToEvaluate = FALSE;
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
 *          completed frame waits for comstxetxEvaluate.
 */
void comstxetxTimeoutCounter ( comstxetx_t* driver )
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
