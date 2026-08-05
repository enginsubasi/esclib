/*
 * Checksum module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * The expected values are not taken from this implementation. Fletcher16 of
 * "abcde" and Adler32 of "Wikipedia" are the published vectors for those two
 * algorithms, and the rest are hand computable from a four byte array.
 *
 * hookCase is the reason two of these return uint16_t. comstxetxInit takes a
 * checksum callback whose type is crc16's, so anything with that exact
 * signature installs with no wrapper. Assigning to a pointer of that type is
 * a compile time proof; the call through it then shows the answer survives.
 */

#include <stdio.h>
#include <stddef.h>

#include "checksum.h"

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

/*
 * Sums to 0x0194, which is past eight bits, so the two sum widths disagree
 * and the difference between them is visible. Xors to 0x88.
 */
static const uint8_t sample[ 4 ] = { 0x12u, 0x34u, 0x56u, 0xF8u };

/* ---------------------------------------------------------- the eight bit */

static void narrowCase ( void )
{
    printf ( "checksum eight bit\n" );

    check ( "xor of the sample",
            ( uint8_t ) ( checksumXor ( sample, 4u ) == 0x88u ) );
    check ( "an empty range xors to zero",
            ( uint8_t ) ( checksumXor ( sample, 0u ) == 0x00u ) );

    /*
     * The sample sums to 0x0194, so the eight bit sum keeps only 0x94 while
     * the sixteen bit one keeps the carry. That difference is the only
     * reason both exist.
     */
    check ( "sum8 of the sample truncates to eight bits",
            ( uint8_t ) ( checksumSum8 ( sample, 4u ) == 0x94u ) );
    check ( "sum16 of the same bytes keeps the carry",
            ( uint8_t ) ( checksumSum16 ( sample, 4u ) == 0x0194u ) );

    check ( "an empty range sums to zero",
            ( uint8_t ) ( checksumSum8 ( sample, 0u ) == 0x00u ) );
    check ( "an empty range sums to zero in sixteen bits too",
            ( uint8_t ) ( checksumSum16 ( sample, 0u ) == 0x0000u ) );
}

/* --------------------------------------------------------- the positional */

/*
 * A plain sum cannot see a reordering; Fletcher and Adler both can, because
 * the second accumulator weights each byte by how far along it sits. That is
 * what these two buy over checksumSum16, and it is checked directly.
 */
static void positionalCase ( void )
{
    static const uint8_t abcde[ 5 ] = { 'a', 'b', 'c', 'd', 'e' };
    static const uint8_t wikipedia[ 9 ] =
    {
        'W', 'i', 'k', 'i', 'p', 'e', 'd', 'i', 'a'
    };
    static const uint8_t forward[ 4 ] = { 0x01u, 0x02u, 0x03u, 0x04u };
    static const uint8_t swapped[ 4 ] = { 0x01u, 0x03u, 0x02u, 0x04u };

    printf ( "checksum positional\n" );

    check ( "Fletcher16 matches the published vector for abcde",
            ( uint8_t ) ( checksumFletcher16 ( abcde, 5u ) == 0xC8F0u ) );
    check ( "Adler32 matches the published vector for Wikipedia",
            ( uint8_t ) ( checksumAdler32 ( wikipedia, 9u ) == 0x11E60398u ) );
    check ( "Adler32 of nothing is one, not zero",
            ( uint8_t ) ( checksumAdler32 ( wikipedia, 0u ) == 0x00000001u ) );
    check ( "Fletcher16 of nothing is zero",
            ( uint8_t ) ( checksumFletcher16 ( abcde, 0u ) == 0x0000u ) );

    check ( "a plain sum cannot see two bytes swapped",
            ( uint8_t ) ( checksumSum16 ( forward, 4u ) ==
                          checksumSum16 ( swapped, 4u ) ) );
    check ( "Fletcher16 can",
            ( uint8_t ) ( checksumFletcher16 ( forward, 4u ) !=
                          checksumFletcher16 ( swapped, 4u ) ) );
    check ( "Adler32 can too",
            ( uint8_t ) ( checksumAdler32 ( forward, 4u ) !=
                          checksumAdler32 ( swapped, 4u ) ) );
}

/* --------------------------------------------------------------- the hook */

/*
 * The signature claim. comstxetxInit's checksum parameter has exactly this
 * type, so a function that fits this pointer goes in with no wrapper. The
 * assignments below fail to compile if the return type or the parameters
 * ever drift, which is a stronger check than any value comparison.
 */
static void hookCase ( void )
{
    uint16_t ( *hook ) ( const uint8_t* const buffer, uint32_t length ) = NULL;

    printf ( "checksum as a protocol hook\n" );

    hook = checksumSum16;
    check ( "sum16 fits the comstxetx checksum callback",
            ( uint8_t ) ( hook ( sample, 4u ) == 0x0194u ) );

    hook = checksumFletcher16;
    check ( "and so does Fletcher16",
            ( uint8_t ) ( hook ( sample, 4u ) ==
                          checksumFletcher16 ( sample, 4u ) ) );
}

int main ( void )
{
    narrowCase ( );

    printf ( "\n" );
    positionalCase ( );

    printf ( "\n" );
    hookCase ( );

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
