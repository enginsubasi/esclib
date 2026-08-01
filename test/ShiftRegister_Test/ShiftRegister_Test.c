/*
 * Proves that the interrupt driven mode drives the pins in exactly the same
 * order as the blocking OneShot, for both shift register drivers.
 *
 * The pin callbacks record every transition instead of touching hardware, so
 * a transfer becomes a list of events. Running OneShot gives the reference
 * list; running Start plus Interrupt to completion has to give the same one.
 * That is the whole claim behind the design: a step boundary sits wherever
 * OneShot delays, so removing the delays cannot reorder anything.
 *
 * Unlike the other tests here this one asserts rather than printing values for
 * a human to compare, so it needs no output.txt. It returns non zero on the
 * first failure.
 */

#include <stdio.h>

#include "hc595_drv.h"
#include "hc597_drv.h"

#define CHAIN_SIZE      3u
#define MAX_EVENTS      512u
#define PATTERN_LEN     7u

typedef struct
{
    char pin;
    uint8_t level;
} event_t;

static event_t events[ MAX_EVENTS ];
static event_t reference[ MAX_EVENTS ];
static uint32_t eventCount = 0;
static uint32_t referenceCount = 0;
static uint8_t eventOverflow = FALSE;

static uint32_t readIndex = 0;
static uint32_t failures = 0;

/* An input pattern whose length is coprime with 8, so the bytes read back
   differ from each other and a stuck byte index would show up. */
static const uint8_t pattern[ PATTERN_LEN ] = { 1, 1, 0, 1, 0, 0, 1 };

static void recordEvent ( char pin, uint8_t level )
{
    if ( eventCount < MAX_EVENTS )
    {
        events[ eventCount ].pin = pin;
        events[ eventCount ].level = level;
        ++eventCount;
    }
    else
    {
        eventOverflow = TRUE;
    }
}

static void resetCapture ( void )
{
    eventCount = 0;
    readIndex = 0;
}

static void keepAsReference ( void )
{
    uint32_t i = 0;

    for ( i = 0; i < eventCount; ++i )
    {
        reference[ i ] = events[ i ];
    }

    referenceCount = eventCount;
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

static uint8_t sameAsReference ( void )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    if ( eventCount != referenceCount )
    {
        printf ( "        event count %lu, reference %lu\n",
                    ( unsigned long ) eventCount, ( unsigned long ) referenceCount );
        retVal = FALSE;
    }
    else
    {
        for ( i = 0; i < eventCount; ++i )
        {
            if ( ( events[ i ].pin != reference[ i ].pin ) ||
                    ( events[ i ].level != reference[ i ].level ) )
            {
                printf ( "        first difference at %lu: got %c=%u, expected %c=%u\n",
                            ( unsigned long ) i,
                            events[ i ].pin, ( unsigned ) events[ i ].level,
                            reference[ i ].pin, ( unsigned ) reference[ i ].level );
                retVal = FALSE;
            }
        }
    }

    return ( retVal );
}

/* ---------------------------------------------------------------- hc595 */

static void sckWrite ( uint8_t level )
{
    recordEvent ( 'S', level );
}

static void rckWrite ( uint8_t level )
{
    recordEvent ( 'R', level );
}

/* Set while a blocking transfer is running, so the checks below can observe
   the driver from inside it. A pin callback is the only place that runs while
   hc595OneShot still holds the driver. */
static hc595_t* probeDriver = NULL;
static uint8_t probeArmed = FALSE;
static uint8_t probeFired = FALSE;
static uint8_t probeSawBlocking = FALSE;
static uint8_t probeStartRefused = FALSE;
static uint8_t probeInterruptInert = FALSE;

static void datWrite ( uint8_t level )
{
    uint32_t before = 0;

    recordEvent ( 'D', level );

    if ( ( probeArmed == TRUE ) && ( probeFired == FALSE ) )
    {
        probeFired = TRUE;

        if ( hc595GetState ( probeDriver ) == HC595_BLOCKING )
        {
            probeSawBlocking = TRUE;
        }

        if ( hc595Start ( probeDriver ) == FALSE )
        {
            probeStartRefused = TRUE;
        }

        before = eventCount;
        hc595Interrupt ( probeDriver );

        if ( eventCount == before )
        {
            probeInterruptInert = TRUE;
        }
    }
}

static void hc595Case ( void )
{
    hc595_t driver;
    uint8_t data[ CHAIN_SIZE ];
    uint32_t steps = 0;

    data[ 0 ] = 0xA5;
    data[ 1 ] = 0x3C;
    data[ 2 ] = 0xFF;

    printf ( "hc595\n" );

    /* The delay callbacks are NULL. HC595_DLY_NO never reaches them. */
    check ( "Init accepts NULL delay callbacks with HC595_DLY_NO",
            hc595Init ( &driver, data, CHAIN_SIZE, HC595_DLY_NO, 0,
                        sckWrite, rckWrite, datWrite, NULL, NULL ) );

    check ( "state is HC595_IDLE after Init",
            ( uint8_t ) ( hc595GetState ( &driver ) == HC595_IDLE ) );

    /* Init drives the three pins to their idle level, which lands in the
       capture buffer. Drop it so only the transfer itself is compared. */
    resetCapture ( );

    check ( "OneShot runs and reports success", hc595OneShot ( &driver ) );
    keepAsReference ( );

    resetCapture ( );

    check ( "Start arms the transfer", hc595Start ( &driver ) );
    check ( "state is HC595_BUSY once armed",
            ( uint8_t ) ( hc595GetState ( &driver ) == HC595_BUSY ) );
    check ( "Start is refused while busy",
            ( uint8_t ) ( hc595Start ( &driver ) == FALSE ) );

    while ( ( hc595GetState ( &driver ) == HC595_BUSY ) && ( steps < MAX_EVENTS ) )
    {
        hc595Interrupt ( &driver );
        ++steps;
    }

    check ( "state is HC595_DONE after the last step",
            ( uint8_t ) ( hc595GetState ( &driver ) == HC595_DONE ) );
    check ( "transfer took ( 24 * size ) + 2 steps",
            ( uint8_t ) ( steps == ( ( 24u * CHAIN_SIZE ) + 2u ) ) );
    check ( "interrupt mode drives the pins exactly like OneShot",
            sameAsReference ( ) );

    check ( "Start is accepted again from HC595_DONE", hc595Start ( &driver ) );
}

/* ---------------------------------------------------------------- hc597 */

static void clkWrite ( uint8_t level )
{
    recordEvent ( 'C', level );
}

static void lodWrite ( uint8_t level )
{
    recordEvent ( 'L', level );
}

static uint8_t datRead ( void )
{
    uint8_t level = 0;

    level = pattern[ readIndex % PATTERN_LEN ];
    ++readIndex;

    recordEvent ( 'I', level );

    return ( level );
}

static void hc597Case ( void )
{
    hc597_t driver;
    uint8_t data[ CHAIN_SIZE ];
    uint8_t expected[ CHAIN_SIZE ];
    uint32_t steps = 0;
    uint32_t i = 0;
    uint8_t sameData = TRUE;

    printf ( "hc597\n" );

    check ( "Init accepts NULL delay callbacks with HC597_DLY_NO",
            hc597Init ( &driver, data, CHAIN_SIZE, HC597_DLY_NO, 0,
                        clkWrite, lodWrite, datRead, NULL, NULL ) );

    resetCapture ( );

    check ( "OneShot runs and reports success", hc597OneShot ( &driver ) );
    keepAsReference ( );

    for ( i = 0; i < CHAIN_SIZE; ++i )
    {
        expected[ i ] = data[ i ];
    }

    resetCapture ( );

    check ( "Start arms the transfer", hc597Start ( &driver ) );
    check ( "Start is refused while busy",
            ( uint8_t ) ( hc597Start ( &driver ) == FALSE ) );

    while ( ( hc597GetState ( &driver ) == HC597_BUSY ) && ( steps < MAX_EVENTS ) )
    {
        hc597Interrupt ( &driver );
        ++steps;
    }

    check ( "state is HC597_DONE after the last step",
            ( uint8_t ) ( hc597GetState ( &driver ) == HC597_DONE ) );
    check ( "transfer took ( 8 * size ) + 1 steps",
            ( uint8_t ) ( steps == ( ( 8u * CHAIN_SIZE ) + 1u ) ) );
    check ( "interrupt mode drives the pins exactly like OneShot",
            sameAsReference ( ) );

    for ( i = 0; i < CHAIN_SIZE; ++i )
    {
        if ( data[ i ] != expected[ i ] )
        {
            printf ( "        byte %lu: got 0x%02X, expected 0x%02X\n",
                        ( unsigned long ) i, ( unsigned ) data[ i ], ( unsigned ) expected[ i ] );
            sameData = FALSE;
        }
    }

    check ( "interrupt mode reads back the same bytes as OneShot", sameData );
}

/* ------------------------------------------------------- argument checks */

static void initRejectionCase ( void )
{
    hc595_t driver;
    uint8_t data[ CHAIN_SIZE ];

    printf ( "Init argument checks\n" );

    check ( "unknown dlyType is rejected",
            ( uint8_t ) ( hc595Init ( &driver, data, CHAIN_SIZE, 99, 1,
                            sckWrite, rckWrite, datWrite, NULL, NULL ) == FALSE ) );

    check ( "HC595_DLY_MS without dlyMs is rejected",
            ( uint8_t ) ( hc595Init ( &driver, data, CHAIN_SIZE, HC595_DLY_MS, 1,
                            sckWrite, rckWrite, datWrite, NULL, NULL ) == FALSE ) );

    check ( "HC595_DLY_NOP without dlyNop is rejected",
            ( uint8_t ) ( hc595Init ( &driver, data, CHAIN_SIZE, HC595_DLY_NOP, 1,
                            sckWrite, rckWrite, datWrite, NULL, NULL ) == FALSE ) );

    check ( "a NULL pin callback is rejected",
            ( uint8_t ) ( hc595Init ( &driver, data, CHAIN_SIZE, HC595_DLY_NO, 0,
                            sckWrite, NULL, datWrite, NULL, NULL ) == FALSE ) );

    check ( "a zero dataSize is rejected",
            ( uint8_t ) ( hc595Init ( &driver, data, 0, HC595_DLY_NO, 0,
                            sckWrite, rckWrite, datWrite, NULL, NULL ) == FALSE ) );
}

/* --------------------------------------------------- one mode at a time */

static void modeExclusionCase ( void )
{
    hc595_t driver;
    uint8_t data[ CHAIN_SIZE ];
    uint32_t steps = 0;

    data[ 0 ] = 0x11;
    data[ 1 ] = 0x22;
    data[ 2 ] = 0x33;

    printf ( "mode exclusion\n" );

    check ( "Init succeeds",
            hc595Init ( &driver, data, CHAIN_SIZE, HC595_DLY_NO, 0,
                        sckWrite, rckWrite, datWrite, NULL, NULL ) );

    /* A stepped transfer has to lock the blocking one out. */
    check ( "Start arms", hc595Start ( &driver ) );
    check ( "OneShot is refused while a stepped transfer runs",
            ( uint8_t ) ( hc595OneShot ( &driver ) == FALSE ) );

    while ( ( hc595GetState ( &driver ) == HC595_BUSY ) && ( steps < MAX_EVENTS ) )
    {
        hc595Interrupt ( &driver );
        ++steps;
    }

    /* And the blocking one has to lock the stepped mode out. The probe runs
       from inside a pin callback, which is the only code that executes while
       OneShot still holds the driver. */
    resetCapture ( );
    probeDriver = &driver;
    probeArmed = TRUE;
    probeFired = FALSE;

    check ( "OneShot runs when the driver is free", hc595OneShot ( &driver ) );

    probeArmed = FALSE;

    check ( "the probe ran", probeFired );
    check ( "state is HC595_BLOCKING during OneShot", probeSawBlocking );
    check ( "Start is refused during OneShot", probeStartRefused );
    check ( "Interrupt drives no pin during OneShot", probeInterruptInert );
    check ( "state is HC595_DONE after OneShot",
            ( uint8_t ) ( hc595GetState ( &driver ) == HC595_DONE ) );
    check ( "Start is accepted again once OneShot has finished",
            hc595Start ( &driver ) );
}

int main ( void )
{
    hc595Case ( );
    printf ( "\n" );
    hc597Case ( );
    printf ( "\n" );
    modeExclusionCase ( );
    printf ( "\n" );
    initRejectionCase ( );

    printf ( "\n" );

    if ( eventOverflow == TRUE )
    {
        printf ( "capture buffer overflowed, raise MAX_EVENTS\n" );
        ++failures;
    }
    else
    {
        /* Intentionally blank. */
    }

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
