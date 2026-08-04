/*
 * Soft timer module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * The check this file exists for is periodicPhaseCase: a periodic timer whose
 * expiry flag goes unread must not lose phase. Reloading by resetting the
 * counter to zero instead of subtracting the period passes every other case
 * in this file and fails that one.
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
