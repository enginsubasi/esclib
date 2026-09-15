/*
 * Covers sched.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The whole module is counting and dispatching through injected callbacks, so
 * the tasks here record that they ran and every check reads that record. A
 * scheduler that dispatched nothing would look identical to a correct one if
 * only the return values were checked.
 */

#include <stddef.h>
#include <stdio.h>

#include "sched.h"

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

/* ------------------------------------------------------------- probes */

static uint32_t callsA = 0;
static uint32_t callsB = 0;
static uint32_t callsC = 0;

/* Order of the first call to each probe since the last reset. */
static uint32_t sequence = 0;
static uint32_t orderA = 0;
static uint32_t orderB = 0;

static void taskA ( void )
{
    ++callsA;
    ++sequence;

    if ( orderA == 0u )
    {
        orderA = sequence;
    }
    else
    {
        /* Intentionally blank. */
    }
}

static void taskB ( void )
{
    ++callsB;
    ++sequence;

    if ( orderB == 0u )
    {
        orderB = sequence;
    }
    else
    {
        /* Intentionally blank. */
    }
}

static void taskC ( void )
{
    ++callsC;
}

static void probeReset ( void )
{
    callsA = 0;
    callsB = 0;
    callsC = 0;
    sequence = 0;
    orderA = 0;
    orderB = 0;
}

/* Runs the given number of ticks, running the loop after each one. */
static void tickAndRun ( sched_t* driver, uint32_t ticks )
{
    uint32_t i = 0;

    for ( i = 0; i < ticks; ++i )
    {
        schedTick ( driver );
        ( void ) schedRun ( driver );
    }
}

/* ---------------------------------------------------------------- init */

static void initCase ( void )
{
    sched_t driver;
    schedTask_t slots[ 3 ];
    uint32_t index = 0xAAAAAAAAu;

    printf ( "schedInit and schedAddTask\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( schedInit ( NULL, slots, 3u ) == FALSE ) );
    check ( "a NULL table is rejected",
            ( uint8_t ) ( schedInit ( &driver, NULL, 3u ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( schedInit ( &driver, slots, 0u ) == FALSE ) );
    check ( "a full init succeeds", schedInit ( &driver, slots, 3u ) );

    check ( "and starts with nothing to run",
            ( uint8_t ) ( schedRun ( &driver ) == 0u ) );
    check ( "and no overruns",
            ( uint8_t ) ( schedGetOverrunCount ( &driver ) == 0u ) );

    /*
     * The table is filled through schedAddTask rather than by the caller, so
     * that every callback and period is checked once. A NULL task in a table
     * the caller filled itself would not surface until the main loop called
     * it.
     */
    check ( "a NULL driver is rejected by add",
            ( uint8_t ) ( schedAddTask ( NULL, taskA, 5u, &index ) == FALSE ) );
    check ( "a NULL task is rejected",
            ( uint8_t ) ( schedAddTask ( &driver, NULL, 5u, &index ) == FALSE ) );
    check ( "a zero period is rejected",
            ( uint8_t ) ( schedAddTask ( &driver, taskA, 0u, &index ) == FALSE ) );

    check ( "the first add succeeds",
            schedAddTask ( &driver, taskA, 5u, &index ) );
    check ( "and reports slot zero", ( uint8_t ) ( index == 0u ) );

    check ( "the second", schedAddTask ( &driver, taskB, 3u, &index ) );
    check ( "and reports slot one", ( uint8_t ) ( index == 1u ) );

    check ( "the third", schedAddTask ( &driver, taskC, 2u, &index ) );
    check ( "a fourth does not fit",
            ( uint8_t ) ( schedAddTask ( &driver, taskA, 2u, &index ) == FALSE ) );
    check ( "and the rejected add left the index alone",
            ( uint8_t ) ( index == 2u ) );

    /* A caller that does not need the slot back may pass NULL for it. */
    check ( "re-init", schedInit ( &driver, slots, 3u ) );
    check ( "a NULL index is accepted",
            schedAddTask ( &driver, taskA, 5u, NULL ) );
}

/* -------------------------------------------------------------- period */

static void periodCase ( void )
{
    sched_t driver;
    schedTask_t slots[ 2 ];

    printf ( "periods\n" );

    check ( "Init", schedInit ( &driver, slots, 2u ) );
    check ( "add a task of period 5", schedAddTask ( &driver, taskA, 5u, NULL ) );

    probeReset ( );

    /*
     * The counter starts at zero and the first run is a full period after the
     * add, so four ticks are not enough.
     */
    tickAndRun ( &driver, 4u );
    check ( "four ticks of a period of five run nothing",
            ( uint8_t ) ( callsA == 0u ) );

    tickAndRun ( &driver, 1u );
    check ( "the fifth runs it once", ( uint8_t ) ( callsA == 1u ) );

    tickAndRun ( &driver, 4u );
    check ( "and the next four do not run it again",
            ( uint8_t ) ( callsA == 1u ) );

    tickAndRun ( &driver, 1u );
    check ( "the tenth does", ( uint8_t ) ( callsA == 2u ) );

    /*
     * The reload subtracts the period rather than clearing the counter, so the
     * phase is kept over a long run: a period of five in a hundred ticks is
     * exactly twenty runs, not nineteen and not twenty one.
     */
    check ( "re-init for a long run", schedInit ( &driver, slots, 2u ) );
    check ( "add a task of period 5 again",
            schedAddTask ( &driver, taskA, 5u, NULL ) );
    check ( "and one of period 3", schedAddTask ( &driver, taskB, 3u, NULL ) );

    probeReset ( );
    tickAndRun ( &driver, 99u );

    check ( "a period of five runs nineteen times in ninety nine ticks",
            ( uint8_t ) ( callsA == 19u ) );
    check ( "and a period of three runs thirty three times",
            ( uint8_t ) ( callsB == 33u ) );
    check ( "with no overruns, since the loop kept up",
            ( uint8_t ) ( schedGetOverrunCount ( &driver ) == 0u ) );

    /* Tasks run in the order they were added. */
    check ( "re-init for the order", schedInit ( &driver, slots, 2u ) );
    check ( "add A first", schedAddTask ( &driver, taskA, 2u, NULL ) );
    check ( "then B", schedAddTask ( &driver, taskB, 2u, NULL ) );

    probeReset ( );
    tickAndRun ( &driver, 2u );

    check ( "both ran", ( uint8_t ) ( ( callsA == 1u ) && ( callsB == 1u ) ) );
    check ( "and A ran before B, which is the order they were added",
            ( uint8_t ) ( ( orderA != 0u ) && ( orderB != 0u ) &&
                          ( orderA < orderB ) ) );
}

/* ------------------------------------------------------------- overrun */

static void overrunCase ( void )
{
    sched_t driver;
    schedTask_t slots[ 1 ];
    uint32_t ran = 0;

    printf ( "overruns\n" );

    check ( "Init", schedInit ( &driver, slots, 1u ) );
    check ( "add a task of period 2", schedAddTask ( &driver, taskA, 2u, NULL ) );

    probeReset ( );

    /*
     * Four ticks with no run between them. The task comes due twice and the
     * second time it is still due from the first, which is an overrun: the
     * main loop is not keeping up.
     */
    schedTick ( &driver );
    schedTick ( &driver );
    schedTick ( &driver );
    schedTick ( &driver );

    check ( "the task came due twice with no run between",
            ( uint8_t ) ( schedGetOverrunCount ( &driver ) == 1u ) );

    ran = schedRun ( &driver );

    check ( "and it runs once rather than twice, catching up being a guess",
            ( uint8_t ) ( ( callsA == 1u ) && ( ran == 1u ) ) );

    /*
     * The phase survives the overrun. Two more ticks and it is due again, on
     * the boundary it would have been on anyway.
     */
    probeReset ( );
    tickAndRun ( &driver, 2u );
    check ( "and the next run lands back on the original boundary",
            ( uint8_t ) ( callsA == 1u ) );

    check ( "the overrun count is still one",
            ( uint8_t ) ( schedGetOverrunCount ( &driver ) == 1u ) );

    /* schedRun reports how many it ran, which is what an idle loop waits on. */
    check ( "a run with nothing due reports zero",
            ( uint8_t ) ( schedRun ( &driver ) == 0u ) );
}

/* -------------------------------------------------------------- enable */

static void enableCase ( void )
{
    sched_t driver;
    schedTask_t slots[ 2 ];
    uint32_t indexA = 0;
    uint32_t indexB = 0;

    printf ( "enable and disable\n" );

    check ( "Init", schedInit ( &driver, slots, 2u ) );
    check ( "add A", schedAddTask ( &driver, taskA, 2u, &indexA ) );
    check ( "add B", schedAddTask ( &driver, taskB, 2u, &indexB ) );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( schedEnable ( NULL, indexA, FALSE ) == FALSE ) );
    check ( "an index past the last task is rejected",
            ( uint8_t ) ( schedEnable ( &driver, 2u, FALSE ) == FALSE ) );
    check ( "an index past the table is rejected too",
            ( uint8_t ) ( schedEnable ( &driver, 99u, FALSE ) == FALSE ) );

    check ( "disabling A succeeds", schedEnable ( &driver, indexA, FALSE ) );

    probeReset ( );
    tickAndRun ( &driver, 10u );

    check ( "a disabled task does not run", ( uint8_t ) ( callsA == 0u ) );
    check ( "and the enabled one still does", ( uint8_t ) ( callsB == 5u ) );

    /*
     * Not running is not enough: a disabled task must not be counted either.
     * schedRun checks the enabled flag too, so a tick that kept counting a
     * disabled task would still never call it — the damage shows up here
     * instead, as overruns piling up for a task nobody is waiting for.
     */
    check ( "and a disabled task accrues no overruns",
            ( uint8_t ) ( schedGetOverrunCount ( &driver ) == 0u ) );

    /*
     * Re-enabling restarts the period from now rather than resuming a part
     * counted one, and a due flag from before the disable does not survive.
     */
    check ( "re-enabling A succeeds", schedEnable ( &driver, indexA, TRUE ) );

    probeReset ( );
    tickAndRun ( &driver, 1u );
    check ( "one tick after re-enabling is not a full period",
            ( uint8_t ) ( callsA == 0u ) );

    tickAndRun ( &driver, 1u );
    check ( "the second tick is", ( uint8_t ) ( callsA == 1u ) );

    /*
     * Disabling a task that is already due drops the pending run, and the
     * check has to survive the round trip to see it: while the task is off,
     * schedRun's own enabled test hides a due flag that was left standing, so
     * a stale flag only shows when the task is switched back on.
     */
    check ( "re-init", schedInit ( &driver, slots, 2u ) );
    check ( "add A again", schedAddTask ( &driver, taskA, 2u, &indexA ) );

    probeReset ( );
    schedTick ( &driver );
    schedTick ( &driver );
    check ( "disabling drops a pending run",
            schedEnable ( &driver, indexA, FALSE ) );
    ( void ) schedRun ( &driver );
    check ( "so the task does not fire once more on the way out",
            ( uint8_t ) ( callsA == 0u ) );

    check ( "switching it back on succeeds",
            schedEnable ( &driver, indexA, TRUE ) );
    ( void ) schedRun ( &driver );
    check ( "and the run that was pending before the disable is gone for good",
            ( uint8_t ) ( callsA == 0u ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    periodCase ( );
    printf ( "\n" );
    overrunCase ( );
    printf ( "\n" );
    enableCase ( );

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
