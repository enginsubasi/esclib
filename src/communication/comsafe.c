/**
  ******************************************************************************
  *
  * @file      comsafe.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      18/03/2023
  *
  * @brief     Safe communication framework.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 18/03/2023 Created @n
  * 15/09/2026 Implemented. The file had held a banner and nothing @n
  *            else since 2023, and comsafe.h declared no types at @n
  *            all. @n
  *
  * @note      This is the black channel pattern. The transport underneath —
  *            comstxetx, a UART, a CAN bus, anything — is treated as opaque
  *            and untrusted, and everything that has to be relied on is
  *            carried end to end inside the payload it hands across. That is
  *            why this is not another byte level state machine: comstxetx
  *            already delivers whole frames, and duplicating its framing here
  *            would add a second thing to get wrong without adding a second
  *            independent check.
  *
  * @note      The four failures a layer like this exists to catch are the ones
  *            a transport's own CRC cannot: a frame **lost**, a frame
  *            **repeated**, frames **reordered**, and a frame arriving from
  *            the **wrong sender**. A sequence number catches the first three
  *            and a connection id the fourth. A watchdog catches the case
  *            where nothing arrives at all, which is the failure no frame can
  *            report because there is no frame.
  *
  * @note      The integrity check is a function installed at Init, exactly as
  *            comstxetx installs one, and for the same two reasons: it keeps
  *            this module independent of crc/, and it lets the safety layer
  *            use a **different** algorithm from the transport underneath. Two
  *            layers sharing one CRC share its blind spots, which defeats the
  *            point of checking twice.
  *
  * @warning   **This is not cryptography and provides no security.** It
  *            detects accidental corruption, loss, repetition and reordering.
  *            An attacker who can modify a frame can recompute the check over
  *            it, forge the connection id and advance the sequence, because
  *            none of the three involves a secret. Resisting that needs a
  *            message authentication code and a key, which this module does
  *            not have and does not pretend to. The connection id tells one
  *            configured peer from another; it is not authentication.
  *
  * @warning   Nor is it a certification. A safety function assessed against
  *            IEC 61508 or ISO 13849 needs evidence about the whole channel —
  *            the residual error rate of the check over the real bit error
  *            rate, the reaction time budget, the diagnostic coverage — none
  *            of which a library can supply on the caller's behalf. What is
  *            here is the framing those arguments are made about.
  *
  * @note      The sequence number is one byte and wraps at 256. A gap of
  *            exactly 256 frames therefore looks like no gap at all. That is
  *            the same bound every protocol of this shape carries and is the
  *            reason the watchdog is not optional: a channel that could lose
  *            256 frames in a row will trip the timeout long before the
  *            sequence stops being able to tell.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "comsafe.h"

/* Offsets into a frame. Connection id, sequence, payload, check. */
#define COMSAFE_ID_HIGH     0u
#define COMSAFE_ID_LOW      1u
#define COMSAFE_SEQUENCE    2u
#define COMSAFE_PAYLOAD     3u

/* Bytes in front of the payload, and the two check bytes behind it. */
#define COMSAFE_HEADER      3u
#define COMSAFE_CHECK       2u

/**
 * @brief   Records a rejection and drops the channel out of service.
 * @param[in,out] driver  Initialized driver.
 * @param[in]     reason  Which check refused the frame.
 * @note    Every rejection moves the state to CS_FAILED rather than leaving it
 *          where it was. A safety channel that has seen one bad frame is not
 *          a channel that is fine except for that frame: whatever produced it
 *          is still there, and the caller decides when to trust it again by
 *          calling comsafeReset.
 */
static void comsafeFail ( comsafe_t* driver, uint8_t reason )
{
    driver->lastError = reason;
    driver->state = ( uint8_t ) CS_FAILED;

    ++driver->errorCount;
}

/**
 * @brief   Initializes the safety layer.
 * @param[out] driver        Driver state to initialize.
 * @param[in]  rxBuffer      Caller owned storage for a received payload.
 * @param[in]  txBuffer      Caller owned storage for a frame being built.
 * @param[in]  rxSize        Bytes of receive storage.
 * @param[in]  txSize        Bytes of transmit storage.
 * @param[in]  connectionId  Identifies this channel and its peer.
 * @param[in]  timeout       Ticks of comsafeTimeoutCounter without a valid
 *                           frame before the channel is declared failed.
 * @param[in]  checksum      Integrity check over the framed bytes.
 * @return  TRUE on success, FALSE when any pointer is NULL, when either
 *          buffer is too small to hold a frame with one payload byte, or when
 *          the timeout is zero.
 * @note    A zero timeout is rejected because a watchdog that never fires is
 *          not a watchdog, and the caller who passed one almost certainly
 *          meant to disable the check rather than to make every frame late.
 *          Deciding which for them would be a guess.
 * @note    The checksum is required rather than optional. A safety layer
 *          without an integrity check of its own is a sequence counter, and
 *          calling it safe would be the misleading part.
 * @note    The channel starts in CS_INIT, not CS_OK. Nothing has been heard
 *          from the peer yet, and a channel that reports itself healthy before
 *          its first frame would let a caller act on a link that was never up.
 */
uint8_t comsafeInit ( comsafe_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                      uint32_t rxSize, uint32_t txSize,
                      uint16_t connectionId, uint32_t timeout,
                      uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ) )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
            ( rxSize != 0 ) && ( txSize > COMSAFE_OVERHEAD ) &&
            ( timeout != 0 ) && ( checksum != NULL ) )
    {
        driver->rxBuffer = rxBuffer;
        driver->txBuffer = txBuffer;
        driver->rxSize = rxSize;
        driver->txSize = txSize;
        driver->connectionId = connectionId;
        driver->timeout = timeout;
        driver->checksum = checksum;

        driver->rxPayloadLength = 0;
        driver->timeoutCounter = 0;
        driver->errorCount = 0;
        driver->txSequence = 0;
        driver->rxSequence = 0;
        driver->rxArmed = FALSE;
        driver->state = ( uint8_t ) CS_INIT;
        driver->lastError = ( uint8_t ) CS_NONE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Wraps a payload into a frame in the transmit buffer.
 * @param[in,out] driver       Initialized driver.
 * @param[in]     payload      Bytes to send.
 * @param[in]     length       Number of bytes.
 * @param[out]    frameLength  Bytes of the finished frame.
 * @return  TRUE on success, FALSE when an argument is bad or the frame does
 *          not fit in the transmit buffer.
 * @note    The sequence number advances on every frame built, including a
 *          retransmission. A safety layer that resent a frame under its old
 *          sequence would be indistinguishable at the far end from the
 *          transport duplicating one, which is exactly what the sequence is
 *          there to catch. Resending means building a new frame.
 * @note    The check is computed over the connection id, the sequence and the
 *          payload together, so none of the three can be altered without it
 *          showing. Computing it over the payload alone would leave the id and
 *          the sequence — the two fields this layer exists for — unprotected.
 */
uint8_t comsafeBuildFrame ( comsafe_t* driver, const uint8_t* const payload,
                            uint32_t length, uint32_t* frameLength )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint16_t check = 0;

    if ( ( driver != NULL ) && ( payload != NULL ) && ( length != 0 ) &&
            ( ( length + COMSAFE_OVERHEAD ) <= driver->txSize ) )
    {
        driver->txBuffer[ COMSAFE_ID_HIGH ] =
                ( uint8_t ) ( ( driver->connectionId >> 8 ) & 0xFFu );
        driver->txBuffer[ COMSAFE_ID_LOW ] =
                ( uint8_t ) ( driver->connectionId & 0xFFu );

        driver->txBuffer[ COMSAFE_SEQUENCE ] = driver->txSequence;

        for ( i = 0; i < length; ++i )
        {
            driver->txBuffer[ COMSAFE_PAYLOAD + i ] = payload[ i ];
        }

        check = driver->checksum ( driver->txBuffer, COMSAFE_HEADER + length );

        driver->txBuffer[ COMSAFE_HEADER + length ] =
                ( uint8_t ) ( ( check >> 8 ) & 0xFFu );
        driver->txBuffer[ COMSAFE_HEADER + length + 1u ] =
                ( uint8_t ) ( check & 0xFFu );

        ++driver->txSequence;

        if ( frameLength != NULL )
        {
            ( *frameLength ) = length + COMSAFE_OVERHEAD;
        }
        else
        {
            /* Intentionally blank */
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
 * @brief   Checks a received frame and, on success, keeps its payload.
 * @param[in,out] driver       Initialized driver.
 * @param[in]     frame        Whole frame as the transport delivered it.
 * @param[in]     frameLength  Bytes in it.
 * @return  TRUE when the frame passed every check, FALSE otherwise.
 * @note    The checks run in this order and stop at the first failure: length,
 *          integrity, connection id, sequence. Integrity comes before the two
 *          field checks because a corrupted frame can carry anything in them,
 *          and reporting a wrong id for what is really a flipped bit would
 *          send the caller looking in the wrong place.
 * @note    The **first** frame after Init or Reset is accepted at whatever
 *          sequence it carries, and the expectation is set from it. There is
 *          no way to know where a peer that has been running without us has
 *          got to, and refusing until the counters happened to line up would
 *          never start.
 * @note    A frame that passes resets the watchdog and puts the channel in
 *          CS_OK. A frame that fails puts it in CS_FAILED and leaves it there
 *          until comsafeReset, because whatever produced a bad frame is still
 *          present after it.
 * @note    While the channel is CS_FAILED this refuses every frame without
 *          checking it, and without counting it. Those frames are not new
 *          failures; they are frames offered to a connection that is out of
 *          service. Recovery is comsafeReset and nothing else, which is the
 *          whole difference between a safety layer and a retry.
 * @note    On FALSE the stored payload is left as it was. A caller that reads
 *          comsafeGetPayload without checking the status gets the last good
 *          payload rather than a corrupted one — which is still stale, and is
 *          why the state is what should be read first.
 */
uint8_t comsafeCheckFrame ( comsafe_t* driver, const uint8_t* const frame, uint32_t frameLength )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t payloadLength = 0;
    uint16_t computed = 0;
    uint16_t carried = 0;
    uint16_t id = 0;
    uint8_t expected = 0;

    if ( ( driver == NULL ) || ( frame == NULL ) )
    {
        retVal = FALSE;
    }
    else if ( driver->state == ( uint8_t ) CS_FAILED )
    {
        /*
         * A channel that has failed stays failed. Nothing is checked and
         * nothing is counted: these frames are not new failures, they are
         * frames offered to a connection that is out of service.
         */
        retVal = FALSE;
    }
    else if ( ( frameLength <= COMSAFE_OVERHEAD ) ||
                ( ( frameLength - COMSAFE_OVERHEAD ) > driver->rxSize ) )
    {
        comsafeFail ( driver, ( uint8_t ) CS_LENGTH );

        retVal = FALSE;
    }
    else
    {
        payloadLength = frameLength - COMSAFE_OVERHEAD;

        computed = driver->checksum ( frame, COMSAFE_HEADER + payloadLength );

        carried = ( uint16_t ) ( ( ( uint16_t ) frame[ COMSAFE_HEADER + payloadLength ] ) << 8 );
        carried = ( uint16_t ) ( carried | ( uint16_t ) frame[ COMSAFE_HEADER + payloadLength + 1u ] );

        id = ( uint16_t ) ( ( ( uint16_t ) frame[ COMSAFE_ID_HIGH ] ) << 8 );
        id = ( uint16_t ) ( id | ( uint16_t ) frame[ COMSAFE_ID_LOW ] );

        expected = ( uint8_t ) ( driver->rxSequence + 1u );

        if ( computed != carried )
        {
            comsafeFail ( driver, ( uint8_t ) CS_CHECK );

            retVal = FALSE;
        }
        else if ( id != driver->connectionId )
        {
            comsafeFail ( driver, ( uint8_t ) CS_ID );

            retVal = FALSE;
        }
        else if ( ( driver->rxArmed == TRUE ) &&
                    ( frame[ COMSAFE_SEQUENCE ] != expected ) )
        {
            comsafeFail ( driver, ( uint8_t ) CS_SEQUENCE );

            retVal = FALSE;
        }
        else
        {
            for ( i = 0; i < payloadLength; ++i )
            {
                driver->rxBuffer[ i ] = frame[ COMSAFE_PAYLOAD + i ];
            }

            driver->rxPayloadLength = payloadLength;
            driver->rxSequence = frame[ COMSAFE_SEQUENCE ];
            driver->rxArmed = TRUE;

            driver->timeoutCounter = 0;
            driver->state = ( uint8_t ) CS_OK;
            driver->lastError = ( uint8_t ) CS_NONE;

            retVal = TRUE;
        }
    }

    return ( retVal );
}

/**
 * @brief   Gets the payload of the last frame that passed.
 * @param[in] driver  Driver state.
 * @return  Pointer to the stored payload.
 * @note    Read comsafeGetState first. This returns the last payload that
 *          passed every check, which after a failure or a timeout is stale
 *          rather than wrong — and acting on a stale safety value is the
 *          failure this whole module exists to make visible.
 */
const uint8_t* comsafeGetPayload ( const comsafe_t* const driver )
{
    return ( driver->rxBuffer );
}

/**
 * @brief   Gets the length of the payload of the last frame that passed.
 * @param[in] driver  Driver state.
 * @return  Bytes, or zero when no frame has passed yet.
 */
uint32_t comsafeGetPayloadLength ( const comsafe_t* const driver )
{
    return ( driver->rxPayloadLength );
}

/**
 * @brief   Advances the watchdog by one tick.
 * @param[in,out] driver  Initialized driver.
 * @note    Call from a periodic tick, as comstxetxTimeoutCounter is called.
 *          The unit is that tick, so the timeout passed to Init is in ticks
 *          and the interrupt rate is what makes it a time — the rule softtimer
 *          and the shift register drivers follow.
 * @note    This is the only check that can fire when **nothing** arrives, and
 *          that is why it exists. Every other check in this module needs a
 *          frame to run on, so a cable pulled out would otherwise leave the
 *          channel reporting the last good state forever.
 * @note    The counter stops at the timeout rather than running on. It is a
 *          watchdog, not a stopwatch, and a counter that kept climbing would
 *          wrap eventually and rearm the channel on its own.
 */
void comsafeTimeoutCounter ( comsafe_t* driver )
{
    if ( driver->timeoutCounter < driver->timeout )
    {
        ++driver->timeoutCounter;

        if ( driver->timeoutCounter >= driver->timeout )
        {
            comsafeFail ( driver, ( uint8_t ) CS_TIMEOUT );
        }
        else
        {
            /* Intentionally blank */
        }
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Gets the state of the channel.
 * @param[in] driver  Driver state.
 * @return  CS_INIT before the first good frame, CS_OK while the channel is
 *          healthy, CS_FAILED after any rejection or timeout.
 * @note    This is the value a caller acts on. CS_OK means the most recent
 *          frame passed every check and the watchdog has not expired since;
 *          anything else means the payload is not to be trusted, whether
 *          because it is stale or because it was never there.
 */
uint8_t comsafeGetState ( const comsafe_t* const driver )
{
    return ( driver->state );
}

/**
 * @brief   Gets the reason for the most recent rejection.
 * @param[in] driver  Driver state.
 * @return  One of COMSAFE_ERROR.
 * @note    Not cleared by reading it, and cleared by the next frame that
 *          passes. It says why the channel last failed, which is what a
 *          diagnostic wants; the count says how often, which is what a trend
 *          wants.
 */
uint8_t comsafeGetLastError ( const comsafe_t* const driver )
{
    return ( driver->lastError );
}

/**
 * @brief   Gets the number of rejections since Init.
 * @param[in] driver  Driver state.
 * @return  Frames refused, plus timeouts.
 * @note    Not cleared by comsafeReset. A reset says the caller has decided to
 *          trust the channel again; it does not say the failures did not
 *          happen, and a channel that has been reset twenty times is telling
 *          the caller something a cleared counter would hide.
 */
uint32_t comsafeGetErrorCount ( const comsafe_t* const driver )
{
    return ( driver->errorCount );
}

/**
 * @brief   Puts the channel back in service.
 * @param[in,out] driver  Initialized driver.
 * @note    Deliberately explicit. A safety channel does not recover on its own
 *          the moment a good frame turns up, because the caller is the only
 *          one that knows whether the process it controls is in a state where
 *          resuming is allowed. This is the call that says it is.
 * @note    The sequence expectation is dropped with it, so the first frame
 *          after a reset is accepted at whatever sequence it carries — the
 *          peer may have kept running, or restarted, and neither is knowable
 *          from here.
 * @note    The error count survives, for the reason comsafeGetErrorCount
 *          gives.
 */
void comsafeReset ( comsafe_t* driver )
{
    driver->state = ( uint8_t ) CS_INIT;
    driver->lastError = ( uint8_t ) CS_NONE;
    driver->timeoutCounter = 0;
    driver->rxArmed = FALSE;
    driver->rxPayloadLength = 0;
}
