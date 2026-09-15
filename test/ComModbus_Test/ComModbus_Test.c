/*
 * Covers commodbus.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The check function is crc16 from this tree, which is the Modbus polynomial
 * already — so this test is also the wiring a caller would write. The expected
 * frames were not taken from the module: the check bytes are computed here
 * with the same function and placed by hand, low byte first, which is what
 * makes the byte order an assertion rather than a coincidence.
 */

#include <stdio.h>
#include <string.h>

#include "commodbus.h"
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

/* What the driver hands back, kept where the checks can look at it. */
static uint8_t deliveredBuffer [ 64 ];
static uint32_t deliveredLength = 0u;
static uint32_t deliveredCount = 0u;

static void deliver ( uint8_t* buffer, uint32_t length )
{
    uint32_t i = 0u;

    deliveredLength = length;
    ++deliveredCount;

    for ( i = 0u; ( i < length ) && ( i < 64u ); ++i )
    {
        deliveredBuffer[ i ] = buffer[ i ];
    }
}

static void deliveredReset ( void )
{
    deliveredLength = 0u;
    deliveredCount = 0u;
    memset ( deliveredBuffer, 0, sizeof ( deliveredBuffer ) );
}

/* The driver and the two buffers it is given, in one place. */
static commodbus_t driver;
static uint8_t rxBuffer [ 64 ];
static uint8_t txBuffer [ 64 ];

#define SILENCE     4u

static uint8_t startDriver ( uint8_t address )
{
    deliveredReset ( );

    return ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u, address,
                             SILENCE, crc16, deliver ) );
}

/* Feeds a whole frame in, one byte at a time, and lets the line go quiet. */
static void feed ( const uint8_t* const frame, uint32_t length )
{
    uint32_t i = 0u;

    for ( i = 0u; i < length; ++i )
    {
        commodbusReceive ( &driver, frame[ i ] );
    }

    for ( i = 0u; i <= SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusEvaluate ( &driver );
}

/*
 * Puts the address, the unit and the check into a caller's array. This is the
 * encoder written a second time, by hand, on purpose: the test must not build
 * its fixtures with the function it is checking, or a wrong byte order would
 * agree with itself.
 */
static uint32_t frame ( uint8_t* out, uint8_t address, const uint8_t* const pdu,
                        uint32_t length )
{
    uint32_t i = 0u;
    uint16_t computed = 0u;

    out[ 0 ] = address;

    for ( i = 0u; i < length; ++i )
    {
        out[ i + 1u ] = pdu[ i ];
    }

    computed = crc16 ( out, length + 1u );

    out[ length + 1u ] = ( uint8_t ) ( computed & 0x00FFu );
    out[ length + 2u ] = ( uint8_t ) ( ( computed >> 8 ) & 0x00FFu );

    return ( length + 3u );
}

/* ----------------------------------------------------------------- init */

static void initCase ( void )
{
    uint8_t ok = FALSE;

    printf ( "commodbusInit\n" );

    ok = commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u, 7u, SILENCE,
                         crc16, deliver );
    check ( "a sound set of arguments", ok );

    check ( "a NULL driver is refused",
            ( uint8_t ) ( commodbusInit ( NULL, rxBuffer, txBuffer, 64u, 64u,
                                          7u, SILENCE, crc16, deliver )
                          == FALSE ) );
    check ( "a NULL receive buffer is refused",
            ( uint8_t ) ( commodbusInit ( &driver, NULL, txBuffer, 64u, 64u,
                                          7u, SILENCE, crc16, deliver )
                          == FALSE ) );
    check ( "a NULL transmit buffer is refused",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, NULL, 64u, 64u,
                                          7u, SILENCE, crc16, deliver )
                          == FALSE ) );
    check ( "a NULL check function is refused",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                          7u, SILENCE, NULL, deliver )
                          == FALSE ) );
    check ( "a NULL delivery is refused",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                          7u, SILENCE, crc16, NULL )
                          == FALSE ) );

    /* A buffer too small to hold the shortest frame there is. */
    check ( "a receive buffer under four bytes is refused",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 3u, 64u,
                                          7u, SILENCE, crc16, deliver )
                          == FALSE ) );
    check ( "and a transmit buffer under four bytes",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 3u,
                                          7u, SILENCE, crc16, deliver )
                          == FALSE ) );

    /*
     * The address range. Zero is the broadcast and belongs to no device, and
     * everything past 247 is reserved — a driver that answered to either would
     * answer to something that cannot be addressed.
     */
    check ( "the broadcast address is refused as an own address",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                          0u, SILENCE, crc16, deliver )
                          == FALSE ) );
    check ( "and a reserved one",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                          248u, SILENCE, crc16, deliver )
                          == FALSE ) );
    check ( "the last usable address is allowed",
            commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u, 247u,
                            SILENCE, crc16, deliver ) );

    /* A silence of nothing would end a frame between any two of its bytes. */
    check ( "a silence of zero ticks is refused",
            ( uint8_t ) ( commodbusInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                          7u, 0u, crc16, deliver )
                          == FALSE ) );
}

/* -------------------------------------------------------------- receive */

static void receiveCase ( void )
{
    uint8_t pdu [ 5 ] = { 0x03u, 0x00u, 0x6Bu, 0x00u, 0x03u };
    uint8_t wire [ 16 ];
    uint32_t length = 0u;

    printf ( "a frame in\n" );

    check ( "init", startDriver ( 17u ) );

    /* The example from the specification: read three registers from 0x006B. */
    length = frame ( wire, 17u, pdu, 5u );
    feed ( wire, length );

    check ( "the frame is delivered once",
            ( uint8_t ) ( deliveredCount == 1u ) );
    check ( "without its check bytes",
            ( uint8_t ) ( deliveredLength == 6u ) );
    check ( "the address comes with it",
            ( uint8_t ) ( deliveredBuffer[ 0 ] == 17u ) );
    check ( "and the unit is intact",
            ( uint8_t ) ( ( deliveredBuffer[ 1 ] == 0x03u ) &&
                          ( deliveredBuffer[ 2 ] == 0x00u ) &&
                          ( deliveredBuffer[ 3 ] == 0x6Bu ) &&
                          ( deliveredBuffer[ 4 ] == 0x00u ) &&
                          ( deliveredBuffer[ 5 ] == 0x03u ) ) );
    check ( "nothing was refused",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 0u ) );

    /* A second frame on the same driver, to show the state was cleared. */
    deliveredReset ( );
    feed ( wire, length );
    check ( "and a second frame arrives the same way",
            ( uint8_t ) ( ( deliveredCount == 1u ) &&
                          ( deliveredLength == 6u ) ) );
}

/* ------------------------------------------------------------- the check */

static void checkCase ( void )
{
    uint8_t pdu [ 3 ] = { 0x06u, 0x00u, 0x01u };
    uint8_t wire [ 16 ];
    uint32_t length = 0u;
    uint8_t swap = 0u;

    printf ( "the check\n" );

    check ( "init", startDriver ( 17u ) );

    length = frame ( wire, 17u, pdu, 3u );

    /* One bit of the payload, which is what a bad line does. */
    wire[ 2 ] = ( uint8_t ) ( wire[ 2 ] ^ 0x01u );
    feed ( wire, length );

    check ( "a corrupted frame is not delivered",
            ( uint8_t ) ( deliveredCount == 0u ) );
    check ( "and is counted",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 1u ) );

    /*
     * The check bytes the other way round. This is the mistake worth pinning:
     * Modbus sends every value in a payload big endian and its own check low
     * byte first, so a frame built the obvious way is exactly as long as a good
     * one, looks right, and is rejected by every peer on the bus.
     */
    check ( "init", startDriver ( 17u ) );
    length = frame ( wire, 17u, pdu, 3u );
    swap = wire[ length - 2u ];
    wire[ length - 2u ] = wire[ length - 1u ];
    wire[ length - 1u ] = swap;
    feed ( wire, length );

    check ( "the check bytes reversed is a bad frame",
            ( uint8_t ) ( deliveredCount == 0u ) );
    check ( "and counted",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 1u ) );

    /* Shorter than a frame can be: an address and one check byte. */
    check ( "init", startDriver ( 17u ) );
    wire[ 0 ] = 17u;
    wire[ 1 ] = 0x03u;
    wire[ 2 ] = 0x00u;
    feed ( wire, 3u );

    check ( "a frame too short to be one is refused",
            ( uint8_t ) ( ( deliveredCount == 0u ) &&
                          ( commodbusGetRejectCount ( &driver ) == 1u ) ) );

    /*
     * Two bytes, and they are the two that matter. A check over nothing is the
     * seed, 0xFFFF here, so a driver that took the length of a frame on trust
     * would compute the check over an empty payload, find it matches, and go
     * on to read an address out of the first check byte. The refusal has to
     * come from the length rather than from the check, and this is the fixture
     * that tells the two apart: without the length test this arrives as a
     * frame for device 255 and lands in the ignored count instead.
     */
    check ( "init", startDriver ( 17u ) );
    wire[ 0 ] = 0xFFu;
    wire[ 1 ] = 0xFFu;
    feed ( wire, 2u );

    check ( "two bytes whose check would pass are still not a frame",
            ( uint8_t ) ( ( deliveredCount == 0u ) &&
                          ( commodbusGetRejectCount ( &driver ) == 1u ) &&
                          ( commodbusGetIgnoredCount ( &driver ) == 0u ) ) );
}

/* ------------------------------------------------------------- addressing */

static void addressCase ( void )
{
    uint8_t pdu [ 3 ] = { 0x05u, 0x00u, 0x11u };
    uint8_t wire [ 16 ];
    uint32_t length = 0u;

    printf ( "addressing\n" );

    check ( "init", startDriver ( 17u ) );

    /* Somebody else's frame. Ordinary traffic, not a fault. */
    length = frame ( wire, 18u, pdu, 3u );
    feed ( wire, length );

    check ( "a frame for another device is not delivered",
            ( uint8_t ) ( deliveredCount == 0u ) );
    check ( "and is not a rejection",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 0u ) );
    check ( "it is counted as ignored",
            ( uint8_t ) ( commodbusGetIgnoredCount ( &driver ) == 1u ) );

    /* The broadcast, which every device takes and none answers. */
    length = frame ( wire, 0u, pdu, 3u );
    feed ( wire, length );

    check ( "a broadcast is delivered",
            ( uint8_t ) ( deliveredCount == 1u ) );
    check ( "with the broadcast address in it, which is how the caller knows "
            "not to answer",
            ( uint8_t ) ( deliveredBuffer[ 0 ] == COMMODBUS_BROADCAST ) );
    check ( "and the ignored count did not move",
            ( uint8_t ) ( commodbusGetIgnoredCount ( &driver ) == 1u ) );

    /* And this device's own frame, after all of that. */
    length = frame ( wire, 17u, pdu, 3u );
    feed ( wire, length );
    check ( "its own frame still arrives",
            ( uint8_t ) ( deliveredCount == 2u ) );
}

/* ---------------------------------------------------------------- silence */

static void silenceCase ( void )
{
    uint8_t pdu [ 3 ] = { 0x03u, 0x00u, 0x01u };
    uint8_t wire [ 16 ];
    uint32_t length = 0u;
    uint32_t i = 0u;

    printf ( "the silence that ends a frame\n" );

    check ( "init", startDriver ( 17u ) );

    length = frame ( wire, 17u, pdu, 3u );

    for ( i = 0u; i < length; ++i )
    {
        commodbusReceive ( &driver, wire[ i ] );
    }

    /*
     * One tick short of the silence. Nothing has ended yet, and this is the
     * whole of RTU framing: there is no end byte, so a frame that is delivered
     * early is a frame the next byte would have belonged to.
     */
    for ( i = 0u; i < SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusEvaluate ( &driver );
    check ( "a frame is not delivered before the silence is complete",
            ( uint8_t ) ( deliveredCount == 0u ) );

    commodbusTimeoutCounter ( &driver );
    commodbusEvaluate ( &driver );
    check ( "and is delivered on the tick that completes it",
            ( uint8_t ) ( deliveredCount == 1u ) );

    /* A byte in the middle of the gap puts the silence back to the start. */
    check ( "init", startDriver ( 17u ) );

    for ( i = 0u; i < ( length - 1u ); ++i )
    {
        commodbusReceive ( &driver, wire[ i ] );
    }

    for ( i = 0u; i < SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusReceive ( &driver, wire[ length - 1u ] );

    for ( i = 0u; i < SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusEvaluate ( &driver );
    check ( "a byte restarts the silence",
            ( uint8_t ) ( deliveredCount == 0u ) );

    commodbusTimeoutCounter ( &driver );
    commodbusEvaluate ( &driver );
    check ( "so the whole frame arrives together",
            ( uint8_t ) ( ( deliveredCount == 1u ) &&
                          ( deliveredLength == 4u ) ) );

    /*
     * An idle line with nothing on it. The silence must not run here, or every
     * gap between two frames would complete an empty one.
     */
    check ( "init", startDriver ( 17u ) );

    for ( i = 0u; i < ( SILENCE * 10u ); ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusEvaluate ( &driver );
    check ( "an idle line completes nothing",
            ( uint8_t ) ( ( deliveredCount == 0u ) &&
                          ( commodbusGetRejectCount ( &driver ) == 0u ) ) );
}

/* -------------------------------------------------------------- overruns */

static void overrunCase ( void )
{
    uint8_t small [ 8 ];
    uint8_t pdu [ 3 ] = { 0x03u, 0x00u, 0x01u };
    uint8_t long5 [ 5 ] = { 0x10u, 0x00u, 0x01u, 0x00u, 0x02u };
    uint8_t wire [ 16 ];
    uint32_t length = 0u;
    uint32_t i = 0u;

    printf ( "more than fits\n" );

    deliveredReset ( );
    check ( "init with a small buffer",
            commodbusInit ( &driver, small, txBuffer, 8u, 64u, 17u, SILENCE,
                            crc16, deliver ) );

    /* Twelve bytes into eight. */
    for ( i = 0u; i < 12u; ++i )
    {
        commodbusReceive ( &driver, ( uint8_t ) i );
    }

    for ( i = 0u; i <= SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusEvaluate ( &driver );

    check ( "a frame past the buffer is refused whole",
            ( uint8_t ) ( deliveredCount == 0u ) );
    check ( "and counted",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 1u ) );

    /*
     * The case that says refusing is not the same as truncating. These first
     * eight bytes are a whole valid frame, and four more arrive behind them
     * with no silence in between — one long message, or two frames run
     * together by a device that did not pause. A driver that dropped the
     * excess and kept what fit would hand the parser a frame that passes its
     * check and is only half of what was sent.
     */
    deliveredReset ( );
    check ( "init with a small buffer",
            commodbusInit ( &driver, small, txBuffer, 8u, 64u, 17u, SILENCE,
                            crc16, deliver ) );

    length = frame ( wire, 17u, long5, 5u );

    for ( i = 0u; i < length; ++i )
    {
        commodbusReceive ( &driver, wire[ i ] );
    }

    for ( i = 0u; i < 4u; ++i )
    {
        commodbusReceive ( &driver, 0xA5u );
    }

    for ( i = 0u; i <= SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    commodbusEvaluate ( &driver );

    check ( "a valid frame with more behind it is refused, not truncated",
            ( uint8_t ) ( deliveredCount == 0u ) );
    check ( "and that refusal is counted",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 1u ) );

    /* And the next frame is unaffected, which is what recovery means here. */
    deliveredReset ( );
    check ( "init", startDriver ( 17u ) );
    length = frame ( wire, 17u, pdu, 3u );
    feed ( wire, length );

    check ( "the frame after it arrives",
            ( uint8_t ) ( deliveredCount == 1u ) );

    /*
     * Bytes arriving while a completed frame is still waiting. They belong to
     * a frame that will now be lost, and losing it silently would hide a main
     * loop that cannot keep up with the bus.
     */
    check ( "init", startDriver ( 17u ) );
    length = frame ( wire, 17u, pdu, 3u );

    for ( i = 0u; i < length; ++i )
    {
        commodbusReceive ( &driver, wire[ i ] );
    }

    for ( i = 0u; i <= SILENCE; ++i )
    {
        commodbusTimeoutCounter ( &driver );
    }

    /* The main loop has not run yet, and the next frame starts arriving. */
    for ( i = 0u; i < length; ++i )
    {
        commodbusReceive ( &driver, wire[ i ] );
    }

    commodbusEvaluate ( &driver );

    check ( "the waiting frame is still delivered",
            ( uint8_t ) ( deliveredCount == 1u ) );
    check ( "and the one that arrived on top of it is counted as lost",
            ( uint8_t ) ( commodbusGetRejectCount ( &driver ) == 1u ) );
}

/* ----------------------------------------------------------------- build */

static void buildCase ( void )
{
    uint8_t pdu [ 3 ] = { 0x03u, 0x00u, 0x6Bu };
    uint32_t built = 0u;
    uint16_t expected = 0u;
    uint8_t wire [ 16 ];
    uint32_t i = 0u;

    printf ( "commodbusBuildFrame\n" );

    check ( "init", startDriver ( 17u ) );

    check ( "a frame is built", commodbusBuildFrame ( &driver, 17u, pdu, 3u,
                                                      &built ) );
    check ( "and is the unit plus an address and a check",
            ( uint8_t ) ( built == 6u ) );
    check ( "the address goes first",
            ( uint8_t ) ( txBuffer[ 0 ] == 17u ) );
    check ( "the unit follows it",
            ( uint8_t ) ( ( txBuffer[ 1 ] == 0x03u ) &&
                          ( txBuffer[ 2 ] == 0x00u ) &&
                          ( txBuffer[ 3 ] == 0x6Bu ) ) );

    /*
     * The check, computed here and compared byte by byte, low first. Asserting
     * the two bytes separately is the point: a frame with them the wrong way
     * round has the right length and the right contents everywhere else.
     */
    expected = crc16 ( txBuffer, 4u );
    check ( "the low check byte comes before the high one",
            ( uint8_t ) ( ( txBuffer[ 4 ] == ( uint8_t ) ( expected & 0x00FFu ) ) &&
                          ( txBuffer[ 5 ] ==
                            ( uint8_t ) ( ( expected >> 8 ) & 0x00FFu ) ) ) );

    /* A frame that does not fit is refused rather than truncated. */
    check ( "a unit longer than the buffer is refused",
            ( uint8_t ) ( commodbusBuildFrame ( &driver, 17u, pdu, 62u, &built )
                          == FALSE ) );
    check ( "an empty unit is refused",
            ( uint8_t ) ( commodbusBuildFrame ( &driver, 17u, pdu, 0u, &built )
                          == FALSE ) );

    /*
     * The round trip: what this module builds, it accepts. The two halves are
     * written separately and this is the only thing that says they agree — the
     * encoder could put the check bytes one way and the decoder read them the
     * other, and each half on its own would look correct.
     */
    check ( "init", startDriver ( 17u ) );
    check ( "build", commodbusBuildFrame ( &driver, 17u, pdu, 3u, &built ) );

    for ( i = 0u; i < built; ++i )
    {
        wire[ i ] = txBuffer[ i ];
    }

    feed ( wire, built );

    check ( "a frame this module built, this module accepts",
            ( uint8_t ) ( ( deliveredCount == 1u ) &&
                          ( deliveredLength == 4u ) ) );
    check ( "with the unit it was given",
            ( uint8_t ) ( ( deliveredBuffer[ 0 ] == 17u ) &&
                          ( deliveredBuffer[ 1 ] == 0x03u ) &&
                          ( deliveredBuffer[ 2 ] == 0x00u ) &&
                          ( deliveredBuffer[ 3 ] == 0x6Bu ) ) );

    /* A broadcast is built like anything else: the module does not send, so it
       is not the one that must decline to answer. */
    check ( "a broadcast frame builds",
            commodbusBuildFrame ( &driver, COMMODBUS_BROADCAST, pdu, 3u,
                                  &built ) );
    check ( "with the broadcast address in it",
            ( uint8_t ) ( txBuffer[ 0 ] == COMMODBUS_BROADCAST ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    receiveCase ( );
    printf ( "\n" );
    checkCase ( );
    printf ( "\n" );
    addressCase ( );
    printf ( "\n" );
    silenceCase ( );
    printf ( "\n" );
    overrunCase ( );
    printf ( "\n" );
    buildCase ( );

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
