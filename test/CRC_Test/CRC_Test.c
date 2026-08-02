/*
 * Covers crc16, crc16Alt and crc32.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The expected values were generated from the polynomials rather than copied
 * from the implementations, so this checks that the two functions compute the
 * algorithms they claim to and not merely that they agree with themselves. The
 * published check values pin them to the named standards: 0x4B37 for MODBUS
 * CRC16 over "123456789", and 0x0376E6E7 for CRC-32/MPEG-2 over the same.
 */

#include <stdio.h>

#include "crc16.h"
#include "crc32.h"

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

/* ------------------------------------------------------------- fixtures */

static const uint8_t checkVector[ 9 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
static const uint8_t modbusFrame[ 6 ] = { 0x01u, 0x03u, 0x00u, 0x00u, 0x00u, 0x0Au };
static const uint8_t oneZero[ 1 ] = { 0x00u };
static const uint8_t oneFF[ 1 ] = { 0xFFu };
static const uint8_t twoZeros[ 2 ] = { 0x00u, 0x00u };

/* ---------------------------------------------------------------- crc16 */

static void crc16Case ( void )
{
    printf ( "crc16\n" );

    /*
     * A zero length run must leave the seed untouched. This is also the only
     * case where the loop body never executes, so it separates the seed from
     * the first table lookup.
     */
    check ( "an empty array returns the 0xFFFF seed",
            ( uint8_t ) ( crc16 ( checkVector, 0u ) == 0xFFFFu ) );

    check ( "the published MODBUS check value over \"123456789\"",
            ( uint8_t ) ( crc16 ( checkVector, 9u ) == 0x4B37u ) );

    check ( "a MODBUS read holding registers frame",
            ( uint8_t ) ( crc16 ( modbusFrame, 6u ) == 0xCDC5u ) );

    check ( "a single zero byte", ( uint8_t ) ( crc16 ( oneZero, 1u ) == 0x40BFu ) );
    check ( "a single 0xFF byte", ( uint8_t ) ( crc16 ( oneFF, 1u ) == 0x00FFu ) );

    /*
     * Two zero bytes rather than one, because a CRC that ignored its input
     * length or dropped the high byte of the seed would still get the single
     * byte case right often enough to look healthy.
     */
    check ( "two zero bytes", ( uint8_t ) ( crc16 ( twoZeros, 2u ) == 0xB001u ) );
}

/* ------------------------------------------------------------- crc16Alt */

static void crc16AltCase ( void )
{
    printf ( "crc16Alt\n" );

    check ( "an empty array returns the 0xFFFF seed",
            ( uint8_t ) ( crc16Alt ( checkVector, 0u ) == 0xFFFFu ) );
    check ( "the published MODBUS check value",
            ( uint8_t ) ( crc16Alt ( checkVector, 9u ) == 0x4B37u ) );
    check ( "a MODBUS frame", ( uint8_t ) ( crc16Alt ( modbusFrame, 6u ) == 0xCDC5u ) );
    check ( "a single zero byte", ( uint8_t ) ( crc16Alt ( oneZero, 1u ) == 0x40BFu ) );
    check ( "a single 0xFF byte", ( uint8_t ) ( crc16Alt ( oneFF, 1u ) == 0x00FFu ) );
    check ( "two zero bytes", ( uint8_t ) ( crc16Alt ( twoZeros, 2u ) == 0xB001u ) );

    /*
     * The table and the bitwise form exist to be swapped for one another, so
     * the thing worth asserting is that they are interchangeable. Comparing
     * them against each other catches a corrupted table entry that a handful
     * of fixed vectors could walk straight past.
     */
    check ( "the table and bitwise forms agree on the check vector",
            ( uint8_t ) ( crc16 ( checkVector, 9u ) == crc16Alt ( checkVector, 9u ) ) );
    check ( "and on a MODBUS frame",
            ( uint8_t ) ( crc16 ( modbusFrame, 6u ) == crc16Alt ( modbusFrame, 6u ) ) );
    check ( "and on an empty array",
            ( uint8_t ) ( crc16 ( oneZero, 0u ) == crc16Alt ( oneZero, 0u ) ) );
}

/* --------------------------------------------- the two forms, exhaustive */

static void crc16EquivalenceCase ( void )
{
    uint8_t buffer[ 2 ] = { 0u, 0u };
    uint32_t a = 0;
    uint32_t b = 0;
    uint8_t agree = TRUE;

    printf ( "crc16 against crc16Alt over every single and double byte\n" );

    /*
     * Every one byte value exercises a different table entry, and every two
     * byte pair exercises a different pair of them. That is 65792 comparisons
     * for a few lines, and it is the only check here that would catch a single
     * wrong entry buried in the middle of the table.
     */
    for ( a = 0; a <= 0xFFu; ++a )
    {
        buffer[ 0 ] = ( uint8_t ) a;

        if ( crc16 ( buffer, 1u ) != crc16Alt ( buffer, 1u ) )
        {
            agree = FALSE;
            break;
        }
    }

    check ( "all 256 single byte values agree", agree );

    agree = TRUE;

    for ( a = 0; a <= 0xFFu; ++a )
    {
        buffer[ 0 ] = ( uint8_t ) a;

        for ( b = 0; b <= 0xFFu; ++b )
        {
            buffer[ 1 ] = ( uint8_t ) b;

            if ( crc16 ( buffer, 2u ) != crc16Alt ( buffer, 2u ) )
            {
                agree = FALSE;
            }
        }
    }

    check ( "all 65536 two byte pairs agree", agree );
}

/* ---------------------------------------------------------------- crc32 */

static void crc32Case ( void )
{
    printf ( "crc32\n" );

    check ( "an empty array returns the 0xFFFFFFFF seed",
            ( uint8_t ) ( crc32 ( checkVector, 0u ) == 0xFFFFFFFFu ) );

    /*
     * This is CRC-32/MPEG-2: no input or output reflection and no final XOR.
     * The far more common CRC-32/ISO-HDLC would give 0xCBF43926 here, so this
     * value is what keeps the two from being confused.
     */
    check ( "the published CRC-32/MPEG-2 check value over \"123456789\"",
            ( uint8_t ) ( crc32 ( checkVector, 9u ) == 0x0376E6E7u ) );

    check ( "it is not the reflected ISO-HDLC variant",
            ( uint8_t ) ( crc32 ( checkVector, 9u ) != 0xCBF43926u ) );

    check ( "a six byte frame", ( uint8_t ) ( crc32 ( modbusFrame, 6u ) == 0xD5CFCF4Bu ) );
    check ( "a single zero byte", ( uint8_t ) ( crc32 ( oneZero, 1u ) == 0x4E08BFB4u ) );
    check ( "a single 0xFF byte", ( uint8_t ) ( crc32 ( oneFF, 1u ) == 0xFFFFFF00u ) );
    check ( "two zero bytes", ( uint8_t ) ( crc32 ( twoZeros, 2u ) == 0x00B7647Du ) );
}

/* --------------------------------------------------------- sensitivity */

static void sensitivityCase ( void )
{
    uint8_t buffer[ 4 ] = { 0x12u, 0x34u, 0x56u, 0x78u };
    uint16_t base16 = 0;
    uint32_t base32 = 0;
    uint8_t distinct = TRUE;
    uint32_t bit = 0;

    printf ( "every single bit flip changes both CRCs\n" );

    base16 = crc16 ( buffer, 4u );
    base32 = crc32 ( buffer, 4u );

    /*
     * A CRC that dropped a byte, or masked the wrong half of its state, can
     * still produce plausible looking values. What it cannot do is keep
     * reacting to every bit of the message.
     */
    for ( bit = 0; bit < 32u; ++bit )
    {
        buffer[ bit / 8u ] ^= ( uint8_t ) ( 1u << ( bit % 8u ) );

        if ( ( crc16 ( buffer, 4u ) == base16 ) || ( crc32 ( buffer, 4u ) == base32 ) )
        {
            distinct = FALSE;
        }

        buffer[ bit / 8u ] ^= ( uint8_t ) ( 1u << ( bit % 8u ) );
    }

    check ( "all 32 bit positions move both results", distinct );

    check ( "and the buffer is back where it started",
            ( uint8_t ) ( ( crc16 ( buffer, 4u ) == base16 ) &&
                          ( crc32 ( buffer, 4u ) == base32 ) ) );
}

int main ( void )
{
    crc16Case ( );
    printf ( "\n" );
    crc16AltCase ( );
    printf ( "\n" );
    crc16EquivalenceCase ( );
    printf ( "\n" );
    crc32Case ( );
    printf ( "\n" );
    sensitivityCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%u check(s) failed\n", ( unsigned ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
