/*
 * Covers pack.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Every expected value here was written out by hand from the byte pattern
 * rather than taken from a run, which for this module is the whole point: the
 * functions exist because the hand written version of them is where the bugs
 * are, so a test that agreed with the implementation by construction would
 * check nothing.
 */

#include <stdio.h>

#include "pack.h"

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

/* Four bytes that are different in every position and in every nibble. */
static const uint8_t ascending[ 4 ] = { 0x12u, 0x34u, 0x56u, 0x78u };

static const uint8_t allOnes[ 4 ] = { 0xFFu, 0xFFu, 0xFFu, 0xFFu };
static const uint8_t signBitFirst[ 4 ] = { 0x80u, 0x00u, 0x00u, 0x00u };
static const uint8_t signBitLast[ 4 ] = { 0x00u, 0x00u, 0x00u, 0x80u };
static const uint8_t maxPositive[ 4 ] = { 0x7Fu, 0xFFu, 0xFFu, 0xFFu };

/*
 * An HX711 load cell reading of minus ten counts. This is the pattern that
 * makes a hand written twenty four bit reader look correct right up until the
 * tare goes slightly negative and the reading jumps to sixteen million.
 */
static const uint8_t hx711Negative[ 3 ] = { 0xFFu, 0xFFu, 0xF6u };

/* --------------------------------------------------------------- readers */

static void readUnsignedCase ( void )
{
    printf ( "pack unsigned readers\n" );

    check ( "u16 big endian takes the first byte as the high one",
            ( uint8_t ) ( packGetU16be ( ascending, 0u ) == 0x1234u ) );
    check ( "u16 little endian takes the second",
            ( uint8_t ) ( packGetU16le ( ascending, 0u ) == 0x3412u ) );

    check ( "u32 big endian",
            ( uint8_t ) ( packGetU32be ( ascending, 0u ) == 0x12345678uL ) );
    check ( "u32 little endian",
            ( uint8_t ) ( packGetU32le ( ascending, 0u ) == 0x78563412uL ) );

    check ( "u24 big endian drops the fourth byte",
            ( uint8_t ) ( packGetU24be ( ascending, 0u ) == 0x123456uL ) );
    check ( "u24 little endian drops it too",
            ( uint8_t ) ( packGetU24le ( ascending, 0u ) == 0x563412uL ) );

    /* The index is a byte offset into the buffer, not an element number. */
    check ( "u16 big endian at an offset reads from there",
            ( uint8_t ) ( packGetU16be ( ascending, 2u ) == 0x5678u ) );
    check ( "and little endian at the same offset",
            ( uint8_t ) ( packGetU16le ( ascending, 2u ) == 0x7856u ) );
    check ( "u16 big endian at an odd offset, which no word access could do",
            ( uint8_t ) ( packGetU16be ( ascending, 1u ) == 0x3456u ) );

    check ( "all ones reads as the full range in every width",
            ( uint8_t ) ( ( packGetU16be ( allOnes, 0u ) == 0xFFFFu ) &&
                          ( packGetU24be ( allOnes, 0u ) == 0xFFFFFFuL ) &&
                          ( packGetU32be ( allOnes, 0u ) == 0xFFFFFFFFuL ) ) );

    /*
     * The two endiannesses of one width are byte reversals of each other, which
     * has to hold for every buffer and is the cheapest way to catch a shift
     * that went the wrong way in only one of the pair.
     */
    check ( "u16 be and le are byte reversals",
            ( uint8_t ) ( packGetU16be ( ascending, 0u ) ==
                          ( ( uint16_t ) ( ( ( packGetU16le ( ascending, 0u ) & 0x00FFu ) << 8 ) |
                                           ( ( packGetU16le ( ascending, 0u ) >> 8 ) & 0x00FFu ) ) ) ) );
}

static void readSignedCase ( void )
{
    printf ( "pack signed readers\n" );

    check ( "i16 reads all ones as minus one, not as 65535",
            ( uint8_t ) ( packGetI16be ( allOnes, 0u ) == -1 ) );
    check ( "and little endian agrees, the pattern being symmetric",
            ( uint8_t ) ( packGetI16le ( allOnes, 0u ) == -1 ) );

    check ( "i16 big endian reads the sign bit in the first byte",
            ( uint8_t ) ( packGetI16be ( signBitFirst, 0u ) == -32768 ) );
    check ( "and little endian does not, the same bytes being a small positive",
            ( uint8_t ) ( packGetI16le ( signBitFirst, 0u ) == 128 ) );

    check ( "i16 little endian reads the sign bit in the second byte",
            ( uint8_t ) ( packGetI16le ( signBitLast, 2u ) == -32768 ) );

    check ( "i16 stops one short of the sign bit at the top of the range",
            ( uint8_t ) ( packGetI16be ( maxPositive, 0u ) == 32767 ) );

    check ( "i32 reads all ones as minus one",
            ( uint8_t ) ( packGetI32be ( allOnes, 0u ) == -1 ) );
    check ( "and little endian too",
            ( uint8_t ) ( packGetI32le ( allOnes, 0u ) == -1 ) );

    /*
     * The most negative int32_t. Written as a subtraction rather than as a
     * literal, because -2147483648 is not a literal in C: it is the unary minus
     * of 2147483648, which does not fit in int32_t.
     */
    check ( "i32 big endian reaches the most negative value",
            ( uint8_t ) ( packGetI32be ( signBitFirst, 0u ) == ( -2147483647 - 1 ) ) );
    check ( "and the same bytes little endian are a small positive",
            ( uint8_t ) ( packGetI32le ( signBitFirst, 0u ) == 128 ) );
    check ( "i32 reaches the most positive value",
            ( uint8_t ) ( packGetI32be ( maxPositive, 0u ) == 2147483647 ) );
}

static void read24Case ( void )
{
    printf ( "pack 24 bit readers\n" );

    /*
     * The reason this module has 24 bit readers at all. There is no 24 bit
     * type, so nothing in C sign extends these, and the version that forgets
     * to returns sixteen million for a reading of minus ten.
     */
    check ( "i24 sign extends an HX711 reading of minus ten",
            ( uint8_t ) ( packGetI24be ( hx711Negative, 0u ) == -10 ) );
    check ( "and the unsigned reader shows what it would have been without",
            ( uint8_t ) ( packGetU24be ( hx711Negative, 0u ) == 16777206uL ) );

    check ( "i24 reads all ones as minus one",
            ( uint8_t ) ( packGetI24be ( allOnes, 0u ) == -1 ) );
    check ( "and little endian too",
            ( uint8_t ) ( packGetI24le ( allOnes, 0u ) == -1 ) );

    check ( "i24 reaches its most negative value",
            ( uint8_t ) ( packGetI24be ( signBitFirst, 0u ) == -8388608 ) );
    check ( "and its most positive one",
            ( uint8_t ) ( packGetI24be ( maxPositive, 0u ) == 8388607 ) );

    /*
     * Either side of the sign boundary, which is where an off by one in the
     * threshold would show and nowhere else.
     */
    check ( "0x7FFFFF is positive and 0x800000 is not",
            ( uint8_t ) ( ( packGetI24be ( maxPositive, 0u ) > 0 ) &&
                          ( packGetI24be ( signBitFirst, 0u ) < 0 ) ) );

    check ( "little endian 24 bit reads the third byte as the high one",
            ( uint8_t ) ( packGetU24le ( signBitLast, 1u ) == 0x800000uL ) );
}

/* --------------------------------------------------------------- writers */

static void writeCase ( void )
{
    uint8_t buffer[ 8 ];
    uint32_t i = 0;

    printf ( "pack writers\n" );

    for ( i = 0u; i < 8u; ++i )
    {
        buffer[ i ] = 0xAAu;
    }

    packPutU16be ( buffer, 2u, 0x1234u );
    check ( "u16 big endian writes the high byte first",
            ( uint8_t ) ( ( buffer[ 2 ] == 0x12u ) && ( buffer[ 3 ] == 0x34u ) ) );
    check ( "and touches nothing on either side",
            ( uint8_t ) ( ( buffer[ 1 ] == 0xAAu ) && ( buffer[ 4 ] == 0xAAu ) ) );

    packPutU16le ( buffer, 2u, 0x1234u );
    check ( "u16 little endian writes the low byte first",
            ( uint8_t ) ( ( buffer[ 2 ] == 0x34u ) && ( buffer[ 3 ] == 0x12u ) ) );

    packPutU32be ( buffer, 0u, 0x12345678uL );
    check ( "u32 big endian",
            ( uint8_t ) ( ( buffer[ 0 ] == 0x12u ) && ( buffer[ 1 ] == 0x34u ) &&
                          ( buffer[ 2 ] == 0x56u ) && ( buffer[ 3 ] == 0x78u ) ) );

    packPutU32le ( buffer, 4u, 0x12345678uL );
    check ( "u32 little endian",
            ( uint8_t ) ( ( buffer[ 4 ] == 0x78u ) && ( buffer[ 5 ] == 0x56u ) &&
                          ( buffer[ 6 ] == 0x34u ) && ( buffer[ 7 ] == 0x12u ) ) );
    check ( "and the big endian word before it is untouched",
            ( uint8_t ) ( ( buffer[ 0 ] == 0x12u ) && ( buffer[ 3 ] == 0x78u ) ) );
}

static void roundTripCase ( void )
{
    uint8_t buffer[ 4 ];

    printf ( "pack round trips\n" );

    packPutU16be ( buffer, 0u, 0u );
    check ( "u16 be round trips zero",
            ( uint8_t ) ( packGetU16be ( buffer, 0u ) == 0u ) );

    packPutU16be ( buffer, 0u, 0xFFFFu );
    check ( "u16 be round trips the top of the range",
            ( uint8_t ) ( packGetU16be ( buffer, 0u ) == 0xFFFFu ) );

    packPutU16le ( buffer, 0u, 0xBEEFu );
    check ( "u16 le round trips",
            ( uint8_t ) ( packGetU16le ( buffer, 0u ) == 0xBEEFu ) );

    packPutU32be ( buffer, 0u, 0xDEADBEEFuL );
    check ( "u32 be round trips",
            ( uint8_t ) ( packGetU32be ( buffer, 0u ) == 0xDEADBEEFuL ) );

    packPutU32le ( buffer, 0u, 0xDEADBEEFuL );
    check ( "u32 le round trips",
            ( uint8_t ) ( packGetU32le ( buffer, 0u ) == 0xDEADBEEFuL ) );

    /*
     * There are no signed writers, because converting a signed value to an
     * unsigned one of the same width is defined to wrap. This is the call that
     * claim licenses, so it is the one the test makes.
     */
    packPutU16be ( buffer, 0u, ( uint16_t ) ( -1234 ) );
    check ( "a negative value cast through the unsigned writer round trips",
            ( uint8_t ) ( packGetI16be ( buffer, 0u ) == -1234 ) );

    packPutU32le ( buffer, 0u, ( uint32_t ) ( -123456789L ) );
    check ( "and so does a negative 32 bit one, little endian",
            ( uint8_t ) ( packGetI32le ( buffer, 0u ) == -123456789L ) );

    packPutU32be ( buffer, 0u, ( uint32_t ) ( -2147483647L - 1L ) );
    check ( "including the most negative value there is",
            ( uint8_t ) ( packGetI32be ( buffer, 0u ) == ( -2147483647L - 1L ) ) );
}

int main ( void )
{
    readUnsignedCase ( );
    printf ( "\n" );
    readSignedCase ( );
    printf ( "\n" );
    read24Case ( );
    printf ( "\n" );
    writeCase ( );
    printf ( "\n" );
    roundTripCase ( );

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
