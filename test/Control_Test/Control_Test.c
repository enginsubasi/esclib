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

int main ( void )
{
    pidInitCase ( );
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
