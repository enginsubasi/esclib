/*
 * Covers crc8, crc8Dallas, crc16, crc16Alt, crc32 and crc32Alt, and the
 * streaming entry point of each of them.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The expected values were generated from the polynomials rather than copied
 * from the implementations, so this checks that the two functions compute the
 * algorithms they claim to and not merely that they agree with themselves. The
 * published check values pin them to the named standards: 0xF4 for CRC-8/SMBUS
 * and 0xA1 for CRC-8/MAXIM-DOW over "123456789", 0x4B37 for MODBUS CRC16 over
 * the same, and 0x0376E6E7 for CRC-32/MPEG-2.
 */

#include <stdio.h>

#include "crc8.h"
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

/*
 * A real DS18B20 ROM code. The last byte is the CRC the device itself
 * supplies, so the first seven bytes must produce it and all eight must
 * produce zero.
 */
static const uint8_t ds18b20Rom[ 8 ] = { 0x28u, 0x1Du, 0x39u, 0x31u,
                                            0x02u, 0x00u, 0x00u, 0xF0u };

/* A single 0x01, which under poly 0x07 has to come out as the poly. */
static const uint8_t oneOne[ 1 ] = { 0x01u };

/* An SMBus write byte transaction: address, command, data. */
static const uint8_t smbusWrite[ 3 ] = { 0xB8u, 0x00u, 0x00u };

/* The MODBUS frame fixture with one bit of its last byte flipped. */
static const uint8_t modbusFlipped[ 6 ] = { 0x01u, 0x03u, 0x00u, 0x00u, 0x00u, 0x0Bu };

/* The same frame with its first two bytes exchanged. */
static const uint8_t modbusSwapped[ 6 ] = { 0x03u, 0x01u, 0x00u, 0x00u, 0x00u, 0x0Au };

/* "123456789" with each algorithm's own CRC appended, for the self check. */
static const uint8_t checkVectorPlusSmbus[ 10 ] =
        { '1', '2', '3', '4', '5', '6', '7', '8', '9', 0xF4u };
static const uint8_t checkVectorPlusDallas[ 10 ] =
        { '1', '2', '3', '4', '5', '6', '7', '8', '9', 0xA1u };

/* ----------------------------------------------------------------- crc8 */

static void crc8Case ( void )
{
    printf ( "crc8\n" );

    check ( "an empty array returns the 0x00 seed",
            ( uint8_t ) ( crc8 ( checkVector, 0u ) == 0x00u ) );

    check ( "the published CRC-8/SMBUS check value over \"123456789\"",
            ( uint8_t ) ( crc8 ( checkVector, 9u ) == 0xF4u ) );

    check ( "a single 0xFF byte",
            ( uint8_t ) ( crc8 ( oneFF, 1u ) == 0xF3u ) );

    /*
     * The polynomial applied once, on its own. A single 0x01 through poly 0x07
     * has to come out as the polynomial itself, which separates the shift
     * direction from the xor.
     */
    check ( "a single 0x01 byte comes out as the polynomial itself",
            ( uint8_t ) ( crc8 ( oneOne, 1u ) == 0x07u ) );

    check ( "an SMBus write byte transaction",
            ( uint8_t ) ( crc8 ( smbusWrite, 3u ) == 0xBBu ) );

    check ( "a MODBUS read holding registers frame",
            ( uint8_t ) ( crc8 ( modbusFrame, 6u ) == 0xB9u ) );

    /*
     * Seeded with zero and not inverted, so a buffer carrying its own CRC as
     * the last byte checks to zero. That is how a receiver normally verifies
     * one, rather than by recomputing and comparing.
     */
    check ( "a buffer carrying its own PEC checks to zero",
            ( uint8_t ) ( crc8 ( checkVectorPlusSmbus, 10u ) == 0x00u ) );
}

/* ----------------------------------------------------------- crc8Dallas */

static void crc8DallasCase ( void )
{
    printf ( "crc8Dallas\n" );

    check ( "an empty array returns the 0x00 seed",
            ( uint8_t ) ( crc8Dallas ( checkVector, 0u ) == 0x00u ) );

    check ( "the published CRC-8/MAXIM-DOW check value over \"123456789\"",
            ( uint8_t ) ( crc8Dallas ( checkVector, 9u ) == 0xA1u ) );

    check ( "a single 0xFF byte",
            ( uint8_t ) ( crc8Dallas ( oneFF, 1u ) == 0x35u ) );

    check ( "a single 0x01 byte, which the reflected polynomial moves elsewhere",
            ( uint8_t ) ( crc8Dallas ( oneOne, 1u ) == 0x5Eu ) );

    /*
     * A real device. The seven data bytes of a DS18B20 ROM code have to
     * produce the eighth, which is the byte the part itself reports.
     */
    check ( "the first seven bytes of a DS18B20 ROM give its eighth",
            ( uint8_t ) ( crc8Dallas ( ds18b20Rom, 7u ) == 0xF0u ) );

    check ( "and the whole ROM including that byte checks to zero",
            ( uint8_t ) ( crc8Dallas ( ds18b20Rom, 8u ) == 0x00u ) );

    check ( "a buffer carrying its own CRC checks to zero too",
            ( uint8_t ) ( crc8Dallas ( checkVectorPlusDallas, 10u ) == 0x00u ) );

    /*
     * The two CRC8s are different polynomials, not two spellings of one. A
     * caller who reaches for the wrong one gets a byte that is wrong and that
     * nothing downstream reports, so this pins them apart on every fixture
     * they share.
     */
    check ( "it disagrees with crc8 on the check vector",
            ( uint8_t ) ( crc8Dallas ( checkVector, 9u ) != crc8 ( checkVector, 9u ) ) );
    check ( "and on a single byte",
            ( uint8_t ) ( crc8Dallas ( oneFF, 1u ) != crc8 ( oneFF, 1u ) ) );
    check ( "and on a frame",
            ( uint8_t ) ( crc8Dallas ( modbusFrame, 6u ) != crc8 ( modbusFrame, 6u ) ) );

    check ( "a MODBUS frame under the 1-Wire polynomial",
            ( uint8_t ) ( crc8Dallas ( modbusFrame, 6u ) == 0x07u ) );
}

/* ------------------------------------------------- crc8 sensitivity */

static void crc8SensitivityCase ( void )
{
    printf ( "crc8 sensitivity\n" );

    /* One flipped bit has to move both of them. */
    check ( "crc8 sees a single flipped bit",
            ( uint8_t ) ( crc8 ( modbusFlipped, 6u ) != crc8 ( modbusFrame, 6u ) ) );
    check ( "crc8Dallas sees it too",
            ( uint8_t ) ( crc8Dallas ( modbusFlipped, 6u ) != crc8Dallas ( modbusFrame, 6u ) ) );

    /*
     * And a reordering, which is what a CRC buys over a plain sum. checksumXor
     * and checksumSum8 are both blind to this exchange.
     */
    check ( "crc8 sees two bytes exchanged",
            ( uint8_t ) ( crc8 ( modbusSwapped, 6u ) != crc8 ( modbusFrame, 6u ) ) );
    check ( "crc8Dallas sees it too",
            ( uint8_t ) ( crc8Dallas ( modbusSwapped, 6u ) != crc8Dallas ( modbusFrame, 6u ) ) );

    /*
     * The one thing a zero seed cannot see. Neither function distinguishes a
     * run of zero bytes from a different length run of zero bytes, because
     * the seed is zero and a zero byte leaves a zero register alone. It is
     * documented in crc8.c and pinned here so that nobody discovers it in the
     * field.
     */
    check ( "a zero seed cannot tell one zero byte from two",
            ( uint8_t ) ( crc8 ( oneZero, 1u ) == crc8 ( twoZeros, 2u ) ) );
    check ( "and crc8Dallas cannot either",
            ( uint8_t ) ( crc8Dallas ( oneZero, 1u ) == crc8Dallas ( twoZeros, 2u ) ) );
}

/* ------------------------------------------------------------ crc32Alt */

static void crc32AltCase ( void )
{
    printf ( "crc32Alt\n" );

    /*
     * The same polynomial without the table. The only claim worth making about
     * a second implementation of one algorithm is that it agrees with the
     * first, so every fixture in this file is run through both.
     */
    check ( "an empty array returns the 0xFFFFFFFF seed",
            ( uint8_t ) ( crc32Alt ( checkVector, 0u ) == 0xFFFFFFFFu ) );

    check ( "the published CRC-32/MPEG-2 check value over \"123456789\"",
            ( uint8_t ) ( crc32Alt ( checkVector, 9u ) == 0x0376E6E7u ) );

    check ( "a six byte frame",
            ( uint8_t ) ( crc32Alt ( modbusFrame, 6u ) == 0xD5CFCF4Bu ) );
    check ( "a single zero byte",
            ( uint8_t ) ( crc32Alt ( oneZero, 1u ) == 0x4E08BFB4u ) );
    check ( "a single 0xFF byte",
            ( uint8_t ) ( crc32Alt ( oneFF, 1u ) == 0xFFFFFF00u ) );
    check ( "two zero bytes",
            ( uint8_t ) ( crc32Alt ( twoZeros, 2u ) == 0x00B7647Du ) );

    check ( "and it agrees with the table on every one of them",
            ( uint8_t ) ( ( crc32Alt ( checkVector, 0u ) == crc32 ( checkVector, 0u ) ) &&
                          ( crc32Alt ( checkVector, 9u ) == crc32 ( checkVector, 9u ) ) &&
                          ( crc32Alt ( modbusFrame, 6u ) == crc32 ( modbusFrame, 6u ) ) &&
                          ( crc32Alt ( oneZero, 1u ) == crc32 ( oneZero, 1u ) ) &&
                          ( crc32Alt ( oneFF, 1u ) == crc32 ( oneFF, 1u ) ) &&
                          ( crc32Alt ( twoZeros, 2u ) == crc32 ( twoZeros, 2u ) ) &&
                          ( crc32Alt ( ds18b20Rom, 8u ) == crc32 ( ds18b20Rom, 8u ) ) &&
                          ( crc32Alt ( smbusWrite, 3u ) == crc32 ( smbusWrite, 3u ) ) &&
                          ( crc32Alt ( modbusSwapped, 6u ) == crc32 ( modbusSwapped, 6u ) ) ) );

    /*
     * A byte at a time through the whole range, which is the only way to be
     * sure the two agree everywhere rather than on the handful of fixtures
     * that happened to be here.
     */
    {
        uint8_t every[ 256 ];
        uint32_t i = 0;
        uint8_t agree = TRUE;

        for ( i = 0; i < 256u; ++i )
        {
            every[ i ] = ( uint8_t ) i;
        }

        for ( i = 0; i <= 256u; ++i )
        {
            if ( crc32Alt ( every, i ) != crc32 ( every, i ) )
            {
                agree = FALSE;
            }
            else
            {
                /* Intentionally blank. */
            }
        }

        check ( "and over every prefix of a buffer holding all 256 byte values",
                agree );
    }

    /* It sees a flipped bit and a reordering, as the table version does. */
    check ( "it sees a single flipped bit",
            ( uint8_t ) ( crc32Alt ( modbusFlipped, 6u ) != crc32Alt ( modbusFrame, 6u ) ) );
    check ( "and two bytes exchanged",
            ( uint8_t ) ( crc32Alt ( modbusSwapped, 6u ) != crc32Alt ( modbusFrame, 6u ) ) );
}

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


/* ------------------------------------------------------------- streaming */

static uint8_t streamBuffer[ 256 ];

static void streamingCase ( void )
{
    uint32_t i = 0;
    uint32_t n = 0;
    uint16_t sixteen = 0;
    uint16_t sixteenAlt = 0;
    uint32_t thirtyTwo = 0;
    uint32_t thirtyTwoAlt = 0;
    uint8_t eight = 0;
    uint8_t dallas = 0;
    uint8_t ok = TRUE;

    printf ( "a byte at a time\n" );

    for ( i = 0; i < 256u; ++i )
    {
        streamBuffer[ i ] = ( uint8_t ) ( ( i * 7u ) + 3u );
    }

    /*
     * Every prefix rather than one length. An Update that agreed with the
     * whole buffer form at 256 bytes and nowhere else would still be wrong,
     * and the ways it goes wrong — a seed applied twice, a byte taken into
     * the wrong end, a table indexed by the byte instead of by the byte mixed
     * with the value so far — show at particular lengths rather than at all
     * of them. The empty prefix is in here too: the seed alone has to be what
     * the whole buffer form gives for no bytes.
     */
    for ( n = 0; n <= 256u; ++n )
    {
        sixteen = CRC16_SEED;
        sixteenAlt = CRC16_SEED;
        thirtyTwo = CRC32_SEED;
        thirtyTwoAlt = CRC32_SEED;
        eight = CRC8_SEED;
        dallas = CRC8_DALLAS_SEED;

        for ( i = 0; i < n; ++i )
        {
            sixteen = crc16Update ( sixteen, streamBuffer[ i ] );
            sixteenAlt = crc16AltUpdate ( sixteenAlt, streamBuffer[ i ] );
            thirtyTwo = crc32Update ( thirtyTwo, streamBuffer[ i ] );
            thirtyTwoAlt = crc32AltUpdate ( thirtyTwoAlt, streamBuffer[ i ] );
            eight = crc8Update ( eight, streamBuffer[ i ] );
            dallas = crc8DallasUpdate ( dallas, streamBuffer[ i ] );
        }

        if ( ( sixteen != crc16 ( streamBuffer, n ) ) ||
             ( sixteenAlt != crc16 ( streamBuffer, n ) ) ||
             ( thirtyTwo != crc32 ( streamBuffer, n ) ) ||
             ( thirtyTwoAlt != crc32 ( streamBuffer, n ) ) ||
             ( eight != crc8 ( streamBuffer, n ) ) ||
             ( dallas != crc8Dallas ( streamBuffer, n ) ) )
        {
            ok = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "every prefix streams to what the whole buffer form gives", ok );

    /* The published check value, reached a byte at a time. */
    sixteen = CRC16_SEED;

    for ( i = 0; i < 9u; ++i )
    {
        sixteen = crc16Update ( sixteen, checkVector[ i ] );
    }

    check ( "the MODBUS check value, streamed",
            ( uint8_t ) ( sixteen == 0x4B37u ) );

    /*
     * What the zero seed is for. A 1-Wire ROM read is seven bytes and the CRC
     * of those seven, and taking all eight into the same running value gives
     * zero — which is how a reader checks the read without holding it.
     */
    dallas = CRC8_DALLAS_SEED;

    for ( i = 0; i < 7u; ++i )
    {
        dallas = crc8DallasUpdate ( dallas, streamBuffer[ i ] );
    }

    dallas = crc8DallasUpdate ( dallas, crc8Dallas ( streamBuffer, 7u ) );

    check ( "a Dallas read including its own CRC streams to zero",
            ( uint8_t ) ( dallas == 0x00u ) );
}

int main ( void )
{
    crc8Case ( );
    printf ( "\n" );
    crc8DallasCase ( );
    printf ( "\n" );
    crc8SensitivityCase ( );
    printf ( "\n" );
    crc16Case ( );
    printf ( "\n" );
    crc16AltCase ( );
    printf ( "\n" );
    crc16EquivalenceCase ( );
    printf ( "\n" );
    crc32Case ( );
    printf ( "\n" );
    crc32AltCase ( );
    printf ( "\n" );
    sensitivityCase ( );
    printf ( "\n" );
    streamingCase ( );

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
