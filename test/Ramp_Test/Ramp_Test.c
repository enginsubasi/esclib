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

/* --------------------------------------------------------- the trajectory */

/*
 * The velocity step is maxAcceleration * ts, which these limits make exactly
 * 2 per iteration. From rest the profile must take that step and no more.
 */
static void accelerationCase ( void )
{
    ramp_t driver;

    printf ( "ramp acceleration limit\n" );

    check ( "init", rampInit ( &driver, VMAX, AMAX, TS, 0.0f ) );

    rampIteration ( &driver, 1000.0f );
    check ( "one step from rest gives exactly one velocity increment",
            nearly ( rampGetVelocity ( &driver ), 2.0f ) );
    check ( "and the ramp is not arrived",
            ( uint8_t ) ( rampIsArrived ( &driver ) == FALSE ) );

    rampIteration ( &driver, 1000.0f );
    check ( "two steps give two increments",
            nearly ( rampGetVelocity ( &driver ), 4.0f ) );

    rampIteration ( &driver, 1000.0f );
    check ( "three steps give three increments",
            nearly ( rampGetVelocity ( &driver ), 6.0f ) );

    check ( "and the position is the sum of the steps so far",
            nearly ( rampGetOutput ( &driver ), 0.12f ) );
}

/*
 * Fifty steps of 2 reach the cap of 100 exactly. Beyond that the velocity
 * must hold rather than keep climbing, and the target is far enough away
 * that braking has not begun: the profile cruises.
 */
static void velocityCapCase ( void )
{
    ramp_t driver;
    uint32_t i = 0;
    uint8_t everExceeded = FALSE;

    printf ( "ramp velocity cap\n" );

    check ( "init", rampInit ( &driver, VMAX, AMAX, TS, 0.0f ) );

    for ( i = 0; i < 50u; ++i )
    {
        rampIteration ( &driver, 1000.0f );

        if ( rampGetVelocity ( &driver ) > VMAX )
        {
            everExceeded = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "fifty steps reach the cap exactly",
            nearly ( rampGetVelocity ( &driver ), VMAX ) );
    check ( "and the distance covered is the area under the ramp",
            nearly ( rampGetOutput ( &driver ), 25.5f ) );

    for ( i = 0; i < 50u; ++i )
    {
        rampIteration ( &driver, 1000.0f );

        if ( rampGetVelocity ( &driver ) > VMAX )
        {
            everExceeded = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "the velocity holds at the cap while cruising",
            nearly ( rampGetVelocity ( &driver ), VMAX ) );
    check ( "and never exceeded it on any step",
            ( uint8_t ) ( everExceeded == FALSE ) );
}

/*
 * The pinned regression.
 *
 * A run long enough to accelerate, cruise and brake. The position is checked
 * against the target on every single step, because the defect this pins —
 * a missing final clamp — shows up as a single step past the target followed
 * by a turn around, and a check made only at the end would miss it.
 *
 * The arrived position and velocity are compared exactly rather than with a
 * tolerance. rampIteration assigns them, so an exact match is what
 * distinguishes the clamp having run from the ramp merely coasting close.
 *
 * The arrival velocity is checked separately because the clamp masks a wrong
 * brake point. Braking at some fixed remaining distance instead of at the
 * square root envelope still lands the ramp exactly on the target — the clamp
 * sees to that — so no position check can tell the two apart. What gives it
 * away is arriving at speed: with the envelope the ramp is down to about 17
 * on the step that arrives, and with a fixed brake distance it is still doing
 * 92 out of a cap of 100.
 */
static void noOvershootCase ( void )
{
    ramp_t driver;
    uint32_t i = 0;
    uint32_t steps = 0;
    uint8_t everPassed = FALSE;
    uint8_t everReversed = FALSE;
    float velocityBefore = 0;
    float arrivalVelocity = 0;

    printf ( "ramp arrival without overshoot\n" );

    check ( "init", rampInit ( &driver, VMAX, AMAX, TS, 0.0f ) );

    for ( i = 0; i < 300u; ++i )
    {
        velocityBefore = rampGetVelocity ( &driver );

        rampIteration ( &driver, 100.0f );

        if ( rampGetOutput ( &driver ) > 100.0f )
        {
            everPassed = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( rampGetVelocity ( &driver ) < 0.0f )
        {
            everReversed = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( ( rampIsArrived ( &driver ) == TRUE ) && ( steps == 0 ) )
        {
            steps = i + 1u;
            arrivalVelocity = velocityBefore;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "the position never passed the target on any step",
            ( uint8_t ) ( everPassed == FALSE ) );
    check ( "the velocity never reversed, so the ramp never turned around",
            ( uint8_t ) ( everReversed == FALSE ) );
    check ( "the ramp arrived", ( uint8_t ) ( rampIsArrived ( &driver ) == TRUE ) );
    check ( "it arrived within the run rather than on the last step",
            ( uint8_t ) ( ( steps > 0 ) && ( steps < 300u ) ) );
    check ( "it had braked well below the cap before arriving, so the envelope ran",
            ( uint8_t ) ( arrivalVelocity < ( VMAX / 4.0f ) ) );
    check ( "the position lands exactly on the target",
            ( uint8_t ) ( rampGetOutput ( &driver ) == 100.0f ) );
    check ( "and the velocity is exactly zero",
            ( uint8_t ) ( rampGetVelocity ( &driver ) == 0.0f ) );
}

/*
 * A move too short for the cap. The peak velocity of a triangular profile
 * over a distance d is sqrt( a * d ), which for d = 10 and a = 200 is about
 * 44.7 — well under the cap of 100, so the cap must never be reached.
 */
static void triangularCase ( void )
{
    ramp_t driver;
    uint32_t i = 0;
    float peak = 0;

    printf ( "ramp triangular profile\n" );

    check ( "init", rampInit ( &driver, VMAX, AMAX, TS, 0.0f ) );

    for ( i = 0; i < 300u; ++i )
    {
        rampIteration ( &driver, 10.0f );

        if ( rampGetVelocity ( &driver ) > peak )
        {
            peak = rampGetVelocity ( &driver );
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "the move is too short to reach the cap",
            ( uint8_t ) ( peak < VMAX ) );
    check ( "but it does get moving",
            ( uint8_t ) ( peak > 40.0f ) );
    check ( "and it still lands exactly on the target",
            ( uint8_t ) ( rampGetOutput ( &driver ) == 10.0f ) );
    check ( "at rest", ( uint8_t ) ( rampGetVelocity ( &driver ) == 0.0f ) );
}

/*
 * The mirror image. A sign error in the velocity envelope or in the step
 * clamp shows up here and nowhere else, because every other case runs in the
 * positive direction.
 */
static void negativeCase ( void )
{
    ramp_t driver;
    uint32_t i = 0;
    uint8_t everPassed = FALSE;

    printf ( "ramp in the negative direction\n" );

    check ( "init", rampInit ( &driver, VMAX, AMAX, TS, 0.0f ) );

    rampIteration ( &driver, -100.0f );
    check ( "the first step moves the other way",
            nearly ( rampGetVelocity ( &driver ), -2.0f ) );

    for ( i = 0; i < 300u; ++i )
    {
        rampIteration ( &driver, -100.0f );

        if ( rampGetOutput ( &driver ) < -100.0f )
        {
            everPassed = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "the position never passed the target on any step",
            ( uint8_t ) ( everPassed == FALSE ) );
    check ( "it lands exactly on the target",
            ( uint8_t ) ( rampGetOutput ( &driver ) == -100.0f ) );
    check ( "at rest", ( uint8_t ) ( rampGetVelocity ( &driver ) == 0.0f ) );
}

/*
 * The target is a parameter of every iteration rather than something set
 * once, which is what makes a moving target fall out for free. Here it is
 * pulled in mid flight, while the ramp is still accelerating away from it.
 *
 * The target also has to be reachable once the ramp is already arrived: a
 * second move from rest must start again rather than stay stuck.
 */
static void movingTargetCase ( void )
{
    ramp_t driver;
    uint32_t i = 0;

    printf ( "ramp with a target that moves\n" );

    check ( "init", rampInit ( &driver, VMAX, AMAX, TS, 0.0f ) );

    for ( i = 0; i < 30u; ++i )
    {
        rampIteration ( &driver, 100.0f );
    }

    check ( "the ramp is under way and not arrived",
            ( uint8_t ) ( rampIsArrived ( &driver ) == FALSE ) );

    for ( i = 0; i < 300u; ++i )
    {
        rampIteration ( &driver, 50.0f );
    }

    check ( "it arrives on the new target, not the old one",
            ( uint8_t ) ( rampGetOutput ( &driver ) == 50.0f ) );
    check ( "at rest", ( uint8_t ) ( rampGetVelocity ( &driver ) == 0.0f ) );
    check ( "and reports arrival",
            ( uint8_t ) ( rampIsArrived ( &driver ) == TRUE ) );

    for ( i = 0; i < 300u; ++i )
    {
        rampIteration ( &driver, 60.0f );
    }

    check ( "a second move from rest starts again rather than staying stuck",
            ( uint8_t ) ( rampGetOutput ( &driver ) == 60.0f ) );
}

/*
 * A target the ramp is already sitting on. remaining is zero, so the step is
 * zero, and the final clamp fires on the first call because zero is not less
 * than zero. Nothing moves and nothing is reported as running.
 */
static void alreadyThereCase ( void )
{
    ramp_t driver;
    uint32_t i = 0;

    printf ( "ramp already on its target\n" );

    check ( "init at 5", rampInit ( &driver, VMAX, AMAX, TS, 5.0f ) );

    for ( i = 0; i < 10u; ++i )
    {
        rampIteration ( &driver, 5.0f );
    }

    check ( "the position did not move",
            ( uint8_t ) ( rampGetOutput ( &driver ) == 5.0f ) );
    check ( "the velocity stayed at zero",
            ( uint8_t ) ( rampGetVelocity ( &driver ) == 0.0f ) );
    check ( "and it reports arrival",
            ( uint8_t ) ( rampIsArrived ( &driver ) == TRUE ) );
}

/* ------------------------------------------------------ Q16 fixed point */

/*
 * The fixed point width, added 06/08/2026, for parts with no FPU. A limit of
 * one unit is 65536.
 *
 * It differs from the float variant in two places and the checks below pin
 * both. There is no ts: the limits are per sample, which takes the period out
 * of the update. And sqrtf is replaced by an integer square root, which is
 * the only real work in the variant — the envelope feeds it a Q32 product and
 * the root of a Q32 value is a Q16 one, so the scale comes out with no
 * shifting of its own.
 *
 * The limits below make the arithmetic checkable by hand: an acceleration of
 * one unit per sample squared and a velocity cap of ten units per sample. The
 * velocity therefore climbs by exactly one unit per call and reaches the cap
 * after ten, having covered 1+2+...+10 = 55 units.
 */
static void fixedPointCase ( void )
{
    rampi32_t driver;
    uint32_t i = 0;
    uint32_t steps = 0;
    uint8_t everPassed = FALSE;
    uint8_t everReversed = FALSE;
    int32_t arrivalVelocity = 0;
    int32_t velocityBefore = 0;

    printf ( "ramp fixed point\n" );

    check ( "NULL driver is rejected",
            ( uint8_t ) ( rampIniti32 ( NULL, 655360, 65536, 0 ) == FALSE ) );
    check ( "a zero maxVelocity is rejected",
            ( uint8_t ) ( rampIniti32 ( &driver, 0, 65536, 0 ) == FALSE ) );
    check ( "a negative maxVelocity is rejected",
            ( uint8_t ) ( rampIniti32 ( &driver, -1, 65536, 0 ) == FALSE ) );
    check ( "a zero maxAcceleration is rejected",
            ( uint8_t ) ( rampIniti32 ( &driver, 655360, 0, 0 ) == FALSE ) );

    check ( "Init", rampIniti32 ( &driver, 655360, 65536, 0 ) );
    check ( "the position starts where it was put",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 0 ) );
    check ( "the ramp starts at rest",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == 0 ) );
    check ( "and with no move pending it counts as arrived",
            ( uint8_t ) ( rampIsArrivedi32 ( &driver ) == TRUE ) );

    rampIterationi32 ( &driver, 1000 );
    check ( "one step from rest gives exactly one velocity increment",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == 65536 ) );
    check ( "and moves the position by that much",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 1 ) );
    check ( "and the ramp is not arrived",
            ( uint8_t ) ( rampIsArrivedi32 ( &driver ) == FALSE ) );

    for ( i = 0; i < 9u; ++i )
    {
        rampIterationi32 ( &driver, 1000 );
    }

    check ( "ten steps reach the cap exactly",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == 655360 ) );
    check ( "and the distance covered is the area under the ramp",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 55 ) );

    rampIterationi32 ( &driver, 1000 );
    check ( "the velocity holds at the cap while cruising",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == 655360 ) );

    /*
     * The same pinned pair the float variant carries. The position is checked
     * on every step, because a missing final clamp shows up as one step past
     * the target followed by a turn around and leaves no trace in the
     * converged value. The arrival velocity is checked separately, because
     * the clamp masks a wrong brake point: braking at a fixed distance still
     * lands exactly on the target, and only arriving at speed gives it away.
     */
    check ( "re-init", rampIniti32 ( &driver, 655360, 65536, 0 ) );

    for ( i = 0; i < 400u; ++i )
    {
        velocityBefore = rampGetVelocityi32 ( &driver );

        rampIterationi32 ( &driver, 1000 );

        if ( rampGetOutputi32 ( &driver ) > 1000 )
        {
            everPassed = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( rampGetVelocityi32 ( &driver ) < 0 )
        {
            everReversed = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( ( rampIsArrivedi32 ( &driver ) == TRUE ) && ( steps == 0 ) )
        {
            steps = i + 1u;
            arrivalVelocity = velocityBefore;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "the position never passed the target on any step",
            ( uint8_t ) ( everPassed == FALSE ) );
    check ( "the velocity never reversed",
            ( uint8_t ) ( everReversed == FALSE ) );
    check ( "the ramp arrived", ( uint8_t ) ( rampIsArrivedi32 ( &driver ) == TRUE ) );
    check ( "it arrived within the run",
            ( uint8_t ) ( ( steps > 0 ) && ( steps < 400u ) ) );
    check ( "the position lands exactly on the target",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 1000 ) );
    check ( "and the velocity is exactly zero",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == 0 ) );
    /*
     * With the envelope the ramp is down to about 4.5 units per sample on the
     * step that arrives; braking at a fixed remaining distance instead slams
     * in at the full cap of 10. Half the cap sits between the two with margin
     * on both sides. The fraction is looser than the float variant's quarter
     * because these limits are coarser: ten steps to the cap here against
     * fifty there, so the last step before arrival is proportionally bigger.
     */
    check ( "it had braked below the cap before arriving, so the envelope ran",
            ( uint8_t ) ( arrivalVelocity < ( 655360 / 2 ) ) );

    /* The mirror. A sign error in the envelope shows up here and nowhere else. */
    check ( "re-init for the negative direction",
            rampIniti32 ( &driver, 655360, 65536, 0 ) );

    rampIterationi32 ( &driver, -1000 );
    check ( "the first step moves the other way",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == -65536 ) );

    everPassed = FALSE;

    for ( i = 0; i < 400u; ++i )
    {
        rampIterationi32 ( &driver, -1000 );

        if ( rampGetOutputi32 ( &driver ) < -1000 )
        {
            everPassed = TRUE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "it never passed the target going down either",
            ( uint8_t ) ( everPassed == FALSE ) );
    check ( "and lands exactly on it",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == -1000 ) );

    /*
     * A move too short to reach the cap must stay under it, which is the
     * triangular profile. Peak velocity over a distance d at an acceleration
     * of one is about sqrt( d ), so ten units peaks near three.
     */
    check ( "re-init for a short move", rampIniti32 ( &driver, 655360, 65536, 0 ) );

    velocityBefore = 0;

    for ( i = 0; i < 400u; ++i )
    {
        rampIterationi32 ( &driver, 10 );

        if ( rampGetVelocityi32 ( &driver ) > velocityBefore )
        {
            velocityBefore = rampGetVelocityi32 ( &driver );
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "a short move never reaches the cap",
            ( uint8_t ) ( velocityBefore < 655360 ) );
    check ( "but it does get moving",
            ( uint8_t ) ( velocityBefore > 65536 ) );
    check ( "and still lands exactly on the target",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 10 ) );

    /* A target the ramp is already sitting on. */
    check ( "re-init on the target", rampIniti32 ( &driver, 655360, 65536, 5 ) );
    rampIterationi32 ( &driver, 5 );
    check ( "nothing moves",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 5 ) );
    check ( "and it reports arrival at once",
            ( uint8_t ) ( rampIsArrivedi32 ( &driver ) == TRUE ) );

    /*
     * The pinned range check. Q16 in an int32_t holds about 32767 units, so a
     * move of a hundred thousand is only reachable because the state is
     * int64_t.
     */
    check ( "re-init for a long move", rampIniti32 ( &driver, 655360, 65536, 0 ) );

    for ( i = 0; i < 20000u; ++i )
    {
        rampIterationi32 ( &driver, 100000 );
    }

    check ( "a move far past the Q16 thirty two bit ceiling arrives exactly",
            ( uint8_t ) ( rampGetOutputi32 ( &driver ) == 100000 ) );
    check ( "at rest",
            ( uint8_t ) ( rampGetVelocityi32 ( &driver ) == 0 ) );
}

int main ( void )
{
    initCase ( );

    printf ( "\n" );
    accelerationCase ( );
    printf ( "\n" );
    velocityCapCase ( );

    printf ( "\n" );
    noOvershootCase ( );
    printf ( "\n" );
    triangularCase ( );
    printf ( "\n" );
    negativeCase ( );

    printf ( "\n" );
    movingTargetCase ( );
    printf ( "\n" );
    alreadyThereCase ( );
    printf ( "\n" );
    fixedPointCase ( );

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
