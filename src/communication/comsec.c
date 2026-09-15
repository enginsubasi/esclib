/**
  ******************************************************************************
  *
  * @file      comsec.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      13/09/2022
  *
  * @brief     Authenticated framing with replay protection.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 13/09/2022 Created @n
  * 15/09/2026 Implemented. The file had been empty since 2022 and @n
  *            the name reserved, because a module that contained @n
  *            cryptography could not stay independent of everything @n
  *            else the way the rest of this library does. It does @n
  *            not contain any: the primitive is injected, exactly @n
  *            as comstxetx injects its check. @n
  *
  * @note      **This module contains no cryptography.** It is the framing
  *            around one: a session id, a monotonic counter and a tag, with
  *            the tag computed by a function the caller installs at Init. That
  *            is the same hook comstxetx uses for its check and it buys the
  *            same two things — the module stays independent of any particular
  *            algorithm, and the caller brings whichever one their platform
  *            already has, which on a part with an accelerator is not a
  *            software implementation at all.
  *
  * @warning   **It provides authenticity, integrity and replay protection. It
  *            does not provide confidentiality.** The payload travels in the
  *            clear and anyone watching the link can read it. A caller who
  *            needs secrecy encrypts the payload before handing it here and
  *            decrypts what comes back; this module will then authenticate the
  *            ciphertext, which is the order those two operations belong in.
  *
  * @warning   The security of this rests entirely on the injected function and
  *            the key behind it. A tag from a plain CRC or a checksum makes
  *            the whole thing worthless against anyone who can compute one,
  *            which is everyone: it has to be a keyed message authentication
  *            code. What is here cannot check that, and nothing at this level
  *            could.
  *
  * @note      The counter is the replay guard and it is also the reason this
  *            is not comsafe with a different name. comsafe's sequence
  *            number expects the very next value, because a safety channel
  *            treats a lost frame as a fault. Here the rule is **strictly
  *            greater**, because a link may legitimately lose frames and the
  *            only thing that must never happen is a frame being accepted
  *            twice.
  *
  * @note      The counter is thirty two bits and does **not** wrap. Once it is
  *            spent, comsecBuildFrame refuses to send rather than starting
  *            again from zero: a repeated counter under the same key is the
  *            one failure that breaks every guarantee at once, and refusing to
  *            transmit is the only safe thing left to do. comsecRekey is how a
  *            caller who has established new key material carries on.
  *
  * @note      The tag comparison is constant time. A byte by byte compare that
  *            returns on the first difference tells an attacker how many
  *            leading bytes of a guess were right, which turns forging a tag
  *            from an infeasible search into a short one. This is the one
  *            place in this library where the obvious loop would be a
  *            security defect rather than merely slower.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "comsec.h"

/* Offsets into a frame. Session id, counter, payload, tag. */
#define COMSEC_ID_HIGH      0u
#define COMSEC_ID_LOW       1u
#define COMSEC_COUNTER      2u
#define COMSEC_PAYLOAD      6u

/**
 * @brief   Records a rejection.
 * @param[in,out] driver  Initialized driver.
 * @param[in]     reason  Which check refused the frame.
 * @note    Unlike comsafe, a rejection does not take the channel out of
 *          service. A network carrying this will be offered forged and
 *          replayed frames as a matter of course, and a link that stopped on
 *          the first one would be trivially denied service by anyone who could
 *          send a packet. The count is what makes an attempt visible.
 */
static void comsecReject ( comsec_t* driver, uint8_t reason )
{
    driver->lastError = reason;

    ++driver->rejectCount;
}

/**
 * @brief   Compares two tags without leaking where they differ.
 * @param[in] a       First tag.
 * @param[in] b       Second tag.
 * @param[in] length  Bytes to compare.
 * @return  TRUE when every byte matches.
 * @note    Every byte is read and the differences are accumulated, so the time
 *          this takes depends only on the length. The obvious loop, which
 *          returns as soon as two bytes differ, tells an attacker measuring it
 *          how many leading bytes of a guessed tag were correct; that turns
 *          forging one from a search over the whole tag into a search over one
 *          byte at a time, which is the difference between infeasible and an
 *          afternoon.
 * @note    The accumulator is uint8_t and the result is compared once, at the
 *          end. There is no early exit and no branch on the data.
 */
static uint8_t comsecTagEqual ( const uint8_t* const a, const uint8_t* const b, uint32_t length )
{
    uint8_t retVal = FALSE;
    uint8_t difference = 0;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        difference = ( uint8_t ) ( difference | ( uint8_t ) ( a[ i ] ^ b[ i ] ) );
    }

    if ( difference == 0 )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes the authenticated channel.
 * @param[out] driver     Driver state to initialize.
 * @param[in]  rxBuffer   Caller owned storage for a received payload.
 * @param[in]  txBuffer   Caller owned storage for a frame being built.
 * @param[in]  rxSize     Bytes of receive storage.
 * @param[in]  txSize     Bytes of transmit storage.
 * @param[in]  sessionId  Identifies the key material in use.
 * @param[in]  tagSize    Bytes of tag, from COMSEC_MIN_TAG to COMSEC_MAX_TAG.
 * @param[in]  mac        Keyed authentication over the framed bytes, writing
 *                        tagSize bytes.
 * @return  TRUE on success, FALSE when any pointer is NULL, when a buffer is
 *          too small for a frame with one payload byte, or when tagSize is
 *          outside its range.
 * @note    A tag shorter than COMSEC_MIN_TAG is refused. Four bytes already
 *          means one forgery attempt in four thousand million succeeds by
 *          chance, and on a link an attacker can hammer that is not a
 *          comfortable margin; eight is the usual floor and sixteen leaves
 *          nothing to argue about. Truncating a longer tag to the size asked
 *          for is the installed function's business.
 * @note    The counters start at zero on both sides and the receiver is not
 *          armed, so the first frame is accepted at whatever counter it
 *          carries and the window follows from there. A caller resuming a
 *          session across a reset must use comsecRekey with the counter it
 *          left off at, or the replay guard starts again from nothing.
 */
uint8_t comsecInit ( comsec_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                     uint32_t rxSize, uint32_t txSize,
                     uint16_t sessionId, uint8_t tagSize,
                     void ( *mac ) ( const uint8_t* const buffer, uint32_t length, uint8_t* tag ) )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
            ( mac != NULL ) && ( rxSize != 0 ) &&
            ( tagSize >= COMSEC_MIN_TAG ) && ( tagSize <= COMSEC_MAX_TAG ) &&
            ( txSize > ( COMSEC_HEADER + ( uint32_t ) tagSize ) ) )
    {
        driver->rxBuffer = rxBuffer;
        driver->txBuffer = txBuffer;
        driver->rxSize = rxSize;
        driver->txSize = txSize;
        driver->sessionId = sessionId;
        driver->tagSize = tagSize;
        driver->mac = mac;

        driver->rxPayloadLength = 0;
        driver->txCounter = 0;
        driver->rxCounter = 0;
        driver->rejectCount = 0;
        driver->rxArmed = FALSE;
        driver->lastError = ( uint8_t ) CSEC_NONE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Wraps a payload into an authenticated frame.
 * @param[in,out] driver       Initialized driver.
 * @param[in]     payload      Bytes to send, already encrypted if secrecy is
 *                             wanted.
 * @param[in]     length       Number of bytes.
 * @param[out]    frameLength  Bytes of the finished frame.
 * @return  TRUE on success, FALSE when an argument is bad, when the frame does
 *          not fit, or when the counter is spent.
 * @note    The tag covers the session id and the counter as well as the
 *          payload. A tag over the payload alone would let an attacker move a
 *          valid payload to a different counter, which is replay with extra
 *          steps.
 * @note    The counter is spent at 0xFFFFFFFF and this refuses rather than
 *          wrapping. Two frames under one key and counter is the failure that
 *          breaks authenticity and replay protection together, so there is
 *          nothing safe left to do but stop. comsecGetCounter is how a caller
 *          sees it coming; comsecRekey is how it carries on.
 */
uint8_t comsecBuildFrame ( comsec_t* driver, const uint8_t* const payload,
                           uint32_t length, uint32_t* frameLength )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t total = 0;

    if ( ( driver == NULL ) || ( payload == NULL ) || ( length == 0 ) )
    {
        retVal = FALSE;
    }
    else if ( driver->txCounter == 0xFFFFFFFFu )
    {
        driver->lastError = ( uint8_t ) CSEC_EXHAUSTED;

        retVal = FALSE;
    }
    else
    {
        total = length + COMSEC_HEADER + ( uint32_t ) driver->tagSize;

        if ( total <= driver->txSize )
        {
            ++driver->txCounter;

            driver->txBuffer[ COMSEC_ID_HIGH ] =
                    ( uint8_t ) ( ( driver->sessionId >> 8 ) & 0xFFu );
            driver->txBuffer[ COMSEC_ID_LOW ] =
                    ( uint8_t ) ( driver->sessionId & 0xFFu );

            driver->txBuffer[ COMSEC_COUNTER ] =
                    ( uint8_t ) ( ( driver->txCounter >> 24 ) & 0xFFu );
            driver->txBuffer[ COMSEC_COUNTER + 1u ] =
                    ( uint8_t ) ( ( driver->txCounter >> 16 ) & 0xFFu );
            driver->txBuffer[ COMSEC_COUNTER + 2u ] =
                    ( uint8_t ) ( ( driver->txCounter >> 8 ) & 0xFFu );
            driver->txBuffer[ COMSEC_COUNTER + 3u ] =
                    ( uint8_t ) ( driver->txCounter & 0xFFu );

            for ( i = 0; i < length; ++i )
            {
                driver->txBuffer[ COMSEC_PAYLOAD + i ] = payload[ i ];
            }

            driver->mac ( driver->txBuffer, COMSEC_HEADER + length,
                            &driver->txBuffer[ COMSEC_HEADER + length ] );

            if ( frameLength != NULL )
            {
                ( *frameLength ) = total;
            }
            else
            {
                /* Intentionally blank */
            }

            retVal = TRUE;
        }
        else
        {
            driver->lastError = ( uint8_t ) CSEC_LENGTH;

            retVal = FALSE;
        }
    }

    return ( retVal );
}

/**
 * @brief   Authenticates a received frame and, on success, keeps its payload.
 * @param[in,out] driver       Initialized driver.
 * @param[in]     frame        Whole frame as the transport delivered it.
 * @param[in]     frameLength  Bytes in it.
 * @return  TRUE when the frame authenticated and was not a replay.
 * @note    The tag is verified **before** anything else in the frame is
 *          believed. Checking the session id or the counter first would mean
 *          acting on fields an attacker controls, and reporting a wrong
 *          session for what is really a forgery sends the caller looking in
 *          the wrong place. The length is the only thing checked earlier,
 *          because the tag cannot be located without it.
 * @note    The counter must be **strictly greater** than the last accepted
 *          one, not the next one along. A link may lose frames legitimately;
 *          what must never happen is the same counter being accepted twice.
 *          Frames that arrive out of order are therefore refused rather than
 *          reordered, which is the conservative reading and the only one that
 *          keeps the guarantee simple.
 * @note    A rejection does not stop the channel. A link exposed to an
 *          attacker will be offered forged frames as a matter of course, and
 *          one that failed closed on the first would be denied service by
 *          anyone able to send a packet. The reject count is what makes the
 *          attempt visible.
 */
uint8_t comsecCheckFrame ( comsec_t* driver, const uint8_t* const frame, uint32_t frameLength )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t payloadLength = 0;
    uint32_t overhead = 0;
    uint32_t counter = 0;
    uint16_t id = 0;
    uint8_t expected[ COMSEC_MAX_TAG ];

    overhead = COMSEC_HEADER + ( uint32_t ) driver->tagSize;

    if ( ( driver == NULL ) || ( frame == NULL ) )
    {
        retVal = FALSE;
    }
    else if ( ( frameLength <= overhead ) ||
                ( ( frameLength - overhead ) > driver->rxSize ) )
    {
        comsecReject ( driver, ( uint8_t ) CSEC_LENGTH );

        retVal = FALSE;
    }
    else
    {
        payloadLength = frameLength - overhead;

        driver->mac ( frame, COMSEC_HEADER + payloadLength, expected );

        if ( comsecTagEqual ( expected, &frame[ COMSEC_HEADER + payloadLength ],
                                ( uint32_t ) driver->tagSize ) == FALSE )
        {
            comsecReject ( driver, ( uint8_t ) CSEC_TAG );

            retVal = FALSE;
        }
        else
        {
            id = ( uint16_t ) ( ( ( uint16_t ) frame[ COMSEC_ID_HIGH ] ) << 8 );
            id = ( uint16_t ) ( id | ( uint16_t ) frame[ COMSEC_ID_LOW ] );

            counter = ( ( uint32_t ) frame[ COMSEC_COUNTER ] ) << 24;
            counter |= ( ( uint32_t ) frame[ COMSEC_COUNTER + 1u ] ) << 16;
            counter |= ( ( uint32_t ) frame[ COMSEC_COUNTER + 2u ] ) << 8;
            counter |= ( uint32_t ) frame[ COMSEC_COUNTER + 3u ];

            if ( id != driver->sessionId )
            {
                comsecReject ( driver, ( uint8_t ) CSEC_SESSION );

                retVal = FALSE;
            }
            else if ( ( driver->rxArmed == TRUE ) &&
                        ( counter <= driver->rxCounter ) )
            {
                comsecReject ( driver, ( uint8_t ) CSEC_REPLAY );

                retVal = FALSE;
            }
            else
            {
                for ( i = 0; i < payloadLength; ++i )
                {
                    driver->rxBuffer[ i ] = frame[ COMSEC_PAYLOAD + i ];
                }

                driver->rxPayloadLength = payloadLength;
                driver->rxCounter = counter;
                driver->rxArmed = TRUE;
                driver->lastError = ( uint8_t ) CSEC_NONE;

                retVal = TRUE;
            }
        }
    }

    return ( retVal );
}

/**
 * @brief   Gets the payload of the last frame that authenticated.
 * @param[in] driver  Driver state.
 * @return  Pointer to the stored payload.
 * @note    Only ever written by a frame that passed every check, so what is
 *          here was authentic when it arrived. It is still the *last* one,
 *          which after a rejection means it is old rather than forged.
 */
const uint8_t* comsecGetPayload ( const comsec_t* const driver )
{
    return ( driver->rxBuffer );
}

/**
 * @brief   Gets the length of the payload of the last frame that
 *          authenticated.
 * @param[in] driver  Driver state.
 * @return  Bytes, or zero when nothing has been accepted yet.
 */
uint32_t comsecGetPayloadLength ( const comsec_t* const driver )
{
    return ( driver->rxPayloadLength );
}

/**
 * @brief   Gets the transmit counter.
 * @param[in] driver  Driver state.
 * @return  The counter the last frame went out under.
 * @note    This is how a caller sees exhaustion coming. A channel sending a
 *          frame every millisecond spends thirty two bits in about seven
 *          weeks, which is well inside the life of a product and is why
 *          comsecRekey exists.
 */
uint32_t comsecGetCounter ( const comsec_t* const driver )
{
    return ( driver->txCounter );
}

/**
 * @brief   Gets the reason for the most recent rejection.
 * @param[in] driver  Driver state.
 * @return  One of COMSEC_ERROR.
 */
uint8_t comsecGetLastError ( const comsec_t* const driver )
{
    return ( driver->lastError );
}

/**
 * @brief   Gets the number of frames rejected since Init.
 * @param[in] driver  Driver state.
 * @return  Frames refused for any reason.
 * @note    On a link nobody is attacking this stays at zero or tracks the
 *          error rate of the medium. A count that climbs while the medium is
 *          quiet is somebody trying, and is the one signal this module can
 *          give that nothing else will.
 */
uint32_t comsecGetRejectCount ( const comsec_t* const driver )
{
    return ( driver->rejectCount );
}

/**
 * @brief   Moves the channel onto new key material.
 * @param[in,out] driver     Initialized driver.
 * @param[in]     sessionId  Identifier of the new key material.
 * @param[in]     txCounter  Counter the next frame will be sent under.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    The key itself is not here — it lives behind the installed
 *          function, and rekeying means the caller has already changed what
 *          that function uses. This call tells the framing about it.
 * @warning **Never rekey to a session id and counter that have been used with
 *          the same key before.** Two frames under one key and one counter is
 *          the single fatal mistake available here: it breaks authenticity and
 *          replay protection at once. A new key means the counter may start
 *          anywhere; the same key means it must carry on from where it was.
 * @note    The receiver is disarmed, so the first frame after a rekey is
 *          accepted at whatever counter it carries. That is unavoidable — the
 *          peer's counter is not knowable from here — and is safe only because
 *          the tag still has to verify under the new key.
 * @note    The reject count survives, for the reason comsafeReset leaves its
 *          error count alone: a channel that has been rekeyed after a run of
 *          rejections is telling the caller something a cleared counter would
 *          hide.
 */
uint8_t comsecRekey ( comsec_t* driver, uint16_t sessionId, uint32_t txCounter )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        driver->sessionId = sessionId;
        driver->txCounter = txCounter;
        driver->rxCounter = 0;
        driver->rxArmed = FALSE;
        driver->rxPayloadLength = 0;
        driver->lastError = ( uint8_t ) CSEC_NONE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
