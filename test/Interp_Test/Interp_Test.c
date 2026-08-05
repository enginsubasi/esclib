/*
 * Table interpolation module test.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero when any check fails.
 *
 * initCase pins the regression this module is most exposed to: a table with
 * two equal x entries. Init must refuse it, because the interpolation divides
 * by the difference between two neighbouring x values. If that reached the
 * arithmetic, interpCalculatei32 would divide by zero and interpCalculate
 * would hand the caller an inf or a nan with nothing downstream to limit it.
 * Every other case in this file passes with that check removed.
 */

#include <stdio.h>
#include <stddef.h>

#include "interp.h"

#define SENTINEL32  0xA5A5A5A5u

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
 * The float variants are compared with a tolerance rather than for equality,
 * the same way ArrayMatrix_Test and ComplexMath_Test do it.
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
 * cannot half initialize an interpolator by ignoring the return value.
 *
 * The sentinel for the two pointers is the address of a real array rather
 * than an integer cast to a pointer. A cast would either warn on a 64 bit
 * host or need a width the library does not otherwise use.
 */
static const float sentinelTable[ 2 ] = { 0.0f, 0.0f };
static const int32_t sentinelTablei32[ 2 ] = { 0, 0 };

static void fillSentinel ( interp_t* driver )
{
    driver->xTable = sentinelTable;
    driver->yTable = sentinelTable;
    driver->length = SENTINEL32;
}

static uint8_t isSentinel ( const interp_t* const driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->xTable == sentinelTable ) &&
         ( driver->yTable == sentinelTable ) &&
         ( driver->length == SENTINEL32 ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

static void fillSentineli32 ( interpi32_t* driver )
{
    driver->xTable = sentinelTablei32;
    driver->yTable = sentinelTablei32;
    driver->length = SENTINEL32;
}

static uint8_t isSentineli32 ( const interpi32_t* const driver )
{
    uint8_t retVal = FALSE;

    if ( ( driver->xTable == sentinelTablei32 ) &&
         ( driver->yTable == sentinelTablei32 ) &&
         ( driver->length == SENTINEL32 ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------------- the tables */

/*
 * An NTC shaped curve: x ascends, y descends. The descent is what catches a
 * sign error in the slope, and a table is allowed to look like this because
 * only x has to ascend.
 */
static const float fallingX[ 4 ] = {   0.0f, 10.0f, 20.0f, 40.0f };
static const float fallingY[ 4 ] = { 100.0f, 80.0f, 50.0f,  0.0f };

/* ------------------------------------------------------------- Init guards */

static void initCase ( void )
{
    interp_t driver;
    interpi32_t driveri32;
    static const float descending[ 3 ] = { 10.0f, 5.0f, 0.0f };
    static const float duplicate[ 4 ] = { 0.0f, 10.0f, 10.0f, 20.0f };
    static const int32_t duplicatei32[ 4 ] = { 0, 10, 10, 20 };
    static const float yAny[ 4 ] = { 0.0f, 1.0f, 2.0f, 3.0f };
    static const int32_t yAnyi32[ 4 ] = { 0, 1, 2, 3 };

    printf ( "interp Init guards\n" );

    fillSentinel ( &driver );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( interpInit ( NULL, fallingX, fallingY, 4u ) == FALSE ) );

    check ( "a NULL x table is rejected",
            ( uint8_t ) ( interpInit ( &driver, NULL, fallingY, 4u ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a NULL y table is rejected",
            ( uint8_t ) ( interpInit ( &driver, fallingX, NULL, 4u ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a length of zero is rejected",
            ( uint8_t ) ( interpInit ( &driver, fallingX, fallingY, 0u ) == FALSE ) );
    check ( "a length of one is rejected, one point spans no interval",
            ( uint8_t ) ( interpInit ( &driver, fallingX, fallingY, 1u ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a descending x table is rejected",
            ( uint8_t ) ( interpInit ( &driver, descending, yAny, 3u ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    /*
     * The pinned regression. A repeated x makes the divisor zero, and nothing
     * downstream of the division would catch it.
     */
    check ( "a duplicated x is rejected, it would zero the divisor",
            ( uint8_t ) ( interpInit ( &driver, duplicate, yAny, 4u ) == FALSE ) );
    check ( "and the driver was left alone", isSentinel ( &driver ) );

    check ( "a two entry table is the shortest accepted",
            interpInit ( &driver, fallingX, fallingY, 2u ) );
    check ( "a well formed table is accepted",
            interpInit ( &driver, fallingX, fallingY, 4u ) );
    check ( "and the x table was stored",
            ( uint8_t ) ( driver.xTable == fallingX ) );
    check ( "and the y table was stored",
            ( uint8_t ) ( driver.yTable == fallingY ) );
    check ( "and the length was stored", ( uint8_t ) ( driver.length == 4u ) );

    fillSentineli32 ( &driveri32 );

    check ( "the i32 Init rejects a NULL driver",
            ( uint8_t ) ( interpIniti32 ( NULL, duplicatei32, yAnyi32, 4u ) == FALSE ) );
    check ( "the i32 Init rejects a duplicated x",
            ( uint8_t ) ( interpIniti32 ( &driveri32, duplicatei32, yAnyi32, 4u ) == FALSE ) );
    check ( "and the i32 driver was left alone", isSentineli32 ( &driveri32 ) );
}

/* ---------------------------------------------------- float compute path */

/*
 * At every table x the answer must be that x's own y exactly, including both
 * ends where the clamp takes over from the interpolation.
 */
static void nodeCase ( void )
{
    interp_t driver;

    printf ( "interp at the table nodes\n" );

    check ( "init", interpInit ( &driver, fallingX, fallingY, 4u ) );

    check ( "the first node", nearly ( interpCalculate ( &driver, 0.0f ), 100.0f ) );
    check ( "an interior node", nearly ( interpCalculate ( &driver, 10.0f ), 80.0f ) );
    check ( "the other interior node", nearly ( interpCalculate ( &driver, 20.0f ), 50.0f ) );
    check ( "the last node", nearly ( interpCalculate ( &driver, 40.0f ), 0.0f ) );
}

/*
 * Halfway between two nodes the answer is the mean of their y values, which
 * holds whichever way y runs. The slope here is negative throughout, so a
 * sign error in the interpolation shows up immediately.
 */
static void betweenCase ( void )
{
    interp_t driver;
    static const float risingX[ 3 ] = { -10.0f, 0.0f, 10.0f };
    static const float risingY[ 3 ] = {  -1.0f, 0.0f,  3.0f };

    printf ( "interp between the nodes\n" );

    check ( "init", interpInit ( &driver, fallingX, fallingY, 4u ) );

    check ( "halfway across the first interval is the mean of its ends",
            nearly ( interpCalculate ( &driver, 5.0f ), 90.0f ) );
    check ( "halfway across the last interval is the mean of its ends",
            nearly ( interpCalculate ( &driver, 30.0f ), 25.0f ) );
    check ( "a quarter of the way across the last interval",
            nearly ( interpCalculate ( &driver, 25.0f ), 37.5f ) );
    check ( "the interval widths differ and each is used on its own",
            nearly ( interpCalculate ( &driver, 15.0f ), 65.0f ) );

    check ( "init an ascending y table",
            interpInit ( &driver, risingX, risingY, 3u ) );

    check ( "a rising slope on the far interval",
            nearly ( interpCalculate ( &driver, 5.0f ), 1.5f ) );
    check ( "a rising slope on the near interval, across zero",
            nearly ( interpCalculate ( &driver, -5.0f ), -0.5f ) );
}

/*
 * Past either end the value is held at the end rather than extrapolated, and
 * interpInRange is the separate question of whether that happened. Clamping
 * is an answer here, not an error, which is why interpCalculate returns the
 * value directly and the range question is asked apart from it.
 */
static void clampCase ( void )
{
    interp_t driver;

    printf ( "interp clamping and range\n" );

    check ( "init", interpInit ( &driver, fallingX, fallingY, 4u ) );

    check ( "below the first x the first y is held",
            nearly ( interpCalculate ( &driver, -5.0f ), 100.0f ) );
    check ( "far below the first x the first y is still held",
            nearly ( interpCalculate ( &driver, -1000.0f ), 100.0f ) );
    check ( "above the last x the last y is held",
            nearly ( interpCalculate ( &driver, 100.0f ), 0.0f ) );

    check ( "the first x is in range",
            ( uint8_t ) ( interpInRange ( &driver, 0.0f ) == TRUE ) );
    check ( "the last x is in range, the endpoints are included",
            ( uint8_t ) ( interpInRange ( &driver, 40.0f ) == TRUE ) );
    check ( "an interior x is in range",
            ( uint8_t ) ( interpInRange ( &driver, 20.0f ) == TRUE ) );
    check ( "just below the first x is out of range",
            ( uint8_t ) ( interpInRange ( &driver, -0.1f ) == FALSE ) );
    check ( "just above the last x is out of range",
            ( uint8_t ) ( interpInRange ( &driver, 40.1f ) == FALSE ) );
}

int main ( void )
{
    initCase ( );

    printf ( "\n" );
    nodeCase ( );
    printf ( "\n" );
    betweenCase ( );
    printf ( "\n" );
    clampCase ( );

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
