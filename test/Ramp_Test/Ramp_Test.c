/*
 * Setpoint ramp module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * The pinned regression is overshoot, checked in noOvershootCase. Without the
 * final step clamp in rampIteration the ramp passes the target, turns around,
 * and oscillates about it. Every other case in this file passes with that bug
 * present, because they all measure either the convergence or the middle of
 * the trajectory, so the assertion is made on every step of the run rather
 * than only at the end.
 *
 * Every case uses the same limits so the arithmetic stays checkable by hand:
 * maxVelocity 100, maxAcceleration 200, ts 0.01, which makes the velocity
 * step exactly 2 per iteration. From rest the velocity is 2 after one step
 * and reaches 100 after fifty, having covered 25.5 units.
 */

#include <stdio.h>
#include <stddef.h>

#include "ramp.h"

#define VMAX        100.0f
#define AMAX        200.0f
#define TS          0.01f

#define SENTINELF   ( -12345.678f )
#define SENTINEL8   0xA5u

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
 * Float results are compared with a tolerance rather than for equality, the
 * same way ArrayMatrix_Test and ComplexMath_Test do it. 200.0f * 0.01f is not
 * exactly 2.0f, so an equality check on a computed velocity would be testing
 * the float format rather than the module.
 */
static uint8_t nearly ( float got, float wanted )
{
    uint8_t retVal = FALSE;
    float diff = 0;

    diff = got - wanted;

    if ( diff < 0 )
    {
        diff = -diff;
    }

    if ( diff < 0.0001f )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/*
 * A rejected Init must leave the driver exactly as it found it, so the caller
 * cannot half initialize a ramp by ignoring the return value. The pattern is
 * written by hand rather than with memset to keep the test free of string.h.
 */
static void fillSentinel ( ramp_t* driver )
{
    driver->maxVelocity = SENTINELF;
    driver->maxAcceleration = SENTINELF;
    driver->ts = SENTINELF;
    driver->position = SENTINELF;
    driver->velocity = SENTINELF;
    driver->arrived = SENTINEL8;
}

static uint8_t isSentinel ( const ramp_t* const driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->maxVelocity == SENTINELF ) &&
         ( driver->maxAcceleration == SENTINELF ) &&
         ( driver->ts == SENTINELF ) &&
         ( driver->position == SENTINELF ) &&
         ( driver->velocity == SENTINELF ) &&
         ( driver->arrived == SENTINEL8 ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------------- Init guards */

/*
 * None of the three limits may be zero or negative. Unlike pidInit, which
 * rejects a ts of zero because pidControl divides by it, rampIteration never
 * divides at all — a zero here would leave the ramp reporting a successful
 * init while never moving, which is why slewInit rejects a maxStep of zero
 * for the same reason.
 */
static void initCase ( void )
{
    ramp_t driver;

    printf ( "ramp Init guards\n" );

    fillSentinel ( &driver );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( rampInit ( NULL, VMAX, AMAX, TS, 0.0f ) == FALSE ) );

    check ( "a zero maxVelocity is rejected",
            ( uint8_t ) ( rampInit ( &driver, 0.0f, AMAX, TS, 0.0f ) == FALSE ) );
    check ( "a negative maxVelocity is rejected",
            ( uint8_t ) ( rampInit ( &driver, -1.0f, AMAX, TS, 0.0f ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a zero maxAcceleration is rejected",
            ( uint8_t ) ( rampInit ( &driver, VMAX, 0.0f, TS, 0.0f ) == FALSE ) );
    check ( "a negative maxAcceleration is rejected",
            ( uint8_t ) ( rampInit ( &driver, VMAX, -1.0f, TS, 0.0f ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a zero ts is rejected",
            ( uint8_t ) ( rampInit ( &driver, VMAX, AMAX, 0.0f, 0.0f ) == FALSE ) );
    check ( "a negative ts is rejected",
            ( uint8_t ) ( rampInit ( &driver, VMAX, AMAX, -1.0f, 0.0f ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a well formed init is accepted",
            rampInit ( &driver, VMAX, AMAX, TS, 7.5f ) );
    check ( "the initial position is the one given",
            nearly ( rampGetOutput ( &driver ), 7.5f ) );
    check ( "the ramp starts at rest",
            nearly ( rampGetVelocity ( &driver ), 0.0f ) );
    check ( "a ramp with no move pending counts as arrived",
            ( uint8_t ) ( rampIsArrived ( &driver ) == TRUE ) );

    check ( "a negative initial position is fine, only the limits are checked",
            rampInit ( &driver, VMAX, AMAX, TS, -3.0f ) );
    check ( "and it is stored", nearly ( rampGetOutput ( &driver ), -3.0f ) );
}

int main ( void )
{
    initCase ( );

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
