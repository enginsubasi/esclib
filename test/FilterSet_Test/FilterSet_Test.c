/*
 * Covers the filters added alongside maf and emaf: median, biquad, slew,
 * alphabeta and deadband, plus the integer variant that went into emaf itself.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on the first failure. Expected values were
 * worked out from an IEEE binary32 model before the C was written.
 */

#include <math.h>
#include <stdio.h>

#include "median.h"
#include "biquad.h"
#include "slew.h"
#include "alphabeta.h"
#include "deadband.h"
#include "emaf.h"

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

static uint8_t near ( float got, float want, float tolerance )
{
    uint8_t retVal = FALSE;
    float difference = 0;

    difference = got - want;

    if ( difference < 0 )
    {
        difference = -difference;
    }
    else
    {
        /* Intentionally blank. */
    }

    if ( difference <= tolerance )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------------------ slew */

static void slewCase ( void )
{
    slew_t driver;
    slewi32_t driveri32;
    uint32_t i = 0;
    uint8_t ramped = TRUE;

    printf ( "slew\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( slewInit ( NULL, 10.0f, 0.0f ) == FALSE ) );
    check ( "a zero maxStep is rejected",
            ( uint8_t ) ( slewInit ( &driver, 0.0f, 0.0f ) == FALSE ) );
    check ( "a negative maxStep is rejected",
            ( uint8_t ) ( slewInit ( &driver, -1.0f, 0.0f ) == FALSE ) );

    check ( "Init", slewInit ( &driver, 10.0f, 0.0f ) );

    /* Ten steps of ten to cover a hundred, then it must stop dead on target. */
    for ( i = 1u; i <= 10u; ++i )
    {
        slewIteration ( &driver, 100.0f );

        if ( near ( slewGetOutput ( &driver ), ( float ) ( i * 10u ), 0.001f ) == FALSE )
        {
            ramped = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "the output climbs by exactly maxStep each call", ramped );

    slewIteration ( &driver, 100.0f );
    check ( "and holds once it arrives",
            near ( slewGetOutput ( &driver ), 100.0f, 0.001f ) );

    slewIteration ( &driver, 95.0f );
    check ( "a move smaller than maxStep is taken in one call",
            near ( slewGetOutput ( &driver ), 95.0f, 0.001f ) );

    slewIteration ( &driver, -1000.0f );
    check ( "the limit applies downwards too",
            near ( slewGetOutput ( &driver ), 85.0f, 0.001f ) );

    check ( "i32 Init", slewIniti32 ( &driveri32, 10, 0 ) );
    slewIterationi32 ( &driveri32, 100 );
    check ( "i32 steps by maxStep",
            ( uint8_t ) ( slewGetOutputi32 ( &driveri32 ) == 10 ) );

    /* newData - output would overflow here if it were computed directly. */
    check ( "i32 Init at the bottom of the range",
            slewIniti32 ( &driveri32, 10, INT32_MIN ) );
    slewIterationi32 ( &driveri32, INT32_MAX );
    check ( "the widest possible move does not overflow",
            ( uint8_t ) ( slewGetOutputi32 ( &driveri32 ) == ( INT32_MIN + 10 ) ) );

    check ( "i32 Init at the top of the range",
            slewIniti32 ( &driveri32, 10, INT32_MAX ) );
    slewIterationi32 ( &driveri32, INT32_MIN );
    check ( "and does not overflow the other way",
            ( uint8_t ) ( slewGetOutputi32 ( &driveri32 ) == ( INT32_MAX - 10 ) ) );
}

/* -------------------------------------------------------------- deadband */

static void deadbandCase ( void )
{
    deadband_t driver;
    deadbandi32_t driveri32;

    printf ( "deadband\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( deadbandInit ( NULL, 5.0f, DB_SNAP, 0.0f ) == FALSE ) );
    check ( "a negative threshold is rejected",
            ( uint8_t ) ( deadbandInit ( &driver, -1.0f, DB_SNAP, 0.0f ) == FALSE ) );
    check ( "an unknown mode is rejected",
            ( uint8_t ) ( deadbandInit ( &driver, 5.0f, 99u, 0.0f ) == FALSE ) );
    check ( "a zero threshold is allowed, it is just a wire",
            deadbandInit ( &driver, 0.0f, DB_SNAP, 0.0f ) );

    check ( "Init in DB_SNAP", deadbandInit ( &driver, 5.0f, DB_SNAP, 100.0f ) );

    deadbandIteration ( &driver, 103.0f );
    check ( "a move inside the band is ignored",
            near ( deadbandGetOutput ( &driver ), 100.0f, 0.001f ) );

    deadbandIteration ( &driver, 105.0f );
    check ( "a move exactly to the edge is still ignored",
            near ( deadbandGetOutput ( &driver ), 100.0f, 0.001f ) );

    deadbandIteration ( &driver, 106.0f );
    check ( "DB_SNAP jumps the output onto the input",
            near ( deadbandGetOutput ( &driver ), 106.0f, 0.001f ) );

    check ( "Init in DB_DRAG", deadbandInit ( &driver, 5.0f, DB_DRAG, 100.0f ) );

    deadbandIteration ( &driver, 106.0f );
    check ( "DB_DRAG leaves the output trailing by the threshold",
            near ( deadbandGetOutput ( &driver ), 101.0f, 0.001f ) );

    deadbandIteration ( &driver, 94.0f );
    check ( "and trails on the way down as well",
            near ( deadbandGetOutput ( &driver ), 99.0f, 0.001f ) );

    check ( "i32 Init at the top of the range",
            deadbandIniti32 ( &driveri32, 5, DB_SNAP, INT32_MAX ) );
    deadbandIterationi32 ( &driveri32, INT32_MAX );
    check ( "building the band edge does not overflow",
            ( uint8_t ) ( deadbandGetOutputi32 ( &driveri32 ) == INT32_MAX ) );

    check ( "i32 Init at the bottom of the range",
            deadbandIniti32 ( &driveri32, 5, DB_DRAG, INT32_MIN ) );
    deadbandIterationi32 ( &driveri32, 0 );
    check ( "DB_DRAG lands one threshold short",
            ( uint8_t ) ( deadbandGetOutputi32 ( &driveri32 ) == -5 ) );
}

/* ---------------------------------------------------------------- median */

static void medianCase ( void )
{
    median_t driver;
    float buffer[ 5 ];
    float sorted[ 5 ];
    medianu32_t driveru32;
    uint32_t bufferu32[ 5 ];
    uint32_t sortedu32[ 5 ];
    mediani32_t driveri32;
    int32_t bufferi32[ 3 ];
    int32_t sortedi32[ 3 ];
    uint32_t i = 0;
    uint8_t held = TRUE;

    printf ( "median\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( medianInit ( NULL, buffer, sorted, 5u, 0.0f ) == FALSE ) );
    check ( "NULL window is rejected",
            ( uint8_t ) ( medianInit ( &driver, NULL, sorted, 5u, 0.0f ) == FALSE ) );
    check ( "NULL scratch array is rejected",
            ( uint8_t ) ( medianInit ( &driver, buffer, NULL, 5u, 0.0f ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( medianInit ( &driver, buffer, sorted, 0u, 0.0f ) == FALSE ) );
    check ( "an even length is rejected",
            ( uint8_t ) ( medianInit ( &driver, buffer, sorted, 4u, 0.0f ) == FALSE ) );

    check ( "Init", medianInit ( &driver, buffer, sorted, 5u, 100.0f ) );
    check ( "a preloaded window reads back its own value",
            near ( medianGetOutput ( &driver ), 100.0f, 0.001f ) );

    /* The whole point: one wild sample must not reach the output at all. */
    medianIteration ( &driver, 1100.0f );
    check ( "a single spike is rejected outright",
            near ( medianGetOutput ( &driver ), 100.0f, 0.001f ) );

    for ( i = 0; i < 4u; ++i )
    {
        medianIteration ( &driver, 100.0f );

        if ( near ( medianGetOutput ( &driver ), 100.0f, 0.001f ) == FALSE )
        {
            held = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and never appears while it works its way out of the window", held );

    /* A real step has to get through once it owns the majority. */
    medianIteration ( &driver, 200.0f );
    medianIteration ( &driver, 200.0f );
    check ( "two of five samples are not yet a majority",
            near ( medianGetOutput ( &driver ), 100.0f, 0.001f ) );
    medianIteration ( &driver, 200.0f );
    check ( "three of five are, so a genuine step passes",
            near ( medianGetOutput ( &driver ), 200.0f, 0.001f ) );

    check ( "u32 Init", medianInitu32 ( &driveru32, bufferu32, sortedu32, 5u, 50u ) );
    medianIterationu32 ( &driveru32, 4000000000u );
    check ( "u32 rejects a spike near the top of the range",
            ( uint8_t ) ( medianGetOutputu32 ( &driveru32 ) == 50u ) );

    check ( "i32 Init", medianIniti32 ( &driveri32, bufferi32, sortedi32, 3u, 0 ) );
    medianIterationi32 ( &driveri32, -100 );
    check ( "i32 rejects a single negative spike",
            ( uint8_t ) ( medianGetOutputi32 ( &driveri32 ) == 0 ) );
    medianIterationi32 ( &driveri32, -100 );
    check ( "and accepts it once it is the majority",
            ( uint8_t ) ( medianGetOutputi32 ( &driveri32 ) == -100 ) );
}

/* ------------------------------------------------------------- alphabeta */

static void alphabetaCase ( void )
{
    alphabeta_t driver;
    uint32_t i = 0;
    float truth = 0;

    printf ( "alphabeta\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( alphabetaInit ( NULL, 0.5f, 0.1f, 0.01f, 0.0f ) == FALSE ) );
    check ( "a zero dt is rejected",
            ( uint8_t ) ( alphabetaInit ( &driver, 0.5f, 0.1f, 0.0f, 0.0f ) == FALSE ) );
    check ( "a zero alpha is rejected",
            ( uint8_t ) ( alphabetaInit ( &driver, 0.0f, 0.1f, 0.01f, 0.0f ) == FALSE ) );
    check ( "an alpha above one is rejected",
            ( uint8_t ) ( alphabetaInit ( &driver, 1.5f, 0.1f, 0.01f, 0.0f ) == FALSE ) );
    check ( "a zero beta is rejected",
            ( uint8_t ) ( alphabetaInit ( &driver, 0.5f, 0.0f, 0.01f, 0.0f ) == FALSE ) );
    check ( "a beta outside the stable region is rejected",
            ( uint8_t ) ( alphabetaInit ( &driver, 0.5f, 3.5f, 0.01f, 0.0f ) == FALSE ) );
    check ( "a beta on the stable boundary is accepted",
            alphabetaInit ( &driver, 0.5f, 3.0f, 0.01f, 0.0f ) );

    check ( "Init", alphabetaInit ( &driver, 0.5f, 0.1f, 0.01f, 0.0f ) );

    /* A ramp of 50 units per second, sampled every 10 ms, for six seconds. */
    for ( i = 0; i < 600u; ++i )
    {
        truth += 50.0f * 0.01f;
        alphabetaIteration ( &driver, truth );
    }

    check ( "a constant velocity ramp is tracked without steady state lag",
            near ( alphabetaGetPosition ( &driver ), truth, 0.05f ) );
    check ( "and the velocity is recovered",
            near ( alphabetaGetVelocity ( &driver ), 50.0f, 0.05f ) );
    check ( "the prediction runs one dt ahead of the position",
            near ( alphabetaGetPrediction ( &driver, 0.01f ),
                    alphabetaGetPosition ( &driver ) + 0.5f, 0.05f ) );
}

/* ---------------------------------------------------------------- biquad */

/* Largest absolute output over the given number of samples of a sine. */
static float sweep ( biquad_t* driver, float sampleRate, float frequency, uint32_t settle, uint32_t measure )
{
    uint32_t i = 0;
    float peak = 0;
    float value = 0;

    for ( i = 0; i < ( settle + measure ); ++i )
    {
        biquadIteration ( driver,
                            sinf ( ( 2.0f * 3.14159265f * frequency * ( float ) i ) / sampleRate ) );

        if ( i >= settle )
        {
            value = biquadGetOutput ( driver );

            if ( value < 0 )
            {
                value = -value;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( value > peak )
            {
                peak = value;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    return ( peak );
}

static void biquadCase ( void )
{
    biquad_t driver;
    uint32_t i = 0;
    uint8_t flat = TRUE;

    printf ( "biquad\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( biquadInit ( NULL, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f ) == FALSE ) );
    check ( "a cutoff at Nyquist is rejected",
            ( uint8_t ) ( biquadInitLowPass ( &driver, 1000.0f, 500.0f, 0.7071f ) == FALSE ) );
    check ( "a cutoff above Nyquist is rejected",
            ( uint8_t ) ( biquadInitLowPass ( &driver, 1000.0f, 600.0f, 0.7071f ) == FALSE ) );
    check ( "a zero sample rate is rejected",
            ( uint8_t ) ( biquadInitLowPass ( &driver, 0.0f, 100.0f, 0.7071f ) == FALSE ) );
    check ( "a zero q is rejected",
            ( uint8_t ) ( biquadInitLowPass ( &driver, 1000.0f, 100.0f, 0.0f ) == FALSE ) );

    /* Mains notch: 50 Hz has to vanish while 60 Hz walks through. */
    check ( "notch design", biquadInitNotch ( &driver, 1000.0f, 50.0f, 30.0f ) );
    check ( "the notch removes its own frequency",
            ( uint8_t ) ( sweep ( &driver, 1000.0f, 50.0f, 2000u, 200u ) < 0.05f ) );

    check ( "notch design again", biquadInitNotch ( &driver, 1000.0f, 50.0f, 30.0f ) );
    check ( "and passes a nearby one almost untouched",
            ( uint8_t ) ( sweep ( &driver, 1000.0f, 60.0f, 2000u, 200u ) > 0.9f ) );

    check ( "low pass design", biquadInitLowPass ( &driver, 1000.0f, 100.0f, 0.7071f ) );
    check ( "the low pass passes its passband",
            ( uint8_t ) ( sweep ( &driver, 1000.0f, 10.0f, 2000u, 400u ) > 0.95f ) );

    check ( "low pass design again", biquadInitLowPass ( &driver, 1000.0f, 100.0f, 0.7071f ) );
    check ( "and stops its stopband",
            ( uint8_t ) ( sweep ( &driver, 1000.0f, 400.0f, 2000u, 200u ) < 0.05f ) );

    check ( "high pass design", biquadInitHighPass ( &driver, 1000.0f, 100.0f, 0.7071f ) );
    check ( "the high pass blocks a slow signal",
            ( uint8_t ) ( sweep ( &driver, 1000.0f, 5.0f, 2000u, 800u ) < 0.05f ) );

    check ( "band pass design", biquadInitBandPass ( &driver, 1000.0f, 100.0f, 10.0f ) );
    check ( "the band pass keeps its centre",
            ( uint8_t ) ( sweep ( &driver, 1000.0f, 100.0f, 4000u, 200u ) > 0.9f ) );

    /* Reset has to leave a unity gain filter already settled. */
    check ( "low pass design for reset",
            biquadInitLowPass ( &driver, 1000.0f, 100.0f, 0.7071f ) );
    biquadReset ( &driver, 1000.0f );
    check ( "reset settles a low pass on its input",
            near ( biquadGetOutput ( &driver ), 1000.0f, 0.5f ) );

    for ( i = 0; i < 20u; ++i )
    {
        biquadIteration ( &driver, 1000.0f );

        if ( near ( biquadGetOutput ( &driver ), 1000.0f, 0.5f ) == FALSE )
        {
            flat = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and there is no startup transient at all", flat );

    /* A filter with no gain at dc must settle on zero, not on the input. */
    check ( "high pass design for reset",
            biquadInitHighPass ( &driver, 1000.0f, 100.0f, 0.7071f ) );
    biquadReset ( &driver, 1000.0f );
    check ( "reset settles a high pass on zero, since it has no dc gain",
            near ( biquadGetOutput ( &driver ), 0.0f, 0.001f ) );

    flat = TRUE;

    for ( i = 0; i < 20u; ++i )
    {
        biquadIteration ( &driver, 1000.0f );

        if ( near ( biquadGetOutput ( &driver ), 0.0f, 0.001f ) == FALSE )
        {
            flat = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and it stays there", flat );
}

/* ---------------------------------------------------- emaf, i32 variant */

static void emafi32Case ( void )
{
    emafi32_t driver;
    uint32_t i = 0;

    printf ( "emaf i32 variant\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( emafIniti32 ( NULL, 4u, 0 ) == FALSE ) );
    check ( "a zero shift is rejected",
            ( uint8_t ) ( emafIniti32 ( &driver, 0u, 0 ) == FALSE ) );
    check ( "a shift past 30 is rejected",
            ( uint8_t ) ( emafIniti32 ( &driver, 31u, 0 ) == FALSE ) );
    check ( "an outputInit too large for the shift is rejected",
            ( uint8_t ) ( emafIniti32 ( &driver, 8u, INT32_MAX ) == FALSE ) );
    check ( "an outputInit that fits is accepted",
            emafIniti32 ( &driver, 8u, ( INT32_MAX >> 8 ) ) );

    check ( "Init", emafIniti32 ( &driver, 4u, 0 ) );

    for ( i = 0; i < 4000u; ++i )
    {
        emafIterationi32 ( &driver, 1000 );
    }

    check ( "it settles exactly on the input, with no dead band",
            ( uint8_t ) ( emafGetOutputi32 ( &driver ) == 1000 ) );

    check ( "Init for the negative case", emafIniti32 ( &driver, 4u, 0 ) );

    for ( i = 0; i < 4000u; ++i )
    {
        emafIterationi32 ( &driver, -1000 );
    }

    check ( "and settles on a negative input too",
            ( uint8_t ) ( emafGetOutputi32 ( &driver ) == -1000 ) );

    check ( "Init to check the response speed", emafIniti32 ( &driver, 4u, 0 ) );

    for ( i = 0; i < 80u; ++i )
    {
        emafIterationi32 ( &driver, 1000 );
    }

    /* The float emaf with the same alpha reaches 994.3 after 80 samples. */
    check ( "80 samples with shift 4 land where the float filter does",
            ( uint8_t ) ( ( emafGetOutputi32 ( &driver ) >= 990 ) &&
                          ( emafGetOutputi32 ( &driver ) <= 998 ) ) );
}

int main ( void )
{
    slewCase ( );
    printf ( "\n" );
    deadbandCase ( );
    printf ( "\n" );
    medianCase ( );
    printf ( "\n" );
    alphabetaCase ( );
    printf ( "\n" );
    biquadCase ( );
    printf ( "\n" );
    emafi32Case ( );

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
