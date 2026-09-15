/**
  ******************************************************************************
  *
  * @file      commodbus.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Modbus RTU framing, with the check function injected.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      This is the framing and nothing above it. There are no function
  *            codes here, no register map and no exception responses, and that
  *            is a decision rather than an unfinished module: a register map is
  *            the application's data, this library allocates nothing, and a
  *            table of handlers is what fsm already is. What a project cannot
  *            write for itself in an afternoon is the part below that — where a
  *            frame begins and ends, whether it is addressed here, and whether
  *            it survived the line — and that is what this is.
  *
  * @note      RTU has no start byte and no end byte, which is the whole
  *            difference from comstxetx. A frame is delimited by **silence**:
  *            three and a half character times of an idle line end it. So this
  *            module counts ticks the way softtimer does, the caller ticks it
  *            from a fixed rate interrupt, and the silence is expressed in
  *            those ticks. It does not know the baud rate and is not told it:
  *            converting 3.5 character times at a given baud into a number of
  *            ticks is one expression, it belongs where the baud rate is
  *            configured, and passing the baud rate in would make this module
  *            own a second thing it cannot check.
  *
  * @note      The intra-character gap, t1.5, is deliberately not implemented,
  *            and the reason belongs here rather than in a list of
  *            shortcomings. It would need a second threshold and a second
  *            piece of state to reject a frame that has a hole in the middle
  *            of it. A hole in the middle of a frame means the bytes on either
  *            side came from different frames, and a check over the result
  *            fails with overwhelming probability — so the second timer costs
  *            state and buys a fraction of a fraction of one percent of the
  *            frames the check would have caught anyway. A caller in an
  *            environment where that fraction matters has the tick and can
  *            keep the gap itself.
  *
  * @note      The check is installed at Init, which is comstxetx's arrangement
  *            and is what keeps this module independent of crc/. Modbus uses
  *            one specific CRC16 and crc16 in this tree already computes it, so
  *            crc16 goes in directly — but the indirection is what lets a part
  *            with a CRC unit use that instead.
  *
  * @note      The two check bytes travel **low byte first**, which is the one
  *            place Modbus disagrees with the big endian order the rest of the
  *            protocol uses for every value in a payload. Getting it the wrong
  *            way round produces a frame that looks right, is exactly as long
  *            as a good one, and is rejected by every peer on the bus.
  *
  * @note      A frame addressed to another device is **ignored, not rejected**,
  *            and the two are counted separately. Every frame on a multidrop
  *            bus is addressed to somebody, so most of what a device sees is
  *            not for it; counting that as a fault would make the reject count
  *            useless for the one thing it is for, which is telling somebody
  *            the line is bad. The broadcast address is accepted, and the
  *            caller sees it as the address in the first byte of the frame it
  *            is handed — a broadcast must never be answered, and only the
  *            caller can decline to answer.
  *
  * @note      Bytes that arrive while a completed frame is still waiting for
  *            commodbusEvaluate are discarded, and the frame they belonged to
  *            is counted once in the reject count when the waiting frame is
  *            evaluated. That count is how a main loop too slow for the bus
  *            becomes visible, which is encoderGetErrorCount's reason exactly.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "commodbus.h"

/* The check field, which sits at the end of every frame. */
#define COMMODBUS_CHECK_SIZE    2u

/**
 * @brief   Initializes a Modbus RTU driver.
 *
 * @param[out]  driver          the driver.
 * @param[in]   rxBuffer        the caller's receive buffer.
 * @param[in]   txBuffer        the caller's transmit buffer.
 * @param[in]   rxSize          how many bytes the receive buffer holds.
 * @param[in]   txSize          how many bytes the transmit buffer holds.
 * @param[in]   address         this device's address, 1 to 247.
 * @param[in]   silenceTicks    how many ticks of silence end a frame.
 * @param[in]   checksum        the check over a buffer. crc16 fits it.
 * @param[in]   packetProcess   called with a frame that is for this device and
 *                              passed its check. The buffer holds the address
 *                              and the protocol data unit, and the length is
 *                              theirs together without the check bytes.
 *
 * @return  TRUE when the driver was initialized, FALSE when an argument was
 *          refused and nothing was written.
 *
 * @note    The address is checked against the range the specification allows.
 *          Zero is the broadcast and belongs to no device, and everything above
 *          247 is reserved, so a driver given either would answer to something
 *          that cannot be addressed.
 *
 * @note    A silence of zero ticks is refused because it would end a frame
 *          between any two bytes of it.
 *
 * @note    Both buffers must hold the shortest frame there is. A compliant
 *          device gives them 256 bytes, which is what the specification allows
 *          a frame to reach.
 */
uint8_t commodbusInit ( commodbus_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                        uint32_t rxSize, uint32_t txSize,
                        uint8_t address, uint32_t silenceTicks,
                        uint16_t ( *checksum ) ( const uint8_t* const buffer, uint32_t length ),
                        void ( *packetProcess ) ( uint8_t* buffer, uint32_t length ) )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( rxBuffer != NULL ) && ( txBuffer != NULL ) &&
         ( checksum != NULL ) && ( packetProcess != NULL ) &&
         ( rxSize >= COMMODBUS_MIN_FRAME ) && ( txSize >= COMMODBUS_MIN_FRAME ) &&
         ( ( uint32_t ) address >= COMMODBUS_ADDRESS_MIN ) &&
         ( ( uint32_t ) address <= COMMODBUS_ADDRESS_MAX ) &&
         ( silenceTicks > 0u ) )
    {
        driver->address = address;

        driver->silenceCounter = 0u;
        driver->silenceTicks = silenceTicks;

        driver->rxIndex = 0u;
        driver->rxSize = rxSize;
        driver->txSize = txSize;

        driver->rxReadyToEvaluate = FALSE;
        driver->rxOverrun = FALSE;
        driver->rxDropped = FALSE;

        driver->rxRejectCount = 0u;
        driver->rxIgnoredCount = 0u;

        driver->rxBuffer = rxBuffer;
        driver->txBuffer = txBuffer;

        driver->checksum = checksum;
        driver->packetProcess = packetProcess;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Takes one received byte. Call it from the receive interrupt.
 *
 * @param[in,out]   driver  the driver.
 * @param[in]       data    the byte.
 *
 * @note    Every byte restarts the silence, which is what makes the gap
 *          between two frames the only thing that separates them.
 *
 * @note    A frame longer than the buffer is marked rather than truncated. The
 *          bytes past the end are dropped, and the frame is refused when the
 *          silence completes it: a truncated frame handed to a parser is worse
 *          than no frame, which is comgenbuf's rule in a second place.
 */
void commodbusReceive ( commodbus_t* driver, uint8_t data )
{
    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxIndex < driver->rxSize )
        {
            driver->rxBuffer[ driver->rxIndex ] = data;
            ++driver->rxIndex;
        }
        else
        {
            driver->rxOverrun = TRUE;
        }

        driver->silenceCounter = 0u;
    }
    else
    {
        driver->rxDropped = TRUE;
    }
}

/**
 * @brief   Counts one tick of the line. Call it from a fixed rate interrupt.
 *
 * @param[in,out]   driver  the driver.
 *
 * @note    The silence only runs while there is a frame to end. An idle line
 *          with nothing received is not the end of anything, and counting
 *          there would complete an empty frame every silenceTicks.
 */
void commodbusTimeoutCounter ( commodbus_t* driver )
{
    if ( ( driver->rxReadyToEvaluate == FALSE ) && ( driver->rxIndex > 0u ) )
    {
        if ( driver->silenceCounter >= driver->silenceTicks )
        {
            driver->rxReadyToEvaluate = TRUE;
        }
        else
        {
            ++driver->silenceCounter;
        }
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Delivers a completed frame. Call it from the main loop.
 *
 * @param[in,out]   driver  the driver.
 *
 * @note    Three things can refuse a frame and each is counted: it overran the
 *          buffer, it is shorter than a frame can be, or it failed its check.
 *          A fourth outcome is not a refusal — a frame for another device is
 *          counted separately, because it is ordinary traffic rather than a
 *          fault.
 */
void commodbusEvaluate ( commodbus_t* driver )
{
    uint16_t received = 0u;
    uint16_t computed = 0u;
    uint32_t payloadLength = 0u;
    uint8_t frameAddress = 0u;

    if ( driver->rxReadyToEvaluate == TRUE )
    {
        if ( driver->rxOverrun == TRUE )
        {
            ++driver->rxRejectCount;
        }
        else if ( driver->rxIndex < COMMODBUS_MIN_FRAME )
        {
            ++driver->rxRejectCount;
        }
        else
        {
            payloadLength = driver->rxIndex - COMMODBUS_CHECK_SIZE;

            /* Low byte first, which is this protocol's one little endian
               field. */
            received = ( uint16_t ) driver->rxBuffer[ payloadLength ];
            received = ( uint16_t ) ( received |
                       ( uint16_t ) ( ( uint16_t ) driver->rxBuffer[ payloadLength + 1u ] << 8 ) );

            computed = driver->checksum ( driver->rxBuffer, payloadLength );

            frameAddress = driver->rxBuffer[ 0 ];

            if ( received != computed )
            {
                ++driver->rxRejectCount;
            }
            else if ( ( frameAddress != driver->address ) &&
                      ( ( uint32_t ) frameAddress != COMMODBUS_BROADCAST ) )
            {
                ++driver->rxIgnoredCount;
            }
            else
            {
                driver->packetProcess ( driver->rxBuffer, payloadLength );
            }
        }

        if ( driver->rxDropped == TRUE )
        {
            ++driver->rxRejectCount;
            driver->rxDropped = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }

        driver->rxIndex = 0u;
        driver->rxOverrun = FALSE;
        driver->rxReadyToEvaluate = FALSE;
        driver->silenceCounter = 0u;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Builds a frame into the transmit buffer.
 *
 * @param[in,out]   driver      the driver.
 * @param[in]       address     the address to send to. The caller's own on a
 *                              reply, the peer's on a request, and zero to
 *                              broadcast.
 * @param[in]       pdu         the protocol data unit: a function code and its
 *                              data.
 * @param[in]       length      how many bytes that is.
 * @param[out]      frameLength how many bytes the frame came to.
 *
 * @return  TRUE when the frame was built, FALSE when it would not fit or the
 *          unit was empty.
 *
 * @note    The address is taken as an argument rather than from the driver,
 *          because the same module frames both halves of the conversation: a
 *          device answering uses its own address and one asking uses somebody
 *          else's.
 */
uint8_t commodbusBuildFrame ( commodbus_t* driver, uint8_t address,
                              const uint8_t* const pdu, uint32_t length,
                              uint32_t* frameLength )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0u;
    uint16_t check = 0u;

    if ( ( length > 0u ) &&
         ( ( length + 1u + COMMODBUS_CHECK_SIZE ) <= driver->txSize ) )
    {
        driver->txBuffer[ 0 ] = address;

        for ( i = 0u; i < length; ++i )
        {
            driver->txBuffer[ i + 1u ] = pdu[ i ];
        }

        check = driver->checksum ( driver->txBuffer, length + 1u );

        driver->txBuffer[ length + 1u ] = ( uint8_t ) ( check & 0x00FFu );
        driver->txBuffer[ length + 2u ] =
            ( uint8_t ) ( ( check >> 8 ) & 0x00FFu );

        *frameLength = length + 1u + COMMODBUS_CHECK_SIZE;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Reports how many frames were refused.
 *
 * @param[in]   driver  the driver.
 *
 * @return  The count: frames that failed their check, frames too short to be
 *          frames, frames that overran the buffer, and frames lost because a
 *          completed one was still waiting to be evaluated.
 *
 * @note    A rising count on a line that used to be quiet says the wiring, the
 *          termination or the baud rate is wrong. Frames for other devices are
 *          not in it.
 */
uint32_t commodbusGetRejectCount ( const commodbus_t* const driver )
{
    return ( driver->rxRejectCount );
}

/**
 * @brief   Reports how many good frames were for somebody else.
 *
 * @param[in]   driver  the driver.
 *
 * @return  The count.
 *
 * @note    This is the bus working. On a multidrop line with ten devices, nine
 *          tenths of what each one hears is in this number, and a count that
 *          stops rising says the master has stopped talking to the others.
 */
uint32_t commodbusGetIgnoredCount ( const commodbus_t* const driver )
{
    return ( driver->rxIgnoredCount );
}
