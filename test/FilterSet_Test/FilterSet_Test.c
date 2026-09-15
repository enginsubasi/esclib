/*
 * Covers the filters added alongside maf and emaf: median, biquad, slew,
 * alphabeta and deadband, plus the integer variants that went into emaf,
 * alphabeta and biquad themselves.
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
    slewu32_t driveru32;
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

    /*
     * The unsigned width, added 06/08/2026. It is not a transliteration of
     * the signed one: an unsigned value has no room below zero, so a
     * subtraction that would go there wraps to near UINT32_MAX instead of
     * going negative, and the downward saturation has to be written against
     * maxStep rather than against a floor constant.
     */
    check ( "u32 NULL driver is rejected",
            ( uint8_t ) ( slewInitu32 ( NULL, 10u, 0u ) == FALSE ) );
    check ( "u32 zero maxStep is rejected",
            ( uint8_t ) ( slewInitu32 ( &driveru32, 0u, 0u ) == FALSE ) );

    check ( "u32 Init", slewInitu32 ( &driveru32, 10u, 0u ) );
    slewIterationu32 ( &driveru32, 100u );
    check ( "u32 steps by maxStep",
            ( uint8_t ) ( slewGetOutputu32 ( &driveru32 ) == 10u ) );

    slewIterationu32 ( &driveru32, 12u );
    check ( "a move smaller than maxStep is taken in one call",
            ( uint8_t ) ( slewGetOutputu32 ( &driveru32 ) == 12u ) );

    /*
     * The pinned edge. Stepping down from below maxStep must land on zero.
     * Computing output - maxStep first would wrap to near UINT32_MAX and send
     * the output to the far end of the range instead of the near one.
     */
    check ( "u32 Init just above zero", slewInitu32 ( &driveru32, 10u, 5u ) );
    slewIterationu32 ( &driveru32, 0u );
    check ( "stepping below zero saturates at zero rather than wrapping",
            ( uint8_t ) ( slewGetOutputu32 ( &driveru32 ) == 0u ) );

    check ( "u32 Init at the top of the range",
            slewInitu32 ( &driveru32, 10u, 0xFFFFFFFFu ) );
    slewIterationu32 ( &driveru32, 0u );
    check ( "the widest possible move down does not wrap",
            ( uint8_t ) ( slewGetOutputu32 ( &driveru32 ) == ( 0xFFFFFFFFu - 10u ) ) );

    check ( "u32 Init at the bottom of the range",
            slewInitu32 ( &driveru32, 10u, 0u ) );
    slewIterationu32 ( &driveru32, 0xFFFFFFFFu );
    check ( "and the widest possible move up does not wrap either",
            ( uint8_t ) ( slewGetOutputu32 ( &driveru32 ) == 10u ) );

    check ( "u32 Init near the ceiling",
            slewInitu32 ( &driveru32, 10u, ( 0xFFFFFFFFu - 5u ) ) );
    slewIterationu32 ( &driveru32, 0xFFFFFFFFu );
    check ( "stepping past the ceiling saturates there",
            ( uint8_t ) ( slewGetOutputu32 ( &driveru32 ) == 0xFFFFFFFFu ) );
}

/* -------------------------------------------------------------- deadband */

static void deadbandCase ( void )
{
    deadband_t driver;
    deadbandi32_t driveri32;
    deadbandu32_t driveru32;

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

    /*
     * The unsigned width, added 06/08/2026. The lower band edge is where it
     * differs from the signed one: subtracting a threshold from an output
     * near zero wraps to near UINT32_MAX, which would put the lower edge
     * above the upper one and make the band read every sample as below it.
     */
    check ( "u32 NULL driver is rejected",
            ( uint8_t ) ( deadbandInitu32 ( NULL, 5u, DB_SNAP, 0u ) == FALSE ) );
    check ( "u32 zero threshold is allowed, the same as the other two widths",
            deadbandInitu32 ( &driveru32, 0u, DB_SNAP, 0u ) );
    check ( "u32 unknown mode is rejected",
            ( uint8_t ) ( deadbandInitu32 ( &driveru32, 5u, 7u, 0u ) == FALSE ) );

    check ( "u32 Init in DB_SNAP",
            deadbandInitu32 ( &driveru32, 5u, DB_SNAP, 100u ) );
    deadbandIterationu32 ( &driveru32, 103u );
    check ( "a move inside the band holds the output",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 100u ) );
    deadbandIterationu32 ( &driveru32, 106u );
    check ( "DB_SNAP jumps the output onto the input",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 106u ) );

    check ( "u32 Init in DB_DRAG",
            deadbandInitu32 ( &driveru32, 5u, DB_DRAG, 100u ) );
    deadbandIterationu32 ( &driveru32, 106u );
    check ( "DB_DRAG leaves the output trailing by the threshold",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 101u ) );
    deadbandIterationu32 ( &driveru32, 94u );
    check ( "and trails on the way down as well",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 99u ) );

    /*
     * The pinned edge. With the output at 2 and a threshold of 5 the lower
     * edge has to clamp to zero. Computed by subtraction it wraps to
     * 0xFFFFFFFD, which is above the upper edge, and every sample then reads
     * as below the band and snaps the output onto itself.
     */
    check ( "u32 Init just above zero",
            deadbandInitu32 ( &driveru32, 5u, DB_SNAP, 2u ) );
    deadbandIterationu32 ( &driveru32, 1u );
    check ( "a sample inside a band clipped at zero still holds",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 2u ) );
    deadbandIterationu32 ( &driveru32, 0u );
    check ( "and so does a sample at zero itself",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 2u ) );

    check ( "u32 Init at the top of the range",
            deadbandInitu32 ( &driveru32, 5u, DB_SNAP, 0xFFFFFFFFu ) );
    deadbandIterationu32 ( &driveru32, 0xFFFFFFFFu );
    check ( "building the upper band edge does not wrap",
            ( uint8_t ) ( deadbandGetOutputu32 ( &driveru32 ) == 0xFFFFFFFFu ) );
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

/*
 * The Q16 fixed point width, added 06/08/2026, for parts with no FPU. A
 * coefficient of 1.0 is 65536.
 *
 * It differs from the float variant in one place and the checks below pin it:
 * there is no dt. The velocity is carried in units per sample rather than per
 * second, which takes the period out of the update entirely — passing one as
 * a float would put back the arithmetic this variant exists to avoid.
 *
 * The first two iterations are hand computed. With both gains at 0.5 and the
 * filter starting at rest, a step to 100 puts the position at 50 and the
 * velocity at 50 per sample; the second sample of 100 then lands the
 * prediction exactly on the measurement, so the residual is zero and both
 * states hold.
 */
static void alphabetai32Case ( void )
{
    alphabetai32_t driver;
    uint32_t i = 0;
    int32_t truth = 0;
    uint8_t tracked = TRUE;

    printf ( "alphabeta fixed point\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( alphabetaIniti32 ( NULL, 32768, 32768, 0 ) == FALSE ) );
    check ( "a zero alpha is rejected",
            ( uint8_t ) ( alphabetaIniti32 ( &driver, 0, 32768, 0 ) == FALSE ) );
    check ( "an alpha above one is rejected",
            ( uint8_t ) ( alphabetaIniti32 ( &driver, 65537, 32768, 0 ) == FALSE ) );
    check ( "an alpha of exactly one is accepted",
            alphabetaIniti32 ( &driver, 65536, 32768, 0 ) );
    check ( "a zero beta is rejected",
            ( uint8_t ) ( alphabetaIniti32 ( &driver, 32768, 0, 0 ) == FALSE ) );

    /* The stability bound is 4 - 2 * alpha, so alpha of 0.5 allows beta of 3. */
    check ( "a beta on the stability bound is accepted",
            alphabetaIniti32 ( &driver, 32768, ( 3 * 65536 ), 0 ) );
    check ( "and one past it is rejected",
            ( uint8_t ) ( alphabetaIniti32 ( &driver, 32768, ( 3 * 65536 ) + 1, 0 ) == FALSE ) );

    check ( "Init", alphabetaIniti32 ( &driver, 32768, 32768, 0 ) );
    check ( "the position starts where it was put",
            ( uint8_t ) ( alphabetaGetPositioni32 ( &driver ) == 0 ) );
    check ( "and the velocity starts at zero",
            ( uint8_t ) ( alphabetaGetVelocityi32 ( &driver ) == 0 ) );

    alphabetaIterationi32 ( &driver, 100 );
    check ( "a step of 100 moves the position half way, by alpha",
            ( uint8_t ) ( alphabetaGetPositioni32 ( &driver ) == 50 ) );
    check ( "and gives it fifty units per sample of velocity, by beta",
            ( uint8_t ) ( alphabetaGetVelocityi32 ( &driver ) == ( 50 * 65536 ) ) );

    alphabetaIterationi32 ( &driver, 100 );
    check ( "the second sample lands the prediction on the measurement",
            ( uint8_t ) ( alphabetaGetPositioni32 ( &driver ) == 100 ) );
    check ( "so the residual is zero and the velocity holds",
            ( uint8_t ) ( alphabetaGetVelocityi32 ( &driver ) == ( 50 * 65536 ) ) );

    /*
     * The property the filter exists for: a constant velocity ramp is tracked
     * with no steady state lag, and the velocity estimate converges on the
     * true rate. A plain smoother lags a ramp for ever.
     */
    check ( "re-init", alphabetaIniti32 ( &driver, 32768, 13107, 0 ) );

    for ( i = 1u; i <= 200u; ++i )
    {
        truth = ( int32_t ) i * 10;
        alphabetaIterationi32 ( &driver, truth );
    }

    check ( "a constant velocity ramp is tracked without lag",
            ( uint8_t ) ( ( alphabetaGetPositioni32 ( &driver ) >= ( truth - 1 ) ) &&
                          ( alphabetaGetPositioni32 ( &driver ) <= ( truth + 1 ) ) ) );
    check ( "and the velocity converges on ten units per sample",
            ( uint8_t ) ( ( alphabetaGetVelocityi32 ( &driver ) > ( 9 * 65536 ) ) &&
                          ( alphabetaGetVelocityi32 ( &driver ) < ( 11 * 65536 ) ) ) );

    check ( "the prediction runs one sample ahead of the position",
            ( uint8_t ) ( ( alphabetaGetPredictioni32 ( &driver, 1 ) >=
                            ( alphabetaGetPositioni32 ( &driver ) + 9 ) ) &&
                          ( alphabetaGetPredictioni32 ( &driver, 1 ) <=
                            ( alphabetaGetPositioni32 ( &driver ) + 11 ) ) ) );
    check ( "and ten samples ahead is ten times as far",
            ( uint8_t ) ( ( alphabetaGetPredictioni32 ( &driver, 10 ) >=
                            ( alphabetaGetPositioni32 ( &driver ) + 95 ) ) &&
                          ( alphabetaGetPredictioni32 ( &driver, 10 ) <=
                            ( alphabetaGetPositioni32 ( &driver ) + 105 ) ) ) );

    /*
     * The pinned width check. The state is int64_t so Q16 does not cost
     * sixteen bits of usable range. An encoder position of a million would
     * overflow a Q16 int32_t, which holds about 32767 units.
     */
    check ( "Init at a position Q16 could not hold in thirty two bits",
            alphabetaIniti32 ( &driver, 65536, 32768, 1000000 ) );
    check ( "and it reads back intact",
            ( uint8_t ) ( alphabetaGetPositioni32 ( &driver ) == 1000000 ) );

    alphabetaIterationi32 ( &driver, 1000000 );
    check ( "and stays intact through an iteration",
            ( uint8_t ) ( alphabetaGetPositioni32 ( &driver ) == 1000000 ) );

    /* A ramp that walks well past the Q16 int32_t ceiling. */
    /*
     * Below zero, on both the starting position and the measurements. Until
     * 15/09/2026 nothing here went negative, and the Q16 scaling on both
     * paths was a left shift of a signed value — which is undefined in C the
     * moment that value is negative, unlike the right shift the module
     * documents and accepts.
     *
     * These checks do not catch that on their own: gcc shifts a negative left
     * exactly as one would expect, so the answers were right. What they do is
     * make the line reachable, and a sanitizer only reports what the tests
     * execute. Verified by putting the shift back — the plain build still
     * passes and the UBSan build traps.
     */
    check ( "Init below zero",
            alphabetaIniti32 ( &driver, 32768, 13107, -1000 ) );
    check ( "and the position reads back negative",
            ( uint8_t ) ( alphabetaGetPositioni32 ( &driver ) == -1000 ) );

    for ( i = 0; i < 200u; ++i )
    {
        alphabetaIterationi32 ( &driver, -2000 );
    }

    check ( "it tracks a negative measurement",
            ( uint8_t ) ( ( alphabetaGetPositioni32 ( &driver ) < -1900 ) &&
                          ( alphabetaGetPositioni32 ( &driver ) > -2100 ) ) );

    check ( "Init for a fall through zero",
            alphabetaIniti32 ( &driver, 32768, 13107, 500 ) );

    for ( i = 0; i < 300u; ++i )
    {
        alphabetaIterationi32 ( &driver, -500 );
    }

    check ( "and it follows a signal across zero",
            ( uint8_t ) ( ( alphabetaGetPositioni32 ( &driver ) < -450 ) &&
                          ( alphabetaGetPositioni32 ( &driver ) > -550 ) ) );

    check ( "re-init high", alphabetaIniti32 ( &driver, 32768, 13107, 1000000 ) );
    tracked = TRUE;

    for ( i = 1u; i <= 100u; ++i )
    {
        truth = 1000000 + ( ( int32_t ) i * 10 );
        alphabetaIterationi32 ( &driver, truth );

        if ( ( alphabetaGetPositioni32 ( &driver ) < ( truth - 100 ) ) ||
             ( alphabetaGetPositioni32 ( &driver ) > ( truth + 100 ) ) )
        {
            tracked = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "a ramp far above the Q16 thirty two bit ceiling still tracks",
            tracked );
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

/* ----------------------------------------------- biquad, Q16 i32 variant */

/*
 * Coefficient sets for a 1 kHz sample rate, taken from the float designers in
 * this module and converted to Q16 by hand, which is what a caller of the
 * fixed point width has to do. The conversion itself is checked below.
 */
#define BQ_LOWPASS      4421, 8841, 4421, -74906, 27053    /* 100 Hz, q 0.7071 */
#define BQ_NOTCH        65200, -124018, 65200, -124018, 64864 /* 50 Hz, q 30   */
#define BQ_HIGHPASS     41874, -83748, 41874, -74906, 27053 /* 100 Hz, q 0.7071 */
#define BQ_BANDPASS     1871, 0, -1871, -103012, 61794     /* 100 Hz, q 10     */

/* The conversion a caller applies to a designed coefficient. */
static int32_t toQ16 ( float value )
{
    int32_t retVal = 0;

    if ( value >= 0.0f )
    {
        retVal = ( int32_t ) ( ( value * 65536.0f ) + 0.5f );
    }
    else
    {
        retVal = ( int32_t ) ( ( value * 65536.0f ) - 0.5f );
    }

    return ( retVal );
}

/* Largest absolute output over the given number of samples of a sine. */
static int32_t sweepi32 ( biquadi32_t* driver, float sampleRate, float frequency,
                            uint32_t settle, uint32_t measure, float amplitude )
{
    uint32_t i = 0;
    int32_t peak = 0;
    int32_t value = 0;

    for ( i = 0; i < ( settle + measure ); ++i )
    {
        biquadIterationi32 ( driver,
                                ( int32_t ) ( amplitude *
                                    sinf ( ( 2.0f * 3.14159265f * frequency * ( float ) i ) / sampleRate ) ) );

        if ( i >= settle )
        {
            value = biquadGetOutputi32 ( driver );

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

static void biquadi32Case ( void )
{
    biquadi32_t driver;
    biquad_t designer;
    uint32_t i = 0;
    uint8_t flat = TRUE;
    int64_t sum = 0;

    printf ( "biquad i32 variant\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( biquadIniti32 ( NULL, 65536, 0, 0, 0, 0 ) == FALSE ) );
    check ( "Init", biquadIniti32 ( &driver, BQ_LOWPASS ) );

    /*
     * The float designer and the Q16 literals above have to describe the same
     * filter, or every expected value below is measuring the wrong thing.
     */
    check ( "the float low pass design converts to the Q16 set used here",
            biquadInitLowPass ( &designer, 1000.0f, 100.0f, 0.7071f ) );
    check ( "b0 converts",  ( uint8_t ) ( toQ16 ( designer.b0 ) == 4421 ) );
    check ( "b1 converts",  ( uint8_t ) ( toQ16 ( designer.b1 ) == 8841 ) );
    check ( "b2 converts",  ( uint8_t ) ( toQ16 ( designer.b2 ) == 4421 ) );
    check ( "a1 converts",  ( uint8_t ) ( toQ16 ( designer.a1 ) == -74906 ) );
    check ( "a2 converts",  ( uint8_t ) ( toQ16 ( designer.a2 ) == 27053 ) );

    /* Dc gain of one, in both signs and over four decades of amplitude. */
    check ( "Init for dc", biquadIniti32 ( &driver, BQ_LOWPASS ) );

    for ( i = 0; i < 5000u; ++i )
    {
        biquadIterationi32 ( &driver, 1000 );
    }

    check ( "a low pass settles exactly on a steady input",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 1000 ) );

    check ( "Init for a negative dc", biquadIniti32 ( &driver, BQ_LOWPASS ) );

    for ( i = 0; i < 5000u; ++i )
    {
        biquadIterationi32 ( &driver, -2500 );
    }

    check ( "and on a negative one, with no rounding offset",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == -2500 ) );

    check ( "Init for a large dc", biquadIniti32 ( &driver, BQ_LOWPASS ) );

    for ( i = 0; i < 5000u; ++i )
    {
        biquadIterationi32 ( &driver, 1000000 );
    }

    check ( "and on a million counts, which Q16 in an int32_t state could not hold",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 1000000 ) );

    /*
     * The reason biquadShifti32 rounds instead of shifting. A truncating shift
     * loses half an LSB every sample, which on a symmetric input shows up as a
     * standing offset: the same run with a plain >> sums to about -20000 here,
     * where the rounded form sums to zero. Checked as a sum rather than as a
     * single sample, because half a count never appears in one reading.
     */
    check ( "Init for the rounding check", biquadIniti32 ( &driver, BQ_LOWPASS ) );

    sum = 0;

    for ( i = 0; i < 44000u; ++i )
    {
        biquadIterationi32 ( &driver,
                                ( int32_t ) ( 10000.0f *
                                    sinf ( ( 2.0f * 3.14159265f * 10.0f * ( float ) i ) / 1000.0f ) ) );

        if ( i >= 4000u )
        {
            sum += biquadGetOutputi32 ( &driver );
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "a symmetric input leaves no standing offset",
            ( uint8_t ) ( ( sum > -400 ) && ( sum < 400 ) ) );

    /* Reset settles the state with no transient at all. */
    check ( "Init for reset", biquadIniti32 ( &driver, BQ_LOWPASS ) );
    biquadReseti32 ( &driver, 1000 );

    check ( "reset puts a low pass straight on its steady output",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 1000 ) );

    flat = TRUE;

    for ( i = 0; i < 200u; ++i )
    {
        biquadIterationi32 ( &driver, 1000 );

        if ( biquadGetOutputi32 ( &driver ) != 1000 )
        {
            flat = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and it stays there, with no settling at all", flat );

    check ( "Init a high pass", biquadIniti32 ( &driver, BQ_HIGHPASS ) );
    biquadReseti32 ( &driver, 1000 );

    check ( "reset settles a high pass on zero, because it has no dc gain",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 0 ) );

    flat = TRUE;

    for ( i = 0; i < 200u; ++i )
    {
        biquadIterationi32 ( &driver, 1000 );

        if ( biquadGetOutputi32 ( &driver ) != 0 )
        {
            flat = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and stays there too", flat );

    check ( "Init a band pass", biquadIniti32 ( &driver, BQ_BANDPASS ) );
    biquadReseti32 ( &driver, 5000 );
    check ( "a band pass settles on zero as well",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 0 ) );

    /*
     * A negative steady input. biquadReseti32 scales it into Q16, and that
     * scaling was a left shift of a signed value, which is undefined in C for
     * a negative one. A load cell reading below tare is the ordinary case that
     * reaches it — and reaching it is the point, since the answer was right
     * either way and only a sanitizer can see the difference.
     */
    check ( "Init for a negative reset", biquadIniti32 ( &driver, BQ_LOWPASS ) );
    biquadReseti32 ( &driver, -2500 );
    check ( "reset settles a low pass on a negative input",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == -2500 ) );

    flat = TRUE;

    for ( i = 0; i < 100u; ++i )
    {
        biquadIterationi32 ( &driver, -2500 );

        if ( biquadGetOutputi32 ( &driver ) != -2500 )
        {
            flat = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and it stays there", flat );

    check ( "Init a notch", biquadIniti32 ( &driver, BQ_NOTCH ) );
    biquadReseti32 ( &driver, 5000 );
    check ( "a notch settles on the input, because its dc gain is one",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 5000 ) );

    /*
     * Poles on the unit circle at dc make 1 + a1 + a2 zero, so there is no
     * settled value to compute and the state is cleared rather than divided.
     */
    check ( "Init a set with no dc gain at all",
            biquadIniti32 ( &driver, 65536, 0, 0, -65536, 0 ) );
    biquadReseti32 ( &driver, 1234 );
    check ( "a zero dc denominator clears rather than divides",
            ( uint8_t ) ( biquadGetOutputi32 ( &driver ) == 0 ) );

    /* The response itself, which is the whole point of the module. */
    check ( "Init the notch for its response", biquadIniti32 ( &driver, BQ_NOTCH ) );
    check ( "the notch cuts its own frequency by better than 50 dB",
            ( uint8_t ) ( sweepi32 ( &driver, 1000.0f, 50.0f, 4000u, 1000u, 10000.0f ) < 32 ) );

    check ( "re-init the notch", biquadIniti32 ( &driver, BQ_NOTCH ) );
    check ( "and passes a decade below it untouched",
            ( uint8_t ) ( sweepi32 ( &driver, 1000.0f, 10.0f, 4000u, 1000u, 10000.0f ) > 9900 ) );

    check ( "re-init the notch again", biquadIniti32 ( &driver, BQ_NOTCH ) );
    check ( "and two octaves above it almost untouched",
            ( uint8_t ) ( sweepi32 ( &driver, 1000.0f, 200.0f, 4000u, 1000u, 10000.0f ) > 9000 ) );

    check ( "Init the low pass for its response", biquadIniti32 ( &driver, BQ_LOWPASS ) );
    check ( "the low pass passes a decade below its corner",
            ( uint8_t ) ( sweepi32 ( &driver, 1000.0f, 10.0f, 4000u, 1000u, 10000.0f ) > 9900 ) );

    check ( "re-init the low pass", biquadIniti32 ( &driver, BQ_LOWPASS ) );

    i = ( uint32_t ) sweepi32 ( &driver, 1000.0f, 100.0f, 4000u, 1000u, 10000.0f );
    check ( "sits 3 dB down at the corner itself",
            ( uint8_t ) ( ( i > 6900u ) && ( i < 7250u ) ) );

    check ( "re-init the low pass again", biquadIniti32 ( &driver, BQ_LOWPASS ) );
    check ( "and is 39 dB down two octaves above it",
            ( uint8_t ) ( sweepi32 ( &driver, 1000.0f, 400.0f, 4000u, 1000u, 10000.0f ) < 200 ) );
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
    alphabetai32Case ( );
    printf ( "\n" );
    biquadCase ( );
    printf ( "\n" );
    biquadi32Case ( );
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
