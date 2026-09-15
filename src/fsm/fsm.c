/**
  ******************************************************************************
  *
  * @file      fsm.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Table driven finite state machine.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      comat and comstxetx are both hand written state machines, and so
  *            is the charger, the menu and the startup sequence in every
  *            project that uses this library. This is the general one: the
  *            caller writes the transitions as data and this walks them.
  *
  * @note      The table is a pointer to const and belongs in flash. A machine
  *            written as a switch inside a switch costs the same flash and
  *            cannot be read at a glance; written as rows, the whole behaviour
  *            of the machine is one table a reviewer can check against a
  *            drawing.
  *
  * @note      An event with no transition out of the current state is
  *            **rejected and counted**, not ignored and not a fault. Some
  *            rejections are normal — a button pressed in a state that does
  *            not use it — so this does not stop; but a rising count where the
  *            caller expected none is the signal that the table is missing a
  *            row, and that is worth being able to see. comstxetxGetRejectCount
  *            and encoderGetErrorCount are the same idea.
  *
  * @note      The search is a linear scan and the first matching row wins. A
  *            table small enough to read is small enough to scan, and any
  *            index over it would have to be built at boot from the same rows.
  *            A duplicate row is therefore not an error; it is unreachable,
  *            which is the caller's to notice.
  *
  * @note      The state changes **before** the action runs. An action that
  *            reads fsmGetState sees where it has arrived rather than where it
  *            came from, and an action that dispatches a follow up event
  *            transitions out of the new state, which is what a nested event
  *            means. Doing it the other way round makes a self dispatching
  *            action loop back into the transition it is already inside.
  *
  * @note      An action may be NULL, which is a transition that only changes
  *            state. That is a legitimate row rather than a caller mistake, so
  *            fsmDispatch checks the pointer before calling it — the one place
  *            in this module that checks anything per call, and it is checking
  *            an optional value, not guarding against a bad argument.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "fsm.h"

/**
 * @brief   Initializes the machine over a caller owned transition table.
 * @param[out] driver        Machine state to initialize.
 * @param[in]  table         Transitions. Belongs in flash; never written.
 * @param[in]  length        Number of rows.
 * @param[in]  initialState  State the machine starts in, and returns to on
 *                           fsmReset.
 * @return  TRUE on success, FALSE when driver or table is NULL, or when length
 *          is zero.
 * @note    Nothing here checks that the table describes a sensible machine.
 *          Reachability and completeness are properties of the whole graph,
 *          not of any row, and a caller writing transitions is assumed to have
 *          drawn them — the position biquadInit and firInit take about their
 *          coefficients. fsmGetRejectCount is what surfaces a missing row at
 *          runtime.
 * @note    The initial state does not have to appear in the table. A machine
 *          whose start state has no outgoing row is a machine that never moves,
 *          which is strange but not wrong, and rejecting it would mean walking
 *          the table at Init to prove a property the caller may be building up
 *          to.
 */
uint8_t fsmInit ( fsm_t* driver, const fsmTransition_t* const table, uint32_t length, uint8_t initialState )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( table != NULL ) && ( length != 0 ) )
    {
        driver->table = table;
        driver->length = length;
        driver->state = initialState;
        driver->initialState = initialState;
        driver->rejectCount = 0;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Offers one event to the machine.
 * @param[in,out] driver  Initialized machine.
 * @param[in]     event   Event to offer.
 * @return  TRUE when a transition fired, FALSE when no row matched the current
 *          state and this event.
 * @note    On FALSE the state is unchanged and the reject count has gone up by
 *          one. That is not a failure by itself: an event that means nothing in
 *          the current state is ordinary. It is a count rather than silence so
 *          that a caller who expected every event to land can tell.
 * @note    The state is written before the action is called, for the reason the
 *          file banner gives. A single break leaves the loop once a row has
 *          matched, which is also what makes the first matching row the one
 *          that wins.
 */
uint8_t fsmDispatch ( fsm_t* driver, uint8_t event )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    void ( *action )( void ) = NULL;

    for ( i = 0; i < driver->length; ++i )
    {
        if ( ( driver->table[ i ].from == driver->state ) &&
                ( driver->table[ i ].event == event ) )
        {
            driver->state = driver->table[ i ].to;

            action = driver->table[ i ].action;

            retVal = TRUE;

            break;
        }
    }

    if ( retVal == TRUE )
    {
        if ( action != NULL )
        {
            action ( );
        }
        else
        {
            /* Intentionally blank */
        }
    }
    else
    {
        ++driver->rejectCount;
    }

    return ( retVal );
}

/**
 * @brief   Gets the current state.
 * @param[in] driver  Machine state.
 * @return  The state the machine is in.
 */
uint8_t fsmGetState ( const fsm_t* const driver )
{
    return ( driver->state );
}

/**
 * @brief   Gets the number of events rejected since Init.
 * @param[in] driver  Machine state.
 * @return  Events offered that no row accepted in the state they arrived in.
 * @note    Not cleared by reading it, unlike bininpGetRisingValue. This is a
 *          running total rather than an event flag: the useful question is
 *          whether it moved over a run, not whether it moved since the last
 *          look.
 */
uint32_t fsmGetRejectCount ( const fsm_t* const driver )
{
    return ( driver->rejectCount );
}

/**
 * @brief   Returns the machine to the state it was initialized in.
 * @param[in,out] driver  Initialized machine.
 * @note    For a fault handler that has to get back to somewhere known. The
 *          table and the reject count are untouched, because the count is a
 *          record of the whole run and a reset is part of what it is recording.
 * @note    This is the only way to move the machine without an event, and it
 *          goes to one fixed state rather than to any state on request. A
 *          general setter would let a caller step around the table, which is
 *          the one thing a table driven machine exists to prevent.
 */
void fsmReset ( fsm_t* driver )
{
    driver->state = driver->initialState;
}
