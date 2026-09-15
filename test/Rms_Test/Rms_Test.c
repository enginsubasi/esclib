/*
 * Covers rms.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The integer expectations were computed in exact decimal arithmetic on a
 * host — the mean, the mean square and the variance from their definitions,
 * the roots to sixty digits, then rounded to Q16 — so they are the
 * mathematics' answers rather than this module's. The float ones are the same
 * definitions with a tolerance, since being within one is the float
 * variant's whole claim.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "rms.h"

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

#define RMS_TEST_MIN        ( -2147483647 - 1 )

static uint8_t near ( float got, float want, float tolerance )
{
    return ( ( uint8_t ) ( fabsf ( got - want ) <= tolerance ) );
}

static uint8_t nearQ ( uint32_t got, uint32_t want )
{
    uint32_t difference = ( got > want ) ? ( got - want ) : ( want - got );

    return ( ( uint8_t ) ( difference <= 1u ) );
}

/* Feeds a whole block of a pattern that repeats every period samples. */
static void feed ( rms_t* driver, const float* const pattern,
                   uint32_t period, uint32_t count )
{
    uint32_t i = 0u;

    for ( i = 0u; i < count; ++i )
    {
        rmsIteration ( driver, pattern[ i % period ] );
    }
}

static void feedi16 ( rmsi16_t* driver, const int16_t* const pattern,
                      uint32_t period, uint32_t count )
{
    uint32_t i = 0u;

    for ( i = 0u; i < count; ++i )
    {
        rmsIterationi16 ( driver, pattern[ i % period ] );
    }
}

/* -------------------------------------------------------------- float init */

static void floatInitCase ( void )
{
    rms_t driver;

    printf ( "rmsInit\n" );

    check ( "a NULL driver is refused",
            ( uint8_t ) ( rmsInit ( NULL, 8u ) == FALSE ) );
    check ( "a block of nothing is refused",
            ( uint8_t ) ( rmsInit ( &driver, 0u ) == FALSE ) );

    /* Poisoned first, so a field Init forgot reads as the pattern rather than
       as the zero the stack happened to hold. */
    memset ( &driver, 0x5A, sizeof ( driver ) );
    check ( "a block of eight", rmsInit ( &driver, 8u ) );
    check ( "nothing is ready before a block completes",
            ( uint8_t ) ( rmsIsReady ( &driver ) == FALSE ) );
    check ( "and every result reads zero",
            ( uint8_t ) ( ( rmsGetMean ( &driver ) == 0.0f ) &&
                          ( rmsGetRms ( &driver ) == 0.0f ) &&
                          ( rmsGetAcRms ( &driver ) == 0.0f ) ) );
}

/* ----------------------------------------------------------- float values */

static void floatValueCase ( void )
{
    rms_t driver;
    float dc [ 1 ] = { 5.0f };
    float square [ 2 ] = { 2148.0f, 1948.0f };
    float alternate [ 2 ] = { 1.0f, -1.0f };
    float sine [ 1000 ];
    uint32_t i = 0u;

    printf ( "float results\n" );

    ( void ) rmsInit ( &driver, 10u );
    feed ( &driver, dc, 1u, 9u );
    check ( "not ready one sample short of the block",
            ( uint8_t ) ( rmsIsReady ( &driver ) == FALSE ) );
    feed ( &driver, dc, 1u, 1u );
    check ( "ready on the sample that completes it", rmsIsReady ( &driver ) );
    check ( "a constant has itself as mean and RMS",
            ( uint8_t ) ( near ( rmsGetMean ( &driver ), 5.0f, 1e-5f ) &&
                          near ( rmsGetRms ( &driver ), 5.0f, 1e-5f ) ) );
    check ( "and no AC at all",
            near ( rmsGetAcRms ( &driver ), 0.0f, 1e-5f ) );

    /*
     * A square wave of 100 counts on a dc level of 2048, which is what a
     * converter biased to mid scale sees. The three results differ here and
     * each is checked: the dc is the bias, the AC is the signal, and the RMS
     * is both — the square root of 2048 squared plus 100 squared.
     */
    ( void ) rmsInit ( &driver, 8u );
    feed ( &driver, square, 2u, 8u );
    check ( "the mean is the dc level",
            near ( rmsGetMean ( &driver ), 2048.0f, 1e-3f ) );
    check ( "the AC RMS is the signal alone",
            near ( rmsGetAcRms ( &driver ), 100.0f, 1e-3f ) );
    check ( "and the RMS is both together",
            near ( rmsGetRms ( &driver ), 2050.43994f, 1e-2f ) );

    /* Seven whole cycles of a sine of amplitude 1000: its RMS is the
       amplitude over the root of two, exactly, over whole cycles. */
    for ( i = 0u; i < 1000u; ++i )
    {
        sine[ i ] = 1000.0f * sinf ( ( 2.0f * 3.14159265f * 7.0f *
                                       ( float ) i ) / 1000.0f );
    }

    ( void ) rmsInit ( &driver, 1000u );
    feed ( &driver, sine, 1000u, 1000u );
    check ( "a sine's RMS is its amplitude over the root of two",
            near ( rmsGetAcRms ( &driver ), 707.10678f, 0.1f ) );

    /*
     * Divided by the block length, not one less. The AC RMS describes this
     * block of this signal, and plus and minus one is exactly one; the sample
     * standard deviation would say 1.1547.
     */
    ( void ) rmsInit ( &driver, 4u );
    feed ( &driver, alternate, 2u, 4u );
    check ( "the divisor is the block length",
            near ( rmsGetAcRms ( &driver ), 1.0f, 1e-6f ) );
}

/* ---------------------------------------------------- float cancellation */

static void floatCancellationCase ( void )
{
    rms_t driver;
    float quiet [ 1 ] = { 0.0f };
    float small [ 2 ] = { 20001.0f, 19999.0f };

    printf ( "a small signal on a large offset\n" );

    /*
     * One count of signal on a dc level of 20000. The textbook form, the mean
     * square less the square of the mean, subtracts two numbers near four
     * hundred million to find one, and in single precision the one is gone:
     * measured, it answers wrong by a factor of hundreds. This is the case the
     * offset exists for.
     *
     * A quiet block comes first, at a different level, so the offset has to
     * be taken again for the second block rather than carried over from the
     * first — an offset taken once at Init would be zero here and the second
     * block would be the textbook form again.
     */
    ( void ) rmsInit ( &driver, 1000u );
    feed ( &driver, quiet, 1u, 1000u );
    ( void ) rmsIsReady ( &driver );
    feed ( &driver, small, 2u, 1000u );

    check ( "the dc is found",
            near ( rmsGetMean ( &driver ), 20000.0f, 1e-2f ) );
    check ( "and one count of AC is still one count",
            near ( rmsGetAcRms ( &driver ), 1.0f, 1e-3f ) );
}

/* ---------------------------------------------------------- float blocks */

static void floatBlockCase ( void )
{
    rms_t driver;
    float one [ 2 ] = { 1.0f, -1.0f };
    float three [ 2 ] = { 3.0f, -3.0f };

    printf ( "blocks and the ready flag\n" );

    ( void ) rmsInit ( &driver, 4u );

    /* Two blocks with no read between them. The result is the second one's,
       and the flag reports it once. */
    feed ( &driver, one, 2u, 4u );
    feed ( &driver, three, 2u, 4u );

    check ( "the flag is up", rmsIsReady ( &driver ) );
    check ( "and down again once read",
            ( uint8_t ) ( rmsIsReady ( &driver ) == FALSE ) );
    check ( "the result is the latest block's, not a mixture",
            near ( rmsGetAcRms ( &driver ), 3.0f, 1e-5f ) );

    /* The block boundary stays where it was: three samples are not a block,
       and the fourth is, however the reads fell. */
    feed ( &driver, one, 2u, 3u );
    check ( "three samples are not a block",
            ( uint8_t ) ( rmsIsReady ( &driver ) == FALSE ) );
    rmsIteration ( &driver, -1.0f );
    check ( "and the fourth completes it", rmsIsReady ( &driver ) );
    check ( "with the value of that block alone",
            near ( rmsGetAcRms ( &driver ), 1.0f, 1e-5f ) );
}

/* ------------------------------------------------------------ i16 init */

static void i16InitCase ( void )
{
    rmsi16_t driver;

    printf ( "rmsIniti16\n" );

    check ( "a NULL driver is refused",
            ( uint8_t ) ( rmsIniti16 ( NULL, 8u ) == FALSE ) );
    check ( "a block of nothing is refused",
            ( uint8_t ) ( rmsIniti16 ( &driver, 0u ) == FALSE ) );
    check ( "a block past the exact range is refused",
            ( uint8_t ) ( rmsIniti16 ( &driver, RMS_I16_BLOCK_MAX + 1u )
                          == FALSE ) );
    check ( "the longest block is accepted",
            rmsIniti16 ( &driver, RMS_I16_BLOCK_MAX ) );

    memset ( &driver, 0x5A, sizeof ( driver ) );
    check ( "a block of eight", rmsIniti16 ( &driver, 8u ) );
    check ( "nothing is ready before a block completes",
            ( uint8_t ) ( rmsIsReadyi16 ( &driver ) == FALSE ) );
    check ( "and every result reads zero",
            ( uint8_t ) ( ( rmsGetMeani16 ( &driver ) == 0 ) &&
                          ( rmsGetRmsi16 ( &driver ) == 0u ) &&
                          ( rmsGetAcRmsi16 ( &driver ) == 0u ) ) );
}

/* ---------------------------------------------------------- i16 values */

static void i16ValueCase ( void )
{
    rmsi16_t driver;
    int16_t bottom [ 1 ] = { -32768 };
    int16_t square [ 2 ] = { 2148, 1948 };
    int16_t pair [ 2 ] = { 1000, 1001 };
    int16_t third [ 3 ] = { 0, 0, 1 };
    int16_t twoThirds [ 3 ] = { 0, 1, 1 };
    int16_t minusTwoThirds [ 3 ] = { 0, -1, -1 };

    printf ( "integer results, in Q16\n" );

    /*
     * The bottom of the range held for a whole block. The mean is exactly the
     * most negative int32_t and the RMS exactly 2^31, which int32_t cannot
     * hold — the reason the RMS is unsigned.
     */
    ( void ) rmsIniti16 ( &driver, 8u );
    feedi16 ( &driver, bottom, 1u, 8u );
    check ( "ready on the sample that completes the block",
            rmsIsReadyi16 ( &driver ) );
    check ( "the bottom of the range is the most negative Q16 mean",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == RMS_TEST_MIN ) );
    check ( "and its RMS is two to the thirty one, unsigned",
            ( uint8_t ) ( rmsGetRmsi16 ( &driver ) == 2147483648u ) );
    check ( "with no AC", ( uint8_t ) ( rmsGetAcRmsi16 ( &driver ) == 0u ) );

    /* The float case's square wave, where the AC is exactly 100 counts. */
    ( void ) rmsIniti16 ( &driver, 8u );
    feedi16 ( &driver, square, 2u, 8u );
    check ( "the dc level",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == 134217728 ) );
    check ( "the AC, exactly, because its root is exact",
            ( uint8_t ) ( rmsGetAcRmsi16 ( &driver ) == 6553600u ) );
    check ( "and the RMS within one LSB",
            nearQ ( rmsGetRmsi16 ( &driver ), 134377633u ) );

    /*
     * Half a count of AC. An integer result would call this nothing, which is
     * why the results are Q16: noise is routinely smaller than one count.
     */
    ( void ) rmsIniti16 ( &driver, 2u );
    feedi16 ( &driver, pair, 2u, 2u );
    check ( "a mean between two counts",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == 65568768 ) );
    check ( "half a count of AC is half a count",
            ( uint8_t ) ( rmsGetAcRmsi16 ( &driver ) == 32768u ) );
    check ( "and the RMS within one LSB",
            nearQ ( rmsGetRmsi16 ( &driver ), 65568776u ) );

    /*
     * Three samples, one of them one. The AC is the root of two over three,
     * and the root of two has one bit to its name as an integer: without the
     * normalization the root comes out as one and the answer as 21845.
     */
    ( void ) rmsIniti16 ( &driver, 3u );
    feedi16 ( &driver, third, 3u, 3u );
    check ( "a root with no whole part is found to the LSB",
            nearQ ( rmsGetAcRmsi16 ( &driver ), 30894u ) );
    check ( "and so is the RMS",
            nearQ ( rmsGetRmsi16 ( &driver ), 37837u ) );
    check ( "a third, rounded down",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == 21845 ) );

    /* Two thirds rounds up, and minus two thirds rounds away from zero. */
    ( void ) rmsIniti16 ( &driver, 3u );
    feedi16 ( &driver, twoThirds, 3u, 3u );
    check ( "two thirds, rounded up",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == 43691 ) );
    feedi16 ( &driver, minusTwoThirds, 3u, 3u );
    check ( "and minus two thirds, rounded away from zero",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == -43691 ) );
}

/* ------------------------------------------------------------ i16 range */

static void i16RangeCase ( void )
{
    rmsi16_t driver;
    int16_t full [ 2 ] = { -32768, 32767 };
    int16_t one [ 2 ] = { 1, -1 };

    printf ( "the whole range for the longest block\n" );

    /*
     * The envelope the arithmetic is sized for, at its edge: 65536 samples
     * swinging across the full int16_t range. The sum of squares reaches
     * 2^46 here and the block length times it 2^62 — a sum kept in 32 bits
     * wraps thousands of times over, and the answer it gives is noise.
     */
    ( void ) rmsIniti16 ( &driver, RMS_I16_BLOCK_MAX );
    feedi16 ( &driver, full, 2u, RMS_I16_BLOCK_MAX );
    check ( "ready after the longest block", rmsIsReadyi16 ( &driver ) );
    check ( "the mean is minus a half",
            ( uint8_t ) ( rmsGetMeani16 ( &driver ) == -32768 ) );
    check ( "the AC RMS is 32767.5",
            nearQ ( rmsGetAcRmsi16 ( &driver ), 2147450880u ) );
    check ( "and so is the RMS, to the LSB",
            nearQ ( rmsGetRmsi16 ( &driver ), 2147450880u ) );

    /* And the next block starts clean. */
    ( void ) rmsIniti16 ( &driver, 4u );
    feedi16 ( &driver, full, 2u, 4u );
    ( void ) rmsIsReadyi16 ( &driver );
    feedi16 ( &driver, one, 2u, 4u );
    check ( "a second block is its own",
            ( uint8_t ) ( ( rmsGetAcRmsi16 ( &driver ) == 65536u ) &&
                          ( rmsGetMeani16 ( &driver ) == 0 ) ) );
    check ( "and the flag came up for it once",
            ( uint8_t ) ( ( rmsIsReadyi16 ( &driver ) == TRUE ) &&
                          ( rmsIsReadyi16 ( &driver ) == FALSE ) ) );
}

int main ( void )
{
    floatInitCase ( );
    printf ( "\n" );
    floatValueCase ( );
    printf ( "\n" );
    floatCancellationCase ( );
    printf ( "\n" );
    floatBlockCase ( );
    printf ( "\n" );
    i16InitCase ( );
    printf ( "\n" );
    i16ValueCase ( );
    printf ( "\n" );
    i16RangeCase ( );

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
