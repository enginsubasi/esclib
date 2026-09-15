/*
 * Covers fsm.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The machine under test is a small run/pause/fault sequencer, which is the
 * shape this module is really for. The interesting checks are not that it
 * moves between states — that is one comparison — but the three decisions the
 * module makes around that: an unaccepted event is counted rather than
 * ignored, the state changes before the action runs, and the first matching
 * row wins.
 */

#include <stddef.h>
#include <stdio.h>

#include "fsm.h"

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

/* ------------------------------------------------------- the machine */

#define ST_IDLE     0u
#define ST_RUNNING  1u
#define ST_PAUSED   2u
#define ST_FAULT    3u

#define EV_START    10u
#define EV_STOP     11u
#define EV_PAUSE    12u
#define EV_RESUME   13u
#define EV_FAIL     14u
#define EV_RECOVER  15u

/*
 * The driver the actions below reach back into. A task callback in this
 * library carries no context pointer — hc595's pin drivers and comat's packet
 * handler do not either — so an action that needs the machine finds it the
 * same way a real caller would, through a file scope pointer.
 */
static fsm_t* activeDriver = NULL;

static uint32_t startCalls = 0;
static uint32_t pauseCalls = 0;
static uint32_t faultCalls = 0;

/* What fsmGetState reported from inside the action. */
static uint8_t stateSeenInAction = 0xFFu;

static void actionStart ( void )
{
    ++startCalls;
    stateSeenInAction = fsmGetState ( activeDriver );
}

static void actionPause ( void )
{
    ++pauseCalls;
}

/* Dispatches a follow up event from inside the transition it is part of. */
static void actionFaultAndRecover ( void )
{
    ++faultCalls;
    ( void ) fsmDispatch ( activeDriver, EV_RECOVER );
}

static void probeReset ( void )
{
    startCalls = 0;
    pauseCalls = 0;
    faultCalls = 0;
    stateSeenInAction = 0xFFu;
}

static const fsmTransition_t machine[ 7 ] =
{
    { ST_IDLE,    EV_START,  ST_RUNNING, actionStart },
    { ST_RUNNING, EV_PAUSE,  ST_PAUSED,  actionPause },
    { ST_PAUSED,  EV_RESUME, ST_RUNNING, NULL },
    { ST_RUNNING, EV_STOP,   ST_IDLE,    NULL },
    { ST_PAUSED,  EV_STOP,   ST_IDLE,    NULL },
    { ST_RUNNING, EV_FAIL,   ST_FAULT,   NULL },
    { ST_FAULT,   EV_RECOVER, ST_IDLE,   NULL }
};

/* The same machine, but the fault transition dispatches its own follow up. */
static const fsmTransition_t nested[ 3 ] =
{
    { ST_IDLE,    EV_FAIL,    ST_FAULT,  actionFaultAndRecover },
    { ST_FAULT,   EV_RECOVER, ST_IDLE,   NULL },
    { ST_IDLE,    EV_START,   ST_RUNNING, NULL }
};

/* Two rows for the same state and event, going to different places. */
static const fsmTransition_t duplicate[ 2 ] =
{
    { ST_IDLE, EV_START, ST_RUNNING, NULL },
    { ST_IDLE, EV_START, ST_FAULT,   NULL }
};

/*
 * A chain: the second row matches the state the first row moves to, on the
 * same event. One dispatch must fire one transition, so this has to stop at
 * ST_RUNNING rather than falling through to ST_FAULT.
 */
static const fsmTransition_t chain[ 2 ] =
{
    { ST_IDLE,    EV_START, ST_RUNNING, NULL },
    { ST_RUNNING, EV_START, ST_FAULT,   NULL }
};

/* ---------------------------------------------------------------- init */

static void initCase ( void )
{
    fsm_t driver;

    printf ( "fsmInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( fsmInit ( NULL, machine, 7u, ST_IDLE ) == FALSE ) );
    check ( "a NULL table is rejected",
            ( uint8_t ) ( fsmInit ( &driver, NULL, 7u, ST_IDLE ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( fsmInit ( &driver, machine, 0u, ST_IDLE ) == FALSE ) );

    check ( "a full init succeeds",
            fsmInit ( &driver, machine, 7u, ST_IDLE ) );
    check ( "and starts in the state it was given",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_IDLE ) );
    check ( "with nothing rejected yet",
            ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 0u ) );

    /*
     * The initial state does not have to appear in the table. A machine that
     * starts somewhere with no outgoing row never moves, which is strange but
     * not wrong, and rejecting it would mean walking the table at Init.
     */
    check ( "a start state absent from the table is accepted",
            fsmInit ( &driver, machine, 7u, 99u ) );
    check ( "and the machine simply rejects everything from there",
            ( uint8_t ) ( fsmDispatch ( &driver, EV_START ) == FALSE ) );
}

/* ------------------------------------------------------------ dispatch */

static void dispatchCase ( void )
{
    fsm_t driver;

    printf ( "fsmDispatch\n" );

    check ( "Init", fsmInit ( &driver, machine, 7u, ST_IDLE ) );
    activeDriver = &driver;
    probeReset ( );

    /*
     * An event that means nothing in the current state is rejected and
     * counted, not ignored. Some rejections are ordinary — a button pressed
     * where it does nothing — so this is a count rather than a fault, but a
     * rising count where the caller expected none says the table is missing a
     * row.
     */
    check ( "stopping while idle is not accepted",
            ( uint8_t ) ( fsmDispatch ( &driver, EV_STOP ) == FALSE ) );
    check ( "the state did not move",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_IDLE ) );
    check ( "and the rejection was counted",
            ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 1u ) );
    check ( "and no action ran", ( uint8_t ) ( startCalls == 0u ) );

    check ( "an event no row mentions at all is rejected too",
            ( uint8_t ) ( fsmDispatch ( &driver, 200u ) == FALSE ) );
    check ( "and counted", ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 2u ) );

    check ( "starting from idle is accepted",
            fsmDispatch ( &driver, EV_START ) );
    check ( "the state moved", ( uint8_t ) ( fsmGetState ( &driver ) == ST_RUNNING ) );
    check ( "the action ran once", ( uint8_t ) ( startCalls == 1u ) );
    check ( "and the reject count did not move",
            ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 2u ) );

    /*
     * The state is written before the action is called, so an action that
     * looks at the machine sees where it has arrived rather than where it came
     * from.
     */
    check ( "the action saw the new state, not the old one",
            ( uint8_t ) ( stateSeenInAction == ST_RUNNING ) );

    check ( "starting again while running is not accepted",
            ( uint8_t ) ( fsmDispatch ( &driver, EV_START ) == FALSE ) );
    check ( "and is counted", ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 3u ) );
    check ( "and the action did not run a second time",
            ( uint8_t ) ( startCalls == 1u ) );

    check ( "pausing", fsmDispatch ( &driver, EV_PAUSE ) );
    check ( "moves to paused",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_PAUSED ) );
    check ( "and runs its own action", ( uint8_t ) ( pauseCalls == 1u ) );

    /* A row with no action is a transition that only changes state. */
    check ( "resuming", fsmDispatch ( &driver, EV_RESUME ) );
    check ( "moves back to running",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_RUNNING ) );
    check ( "a row with a NULL action changes state and calls nothing",
            ( uint8_t ) ( ( startCalls == 1u ) && ( pauseCalls == 1u ) ) );

    check ( "stopping from running", fsmDispatch ( &driver, EV_STOP ) );
    check ( "returns to idle",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_IDLE ) );
}

/* ------------------------------------------------------ nested dispatch */

static void nestedCase ( void )
{
    fsm_t driver;

    printf ( "an action that dispatches\n" );

    check ( "Init", fsmInit ( &driver, nested, 3u, ST_IDLE ) );
    activeDriver = &driver;
    probeReset ( );

    /*
     * The fault action dispatches a recover event from inside the transition
     * it is part of. Because the state was written before the action ran, that
     * nested event is offered from ST_FAULT and finds its row; had the state
     * still been ST_IDLE it would have been rejected and the machine would
     * have been left in the fault state.
     */
    check ( "failing from idle is accepted", fsmDispatch ( &driver, EV_FAIL ) );
    check ( "the action ran", ( uint8_t ) ( faultCalls == 1u ) );
    check ( "and its nested recover transitioned out of the fault state",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_IDLE ) );
    check ( "with nothing rejected along the way",
            ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 0u ) );
}

/* ----------------------------------------------------------- duplicates */

static void duplicateCase ( void )
{
    fsm_t driver;

    printf ( "duplicate rows\n" );

    /*
     * The search is a linear scan and the first matching row wins, so a second
     * row for the same state and event is unreachable rather than an error.
     * That is the caller's to notice; saying which one wins is this module's
     * job.
     */
    check ( "Init", fsmInit ( &driver, duplicate, 2u, ST_IDLE ) );

    check ( "the event is accepted", fsmDispatch ( &driver, EV_START ) );
    check ( "and the first matching row is the one that fired",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_RUNNING ) );

    /*
     * The table above cannot tell whether the scan stopped, because the state
     * changes inside the loop and the second row no longer matches once it
     * has. This one can: its second row matches the state the first row moves
     * to, so a scan that ran on would fire both and land in ST_FAULT. One
     * dispatch is one transition.
     */
    check ( "Init a chained table", fsmInit ( &driver, chain, 2u, ST_IDLE ) );

    check ( "the event is accepted", fsmDispatch ( &driver, EV_START ) );
    check ( "and the scan stopped at the row it matched",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_RUNNING ) );

    check ( "offering it again takes the second row, one transition at a time",
            fsmDispatch ( &driver, EV_START ) );
    check ( "and lands in the fault state",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_FAULT ) );
}

/* --------------------------------------------------------------- reset */

static void resetCase ( void )
{
    fsm_t driver;

    printf ( "fsmReset\n" );

    check ( "Init in the running state",
            fsmInit ( &driver, machine, 7u, ST_RUNNING ) );
    activeDriver = &driver;
    probeReset ( );

    check ( "failing", fsmDispatch ( &driver, EV_FAIL ) );
    check ( "lands in the fault state",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_FAULT ) );

    check ( "an event the fault state does not take is rejected",
            ( uint8_t ) ( fsmDispatch ( &driver, EV_PAUSE ) == FALSE ) );
    check ( "and counted", ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 1u ) );

    fsmReset ( &driver );

    check ( "reset returns to the state Init was given, not to zero",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_RUNNING ) );

    /*
     * The reject count survives a reset. It is a record of the whole run, and
     * a reset is part of what it is recording.
     */
    check ( "and the reject count survives it",
            ( uint8_t ) ( fsmGetRejectCount ( &driver ) == 1u ) );

    check ( "the machine works again from there",
            fsmDispatch ( &driver, EV_PAUSE ) );
    check ( "and is in the paused state",
            ( uint8_t ) ( fsmGetState ( &driver ) == ST_PAUSED ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    dispatchCase ( );
    printf ( "\n" );
    nestedCase ( );
    printf ( "\n" );
    duplicateCase ( );
    printf ( "\n" );
    resetCase ( );

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
