/*
 * Quadrature encoder module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * Two checks are pinned.
 *
 * invalidCase pins the transition where both channels changed at once. A step
 * was missed, so the direction is genuinely unknown, and guessing it is what
 * makes a noisy line accumulate silent position error. The position must not
 * move and the event must be counted.
 *
 * initCase pins the initial channel state. encoderInit takes the levels the
 * pins are sitting at, and without them the first encoderUpdate would see a
 * transition out of a default state that never happened and count it.
 *
 * The state is ( A << 1 ) | B, so the forward Gray sequence 00 01 11 10 reads
 * 0 1 3 2 and each of its four transitions is one count.
 */

#include <stdio.h>
#include <stddef.h>

#include "encoder.h"

#define SENTINEL8   0xA5u
#define SENTINEL32  0x5A5A5A5A

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
 * cannot half initialize an encoder by ignoring the return value.
 */
static void fillSentinel ( encoder_t* driver )
{
    driver->state = SENTINEL8;
    driver->position = SENTINEL32;
    driver->direction = -86;
    driver->errorCount = SENTINEL32;
}

static uint8_t isSentinel ( const encoder_t* const driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->state == SENTINEL8 ) &&
         ( driver->position == SENTINEL32 ) &&
         ( driver->direction == -86 ) &&
         ( driver->errorCount == ( uint32_t ) SENTINEL32 ) )
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
 * Walks the forward Gray sequence for the given number of whole cycles,
 * starting from and ending at 00. Each cycle is four transitions.
 */
static void driveForward ( encoder_t* driver, uint32_t cycles )
{
    uint32_t i = 0;

    for ( i = 0; i < cycles; ++i )
    {
        encoderUpdate ( driver, 0u, 1u );
        encoderUpdate ( driver, 1u, 1u );
        encoderUpdate ( driver, 1u, 0u );
        encoderUpdate ( driver, 0u, 0u );
    }
}

static void driveBackward ( encoder_t* driver, uint32_t cycles )
{
    uint32_t i = 0;

    for ( i = 0; i < cycles; ++i )
    {
        encoderUpdate ( driver, 1u, 0u );
        encoderUpdate ( driver, 1u, 1u );
        encoderUpdate ( driver, 0u, 1u );
        encoderUpdate ( driver, 0u, 0u );
    }
}

/* ------------------------------------------------------------- Init guards */

static void initCase ( void )
{
    encoder_t driver;

    printf ( "encoder Init\n" );

    fillSentinel ( &driver );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( encoderInit ( NULL, 0u, 0u ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a well formed init is accepted", encoderInit ( &driver, 0u, 0u ) );
    check ( "the position starts at zero",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );
    check ( "the direction starts stopped",
            ( uint8_t ) ( encoderGetDirection ( &driver ) == ( int8_t ) ENC_STOPPED ) );
    check ( "no errors have been seen",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );

    /*
     * The pinned check. Init captures the levels the pins are sitting at, so
     * an update repeating them is not a transition. Without that capture the
     * driver would start from 00 and this update would look like a step.
     */
    check ( "init at a non zero pin state", encoderInit ( &driver, 1u, 1u ) );
    encoderUpdate ( &driver, 1u, 1u );
    check ( "repeating the initial pin state is not a step",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );
    check ( "and is not an error either",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );

    /*
     * A real GPIO read is a masked register value, not a clean 1. Anything
     * non zero has to mean the same thing or the state index goes wrong.
     */
    check ( "init with the pin levels as raw masked bits",
            encoderInit ( &driver, 0x80u, 0x04u ) );
    encoderUpdate ( &driver, 1u, 1u );
    check ( "which the update reads as the same state",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );
    check ( "and does not invent a diagonal out of it",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );

    /*
     * The update side has to read a masked value the same way Init does.
     * Dropping A from 0x80 to zero is the single transition 3 to 2, which is
     * one count forward. Reading 0x80 as low instead would make it a
     * diagonal.
     */
    encoderUpdate ( &driver, 0x80u, 0x00u );
    check ( "and the update takes masked bits too",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 1 ) );
    check ( "still with no error",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );
}

/* ------------------------------------------------------------- the counting */

static void forwardCase ( void )
{
    encoder_t driver;

    printf ( "encoder counting forward\n" );

    check ( "init", encoderInit ( &driver, 0u, 0u ) );

    encoderUpdate ( &driver, 0u, 1u );
    check ( "the first transition is one count",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 1 ) );
    check ( "and reports forward",
            ( uint8_t ) ( encoderGetDirection ( &driver ) == ( int8_t ) ENC_FORWARD ) );

    encoderUpdate ( &driver, 1u, 1u );
    encoderUpdate ( &driver, 1u, 0u );
    encoderUpdate ( &driver, 0u, 0u );
    check ( "a whole cycle is four counts, not one",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 4 ) );

    driveForward ( &driver, 99u );
    check ( "a hundred cycles are four hundred counts",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 400 ) );
    check ( "with no errors",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );
}

static void backwardCase ( void )
{
    encoder_t driver;

    printf ( "encoder counting backward\n" );

    check ( "init", encoderInit ( &driver, 0u, 0u ) );

    encoderUpdate ( &driver, 1u, 0u );
    check ( "the first transition counts down",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == -1 ) );
    check ( "and reports backward",
            ( uint8_t ) ( encoderGetDirection ( &driver ) == ( int8_t ) ENC_BACKWARD ) );

    encoderUpdate ( &driver, 1u, 1u );
    encoderUpdate ( &driver, 0u, 1u );
    encoderUpdate ( &driver, 0u, 0u );
    check ( "a whole cycle backward is minus four",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == -4 ) );

    driveBackward ( &driver, 24u );
    check ( "twenty five cycles back are minus one hundred",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == -100 ) );

    /* The same distance travelled the other way must return to where it was. */
    driveForward ( &driver, 25u );
    check ( "driving back over the same ground returns to zero",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );
    check ( "with no errors",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );
}

static void steadyCase ( void )
{
    encoder_t driver;
    uint32_t i = 0;

    printf ( "encoder standing still\n" );

    check ( "init", encoderInit ( &driver, 0u, 1u ) );

    for ( i = 0; i < 50u; ++i )
    {
        encoderUpdate ( &driver, 0u, 1u );
    }

    check ( "an unchanged pin state never counts",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );
    check ( "and is never an error",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 0u ) );
    check ( "and the direction stays stopped",
            ( uint8_t ) ( encoderGetDirection ( &driver ) == ( int8_t ) ENC_STOPPED ) );
}

/* ------------------------------------------------------- the pinned failure */

/*
 * The pinned regression. Both channels changing between two samples means a
 * step was missed, so which way it went is unknowable. A table that answered
 * plus or minus two here would look right on a clean signal and quietly drift
 * on a noisy one, which is the worst failure an encoder can have.
 */
static void invalidCase ( void )
{
    encoder_t driver;

    printf ( "encoder on a missed step\n" );

    check ( "init", encoderInit ( &driver, 0u, 0u ) );

    encoderUpdate ( &driver, 1u, 1u );
    check ( "both channels changing at once does not move the position",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );
    check ( "and is counted",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 1u ) );
    check ( "and leaves the direction alone",
            ( uint8_t ) ( encoderGetDirection ( &driver ) == ( int8_t ) ENC_STOPPED ) );

    /*
     * The driver still adopts the new pin state, so it resynchronises rather
     * than reporting an error on every sample from here on.
     */
    encoderUpdate ( &driver, 1u, 0u );
    check ( "it picks up again from the new state",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 1 ) );
    check ( "with no further error",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 1u ) );

    encoderUpdate ( &driver, 0u, 1u );
    check ( "the other diagonal is caught too",
            ( uint8_t ) ( encoderGetErrorCount ( &driver ) == 2u ) );
    check ( "and it did not move either",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 1 ) );
}

/* ------------------------------------------------------------- the position */

static void setPositionCase ( void )
{
    encoder_t driver;

    printf ( "encoder position override\n" );

    check ( "init", encoderInit ( &driver, 0u, 0u ) );

    driveForward ( &driver, 3u );
    check ( "three cycles are twelve counts",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 12 ) );

    encoderSetPosition ( &driver, 0 );
    check ( "homing sets the count without disturbing the pin state",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 0 ) );

    encoderUpdate ( &driver, 0u, 1u );
    check ( "and counting carries on from there",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == 1 ) );

    encoderSetPosition ( &driver, -1000 );
    check ( "a negative origin is fine",
            ( uint8_t ) ( encoderGetPosition ( &driver ) == -1000 ) );
}

int main ( void )
{
    initCase ( );

    printf ( "\n" );
    forwardCase ( );
    printf ( "\n" );
    backwardCase ( );
    printf ( "\n" );
    steadyCase ( );

    printf ( "\n" );
    invalidCase ( );
    printf ( "\n" );
    setPositionCase ( );

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
