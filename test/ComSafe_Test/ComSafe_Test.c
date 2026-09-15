/*
 * Covers comsafe.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Most checks here are built by making a good frame and then breaking exactly
 * one thing about it, because that is the only way to prove which check caught
 * it. A frame with two faults would pass the test whichever one fired.
 *
 * The integrity function is a plain sum, so that every expected frame in this
 * file can be worked out by hand. One case installs crc16 instead, to prove
 * the hook takes a real one.
 */

#include <stddef.h>
#include <stdio.h>

#include "comsafe.h"
#include "crc16.h"

static uint32_t failures = 0;

static void check ( const char* what, uint8_t condition )
{
    if ( condition == TRUE )
    {
        printf ( "  PASS  %s\n", what );
    }
    else
    {
        printf ( "  FAIL  %s\n", what );
        ++failures;
    }
}

/* A sum, so the frames below can be checked with a pencil. */
static uint16_t sumCheck ( const uint8_t* const buffer, uint32_t length )
{
    uint16_t sum = 0;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        sum = ( uint16_t ) ( sum + buffer[ i ] );
    }

    return ( sum );
}

#define TEST_ID     0xABCDu

static const uint8_t payload[ 3 ] = { 0x11u, 0x22u, 0x33u };

/* Builds a frame into the driver's transmit buffer and copies it out. */
static uint32_t buildInto ( comsafe_t* driver, const uint8_t* const tx, uint8_t* out )
{
    uint32_t length = 0;
    uint32_t i = 0;

    if ( comsafeBuildFrame ( driver, payload, 3u, &length ) == TRUE )
    {
        for ( i = 0; i < length; ++i )
        {
            out[ i ] = tx[ i ];
        }
    }
    else
    {
        length = 0;
    }

    return ( length );
}

/* ---------------------------------------------------------------- init */

static void initCase ( void )
{
    comsafe_t driver;
    uint8_t rx[ 16 ];
    uint8_t tx[ 16 ];

    printf ( "comsafeInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comsafeInit ( NULL, rx, tx, 16u, 16u, TEST_ID, 10u, sumCheck ) == FALSE ) );
    check ( "a NULL receive buffer is rejected",
            ( uint8_t ) ( comsafeInit ( &driver, NULL, tx, 16u, 16u, TEST_ID, 10u, sumCheck ) == FALSE ) );
    check ( "a NULL transmit buffer is rejected",
            ( uint8_t ) ( comsafeInit ( &driver, rx, NULL, 16u, 16u, TEST_ID, 10u, sumCheck ) == FALSE ) );
    check ( "a zero receive size is rejected",
            ( uint8_t ) ( comsafeInit ( &driver, rx, tx, 0u, 16u, TEST_ID, 10u, sumCheck ) == FALSE ) );

    /*
     * A transmit buffer that cannot hold the overhead plus one payload byte
     * could never build a frame, so it is refused rather than accepted as a
     * driver that fails on every send.
     */
    check ( "a transmit buffer of exactly the overhead is rejected",
            ( uint8_t ) ( comsafeInit ( &driver, rx, tx, 16u, COMSAFE_OVERHEAD,
                                        TEST_ID, 10u, sumCheck ) == FALSE ) );
    check ( "one byte more is accepted",
            comsafeInit ( &driver, rx, tx, 16u, COMSAFE_OVERHEAD + 1u,
                          TEST_ID, 10u, sumCheck ) );

    /*
     * A watchdog that never fires is not a watchdog, and the timeout is the
     * only check in this module that can fire when nothing arrives at all.
     */
    check ( "a zero timeout is rejected",
            ( uint8_t ) ( comsafeInit ( &driver, rx, tx, 16u, 16u, TEST_ID, 0u, sumCheck ) == FALSE ) );

    /* A safety layer with no integrity check of its own is a sequence counter. */
    check ( "a NULL checksum is rejected",
            ( uint8_t ) ( comsafeInit ( &driver, rx, tx, 16u, 16u, TEST_ID, 10u, NULL ) == FALSE ) );

    check ( "a full init succeeds",
            comsafeInit ( &driver, rx, tx, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    /*
     * CS_INIT rather than CS_OK. Nothing has been heard from the peer, and a
     * channel reporting itself healthy before its first frame would let a
     * caller act on a link that was never up.
     */
    check ( "and starts in CS_INIT, not CS_OK",
            ( uint8_t ) ( comsafeGetState ( &driver ) == ( uint8_t ) CS_INIT ) );
    check ( "with no error recorded",
            ( uint8_t ) ( comsafeGetLastError ( &driver ) == ( uint8_t ) CS_NONE ) );
    check ( "and none counted",
            ( uint8_t ) ( comsafeGetErrorCount ( &driver ) == 0u ) );
    check ( "and no payload",
            ( uint8_t ) ( comsafeGetPayloadLength ( &driver ) == 0u ) );
}

/* --------------------------------------------------------- frame layout */

static void buildCase ( void )
{
    comsafe_t driver;
    uint8_t rx[ 16 ];
    uint8_t tx[ 16 ];
    uint32_t length = 0;

    printf ( "comsafeBuildFrame\n" );

    check ( "Init", comsafeInit ( &driver, rx, tx, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    check ( "a NULL payload is rejected",
            ( uint8_t ) ( comsafeBuildFrame ( &driver, NULL, 3u, &length ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( comsafeBuildFrame ( &driver, payload, 0u, &length ) == FALSE ) );
    check ( "a payload that does not fit is rejected",
            ( uint8_t ) ( comsafeBuildFrame ( &driver, payload,
                            16u - COMSAFE_OVERHEAD + 1u, &length ) == FALSE ) );

    check ( "building", comsafeBuildFrame ( &driver, payload, 3u, &length ) );
    check ( "the frame is the payload plus the overhead",
            ( uint8_t ) ( length == ( 3u + COMSAFE_OVERHEAD ) ) );

    /*
     * The layout, worked out by hand: connection id high and low, sequence,
     * payload, then the sum over all of that. 0xAB + 0xCD + 0x00 + 0x11 +
     * 0x22 + 0x33 is 478, which is 0x01DE.
     */
    check ( "the connection id leads, high byte first",
            ( uint8_t ) ( ( tx[ 0 ] == 0xABu ) && ( tx[ 1 ] == 0xCDu ) ) );
    check ( "the first frame carries sequence zero",
            ( uint8_t ) ( tx[ 2 ] == 0x00u ) );
    check ( "the payload follows",
            ( uint8_t ) ( ( tx[ 3 ] == 0x11u ) && ( tx[ 4 ] == 0x22u ) &&
                          ( tx[ 5 ] == 0x33u ) ) );
    check ( "and the check covers the id and the sequence as well as the payload",
            ( uint8_t ) ( ( tx[ 6 ] == 0x01u ) && ( tx[ 7 ] == 0xDEu ) ) );

    /*
     * The sequence advances on every frame built, including a resend. A frame
     * repeated under its old sequence is indistinguishable at the far end from
     * the transport duplicating one, which is what the sequence is for.
     */
    check ( "building again", comsafeBuildFrame ( &driver, payload, 3u, &length ) );
    check ( "advances the sequence", ( uint8_t ) ( tx[ 2 ] == 0x01u ) );
    check ( "and the check moves with it",
            ( uint8_t ) ( ( tx[ 6 ] == 0x01u ) && ( tx[ 7 ] == 0xDFu ) ) );

    check ( "a NULL frameLength is allowed",
            comsafeBuildFrame ( &driver, payload, 3u, NULL ) );
    check ( "and still advances the sequence", ( uint8_t ) ( tx[ 2 ] == 0x02u ) );
}

/* ---------------------------------------------------------- round trip */

static void roundTripCase ( void )
{
    comsafe_t sender;
    comsafe_t receiver;
    uint8_t txS[ 16 ];
    uint8_t rxS[ 16 ];
    uint8_t txR[ 16 ];
    uint8_t rxR[ 16 ];
    uint8_t frame[ 16 ];
    uint32_t length = 0;

    printf ( "a frame across the channel\n" );

    check ( "init the sender",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 10u, sumCheck ) );
    check ( "init the receiver",
            comsafeInit ( &receiver, rxR, txR, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame was built", ( uint8_t ) ( length == 8u ) );

    check ( "and it passes every check",
            comsafeCheckFrame ( &receiver, frame, length ) );
    check ( "the channel is in service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );
    check ( "the payload length is right",
            ( uint8_t ) ( comsafeGetPayloadLength ( &receiver ) == 3u ) );
    check ( "and the payload came through unchanged",
            ( uint8_t ) ( ( comsafeGetPayload ( &receiver )[ 0 ] == 0x11u ) &&
                          ( comsafeGetPayload ( &receiver )[ 1 ] == 0x22u ) &&
                          ( comsafeGetPayload ( &receiver )[ 2 ] == 0x33u ) ) );
    check ( "with nothing counted against it",
            ( uint8_t ) ( comsafeGetErrorCount ( &receiver ) == 0u ) );

    /* The next frame in sequence is accepted too. */
    length = buildInto ( &sender, txS, frame );
    check ( "the next frame in sequence passes",
            comsafeCheckFrame ( &receiver, frame, length ) );
    check ( "and the channel is still in service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );

    /*
     * The hook takes a real CRC as readily as the sum above, which is the same
     * claim comstxetx makes about its own. crc16's signature is the callback's.
     */
    check ( "init a sender with crc16",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 10u, crc16 ) );
    check ( "and a receiver with crc16",
            comsafeInit ( &receiver, rxR, txR, 16u, 16u, TEST_ID, 10u, crc16 ) );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame checked with crc16 round trips",
            comsafeCheckFrame ( &receiver, frame, length ) );
    check ( "and its payload is right",
            ( uint8_t ) ( comsafeGetPayload ( &receiver )[ 2 ] == 0x33u ) );
}

/* ------------------------------------------------------------ failures */

static void failureCase ( void )
{
    comsafe_t sender;
    comsafe_t other;
    comsafe_t receiver;
    uint8_t txS[ 16 ];
    uint8_t rxS[ 16 ];
    uint8_t txO[ 16 ];
    uint8_t rxO[ 16 ];
    uint8_t txR[ 16 ];
    uint8_t rxR[ 16 ];
    uint8_t frame[ 16 ];
    uint32_t length = 0;

    printf ( "the four things a transport check cannot catch\n" );

    check ( "init the sender",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 10u, sumCheck ) );
    check ( "init the receiver",
            comsafeInit ( &receiver, rxR, txR, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    /* A corrupted payload byte. */
    length = buildInto ( &sender, txS, frame );
    frame[ 4 ] = ( uint8_t ) ( frame[ 4 ] ^ 0x01u );

    check ( "a flipped bit is refused",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as an integrity failure",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_CHECK ) );
    check ( "and the channel drops out of service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_FAILED ) );
    check ( "and it is counted",
            ( uint8_t ) ( comsafeGetErrorCount ( &receiver ) == 1u ) );

    /*
     * A frame from the wrong sender. Built by a second driver with a different
     * connection id, so its check is correct for what it carries — otherwise
     * the integrity check would fire first and this would prove nothing.
     */
    comsafeReset ( &receiver );

    check ( "init a sender on another connection",
            comsafeInit ( &other, rxO, txO, 16u, 16u, 0x1234u, 10u, sumCheck ) );

    length = buildInto ( &other, txO, frame );

    check ( "a well formed frame from the wrong peer is refused",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as a connection id failure, not an integrity one",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_ID ) );

    /* A repeated frame. */
    comsafeReset ( &receiver );

    check ( "re-init the sender",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    length = buildInto ( &sender, txS, frame );
    check ( "the first frame passes",
            comsafeCheckFrame ( &receiver, frame, length ) );
    check ( "the same frame again is refused",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as a sequence failure",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_SEQUENCE ) );

    /* A lost frame, which shows up as a gap in the sequence. */
    comsafeReset ( &receiver );

    check ( "re-init the sender again",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    length = buildInto ( &sender, txS, frame );
    check ( "frame zero passes", comsafeCheckFrame ( &receiver, frame, length ) );

    ( void ) buildInto ( &sender, txS, frame );        /* frame one, thrown away */
    length = buildInto ( &sender, txS, frame );        /* frame two */

    check ( "a frame with one missing before it is refused",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as a sequence failure too",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_SEQUENCE ) );

    /* Lengths that cannot describe a frame. */
    comsafeReset ( &receiver );

    check ( "a frame of exactly the overhead has no payload and is refused",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame,
                            COMSAFE_OVERHEAD ) == FALSE ) );
    check ( "as a length failure",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_LENGTH ) );

    comsafeReset ( &receiver );

    check ( "a payload longer than the receive buffer is refused",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame,
                            16u + COMSAFE_OVERHEAD + 1u ) == FALSE ) );
    check ( "as a length failure as well",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_LENGTH ) );
}

/* ------------------------------------------------------------- watchdog */

static void timeoutCase ( void )
{
    comsafe_t sender;
    comsafe_t receiver;
    uint8_t txS[ 16 ];
    uint8_t rxS[ 16 ];
    uint8_t txR[ 16 ];
    uint8_t rxR[ 16 ];
    uint8_t frame[ 16 ];
    uint32_t length = 0;
    uint32_t i = 0;

    printf ( "the watchdog\n" );

    check ( "init the sender",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 5u, sumCheck ) );
    check ( "init the receiver",
            comsafeInit ( &receiver, rxR, txR, 16u, 16u, TEST_ID, 5u, sumCheck ) );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame passes", comsafeCheckFrame ( &receiver, frame, length ) );
    check ( "and the channel is in service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );

    for ( i = 0; i < 4u; ++i )
    {
        comsafeTimeoutCounter ( &receiver );
    }

    check ( "four ticks of a timeout of five leave it in service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );

    comsafeTimeoutCounter ( &receiver );

    /*
     * This is the check that can fire when nothing arrives, which is the one
     * failure no frame can report because there is no frame.
     */
    check ( "the fifth drops the channel",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_FAILED ) );
    check ( "as a timeout",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_TIMEOUT ) );
    check ( "counted once",
            ( uint8_t ) ( comsafeGetErrorCount ( &receiver ) == 1u ) );

    /*
     * The counter stops at the timeout rather than running on. A watchdog that
     * kept climbing would wrap eventually and rearm the channel by itself.
     */
    for ( i = 0; i < 1000u; ++i )
    {
        comsafeTimeoutCounter ( &receiver );
    }

    check ( "and a thousand more ticks count it only that once",
            ( uint8_t ) ( comsafeGetErrorCount ( &receiver ) == 1u ) );
    check ( "leaving the channel out of service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_FAILED ) );

    /* A good frame resets the counter, so the next timeout is a full one. */
    comsafeReset ( &receiver );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame after the reset passes",
            comsafeCheckFrame ( &receiver, frame, length ) );

    for ( i = 0; i < 4u; ++i )
    {
        comsafeTimeoutCounter ( &receiver );
    }

    length = buildInto ( &sender, txS, frame );
    check ( "another frame arrives late in the budget",
            comsafeCheckFrame ( &receiver, frame, length ) );

    for ( i = 0; i < 4u; ++i )
    {
        comsafeTimeoutCounter ( &receiver );
    }

    check ( "and the next frame gets a full budget, not the remains of the last",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );
}

/* ---------------------------------------------------------------- reset */

static void resetCase ( void )
{
    comsafe_t sender;
    comsafe_t receiver;
    uint8_t txS[ 16 ];
    uint8_t rxS[ 16 ];
    uint8_t txR[ 16 ];
    uint8_t rxR[ 16 ];
    uint8_t frame[ 16 ];
    uint32_t length = 0;

    printf ( "comsafeReset\n" );

    check ( "init the sender",
            comsafeInit ( &sender, rxS, txS, 16u, 16u, TEST_ID, 10u, sumCheck ) );
    check ( "init the receiver",
            comsafeInit ( &receiver, rxR, txR, 16u, 16u, TEST_ID, 10u, sumCheck ) );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame passes", comsafeCheckFrame ( &receiver, frame, length ) );

    frame[ 4 ] = ( uint8_t ) ( frame[ 4 ] ^ 0x80u );
    check ( "a corrupted one does not",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "and the channel is out of service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_FAILED ) );

    /*
     * A good frame does not put it back. Recovery is explicit, because only
     * the caller knows whether the process it controls is in a state where
     * resuming is allowed.
     */
    length = buildInto ( &sender, txS, frame );
    ( void ) comsafeCheckFrame ( &receiver, frame, length );
    check ( "a later good frame does not rearm the channel on its own",
            ( uint8_t ) ( comsafeGetState ( &receiver ) != ( uint8_t ) CS_OK ) );

    comsafeReset ( &receiver );

    check ( "reset puts it back to CS_INIT",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_INIT ) );
    check ( "and clears the last error",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_NONE ) );

    /*
     * The count survives. A channel that has been reset twenty times is
     * telling the caller something a cleared counter would hide.
     */
    check ( "but not the count",
            ( uint8_t ) ( comsafeGetErrorCount ( &receiver ) > 0u ) );

    /*
     * The sequence expectation is dropped too, so the peer may have kept
     * running or restarted and either is accepted. The sender here is well
     * past the sequence the receiver last saw.
     */
    length = buildInto ( &sender, txS, frame );
    length = buildInto ( &sender, txS, frame );
    length = buildInto ( &sender, txS, frame );

    check ( "and the first frame after a reset is taken at whatever sequence it carries",
            comsafeCheckFrame ( &receiver, frame, length ) );
    check ( "putting the channel back in service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );

    /* And the sequence is tracked from there. */
    length = buildInto ( &sender, txS, frame );
    check ( "the frame after it follows in sequence",
            comsafeCheckFrame ( &receiver, frame, length ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    buildCase ( );
    printf ( "\n" );
    roundTripCase ( );
    printf ( "\n" );
    failureCase ( );
    printf ( "\n" );
    timeoutCase ( );
    printf ( "\n" );
    resetCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%lu check(s) failed\n", ( unsigned long ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
