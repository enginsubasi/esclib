/*
 * Covers the parts of the filter module that MAF_Test and EMAF_Test do not
 * reach: both u32 variants and the argument checks. Those were exactly the
 * places the August 2026 findings lived in.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on the first failure.
 */

#include <stdio.h>

#include "maf.h"
#include "emaf.h"

#define WINDOW      64u
#define SAMPLES     200000u

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

/* ------------------------------------------------- emaf, unsigned 32-bit */

/*
 * The regression this file exists for. Holding the running value as uint32_t
 * truncated it every iteration, so the filter stopped moving once the
 * remaining difference fell below 1 / alpha and sat there for good. With
 * alpha at 0.01 it used to stop at 101 while chasing 200.
 */
static void emafu32DeadBandCase ( void )
{
    emafu32_t driver;
    uint32_t i = 0;
    uint32_t out = 0;

    printf ( "emafu32 dead band\n" );

    check ( "Init with alpha 0.01", emafInitu32 ( &driver, 0.01f, 100 ) );

    for ( i = 0; i < 5000u; ++i )
    {
        emafIterationu32 ( &driver, 200 );
    }

    out = emafGetOutputu32 ( &driver );
    printf ( "        output after 5000 samples of 200: %lu\n", ( unsigned long ) out );

    check ( "a small alpha still reaches the input", ( uint8_t ) ( out >= 199u ) );

    check ( "Init with alpha 1", emafInitu32 ( &driver, 1.0f, 0 ) );
    emafIterationu32 ( &driver, 12345 );
    check ( "alpha of 1 passes the sample straight through",
            ( uint8_t ) ( emafGetOutputu32 ( &driver ) == 12345u ) );

    check ( "Init with alpha 0.5", emafInitu32 ( &driver, 0.5f, 0 ) );
    emafIterationu32 ( &driver, 100 );
    check ( "alpha of 0.5 halves the step",
            ( uint8_t ) ( emafGetOutputu32 ( &driver ) == 50u ) );

    /*
     * Sharpest discriminator against the old behaviour. Chasing 1000 from 0
     * with alpha at 0.001, the first step lands on 1.0 and every later one
     * added less than a whole unit, so truncating into uint32_t every
     * iteration pinned the output at 1 forever. Keeping the fraction, 100
     * samples reach roughly 95.
     */
    check ( "Init with alpha 0.001", emafInitu32 ( &driver, 0.001f, 0 ) );

    for ( i = 0; i < 100u; ++i )
    {
        emafIterationu32 ( &driver, 1000 );
    }

    out = emafGetOutputu32 ( &driver );
    printf ( "        100 samples of 1000 with alpha 0.001: %lu\n", ( unsigned long ) out );

    check ( "sub unit steps accumulate instead of being dropped",
            ( uint8_t ) ( ( out > 90u ) && ( out < 100u ) ) );
}

/* ------------------------------------------------------ emaf, alpha range */

static void emafAlphaCase ( void )
{
    emaf_t driver;
    emafu32_t driveru32;

    printf ( "emaf alpha validation\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( emafInit ( NULL, 0.5f, 0 ) == FALSE ) );
    check ( "a negative alpha is rejected",
            ( uint8_t ) ( emafInit ( &driver, -0.1f, 0 ) == FALSE ) );
    check ( "an alpha above one is rejected",
            ( uint8_t ) ( emafInit ( &driver, 1.5f, 0 ) == FALSE ) );

    /* Both of these leave 1 - alpha equal to 1, so the filter could never
       respond while still reporting a successful init. */
    check ( "an alpha of zero is rejected",
            ( uint8_t ) ( emafInit ( &driver, 0.0f, 0 ) == FALSE ) );
    check ( "an alpha too small to survive float is rejected",
            ( uint8_t ) ( emafInit ( &driver, 1e-8f, 0 ) == FALSE ) );

    check ( "the smallest alpha that does survive is accepted",
            emafInit ( &driver, 1e-7f, 0 ) );
    check ( "an alpha of one is accepted", emafInit ( &driver, 1.0f, 0 ) );

    check ( "the u32 variant applies the same rule",
            ( uint8_t ) ( emafInitu32 ( &driveru32, 1e-8f, 0 ) == FALSE ) );
    check ( "and accepts the same values",
            emafInitu32 ( &driveru32, 1e-7f, 0 ) );
}

/* -------------------------------------------------- maf, unsigned 32-bit */

static void mafu32Case ( void )
{
    mafu32_t driver;
    uint32_t buffer[ 8 ];
    uint32_t i = 0;

    printf ( "mafu32\n" );

    check ( "Init", mafInitu32 ( &driver, buffer, 8u, 100u ) );
    check ( "a preloaded window reads back its own value",
            ( uint8_t ) ( mafGetOutputu32 ( &driver ) == 100u ) );

    for ( i = 0; i < 8u; ++i )
    {
        mafIterationu32 ( &driver, 200u );
    }

    check ( "a full window of a new value gives that value",
            ( uint8_t ) ( mafGetOutputu32 ( &driver ) == 200u ) );

    /* length * outputInit is computed into a uint32_t sum. */
    check ( "an outputInit that overflows the window sum is rejected",
            ( uint8_t ) ( mafInitu32 ( &driver, buffer, 1000u, 10000000u ) == FALSE ) );
    check ( "a preload that just fits is accepted",
            mafInitu32 ( &driver, buffer, 8u, 100000000u ) );
    check ( "a zero outputInit never overflows",
            mafInitu32 ( &driver, buffer, 8u, 0u ) );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( mafInitu32 ( NULL, buffer, 8u, 0u ) == FALSE ) );
    check ( "NULL buffer is rejected",
            ( uint8_t ) ( mafInitu32 ( &driver, NULL, 8u, 0u ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( mafInitu32 ( &driver, buffer, 0u, 0u ) == FALSE ) );
}

/* ------------------------------------------------------- maf, float drift */

/*
 * The float window sum is carried from call to call, which is not exact, so
 * mafIteration rebuilds it from the buffer on every wrap. After a wrap the
 * carried sum must therefore equal a fresh left to right sum of the same
 * buffer, bit for bit.
 */
static void mafDriftCase ( void )
{
    maf_t driver;
    float buffer[ WINDOW ];
    float trueSum = 0;
    uint32_t i = 0;
    uint32_t seed = 12345u;

    printf ( "maf float drift\n" );

    check ( "Init", mafInit ( &driver, buffer, WINDOW, 0.0f ) );

    /* A multiple of the window, so the last call was a wrap. */
    for ( i = 0; i < SAMPLES; ++i )
    {
        seed = ( seed * 1103515245u ) + 12345u;
        mafIteration ( &driver, ( float ) ( ( seed >> 16 ) & 0x3FFu ) );
    }

    check ( "the run ended on a wrap", ( uint8_t ) ( driver.index == 0u ) );

    for ( i = 0; i < WINDOW; ++i )
    {
        trueSum += buffer[ i ];
    }

    printf ( "        carried sum %.6f, rebuilt sum %.6f\n",
                ( double ) driver.sumOfArray, ( double ) trueSum );

    check ( "the carried sum matches a fresh one after 200000 samples",
            ( uint8_t ) ( driver.sumOfArray == trueSum ) );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( mafInit ( NULL, buffer, WINDOW, 0.0f ) == FALSE ) );
    check ( "NULL buffer is rejected",
            ( uint8_t ) ( mafInit ( &driver, NULL, WINDOW, 0.0f ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( mafInit ( &driver, buffer, 0u, 0.0f ) == FALSE ) );
}

int main ( void )
{
    emafu32DeadBandCase ( );
    printf ( "\n" );
    emafAlphaCase ( );
    printf ( "\n" );
    mafu32Case ( );
    printf ( "\n" );
    mafDriftCase ( );

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
