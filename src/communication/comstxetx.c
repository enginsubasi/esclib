/**
  ******************************************************************************
  *
  * @file      comstxetx.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   1.0.0
  * @date      26/08/2020
  *
  * @brief     Basic STX, ETX communication framework.
  *
  * @par Device
  * Generic
  *
  * @note      Any payload byte value is allowed. A byte equal to stx, etx or
  *            dle travels preceded by dle, and the byte after dle is always
  *            data.
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
  * 05/08/2026 Breaking change. Frames now carry a DLE escape and a @n
  *            two byte check. Init takes the escape byte and a @n
  *            checksum callback, and packetProcess receives the @n
  *            payload alone, without the STX byte and without the @n
  *            check bytes. Existing callers will not compile, which @n
  *            is deliberate: a format change that still compiled @n
  *            would corrupt a working link silently. @n
  *
  ******************************************************************************
  */
  
#include <stddef.h>

#include "comstxetx.h"

// comstxetxReceive writes rxBuffer[ rxIndex ] and only afterwards checks the
// index against rxSize. The smallest meaningful frame carries an empty payload
// and the two checksum bytes, so that pair is also the smallest buffer that
// never takes a write past its end.
#define COMSTXETX_MIN_RX_SIZE   2

// The smallest frame comstxetxBuildFrame can emit is STX, two checksum bytes
// and ETX, with no byte needing an escape.
#define COMSTXETX_MIN_TX_SIZE   4

// Bytes of checksum carried at the end of every frame, low byte first.
#define COMSTXETX_CHECKSUM_SIZE 2

/**
 * @brief   Initializes the STX, ETX communication framework.
 * @param[out] driver         Framework state to initialize.
 * @param[in]  rxBuffer       Caller owned receive buffer.
 * @param[in]  txBuffer       Caller owned transmit buffer.
 * @param[in]  rxSize         Size of rxBuffer in bytes.
 * @param[in]  txSize         Size of txBuffer in bytes.
 * @param[in]  stx            Byte that marks the start of a frame.
 * @param[in]  etx            Byte that marks the end of a frame.
 * @param[in]  dle            Escape byte. A payload byte equal to stx, etx or
 *                            dle is sent preceded by this byte, and the byte
 *                            after it is always data.
 * @param[in]  rxTimeout      Number of comstxetxTimeoutCounter ticks a
 *                            partial frame may stay pending. It is discarded
 *                            once the tick count exceeds this value, so the
 *                            budget is rxTimeout + 1 ticks. This is a whole
 *                            frame timeout, not an inter-byte one.
 * @param[in]  checksum       Computes the frame check over the unescaped
 *                            payload. The signature is that of crc16, so
 *                            crc16 and crc16Alt can be passed directly.
 * @param[in]  packetProcess  Called with the payload alone, without the STX
 *                            byte and without the checksum bytes.
 * @return  TRUE on success, FALSE when a pointer is NULL, rxSize is below
 *          two, txSize is below four, or stx, etx and dle are not three
 *          distinct values.
 * @note    Both buffers are zero filled here and are not copied. They must
 *          outlive the driver.
 * @note    Both callbacks are required. comstxetxEvaluate calls them without
 *          checking, so a NULL here would only surface as a crash on the
 *          first complete frame.
 * @note    rxSize must hold at least the two checksum bytes.
 *          comstxetxReceive stores a byte before it compares the index
 *          against rxSize, so a shorter buffer would take a write past its
 *          end.
 * @note    The three framing bytes must differ from one another. Two equal
 *          values leave at least one of them unable to mean what it names.
 */
uint8_t comstxetxInit ( comstxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                        uint32_t rxSize, uint32_t txSize,
                        uint8_t stx, uint8_t etx, uint8_t dle,
                        uint32_t rxTimeout,
                        uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ),
                        void ( *packetProcess ) ( uint8_t* buffer, uint32_t index ) )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
            ( rxSize >= COMSTXETX_MIN_RX_SIZE ) && ( txSize >= COMSTXETX_MIN_TX_SIZE ) &&
            ( stx != etx ) && ( stx != dle ) && ( etx != dle ) &&
            ( checksum != NULL ) && ( packetProcess != NULL ) )
    {
        // Function assignment.
        driver->checksum = checksum;
        driver->packetProcess = packetProcess;

        // Parameter settings.
        driver->rxBuffer = rxBuffer;
        driver->txBuffer = txBuffer;

        driver->rxSize = rxSize;
        driver->txSize = txSize;

        driver->stx = stx;
        driver->etx = etx;
        driver->dle = dle;

        driver->rxTimeoutCounter = 0;
        driver->rxTimeout = rxTimeout;

        // Initialize to zero and FALSE
        driver->rxIndex = 0;
        driver->rxFrameOpen = FALSE;
        driver->rxEscape = FALSE;
        driver->rxReadyToEvaluate = FALSE;
        driver->rxRejectCount = 0;

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
 *          the unescaped payload of a frame that starts at the STX byte and
 *          completes at an unescaped ETX byte.
 * @param[in,out] driver  Framework state.
 * @param[in]     data    Byte received from the interface.
 * @note    The buffer holds the payload alone. STX is not stored, ETX is not
 *          stored, and the escape bytes are consumed as they are seen. The
 *          two checksum bytes are stored and are stripped by
 *          comstxetxEvaluate, which is where they are checked.
 * @note    The byte after DLE is always data, whatever it is. That is the
 *          whole escape rule, and it is why a payload may contain any byte.
 * @note    An unescaped STX inside an open frame restarts the payload rather
 *          than being stored. A payload STX always arrives escaped, so an
 *          unescaped one can only mean the sender began again.
 * @note    If rxBuffer fills before ETX arrives, the partial frame is
 *          discarded and the driver goes back to looking for STX. Bytes are
 *          ignored while a completed frame is still waiting for
 *          comstxetxEvaluate.
 */
void comstxetxReceive ( comstxetx_t* driver, uint8_t data )
{
    uint8_t store = FALSE;

    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxFrameOpen == FALSE )
        {
            if ( data == driver->stx )
            {
                driver->rxFrameOpen = TRUE;
                driver->rxIndex = 0;
                driver->rxEscape = FALSE;
            }
            else
            {
                /* Intentionally blank */
            }
        }
        else if ( driver->rxEscape == TRUE )
        {
            driver->rxEscape = FALSE;
            store = TRUE;
        }
        else if ( data == driver->dle )
        {
            driver->rxEscape = TRUE;
        }
        else if ( data == driver->etx )
        {
            driver->rxReadyToEvaluate = TRUE;
        }
        else if ( data == driver->stx )
        {
            driver->rxIndex = 0;
        }
        else
        {
            store = TRUE;
        }

        if ( store == TRUE )
        {
            driver->rxBuffer[ driver->rxIndex ] = data;
            ++driver->rxIndex;

            if ( driver->rxIndex >= driver->rxSize )
            {
                // Terminate all received bytes.
                driver->rxFrameOpen = FALSE;
                driver->rxEscape = FALSE;
                driver->rxIndex = 0;
                driver->rxTimeoutCounter = 0;
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
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Called from the main loop; verifies the frame check and runs the
 *          packet callback when a complete frame is waiting.
 * @param[in,out] driver  Framework state.
 * @note    The check runs here rather than in comstxetxReceive because it
 *          walks the whole payload, and comstxetxReceive runs per byte from
 *          an interrupt.
 * @note    The last two stored bytes are the received check, low byte first.
 *          packetProcess is handed everything before them.
 * @note    A frame that fails, and a frame too short to carry a check at all,
 *          are both discarded and counted in rxRejectCount. A frame dropped
 *          with no trace is the worst thing a link can do to whoever has to
 *          diagnose it.
 */
void comstxetxEvaluate ( comstxetx_t* driver )
{
    uint16_t received = 0;
    uint16_t computed = 0;
    uint32_t payloadLength = 0;

    if ( driver->rxReadyToEvaluate == TRUE )
    {
        if ( driver->rxIndex >= COMSTXETX_CHECKSUM_SIZE )
        {
            payloadLength = driver->rxIndex - COMSTXETX_CHECKSUM_SIZE;

            received = ( uint16_t ) driver->rxBuffer[ payloadLength ];
            received = ( uint16_t ) ( received |
                       ( uint16_t ) ( ( uint16_t ) driver->rxBuffer[ payloadLength + 1u ] << 8 ) );

            computed = driver->checksum ( driver->rxBuffer, payloadLength );

            if ( received == computed )
            {
                driver->packetProcess ( driver->rxBuffer, payloadLength );
            }
            else
            {
                ++driver->rxRejectCount;
            }
        }
        else
        {
            ++driver->rxRejectCount;
        }

        driver->rxIndex = 0;
        driver->rxFrameOpen = FALSE;
        driver->rxEscape = FALSE;
        driver->rxReadyToEvaluate = FALSE;
        driver->rxTimeoutCounter = 0;
    }
    else
    {
        /* Intentionally blank */
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
 * @note    Pending means a frame is open, not that a byte has been stored.
 *          A frame that has seen its STX and nothing since must still time
 *          out, and keying this off rxIndex would leave it pending for good
 *          now that the STX byte is no longer stored.
 */
void comstxetxTimeoutCounter ( comstxetx_t* driver )
{
    if ( ( driver->rxFrameOpen == TRUE ) && ( driver->rxReadyToEvaluate == FALSE ) )
    {
        if ( driver->rxTimeoutCounter > driver->rxTimeout )
        {
            // Terminate all received bytes.
            driver->rxFrameOpen = FALSE;
            driver->rxEscape = FALSE;
            driver->rxIndex = 0;
            driver->rxTimeoutCounter = 0;
        }
        else
        {
            ++driver->rxTimeoutCounter;
        }
    }
}

/**
 * @brief   Builds a complete wire frame in txBuffer from a payload.
 * @param[in,out] driver       Framework state. txBuffer is written.
 * @param[in]     payload      Bytes to carry. Any byte value is allowed.
 * @param[in]     length       Number of payload bytes. Zero is legal and
 *                             produces a frame carrying only the check.
 * @param[out]    frameLength  Number of bytes written to txBuffer.
 * @return  TRUE on success, FALSE when an argument is NULL or the escaped
 *          frame would not fit in txBuffer.
 * @note    On FALSE the contents of txBuffer are undefined and frameLength is
 *          set to zero. The function does not pre-check the worst case of
 *          1 + 2 * ( length + 2 ) + 1, because that would refuse frames that
 *          fit comfortably whenever few payload bytes need an escape.
 * @note    It does not transmit. comstxetx has no transmission trigger the
 *          way comat does; the buffer is filled and the length reported, and
 *          sending is the caller's.
 * @note    The check is computed over the unescaped payload, so escaping
 *          cannot change it. The check bytes are themselves escaped, without
 *          which a check byte equal to etx would close the frame it protects.
 */
uint8_t comstxetxBuildFrame ( comstxetx_t* driver, const uint8_t* const payload,
                              uint32_t length, uint32_t* frameLength )
{
    uint8_t retVal = FALSE;
    uint8_t overflow = FALSE;
    uint8_t byte = 0;
    uint8_t needed = 0;
    uint16_t sum = 0;
    uint32_t i = 0;
    uint32_t out = 0;
    uint8_t check[ COMSTXETX_CHECKSUM_SIZE ];

    if ( ( driver != NULL ) && ( payload != NULL ) && ( frameLength != NULL ) )
    {
        sum = driver->checksum ( payload, length );
        check[ 0 ] = ( uint8_t ) ( sum & 0xFFu );
        check[ 1 ] = ( uint8_t ) ( ( sum >> 8 ) & 0xFFu );

        // txSize is at least four, so the STX always fits.
        driver->txBuffer[ out ] = driver->stx;
        ++out;

        for ( i = 0; ( i < ( length + COMSTXETX_CHECKSUM_SIZE ) ) && ( overflow == FALSE ); ++i )
        {
            if ( i < length )
            {
                byte = payload[ i ];
            }
            else
            {
                byte = check[ i - length ];
            }

            if ( ( byte == driver->stx ) || ( byte == driver->etx ) ||
                    ( byte == driver->dle ) )
            {
                needed = 2;
            }
            else
            {
                needed = 1;
            }

            // The trailing ETX is reserved here so the frame cannot fail late.
            if ( ( out + needed + 1u ) > driver->txSize )
            {
                overflow = TRUE;
            }
            else
            {
                if ( needed == 2 )
                {
                    driver->txBuffer[ out ] = driver->dle;
                    ++out;
                }
                else
                {
                    /* Intentionally blank */
                }

                driver->txBuffer[ out ] = byte;
                ++out;
            }
        }

        if ( overflow == FALSE )
        {
            driver->txBuffer[ out ] = driver->etx;
            ++out;

            *frameLength = out;
            retVal = TRUE;
        }
        else
        {
            *frameLength = 0;
            retVal = FALSE;
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reports how many frames have been discarded without reaching
 *          packetProcess.
 * @param[in] driver  Framework state.
 * @return  Count of frames rejected since Init.
 * @note    It counts both causes together: a frame whose checksum did not
 *          match, and a frame too short to carry a checksum at all. Nothing a
 *          caller does differs between the two, so they are not separated.
 */
uint32_t comstxetxGetRejectCount ( const comstxetx_t* const driver )
{
    uint32_t retVal = 0;

    retVal = driver->rxRejectCount;

    return ( retVal );
}
