/*
 * Covers text.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The base64 vectors are the ones RFC 4648 gives in its section 10, so they
 * are the standard's answers rather than this module's. The Q16 expectations
 * were computed in exact decimal arithmetic on a host, rounding half away from
 * zero, and everything else follows from the definitions.
 *
 * Two cases are here for a sanitizer rather than for an assertion. Writing and
 * reading the most negative int32_t is where a hand written version negates a
 * value that has no positive counterpart — undefined, and on every compiler in
 * reach it happens to give the right answer anyway. The checks below would
 * pass against that version; what they do is make the lines reachable, so the
 * sanitized run in CI can see what an assertion cannot.
 */

#include <stdio.h>
#include <string.h>

#include "text.h"

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

/* The ends of int32_t, written as expressions because -2147483648 is not a
 * literal in C: it is the unary minus of a value that does not fit. */
#define TEXT_TEST_MAX       2147483647
#define TEXT_TEST_MIN       ( -2147483647 - 1 )

static uint8_t buffer [ 128 ];
static uint8_t bytes [ 300 ];
static uint8_t back [ 300 ];

/* Whether a run of text is exactly the string given. */
static uint8_t same ( const uint8_t* const got, uint32_t length,
                      const char* const want )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0u;

    if ( length != ( uint32_t ) strlen ( want ) )
    {
        retVal = FALSE;
    }
    else
    {
        for ( i = 0u; i < length; ++i )
        {
            if ( got[ i ] != ( uint8_t ) want[ i ] )
            {
                retVal = FALSE;
            }
            else
            {
                /* Intentionally blank */
            }
        }
    }

    return ( retVal );
}

/* Fills the buffer with a marker, so a refusal can be shown to have written
   nothing. */
static void mark ( void )
{
    memset ( buffer, 0xEE, sizeof ( buffer ) );
}

static uint8_t untouched ( void )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0u;

    for ( i = 0u; i < sizeof ( buffer ); ++i )
    {
        if ( buffer[ i ] != 0xEEu )
        {
            retVal = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    return ( retVal );
}

/* The parsers take bytes and a length; the fixtures are string literals. */
static const uint8_t* literal ( const char* const s )
{
    return ( ( const uint8_t* ) s );
}

static uint32_t size ( const char* const s )
{
    return ( ( uint32_t ) strlen ( s ) );
}

/* ------------------------------------------------------------------ hex */

static void hexCase ( void )
{
    uint8_t source [ 4 ] = { 0x00u, 0x1Fu, 0xA5u, 0xFFu };
    uint32_t written = 0u;
    uint32_t i = 0u;
    uint8_t ok = TRUE;

    printf ( "hexadecimal\n" );

    check ( "four bytes encode",
            textHexEncode ( source, 4u, buffer, sizeof ( buffer ), &written ) );
    check ( "to eight capitals, high nibble first",
            same ( buffer, written, "001FA5FF" ) );

    check ( "nothing encodes to nothing",
            ( uint8_t ) ( textHexEncode ( source, 0u, buffer, 0u, &written ) &&
                          ( written == 0u ) ) );

    mark ( );
    check ( "a destination one byte short is refused",
            ( uint8_t ) ( textHexEncode ( source, 4u, buffer, 7u, &written )
                          == FALSE ) );
    check ( "and nothing was written", untouched ( ) );

    /* Read back in mixed case, which is the case the world writes it in. */
    check ( "mixed case decodes",
            textHexDecode ( literal ( "001fA5Ff" ), 8u, buffer,
                            sizeof ( buffer ), &written ) );
    check ( "to the bytes it came from",
            ( uint8_t ) ( ( written == 4u ) && ( buffer[ 0 ] == 0x00u ) &&
                          ( buffer[ 1 ] == 0x1Fu ) && ( buffer[ 2 ] == 0xA5u ) &&
                          ( buffer[ 3 ] == 0xFFu ) ) );

    mark ( );
    check ( "an odd length is refused",
            ( uint8_t ) ( textHexDecode ( literal ( "ABC" ), 3u, buffer,
                                          sizeof ( buffer ), &written )
                          == FALSE ) );
    check ( "a character that is not a digit is refused",
            ( uint8_t ) ( textHexDecode ( literal ( "0G" ), 2u, buffer,
                                          sizeof ( buffer ), &written )
                          == FALSE ) );
    check ( "a destination too small is refused",
            ( uint8_t ) ( textHexDecode ( literal ( "ABCD" ), 4u, buffer, 1u,
                                          &written ) == FALSE ) );
    check ( "and none of them wrote anything", untouched ( ) );

    /* Every byte value there and back. */
    for ( i = 0u; i < 256u; ++i )
    {
        bytes[ i ] = ( uint8_t ) i;
    }

    {
        static uint8_t wide [ 512 ];

        if ( ( textHexEncode ( bytes, 256u, wide, 512u, &written ) == FALSE ) ||
             ( textHexDecode ( wide, written, back, 300u, &written ) == FALSE ) ||
             ( written != 256u ) )
        {
            ok = FALSE;
        }
        else
        {
            for ( i = 0u; i < 256u; ++i )
            {
                if ( back[ i ] != ( uint8_t ) i )
                {
                    ok = FALSE;
                }
                else
                {
                    /* Intentionally blank */
                }
            }
        }
    }

    check ( "every byte value survives the round trip", ok );
}

/* --------------------------------------------------------------- base64 */

static uint8_t base64Vector ( const char* const plain, const char* const coded )
{
    uint8_t retVal = TRUE;
    uint32_t written = 0u;

    if ( ( textBase64Encode ( literal ( plain ), size ( plain ), buffer,
                              sizeof ( buffer ), &written ) == FALSE ) ||
         ( same ( buffer, written, coded ) == FALSE ) )
    {
        retVal = FALSE;
    }
    else if ( ( textBase64Decode ( literal ( coded ), size ( coded ), buffer,
                                   sizeof ( buffer ), &written ) == FALSE ) ||
              ( same ( buffer, written, plain ) == FALSE ) )
    {
        retVal = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

static uint8_t base64Refused ( const char* const coded )
{
    uint32_t written = 0u;

    return ( ( uint8_t ) ( textBase64Decode ( literal ( coded ), size ( coded ),
                                              buffer, sizeof ( buffer ),
                                              &written ) == FALSE ) );
}

static void base64Case ( void )
{
    uint8_t ends [ 2 ] = { 0xFBu, 0xFFu };
    uint32_t written = 0u;
    uint32_t length = 0u;
    uint32_t i = 0u;
    uint8_t ok = TRUE;
    static uint8_t coded [ 64 ];

    printf ( "base64\n" );

    /* RFC 4648, section 10, both ways. */
    check ( "the empty vector", base64Vector ( "", "" ) );
    check ( "f", base64Vector ( "f", "Zg==" ) );
    check ( "fo", base64Vector ( "fo", "Zm8=" ) );
    check ( "foo", base64Vector ( "foo", "Zm9v" ) );
    check ( "foob", base64Vector ( "foob", "Zm9vYg==" ) );
    check ( "fooba", base64Vector ( "fooba", "Zm9vYmE=" ) );
    check ( "foobar", base64Vector ( "foobar", "Zm9vYmFy" ) );

    /* The two characters at the top of the alphabet, which the vectors above
       never reach. */
    check ( "the last two digits of the alphabet encode",
            ( uint8_t ) ( textBase64Encode ( ends, 2u, buffer,
                                             sizeof ( buffer ), &written ) &&
                          same ( buffer, written, "+/8=" ) ) );

    check ( "the size of nothing is nothing",
            ( uint8_t ) ( textBase64EncodedSize ( 0u ) == 0u ) );
    check ( "one to three bytes take four characters",
            ( uint8_t ) ( ( textBase64EncodedSize ( 1u ) == 4u ) &&
                          ( textBase64EncodedSize ( 3u ) == 4u ) ) );
    check ( "and a fourth byte takes four more",
            ( uint8_t ) ( textBase64EncodedSize ( 4u ) == 8u ) );

    /*
     * Canonical form. Zg== and Zh== both decode to "f" in a lenient decoder,
     * because the only difference between them is in bits the padding says
     * are unused. Accepting both means one payload has two encodings, which
     * fools anything that compares or signs the text.
     */
    check ( "a canonical encoding is accepted", base64Vector ( "f", "Zg==" ) );
    check ( "the same byte with a stray unused bit is refused",
            base64Refused ( "Zh==" ) );
    check ( "and with one padding character",
            base64Refused ( "Zm9=" ) );

    check ( "a length that is not a multiple of four is refused",
            base64Refused ( "Zg=" ) );
    check ( "padding in the middle is refused", base64Refused ( "Zg=A" ) );
    check ( "three padding characters are refused", base64Refused ( "Z===" ) );
    check ( "padding alone is refused", base64Refused ( "====" ) );
    check ( "a space is refused", base64Refused ( "Zm 9" ) );
    check ( "and so is a line break", base64Refused ( "Zm9\nYg==" ) );

    mark ( );
    check ( "a destination too small is refused",
            ( uint8_t ) ( textBase64Decode ( literal ( "Zm9vYmFy" ), 8u, buffer,
                                             5u, &written ) == FALSE ) );
    check ( "and nothing was written", untouched ( ) );
    check ( "a destination exactly large enough is fine",
            textBase64Decode ( literal ( "Zm9vYmFy" ), 8u, buffer, 6u,
                               &written ) );

    mark ( );
    check ( "an encoding that will not fit is refused",
            ( uint8_t ) ( textBase64Encode ( literal ( "foobar" ), 6u, buffer,
                                             7u, &written ) == FALSE ) );
    check ( "and nothing was written", untouched ( ) );

    /* Every length up to forty, so each remainder is met many times. */
    for ( length = 0u; length <= 40u; ++length )
    {
        for ( i = 0u; i < length; ++i )
        {
            bytes[ i ] = ( uint8_t ) ( ( i * 37u ) + 11u );
        }

        if ( ( textBase64Encode ( bytes, length, coded, 64u,
                                  &written ) == FALSE ) ||
             ( written != textBase64EncodedSize ( length ) ) ||
             ( textBase64Decode ( coded, written, back, 300u,
                                  &written ) == FALSE ) ||
             ( written != length ) )
        {
            ok = FALSE;
        }
        else
        {
            for ( i = 0u; i < length; ++i )
            {
                if ( back[ i ] != bytes[ i ] )
                {
                    ok = FALSE;
                }
                else
                {
                    /* Intentionally blank */
                }
            }
        }
    }

    check ( "every length up to forty survives the round trip", ok );
}

/* -------------------------------------------------------------- decimal */

static void decimalCase ( void )
{
    uint32_t written = 0u;
    uint32_t u = 0u;
    int32_t s = 0;
    int64_t sweep = 0;
    uint32_t k = 0u;
    uint8_t ok = TRUE;

    printf ( "decimal\n" );

    check ( "zero", ( uint8_t ) ( textFromU32 ( 0u, buffer, 10u, &written ) &&
                                  same ( buffer, written, "0" ) ) );
    check ( "the largest unsigned value",
            ( uint8_t ) ( textFromU32 ( 4294967295u, buffer, TEXT_U32_MAX,
                                        &written ) &&
                          same ( buffer, written, "4294967295" ) ) );
    check ( "and one byte short of room for it",
            ( uint8_t ) ( textFromU32 ( 4294967295u, buffer, 9u, &written )
                          == FALSE ) );

    check ( "minus one",
            ( uint8_t ) ( textFromI32 ( -1, buffer, 10u, &written ) &&
                          same ( buffer, written, "-1" ) ) );
    check ( "the largest signed value",
            ( uint8_t ) ( textFromI32 ( TEXT_TEST_MAX, buffer, TEXT_I32_MAX,
                                        &written ) &&
                          same ( buffer, written, "2147483647" ) ) );
    check ( "the most negative value, which has no positive counterpart",
            ( uint8_t ) ( textFromI32 ( TEXT_TEST_MIN, buffer, TEXT_I32_MAX,
                                        &written ) &&
                          same ( buffer, written, "-2147483648" ) ) );
    check ( "and one byte short of room for it",
            ( uint8_t ) ( textFromI32 ( TEXT_TEST_MIN, buffer, 10u, &written )
                          == FALSE ) );

    check ( "reads zero",
            ( uint8_t ) ( textToU32 ( literal ( "0" ), 1u, &u ) && ( u == 0u ) ) );
    check ( "reads leading zeros",
            ( uint8_t ) ( textToU32 ( literal ( "007" ), 3u, &u ) &&
                          ( u == 7u ) ) );
    check ( "reads the largest unsigned value",
            ( uint8_t ) ( textToU32 ( literal ( "4294967295" ), 10u, &u ) &&
                          ( u == 4294967295u ) ) );

    /*
     * One past it. atoi's version of this is undefined, and in practice it is
     * zero or some other wrapped number with no sign that anything happened.
     */
    u = 12345u;
    check ( "refuses one past the largest",
            ( uint8_t ) ( textToU32 ( literal ( "4294967296" ), 10u, &u )
                          == FALSE ) );
    check ( "and far past it",
            ( uint8_t ) ( textToU32 ( literal ( "99999999999" ), 11u, &u )
                          == FALSE ) );
    check ( "and the value was left alone", ( uint8_t ) ( u == 12345u ) );

    check ( "refuses nothing",
            ( uint8_t ) ( textToU32 ( literal ( "" ), 0u, &u ) == FALSE ) );
    check ( "refuses a trailing letter",
            ( uint8_t ) ( textToU32 ( literal ( "12a" ), 3u, &u ) == FALSE ) );
    check ( "refuses a sign on an unsigned value",
            ( uint8_t ) ( ( textToU32 ( literal ( "-1" ), 2u, &u ) == FALSE ) &&
                          ( textToU32 ( literal ( "+1" ), 2u, &u ) == FALSE ) ) );

    check ( "reads the most negative value",
            ( uint8_t ) ( textToI32 ( literal ( "-2147483648" ), 11u, &s ) &&
                          ( s == TEXT_TEST_MIN ) ) );
    check ( "reads the largest",
            ( uint8_t ) ( textToI32 ( literal ( "2147483647" ), 10u, &s ) &&
                          ( s == TEXT_TEST_MAX ) ) );
    check ( "refuses one past the largest",
            ( uint8_t ) ( textToI32 ( literal ( "2147483648" ), 10u, &s )
                          == FALSE ) );
    check ( "refuses one past the most negative",
            ( uint8_t ) ( textToI32 ( literal ( "-2147483649" ), 11u, &s )
                          == FALSE ) );
    check ( "refuses a sign with nothing after it",
            ( uint8_t ) ( textToI32 ( literal ( "-" ), 1u, &s ) == FALSE ) );
    check ( "refuses two signs",
            ( uint8_t ) ( textToI32 ( literal ( "--1" ), 3u, &s ) == FALSE ) );
    check ( "refuses a leading space",
            ( uint8_t ) ( textToI32 ( literal ( " 1" ), 2u, &s ) == FALSE ) );
    check ( "reads minus zero as zero",
            ( uint8_t ) ( textToI32 ( literal ( "-0" ), 2u, &s ) && ( s == 0 ) ) );

    /* Ten thousand values across the whole range, and back. */
    for ( k = 0u; k < 10000u; ++k )
    {
        sweep = ( ( int64_t ) TEXT_TEST_MIN ) + ( ( int64_t ) k * 429497 );

        if ( ( textFromI32 ( ( int32_t ) sweep, buffer, TEXT_I32_MAX,
                             &written ) == FALSE ) ||
             ( textToI32 ( buffer, written, &s ) == FALSE ) ||
             ( s != ( int32_t ) sweep ) )
        {
            ok = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "ten thousand signed values survive the round trip", ok );
}

/* ------------------------------------------------------------------ q16 */

static uint8_t q16Writes ( int32_t value, uint32_t decimals,
                           const char* const want )
{
    uint32_t written = 0u;

    return ( ( uint8_t ) ( textFromQ16 ( value, decimals, buffer, TEXT_Q16_MAX,
                                         &written ) &&
                           same ( buffer, written, want ) ) );
}

static uint8_t q16Reads ( const char* const text, int32_t want )
{
    int32_t value = 0;

    return ( ( uint8_t ) ( textToQ16 ( literal ( text ), size ( text ),
                                       &value ) && ( value == want ) ) );
}

static uint8_t q16Refused ( const char* const text )
{
    int32_t value = 777;

    return ( ( uint8_t ) ( ( textToQ16 ( literal ( text ), size ( text ),
                                         &value ) == FALSE ) &&
                           ( value == 777 ) ) );
}

static void q16Case ( void )
{
    uint32_t written = 0u;
    uint32_t k = 0u;
    int64_t sweep = 0;
    int32_t value = 0;
    uint8_t ok = TRUE;

    printf ( "Q16 as decimal text\n" );

    check ( "one, to two places", q16Writes ( 65536, 2u, "1.00" ) );
    check ( "one and a half to no places rounds away from zero",
            q16Writes ( 98304, 0u, "2" ) );
    check ( "and below zero the same way", q16Writes ( -98304, 0u, "-2" ) );

    /* 0.999 at two places is 1.00, and a writer that forgets the carry prints
       the rounded fraction, 100, where two digits should be. */
    check ( "a fraction that rounds up carries into the integer",
            q16Writes ( 65470, 2u, "1.00" ) );

    /* -0.001 at two places. Printing -0.00 reads as a measurement below zero
       that is not one. */
    check ( "a value that rounds to zero has no sign",
            q16Writes ( -66, 2u, "0.00" ) );

    /* 0.05 at four places, which needs a leading zero in the fraction. */
    check ( "the fraction keeps its leading zeros",
            q16Writes ( 3277, 4u, "0.0500" ) );
    check ( "and below zero", q16Writes ( -3277, 4u, "-0.0500" ) );

    check ( "the most negative value",
            q16Writes ( TEXT_TEST_MIN, 5u, "-32768.00000" ) );
    check ( "the largest", q16Writes ( TEXT_TEST_MAX, 5u, "32767.99998" ) );
    check ( "one LSB to nine places", q16Writes ( 1, 9u, "0.000015259" ) );

    check ( "ten places are refused",
            ( uint8_t ) ( textFromQ16 ( 1, 10u, buffer, sizeof ( buffer ),
                                        &written ) == FALSE ) );
    check ( "room for exactly the text is enough",
            textFromQ16 ( 65536, 2u, buffer, 4u, &written ) );
    check ( "and one byte less is not",
            ( uint8_t ) ( textFromQ16 ( 65536, 2u, buffer, 3u, &written )
                          == FALSE ) );

    check ( "reads one", q16Reads ( "1", 65536 ) );
    check ( "reads minus one", q16Reads ( "-1", -65536 ) );
    check ( "reads a half", q16Reads ( "0.5", 32768 ) );

    /* The digits after the point are counted, not valued: 0.05 is five
       hundredths, and reading "05" as the integer 5 makes it five tenths. */
    check ( "reads five hundredths, not five tenths", q16Reads ( "0.05", 3277 ) );
    check ( "reads an exact fraction below zero",
            q16Reads ( "-12.375", -811008 ) );
    check ( "rounds to the nearest LSB", q16Reads ( "1.00001", 65537 ) );
    check ( "reads past nine places without being moved by them",
            q16Reads ( "0.1234567890123", 8091 ) );

    check ( "reads the largest value", q16Reads ( "32767.99998", TEXT_TEST_MAX ) );
    check ( "reads the most negative", q16Reads ( "-32768", TEXT_TEST_MIN ) );
    check ( "refuses one past the largest", q16Refused ( "32768" ) );
    check ( "refuses a fraction that rounds past it",
            q16Refused ( "32767.999995" ) );
    check ( "refuses just past the most negative",
            q16Refused ( "-32768.00001" ) );

    check ( "refuses a point with nothing after it", q16Refused ( "1." ) );
    check ( "refuses a point with nothing before it", q16Refused ( ".5" ) );
    check ( "and after a sign", q16Refused ( "-.5" ) );
    check ( "refuses two points", q16Refused ( "1.2.3" ) );
    check ( "refuses nothing", q16Refused ( "" ) );
    check ( "refuses a sign alone", q16Refused ( "-" ) );
    check ( "refuses an exponent", q16Refused ( "1e3" ) );

    /*
     * Five places are enough for every value to come back exactly — the text
     * is within a third of an LSB of the value, so the reader rounds it home.
     * Checked over a hundred thousand values spread across the whole range.
     */
    for ( k = 0u; k < 100000u; ++k )
    {
        sweep = ( ( int64_t ) TEXT_TEST_MIN ) + ( ( int64_t ) k * 42949 );

        if ( ( textFromQ16 ( ( int32_t ) sweep, 5u, buffer, TEXT_Q16_MAX,
                             &written ) == FALSE ) ||
             ( textToQ16 ( buffer, written, &value ) == FALSE ) ||
             ( value != ( int32_t ) sweep ) )
        {
            ok = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "five places bring every value back exactly", ok );

    /* And four are not: one LSB is 0.0000153, which four places print as
       nothing at all. */
    ( void ) textFromQ16 ( 1, 4u, buffer, TEXT_Q16_MAX, &written );
    check ( "four places do not",
            ( uint8_t ) ( textToQ16 ( buffer, written, &value ) &&
                          ( value != 1 ) ) );
}

int main ( void )
{
    hexCase ( );
    printf ( "\n" );
    base64Case ( );
    printf ( "\n" );
    decimalCase ( );
    printf ( "\n" );
    q16Case ( );

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
