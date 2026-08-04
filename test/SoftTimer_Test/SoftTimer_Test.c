/*
 * Soft timer module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * periodicCase and periodicPhaseCase pin the periodic mode split: that a
 * periodic timer reloads and keeps running rather than stopping at its first
 * expiry the way a one shot does, and that the reload happens at all. They do
 * not distinguish subtracting the period from clearing the counter, and no
 * test of this API can: the counter is below the period on entry to every
 * tick, so it lands exactly on the period and the two reloads agree.
 */

#include <stdio.h>

#include "softtimer.h"

#define SENTINEL32  0xA5A5A5A5u
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
 * A rejected Init must leave the driver exactly as it found it, so the caller
 * cannot half initialize a timer by ignoring the return value. The pattern is
 * written by hand rather than with memset to keep the test free of string.h.
 */
static void fillSentinel ( softtimer_t* driver )
{
    driver->period = SENTINEL32;
    driver->counter = SENTINEL32;
    driver->mode = SENTINEL8;
    driver->state = SENTINEL8;
    driver->expired = SENTINEL8;
}

static uint8_t isSentinel ( const softtimer_t* const driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->period == SENTINEL32 ) && ( driver->counter == SENTINEL32 ) &&
         ( driver->mode == SENTINEL8 ) && ( driver->state == SENTINEL8 ) &&
         ( driver->expired == SENTINEL8 ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------- the Init contract */

static void initCase ( void )
{
    softtimer_t driver;

    printf ( "softtimerInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( softtimerInit ( NULL, 10u, ( uint8_t ) STM_ONESHOT ) == FALSE ) );

    fillSentinel ( &driver );
    check ( "a zero period is rejected",
            ( uint8_t ) ( softtimerInit ( &driver, 0u, ( uint8_t ) STM_ONESHOT ) == FALSE ) );
    check ( "the rejected zero period left the driver untouched", isSentinel ( &driver ) );

    fillSentinel ( &driver );
    check ( "an out of range mode is rejected",
            ( uint8_t ) ( softtimerInit ( &driver, 10u, 7u ) == FALSE ) );
    check ( "the rejected mode left the driver untouched", isSentinel ( &driver ) );

    check ( "one shot init succeeds",
            softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );
    check ( "init leaves the timer stopped",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_STOPPED ) );

    check ( "periodic init succeeds",
            softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );
    check ( "init leaves the periodic timer stopped too",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_STOPPED ) );
}

/* --------------------------------------------------------- one shot timing */

/*
 * A period of N must expire on the Nth tick after Start, not the N-1th, and
 * the flag must read TRUE exactly once. Once expired the counter stops, which
 * is what keeps a long lived one shot from wrapping its counter.
 */
static void oneShotCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "one shot timing\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );

    softtimerStart ( &driver );
    check ( "start sets the timer running",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_RUNNING ) );

    for ( i = 0; i < 9u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "not expired after 9 of 10 ticks",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "state is expired after the 10th tick",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_EXPIRED ) );
    check ( "the flag reads TRUE once",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
    check ( "and FALSE on the second read",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );
    check ( "the state survives the flag being consumed",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_EXPIRED ) );

    for ( i = 0; i < 100u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "a finished one shot does not expire again",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );
}

/* ------------------------------------------------------- start, stop, ticks */

/*
 * Stop freezes, Start resets. A stopped timer must ignore ticks outright so
 * the caller can tick every timer it owns from the ISR without asking which
 * ones are live.
 */
static void startStopCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "start and stop\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );

    for ( i = 0; i < 20u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "an unstarted timer ignores ticks",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );
    check ( "and stays stopped",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_STOPPED ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 5u; ++i )
    {
        softtimerTick ( &driver );
    }

    softtimerStop ( &driver );

    for ( i = 0; i < 20u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "a stopped timer ignores ticks",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 9u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "start reset the counter, so 9 ticks is not enough",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "and the 10th tick after start expires it",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
}

/* --------------------------------------------------------- periodic timing */

static void periodicCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;
    uint32_t expiries = 0;

    printf ( "periodic timing\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 100u; ++i )
    {
        softtimerTick ( &driver );

        if ( softtimerExpired ( &driver ) == TRUE )
        {
            ++expiries;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    printf ( "        expiries in 100 ticks at period 10: %lu\n",
             ( unsigned long ) expiries );

    check ( "a period of 10 expires 10 times in 100 ticks",
            ( uint8_t ) ( expiries == 10u ) );
    check ( "a periodic timer stays running rather than reaching STS_EXPIRED",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_RUNNING ) );
}

/*
 * Expiries that the main loop never reads must not disturb the tick the next
 * one lands on. Ticking 35 times without reading the flag leaves 5 counts of
 * the current period standing, so the next expiry is 5 ticks away, at tick 40.
 *
 * This pins the reload against a tick that forgets to reload at all, which
 * would leave the counter at 35 and never expire again. It does not pin the
 * subtracting reload against one that clears the counter — those two are the
 * same here, because the counter is always below the period on entry to a
 * tick and therefore lands exactly on it.
 */
static void periodicPhaseCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "periodic phase after unread expiries\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 35u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "the unread expiries left the flag set",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
    check ( "5 ticks of the current period have already been counted",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 5u ) );

    for ( i = 0; i < 4u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "tick 39 is still short of the next expiry",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "the next expiry lands on tick 40, not tick 45",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
}

/* ------------------------------------------------- remaining and the period */

static void remainingCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "elapsed and remaining\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_ONESHOT ) );

    check ( "a fresh timer has its whole period remaining",
            ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 10u ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 4u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "4 ticks elapsed", ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 4u ) );
    check ( "6 ticks remaining", ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 6u ) );

    softtimerStop ( &driver );

    for ( i = 0; i < 20u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "stop froze the counter rather than clearing it",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 4u ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 10u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "remaining is zero once the timer has expired",
            ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 0u ) );
}

static void changePeriodCase ( void )
{
    softtimer_t driver;
    uint32_t i = 0;

    printf ( "softtimerChangePeriod\n" );

    check ( "init", softtimerInit ( &driver, 10u, ( uint8_t ) STM_PERIODIC ) );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( softtimerChangePeriod ( NULL, 5u ) == FALSE ) );

    softtimerStart ( &driver );

    for ( i = 0; i < 7u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "a zero period is rejected",
            ( uint8_t ) ( softtimerChangePeriod ( &driver, 0u ) == FALSE ) );
    check ( "the rejected period left the timer counting where it was",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 7u ) );
    check ( "and left the old period in place",
            ( uint8_t ) ( softtimerGetRemaining ( &driver ) == 3u ) );

    check ( "a change to 5 is accepted", softtimerChangePeriod ( &driver, 5u ) );
    check ( "the change reset the counter",
            ( uint8_t ) ( softtimerGetElapsed ( &driver ) == 0u ) );
    check ( "the timer is still running",
            ( uint8_t ) ( softtimerGetState ( &driver ) == ( uint8_t ) STS_RUNNING ) );

    for ( i = 0; i < 4u; ++i )
    {
        softtimerTick ( &driver );
    }

    check ( "4 ticks is short of the new period",
            ( uint8_t ) ( softtimerExpired ( &driver ) == FALSE ) );

    softtimerTick ( &driver );

    check ( "and the 5th tick expires it",
            ( uint8_t ) ( softtimerExpired ( &driver ) == TRUE ) );
}

int main ( void )
{
    initCase ( );

    printf ( "\n" );
    oneShotCase ( );
    printf ( "\n" );
    startStopCase ( );

    printf ( "\n" );
    periodicCase ( );
    printf ( "\n" );
    periodicPhaseCase ( );

    printf ( "\n" );
    remainingCase ( );
    printf ( "\n" );
    changePeriodCase ( );

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
