/**
  ******************************************************************************
  *
  * @file      comstxetx.c
  * @author    Engin Subaşı <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
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
  *
  ******************************************************************************
  */
  
#include "comstxetx.h"

/**
 * @brief   Initializes the STX, ETX communication framework.
 * @param[out] driver         Framework state to initialize.
 * @param[in]  rxBuffer       Caller owned receive buffer.
 * @param[in]  txBuffer       Caller owned transmit buffer.
 * @param[in]  rxSize         Size of rxBuffer in bytes.
 * @param[in]  txSize         Size of txBuffer in bytes.
 * @param[in]  stx            Byte that marks the start of a frame.
 * @param[in]  etx            Byte that marks the end of a frame.
 * @param[in]  rxTimeout      Threshold that rxTimeoutCounter must exceed
 *                            before a partial frame is discarded. Not a
 *                            per-frame silence or inter-byte timeout; see
 *                            comstxetxTimeoutCounter for how the counter
 *                            accumulates across frames.
 * @param[in]  packetProcess  Called with the completed frame and its length.
 *                            The frame holds the STX byte at index 0
 *                            followed by the payload bytes; ETX itself is
 *                            not stored, unlike comatReceive, which stores
 *                            every byte of its frame including the leading
 *                            'A', 'T' and the trailing CR LF.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 */
void comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer, uint32_t rxSize, uint32_t txSize, uint8_t stx, uint8_t etx, uint32_t rxTimeout, void (*packetProcess) ( uint8_t* buffer, uint32_t index ) )
{
    uint32_t i = 0;
    
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
    }
}

/**
 * @brief   Called from a periodic timer tick; increments rxTimeoutCounter
 *          while a partial frame is pending and discards it once the
 *          counter exceeds rxTimeout.
 * @param[in,out] driver  Framework state.
 * @note    rxTimeoutCounter is ticks accumulated while any partial frame
 *          was pending since the last comstxetxInit or the last timeout.
 *          It is not cleared when a frame completes, since
 *          comstxetxEvaluate does not touch it, so the tick budget
 *          available to a frame depends on how long earlier frames took.
 *          Once the counter has drifted past rxTimeout, the next partial
 *          frame is discarded on its very first tick.
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
