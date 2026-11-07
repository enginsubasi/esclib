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

/*
 * Fills a driver with a non-zero byte pattern before Init. A driver is a
 * stack local, and a stack that happens to be zero makes a field Init never
 * wrote read as if it had been cleared. 0x5A is TRUE to every flag here,
 * which is the value a missing clear would leave behind.
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
    poison ( &driver, ( uint32_t ) sizeof ( driver ) );
    check ( "a valid driver is accepted", bininpInit ( &driver, 3u ) );
    check ( "and starts cleared", ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );
    check ( "with no rising edge pending",
            ( uint8_t ) ( bininpGetRisingValue ( &driver ) == FALSE ) );
    check ( "and no falling edge pending",
            ( uint8_t ) ( bininpGetFallingValue ( &driver ) == FALSE ) );

    /*
     * The counter must start at zero too. Left at the poison it is already
     * past the threshold, so the very first differing sample would flip the
     * output where four are needed.
     */
    poison ( &driver, ( uint32_t ) sizeof ( driver ) );
    ( void ) bininpInit ( &driver, 3u );
    bininpUpdate ( &driver, TRUE );
    check ( "and the debounce count starts from zero",
            ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );

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

static void bininpFallingCase ( void )
{
    bininp_t driver;
    uint32_t i = 0;

    printf ( "bininpGetFallingValue\n" );

    check ( "Init", bininpInit ( &driver, 1u ) );

    /* A rising edge is not a falling one. */
    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    check ( "the output rose", ( uint8_t ) ( bininpGetValue ( &driver ) == TRUE ) );
    check ( "and that did not set the falling flag",
            ( uint8_t ) ( bininpGetFallingValue ( &driver ) == FALSE ) );

    /*
     * One low sample is below the threshold of a filter count of one. A
     * flag set on the raw input rather than on the filtered output would
     * already be standing here.
     */
    bininpUpdate ( &driver, FALSE );
    check ( "a bounce below the threshold is not an edge",
            ( uint8_t ) ( bininpGetFallingValue ( &driver ) == FALSE ) );

    bininpUpdate ( &driver, FALSE );
    check ( "the output fell", ( uint8_t ) ( bininpGetValue ( &driver ) == FALSE ) );
    check ( "the falling edge is reported", bininpGetFallingValue ( &driver ) );
    check ( "and reading it cleared the flag",
            ( uint8_t ) ( bininpGetFallingValue ( &driver ) == FALSE ) );

    /*
     * The two flags are independent. A press and a release between two
     * reads are both reported, and reading one leaves the other standing.
     */
    check ( "Init again", bininpInit ( &driver, 1u ) );
    ( void ) bininpGetRisingValue ( &driver );

    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, FALSE );
    }

    check ( "a press and a release unread: the falling edge is there",
            bininpGetFallingValue ( &driver ) );
    check ( "and reading it left the rising edge standing",
            bininpGetRisingValue ( &driver ) );

    /* The other way round: reading the rising flag leaves the falling one. */
    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, FALSE );
    }

    check ( "the rising edge read first", bininpGetRisingValue ( &driver ) );
    check ( "left the falling edge standing", bininpGetFallingValue ( &driver ) );

    /* The flag latches, as the rising one does. */
    for ( i = 0; i < 2u; ++i )
    {
        bininpUpdate ( &driver, TRUE );
    }

    for ( i = 0; i < 22u; ++i )
    {
        bininpUpdate ( &driver, FALSE );
    }

    check ( "an unread falling edge is still waiting twenty calls later",
            bininpGetFallingValue ( &driver ) );
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
    bininpFallingCase ( );

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
