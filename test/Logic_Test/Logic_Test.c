/*
 * Covers logic and bininp.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Both modules are about what happens over a sequence of calls rather than what
 * one call returns, so most cases here drive a series of inputs and check the
 * state at each step.
 */

#include <stddef.h>
#include <stdio.h>

#include "logic.h"
#include "bininp.h"

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

/* ------------------------------------------------------------ logicRsff */

static void rsffCase ( void )
{
    uint8_t mem = FALSE;

    printf ( "logicRsff\n" );

    check ( "idle holds the cleared state",
            ( uint8_t ) ( logicRsff ( FALSE, FALSE, &mem ) == FALSE ) );

    check ( "set raises the output", ( uint8_t ) ( logicRsff ( FALSE, TRUE, &mem ) == TRUE ) );
    check ( "and the state is stored", ( uint8_t ) ( mem == TRUE ) );

    check ( "idle holds the raised state",
            ( uint8_t ) ( logicRsff ( FALSE, FALSE, &mem ) == TRUE ) );

    check ( "reset clears it", ( uint8_t ) ( logicRsff ( TRUE, FALSE, &mem ) == FALSE ) );
    check ( "idle holds the cleared state again",
            ( uint8_t ) ( logicRsff ( FALSE, FALSE, &mem ) == FALSE ) );

    /*
     * Reset dominance is the whole character of this flip-flop, so it is
     * checked from both starting states: it must clear a raised output and it
     * must refuse to let set raise a cleared one.
     */
    ( void ) logicRsff ( FALSE, TRUE, &mem );
    check ( "reset wins over a simultaneous set, from the raised state",
            ( uint8_t ) ( logicRsff ( TRUE, TRUE, &mem ) == FALSE ) );
    check ( "reset wins over a simultaneous set, from the cleared state",
            ( uint8_t ) ( logicRsff ( TRUE, TRUE, &mem ) == FALSE ) );

    /* Any non-zero value counts as asserted, not only the value of TRUE. */
    mem = FALSE;
    check ( "a set input of 200 also raises the output",
            ( uint8_t ) ( logicRsff ( 0u, 200u, &mem ) == TRUE ) );
    check ( "a reset input of 200 also clears it",
            ( uint8_t ) ( logicRsff ( 200u, 0u, &mem ) == FALSE ) );
}

/* ------------------------------------------------------------- logicDff */

static void dffCase ( void )
{
    uint8_t mem = FALSE;

    printf ( "logicDff\n" );

    /*
     * This returns the value from before the call, not the one being written,
     * which is the property that makes it a register rather than a wire.
     */
    check ( "the first call reports the initial state",
            ( uint8_t ) ( logicDff ( TRUE, &mem ) == FALSE ) );
    check ( "and stores the new one", ( uint8_t ) ( mem == TRUE ) );

    check ( "the next call reports what was stored",
            ( uint8_t ) ( logicDff ( FALSE, &mem ) == TRUE ) );
    check ( "and stores the new one again", ( uint8_t ) ( mem == FALSE ) );

    check ( "holding the input steady reports it from the second call on",
            ( uint8_t ) ( logicDff ( FALSE, &mem ) == FALSE ) );
}

/* ----------------------------------------------------------- bininp */

static void bininpInitCase ( void )
{
    bininp_t driver;

    printf ( "bininpInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( bininpInit ( NULL, 3u ) == FALSE ) );
    check ( "a valid driver is accepted", bininpInit ( &driver, 3u ) );
    check ( "and starts cleared", ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );
    check ( "with no rising edge pending",
            ( uint8_t ) ( bininpGetRisingValue ( &driver ) == FALSE ) );

    /* A filter count of zero is legal: it accepts the first differing sample. */
    check ( "a zero filter count is accepted", bininpInit ( &driver, 0u ) );
    bininpUpdate ( &driver, TRUE );
    check ( "and takes effect on the very first call",
            ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );
}

static void bininpDebounceCase ( void )
{
    bininp_t driver;
    uint32_t i = 0;

    printf ( "bininpUpdate debouncing\n" );

    check ( "Init with a filter count of 3", bininpInit ( &driver, 3u ) );

    /*
     * The threshold is "more than filterCount consecutive calls", so three
     * calls are not enough and the fourth is what flips the output.
     */
    for ( i = 0; i < 3u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    check ( "three matching samples are not enough",
            ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );

    bininpUpdate ( &driver, TRUE );
    check ( "the fourth flips the output", ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );

    /*
     * A run broken before the threshold has to start over, which is the entire
     * point of the module. Three highs, one low, then three highs again must
     * still leave the output where it was.
     */
    check ( "Init again", bininpInit ( &driver, 3u ) );

    bininpUpdate ( &driver, TRUE );
    bininpUpdate ( &driver, TRUE );
    bininpUpdate ( &driver, TRUE );
    bininpUpdate ( &driver, FALSE );
    bininpUpdate ( &driver, TRUE );
    bininpUpdate ( &driver, TRUE );
    bininpUpdate ( &driver, TRUE );

    check ( "an interrupted run does not accumulate",
            ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );

    bininpUpdate ( &driver, TRUE );
    check ( "but the run that follows still completes",
            ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );

    /* Falling back down takes the same number of samples. */
    for ( i = 0; i < 3u; ++i )
    {
        bininpUpdate ( &driver, FALSE );
    }

    check ( "three low samples are not enough to fall",
            ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );

    bininpUpdate ( &driver, FALSE );
    check ( "the fourth clears it", ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );

    /* Any non-zero sample is a high, not only the value of TRUE. */
    check ( "Init once more", bininpInit ( &driver, 1u ) );
    bininpUpdate ( &driver, 42u );
    bininpUpdate ( &driver, 200u );
    check ( "any non-zero sample counts as high",
            ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );
}

static void bininpRisingCase ( void )
{
    bininp_t driver;
    uint32_t i = 0;

    printf ( "bininpGetRisingValue\n" );

    check ( "Init", bininpInit ( &driver, 1u ) );

    bininpUpdate ( &driver, TRUE );
    bininpUpdate ( &driver, TRUE );

    check ( "the output rose", ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );

    /*
     * The flag is cleared by the act of reading it, so the first read reports
     * the edge and an immediate second read must not.
     */
    check ( "the rising edge is reported", bininpGetRisingValue ( &driver ) );
    check ( "and reading it cleared the flag",
            ( uint8_t ) ( bininpGetRisingValue ( &driver ) == FALSE ) );

    /* A falling edge is not a rising one. */
    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, FALSE );
    }

    check ( "the output fell", ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );
    check ( "and that did not set the rising flag",
            ( uint8_t ) ( bininpGetRisingValue ( &driver ) == FALSE ) );

    /* The next rise sets it again. */
    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    check ( "a second rise sets the flag again", bininpGetRisingValue ( &driver ) );

    /*
     * The flag latches: an edge that nobody reads for a while is still there
     * when the main loop finally gets round to it.
     */
    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, FALSE );
    }

    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    for ( i = 0; i < 20u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    check ( "an unread edge is still waiting twenty calls later",
            bininpGetRisingValue ( &driver ) );
}

int main ( void )
{
    rsffCase ( );
    printf ( "\n" );
    dffCase ( );
    printf ( "\n" );
    bininpInitCase ( );
    printf ( "\n" );
    bininpDebounceCase ( );
    printf ( "\n" );
    bininpRisingCase ( );

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
