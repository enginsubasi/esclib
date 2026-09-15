/*
 * Covers pid and hysteresis.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure. PID_Test already exists and
 * prints; this one pins the parts that one does not reach, which are the four
 * separate limiters, the two Change functions and the argument checks.
 *
 * Every case drives one term at a time by zeroing the other two coefficients,
 * so a failure names the term rather than the sum.
 */

#include <stddef.h>
#include <stdio.h>

#include "pid.h"
#include "hysteresis.h"

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

static uint8_t nearly ( float got, float wanted )
{
    uint8_t retVal = FALSE;
    float diff = 0;

    diff = got - wanted;

    if ( diff < 0 )
    {
        diff = -diff;
    }

    if ( diff < 0.001f )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* A wide open init, so that only the term under test can move the output. */
static uint8_t openInit ( pidc_t* driver, float kp, float ki, float kd, float ts )
{
    return ( pidInit ( driver, kp, ki, kd, ts,
                       1000.0f, -1000.0f,
                       1000.0f, -1000.0f,
                       1000.0f, -1000.0f,
                       1000.0f, -1000.0f ) );
}

/*
 * Fills a driver with a non zero byte pattern, so that any field Init fails to
 * write shows up as a wrong number rather than as an accidental zero.
 *
 * 0x5A5A5A5A is a valid float of about 1.5e16, chosen over 0xFF so that the
 * failure is a wrong value rather than a nan — a nan compares false against
 * everything and could pass a check by accident, which is the whole problem
 * being guarded against here.
 */
static void poison ( void* driver, uint32_t size )
{
    uint8_t* bytes = ( uint8_t* ) driver;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        bytes[ i ] = 0x5Au;
    }
}

/* --------------------------------------------------------------- pidInit */

static void pidInitCase ( void )
{
    pidc_t driver;

    printf ( "pidInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( openInit ( NULL, 1.0f, 0.0f, 0.0f, 0.1f ) == FALSE ) );

    /*
     * A sampling time of zero is rejected because pidControl divides the
     * derivative term by it. The nan that would produce compares false against
     * both output bounds, so it would pass the limiter untouched and reach the
     * actuator.
     */
    check ( "a zero sampling time is rejected",
            ( uint8_t ) ( openInit ( &driver, 1.0f, 0.0f, 0.0f, 0.0f ) == FALSE ) );

    check ( "a valid init succeeds", openInit ( &driver, 1.0f, 0.0f, 0.0f, 0.1f ) );

    /* Init parks the output at the lower limit rather than leaving it unset. */
    check ( "and parks the output at the lower output limit",
            nearly ( pidGetOutput ( &driver ), -1000.0f ) );
}

/* ------------------------------------------------------- the three terms */

static void proportionalCase ( void )
{
    pidc_t driver;

    printf ( "pid proportional term\n" );

    check ( "Init", openInit ( &driver, 2.0f, 0.0f, 0.0f, 0.1f ) );

    pidControl ( &driver, 5.0f );
    check ( "output is kp times the error", nearly ( pidGetOutput ( &driver ), 10.0f ) );

    pidControl ( &driver, -5.0f );
    check ( "and follows the error negative",
            nearly ( pidGetOutput ( &driver ), -10.0f ) );

    /* The proportional limiter clamps the term, not the output. */
    check ( "Init with a proportional limit of 3",
            pidInit ( &driver, 2.0f, 0.0f, 0.0f, 0.1f,
                      3.0f, -3.0f, 1000.0f, -1000.0f,
                      1000.0f, -1000.0f, 1000.0f, -1000.0f ) );

    pidControl ( &driver, 5.0f );
    check ( "an error past the limit is clamped before kp is applied",
            nearly ( pidGetOutput ( &driver ), 6.0f ) );

    pidControl ( &driver, -5.0f );
    check ( "and on the negative side too",
            nearly ( pidGetOutput ( &driver ), -6.0f ) );
}

static void integralCase ( void )
{
    pidc_t driver;

    printf ( "pid integral term\n" );

    check ( "Init", openInit ( &driver, 0.0f, 1.0f, 0.0f, 0.5f ) );

    /* The integral accumulates error times the sampling time each call. */
    pidControl ( &driver, 2.0f );
    check ( "one step of integration", nearly ( pidGetOutput ( &driver ), 1.0f ) );

    pidControl ( &driver, 2.0f );
    check ( "two steps", nearly ( pidGetOutput ( &driver ), 2.0f ) );

    pidControl ( &driver, 2.0f );
    check ( "three steps", nearly ( pidGetOutput ( &driver ), 3.0f ) );

    /*
     * The integral limiter is what stops wind up. Without it the accumulator
     * keeps climbing while the actuator is already saturated, and the loop
     * cannot come back until it has unwound again.
     */
    check ( "Init with an integral limit of 2.5",
            pidInit ( &driver, 0.0f, 1.0f, 0.0f, 0.5f,
                      1000.0f, -1000.0f, 2.5f, -2.5f,
                      1000.0f, -1000.0f, 1000.0f, -1000.0f ) );

    pidControl ( &driver, 2.0f );
    pidControl ( &driver, 2.0f );
    pidControl ( &driver, 2.0f );
    check ( "the accumulator stops at the limit",
            nearly ( pidGetOutput ( &driver ), 2.5f ) );

    pidControl ( &driver, 2.0f );
    pidControl ( &driver, 2.0f );
    check ( "and stays there however long the error persists",
            nearly ( pidGetOutput ( &driver ), 2.5f ) );

    /* It must unwind again once the error reverses. */
    pidControl ( &driver, -2.0f );
    check ( "a reversed error brings it straight back down",
            nearly ( pidGetOutput ( &driver ), 1.5f ) );
}

static void derivativeCase ( void )
{
    pidc_t driver;

    printf ( "pid derivative term\n" );

    check ( "Init", openInit ( &driver, 0.0f, 0.0f, 1.0f, 0.5f ) );

    /*
     * The first call sees a step from the zero lastError that Init wrote. That
     * zero is deliberate: before it was set the first derivative term read
     * whatever was on the stack.
     */
    pidControl ( &driver, 1.0f );
    check ( "the first step is the error divided by the sampling time",
            nearly ( pidGetOutput ( &driver ), 2.0f ) );

    pidControl ( &driver, 1.0f );
    check ( "a steady error has no derivative",
            nearly ( pidGetOutput ( &driver ), 0.0f ) );

    pidControl ( &driver, 3.0f );
    check ( "a rising error gives a positive derivative",
            nearly ( pidGetOutput ( &driver ), 4.0f ) );

    pidControl ( &driver, 1.0f );
    check ( "a falling error gives a negative one",
            nearly ( pidGetOutput ( &driver ), -4.0f ) );

    check ( "Init with a derivative limit of 3",
            pidInit ( &driver, 0.0f, 0.0f, 1.0f, 0.5f,
                      1000.0f, -1000.0f, 1000.0f, -1000.0f,
                      3.0f, -3.0f, 1000.0f, -1000.0f ) );

    pidControl ( &driver, 5.0f );
    check ( "a step past the derivative limit is clamped",
            nearly ( pidGetOutput ( &driver ), 3.0f ) );
}

static void outputLimitCase ( void )
{
    pidc_t driver;

    printf ( "pid output limiter\n" );

    check ( "Init with an output limit of 6",
            pidInit ( &driver, 2.0f, 0.0f, 0.0f, 0.1f,
                      1000.0f, -1000.0f, 1000.0f, -1000.0f,
                      1000.0f, -1000.0f, 6.0f, -6.0f ) );

    pidControl ( &driver, 5.0f );
    check ( "the sum is clamped at the top", nearly ( pidGetOutput ( &driver ), 6.0f ) );

    pidControl ( &driver, -5.0f );
    check ( "and at the bottom", nearly ( pidGetOutput ( &driver ), -6.0f ) );

    pidControl ( &driver, 1.0f );
    check ( "a value inside the band passes through",
            nearly ( pidGetOutput ( &driver ), 2.0f ) );
}

/* ------------------------------------------------------ the two Changes */

static void pidChangeCase ( void )
{
    pidc_t driver;

    printf ( "pidChangeCoefficients and pidChangeLimits\n" );

    check ( "Init", openInit ( &driver, 2.0f, 0.0f, 0.0f, 0.1f ) );

    pidControl ( &driver, 5.0f );
    check ( "the original gain applies", nearly ( pidGetOutput ( &driver ), 10.0f ) );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( pidChangeCoefficients ( NULL, 4.0f, 0.0f, 0.0f, 0.1f ) == FALSE ) );

    /*
     * This one returns a status for the same reason Init does: it can install a
     * zero sampling time, and pidControl divides by it.
     */
    check ( "a zero sampling time is rejected",
            ( uint8_t ) ( pidChangeCoefficients ( &driver, 4.0f, 0.0f, 0.0f, 0.0f ) == FALSE ) );

    pidControl ( &driver, 5.0f );
    check ( "and the rejected change left the driver alone",
            nearly ( pidGetOutput ( &driver ), 10.0f ) );

    check ( "a valid change is accepted",
            pidChangeCoefficients ( &driver, 4.0f, 0.0f, 0.0f, 0.1f ) );

    pidControl ( &driver, 5.0f );
    check ( "and the new gain takes effect",
            nearly ( pidGetOutput ( &driver ), 20.0f ) );

    check ( "pidChangeLimits rejects a NULL driver",
            ( uint8_t ) ( pidChangeLimits ( NULL,
                                            1000.0f, -1000.0f, 1000.0f, -1000.0f,
                                            1000.0f, -1000.0f, 15.0f, -15.0f ) == FALSE ) );

    check ( "and accepts a valid call",
            pidChangeLimits ( &driver,
                              1000.0f, -1000.0f, 1000.0f, -1000.0f,
                              1000.0f, -1000.0f, 15.0f, -15.0f ) );

    pidControl ( &driver, 5.0f );
    check ( "the new output limit takes effect",
            nearly ( pidGetOutput ( &driver ), 15.0f ) );
}

/* ----------------------------------------------------------- hysteresis */

static void hysteresisCase ( void )
{
    hysteresis_t driver;

    printf ( "hysteresis\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( hysteresisInit ( NULL, 10.0f, 5.0f ) == FALSE ) );

    /* An upper threshold below the lower one has no band to sit in. */
    check ( "an inverted band is rejected",
            ( uint8_t ) ( hysteresisInit ( &driver, 5.0f, 10.0f ) == FALSE ) );
    check ( "a zero width band is allowed",
            hysteresisInit ( &driver, 5.0f, 5.0f ) );

    check ( "Init", hysteresisInit ( &driver, 10.0f, 5.0f ) );
    check ( "the output starts cleared",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == FALSE ) );

    /* Inside the band nothing changes, which is the whole point. */
    hysteresisControl ( &driver, 7.0f );
    check ( "a value inside the band leaves a cleared output cleared",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == FALSE ) );

    /* Both thresholds are strict, so landing exactly on one changes nothing. */
    hysteresisControl ( &driver, 10.0f );
    check ( "exactly at the upper threshold is not above it",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == FALSE ) );

    hysteresisControl ( &driver, 11.0f );
    check ( "above the upper threshold sets the output",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == TRUE ) );

    hysteresisControl ( &driver, 7.0f );
    check ( "and back inside the band it stays set",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == TRUE ) );

    hysteresisControl ( &driver, 5.0f );
    check ( "exactly at the lower threshold is not below it",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == TRUE ) );

    hysteresisControl ( &driver, 4.0f );
    check ( "below the lower threshold clears it",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == FALSE ) );

    hysteresisControl ( &driver, 9.99f );
    check ( "and a value just short of the upper threshold does not set it again",
            ( uint8_t ) ( hysteresisGetOutput ( &driver ) == FALSE ) );
}

/*
 * The integer widths, added 06/08/2026. hysteresisControl is two comparisons
 * and no arithmetic at all, so these carry no edge case the float variant
 * does not — which is exactly why the checks below are about the band and the
 * strictness of the thresholds rather than about overflow.
 *
 * The two ends of each range are worth one check each anyway, because a
 * threshold sitting on INT32_MIN or on zero is where a comparison written the
 * wrong way round would still look reasonable.
 */
static void hysteresisWidthCase ( void )
{
    hysteresisi32_t driveri32;
    hysteresisu32_t driveru32;

    printf ( "hysteresis integer widths\n" );

    check ( "i32 NULL driver is rejected",
            ( uint8_t ) ( hysteresisIniti32 ( NULL, 10, 5 ) == FALSE ) );
    check ( "i32 an inverted pair is rejected",
            ( uint8_t ) ( hysteresisIniti32 ( &driveri32, 5, 10 ) == FALSE ) );
    check ( "i32 equal thresholds are accepted, that is a comparator",
            hysteresisIniti32 ( &driveri32, 5, 5 ) );

    check ( "i32 Init", hysteresisIniti32 ( &driveri32, 10, -10 ) );
    check ( "the output starts cleared",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == FALSE ) );

    hysteresisControli32 ( &driveri32, 0 );
    check ( "a value inside a band spanning zero changes nothing",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == FALSE ) );

    hysteresisControli32 ( &driveri32, 10 );
    check ( "exactly at the upper threshold is not above it",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == FALSE ) );

    hysteresisControli32 ( &driveri32, 11 );
    check ( "above it sets the output",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == TRUE ) );

    hysteresisControli32 ( &driveri32, -10 );
    check ( "exactly at the lower threshold is not below it",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == TRUE ) );

    hysteresisControli32 ( &driveri32, -11 );
    check ( "below it clears the output",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == FALSE ) );

    /* A band pinned at the bottom of the range. */
    check ( "i32 Init at the bottom of the range",
            hysteresisIniti32 ( &driveri32, INT32_MIN + 1, INT32_MIN ) );
    hysteresisControli32 ( &driveri32, INT32_MAX );
    check ( "the widest possible input sets it",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == TRUE ) );
    hysteresisControli32 ( &driveri32, INT32_MIN );
    check ( "and the lowest possible input is not below the floor threshold",
            ( uint8_t ) ( hysteresisGetOutputi32 ( &driveri32 ) == TRUE ) );

    check ( "u32 NULL driver is rejected",
            ( uint8_t ) ( hysteresisInitu32 ( NULL, 10u, 5u ) == FALSE ) );
    check ( "u32 an inverted pair is rejected",
            ( uint8_t ) ( hysteresisInitu32 ( &driveru32, 5u, 10u ) == FALSE ) );

    check ( "u32 Init", hysteresisInitu32 ( &driveru32, 200u, 100u ) );
    hysteresisControlu32 ( &driveru32, 150u );
    check ( "a value inside the band changes nothing",
            ( uint8_t ) ( hysteresisGetOutputu32 ( &driveru32 ) == FALSE ) );
    hysteresisControlu32 ( &driveru32, 201u );
    check ( "above the upper threshold sets it",
            ( uint8_t ) ( hysteresisGetOutputu32 ( &driveru32 ) == TRUE ) );
    hysteresisControlu32 ( &driveru32, 99u );
    check ( "below the lower threshold clears it",
            ( uint8_t ) ( hysteresisGetOutputu32 ( &driveru32 ) == FALSE ) );

    /*
     * A lower threshold of zero can never be crossed, because no unsigned
     * input sits below it. That makes the output latch on for good, which is
     * a use rather than a mistake, and Init accepts it.
     */
    check ( "u32 Init with a lower threshold of zero",
            hysteresisInitu32 ( &driveru32, 100u, 0u ) );
    hysteresisControlu32 ( &driveru32, 101u );
    check ( "it sets on the way up",
            ( uint8_t ) ( hysteresisGetOutputu32 ( &driveru32 ) == TRUE ) );
    hysteresisControlu32 ( &driveru32, 0u );
    check ( "and zero itself cannot clear it, so the output latches",
            ( uint8_t ) ( hysteresisGetOutputu32 ( &driveru32 ) == TRUE ) );

    check ( "u32 Init at the top of the range",
            hysteresisInitu32 ( &driveru32, 0xFFFFFFFFu, 0xFFFFFFFEu ) );
    hysteresisControlu32 ( &driveru32, 0xFFFFFFFFu );
    check ( "the largest input is not above the largest threshold",
            ( uint8_t ) ( hysteresisGetOutputu32 ( &driveru32 ) == FALSE ) );
}

/* ------------------------------------------------------- pid, Q16 fixed */

/*
 * The fixed point width, added 06/08/2026, for parts with no FPU. A gain of
 * 1.0 is 65536.
 *
 * It differs from the float variant in one place: there is no ts. The
 * integral is a running sum of errors and the derivative a plain difference,
 * so the caller folds the period into the gains once when converting from
 * continuous ones. Every expected value below is therefore hand computable as
 * gain times term, with no period anywhere.
 *
 * Two things only this width can get wrong, and both are checked: the
 * integral accumulator is int64_t, because a running sum of errors is exactly
 * what an int32_t loses; and each gain product is formed in int64_t before it
 * is shifted back down, because a Q16 gain against a realistic error passes
 * thirty two bits immediately.
 */
static void pidFixedPointCase ( void )
{
    pidci32_t driver;
    uint32_t i = 0;

    printf ( "pid fixed point\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( pidIniti32 ( NULL, 65536, 0, 0,
                                       10000, -10000, 10000, -10000,
                                       10000, -10000, 10000, -10000 ) == FALSE ) );

    /*
     * There is no ts to reject, so a well formed driver pointer is the only
     * thing Init can refuse. The float variant checks a ts of zero because
     * pidControl divides by it; nothing here divides.
     */
    check ( "proportional only: kp of 2.0",
            pidIniti32 ( &driver, ( 2 * 65536 ), 0, 0,
                         10000, -10000, 10000, -10000,
                         10000, -10000, 100000, -100000 ) );

    pidControli32 ( &driver, 10 );
    check ( "the output is the gain times the error",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 20 ) );

    pidControli32 ( &driver, -10 );
    check ( "and it follows the error negative",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == -20 ) );

    /* A fractional gain is the reason for Q16 in the first place. */
    check ( "kp of 0.5",
            pidIniti32 ( &driver, 32768, 0, 0,
                         10000, -10000, 10000, -10000,
                         10000, -10000, 100000, -100000 ) );
    pidControli32 ( &driver, 10 );
    check ( "half of the error survives the fixed point",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 5 ) );

    /* The proportional term is limited before the gain is applied. */
    check ( "kp of 2.0 with the p term limited to 3",
            pidIniti32 ( &driver, ( 2 * 65536 ), 0, 0,
                         3, -3, 10000, -10000,
                         10000, -10000, 100000, -100000 ) );
    pidControli32 ( &driver, 100 );
    check ( "the limit binds on the term, not on the output",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 6 ) );

    /* Integral. One call is one period, so the accumulator is a plain sum. */
    check ( "integral only: ki of 0.5",
            pidIniti32 ( &driver, 0, 32768, 0,
                         10000, -10000, 10000, -10000,
                         10000, -10000, 100000, -100000 ) );

    pidControli32 ( &driver, 10 );
    check ( "one sample integrates one error",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 5 ) );
    pidControli32 ( &driver, 10 );
    check ( "two samples integrate two",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 10 ) );
    pidControli32 ( &driver, 10 );
    check ( "three samples integrate three",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 15 ) );

    /*
     * The anti windup bound. The accumulator is clamped rather than the term
     * after its gain, so it cannot run away and the controller recovers as
     * soon as the error changes sign. Clamping the term instead would let the
     * accumulator keep climbing behind the limit and stall the recovery.
     */
    check ( "ki of 1.0 with the accumulator limited to 50",
            pidIniti32 ( &driver, 0, 65536, 0,
                         10000, -10000, 50, -50,
                         10000, -10000, 100000, -100000 ) );

    for ( i = 0; i < 100u; ++i )
    {
        pidControli32 ( &driver, 10 );
    }

    check ( "a long saturating run stops at the accumulator limit",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 50 ) );

    pidControli32 ( &driver, -10 );
    check ( "and one sample of the other sign moves it straight away",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 40 ) );

    /* Derivative. Again one call is one period, so it is a difference. */
    check ( "derivative only: kd of 2.0",
            pidIniti32 ( &driver, 0, 0, ( 2 * 65536 ),
                         10000, -10000, 10000, -10000,
                         10000, -10000, 100000, -100000 ) );

    pidControli32 ( &driver, 10 );
    check ( "the first step is the whole error, since lastError starts cleared",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 20 ) );
    pidControli32 ( &driver, 10 );
    check ( "a steady error differentiates to nothing",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 0 ) );
    pidControli32 ( &driver, 15 );
    check ( "and a step of five gives twice that",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 10 ) );

    /* The output limiter is separate from the three term limiters. */
    check ( "kp of 10 with the output limited to 25",
            pidIniti32 ( &driver, ( 10 * 65536 ), 0, 0,
                         10000, -10000, 10000, -10000,
                         10000, -10000, 25, -25 ) );
    pidControli32 ( &driver, 100 );
    check ( "the output limit binds",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 25 ) );
    pidControli32 ( &driver, -100 );
    check ( "in both directions",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == -25 ) );

    /*
     * The pinned width check. A gain of ten against an error of a hundred
     * thousand is a Q16 product of 6.5e10, which is past thirty two bits. In
     * int32_t it wraps and the sign of the output flips.
     */
    check ( "kp of 10 with room for a large output",
            pidIniti32 ( &driver, ( 10 * 65536 ), 0, 0,
                         1000000, -1000000, 10, -10,
                         10, -10, 2000000, -2000000 ) );
    pidControli32 ( &driver, 100000 );
    check ( "a Q16 product past thirty two bits is not truncated",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 1000000 ) );

    /*
     * The other pinned width check, and it is narrower than it first looks.
     * The clamp runs every call and iPartMaxLimit is an int32_t, so the
     * accumulator can never end a call outside int32 range. What it can do is
     * leave that range between the addition and the clamp: with the limit at
     * the very top of int32, one more error overflows before anything bounds
     * it. An int32_t accumulator wraps to a large negative there and the
     * clamp then pins it to iMin instead of iMax, so the output comes back
     * with the wrong sign entirely.
     */
    check ( "ki of 1.0 with the accumulator limit at the top of int32",
            pidIniti32 ( &driver, 0, 65536, 0,
                         10, -10, 2147483647, ( -2147483647 - 1 ),
                         10, -10, 2000000000, -2000000000 ) );

    for ( i = 0; i < 30000u; ++i )
    {
        pidControli32 ( &driver, 100000 );
    }

    check ( "the accumulator saturates at its limit rather than wrapping",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 2000000000 ) );
    check ( "and it is still positive, so it never went round the bottom",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) > 0 ) );

    /* The two Change functions. */
    check ( "changing coefficients on a NULL driver is refused",
            ( uint8_t ) ( pidChangeCoefficientsi32 ( NULL, 0, 0, 0 ) == FALSE ) );
    check ( "changing limits on a NULL driver is refused",
            ( uint8_t ) ( pidChangeLimitsi32 ( NULL, 0, 0, 0, 0, 0, 0, 0, 0 ) == FALSE ) );

    check ( "re-init proportional only",
            pidIniti32 ( &driver, 65536, 0, 0,
                         10000, -10000, 10000, -10000,
                         10000, -10000, 100000, -100000 ) );
    pidControli32 ( &driver, 10 );
    check ( "a gain of one passes the error through",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 10 ) );

    check ( "the gain is changed to 3.0",
            pidChangeCoefficientsi32 ( &driver, ( 3 * 65536 ), 0, 0 ) );
    pidControli32 ( &driver, 10 );
    check ( "and the next iteration uses it",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 30 ) );

    check ( "the output limit is tightened to 12",
            pidChangeLimitsi32 ( &driver, 10000, -10000, 10000, -10000,
                                 10000, -10000, 12, -12 ) );
    pidControli32 ( &driver, 10 );
    check ( "and it binds on the next iteration",
            ( uint8_t ) ( pidGetOutputi32 ( &driver ) == 12 ) );
}

/*
 * pidInit has to write every field it promises to, and a stack local driver
 * cannot show that on its own: the memory is usually zero already, or holds
 * the previous test's values, so a missing assignment reads as if it had
 * happened. Until 15/09/2026 that is exactly what the checks below relied on —
 * a mutation that deleted the whole initial state block from pidInit passed
 * the entire suite. Poisoning first is what makes the assignment observable.
 *
 * The defect being pinned is real and was fixed in July 2026: pidInit left
 * error, lastError, partP, partI and partD unset, so the first derivative term
 * differentiated against whatever was on the stack and the integrator started
 * from it.
 */
static void pidInitClearsStateCase ( void )
{
    pidc_t driver;

    printf ( "pidInit clears its state\n" );

    poison ( &driver, ( uint32_t ) sizeof ( driver ) );

    check ( "Init over a poisoned driver",
            openInit ( &driver, 0.0f, 0.0f, 1.0f, 0.5f ) );

    /*
     * Derivative only, so the output is ( error - lastError ) / ts. With
     * lastError cleared that is 1.0 / 0.5, which is 2. With the poison left in
     * place it is about minus three times ten to the sixteen.
     */
    pidControl ( &driver, 1.0f );
    check ( "the first derivative differentiates against a cleared lastError",
            nearly ( pidGetOutput ( &driver ), 2.0f ) );

    /* Integral only, so the output is the running sum of error times ts. */
    poison ( &driver, ( uint32_t ) sizeof ( driver ) );

    check ( "Init over a poisoned driver again",
            openInit ( &driver, 0.0f, 1.0f, 0.0f, 0.5f ) );

    pidControl ( &driver, 2.0f );
    check ( "and the integrator starts from a cleared partI",
            nearly ( pidGetOutput ( &driver ), 1.0f ) );

    /* Proportional only, which reads no memory at all and so must be exact. */
    poison ( &driver, ( uint32_t ) sizeof ( driver ) );

    check ( "Init over a poisoned driver once more",
            openInit ( &driver, 3.0f, 0.0f, 0.0f, 0.5f ) );

    pidControl ( &driver, 2.0f );
    check ( "and the proportional term is untouched by what was there",
            nearly ( pidGetOutput ( &driver ), 6.0f ) );
}

int main ( void )
{
    pidInitCase ( );
    printf ( "\n" );
    pidInitClearsStateCase ( );
    printf ( "\n" );
    proportionalCase ( );
    printf ( "\n" );
    integralCase ( );
    printf ( "\n" );
    derivativeCase ( );
    printf ( "\n" );
    outputLimitCase ( );
    printf ( "\n" );
    pidChangeCase ( );
    printf ( "\n" );
    hysteresisCase ( );
    printf ( "\n" );
    hysteresisWidthCase ( );
    printf ( "\n" );
    pidFixedPointCase ( );

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
