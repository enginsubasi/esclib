/**
  ******************************************************************************
  *
  * @file      sched.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Cooperative table of periodic tasks, driven by a tick.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      softtimer is one timer. This is the table of them that every
  *            project with a ten millisecond sensor read, a hundred
  *            millisecond display update and a one second log ends up writing
  *            by hand. It does not include softtimer.h and keeps its own
  *            counters, because no module here includes another's header.
  *
  * @note      The tick and the run are separate calls, and that separation is
  *            the whole design. schedTick advances the counters from a fixed
  *            rate interrupt and marks a task due; schedRun executes the due
  *            ones from the main loop. A scheduler that called the tasks from
  *            inside the tick would be running a display update in an
  *            interrupt. comat draws the same line between comatReceive and
  *            comatEvaluate, and for the same reason.
  *
  * @note      Periods are in ticks, so the interrupt rate is the unit — the
  *            rule softtimer and hc595Interrupt already follow. Nothing here
  *            knows what a millisecond is.
  *
  * @note      A task still due when its period comes round again is an
  *            **overrun**: the main loop is not keeping up. The event is
  *            counted in schedGetOverrunCount rather than swallowed, the way
  *            encoderGetErrorCount counts a missed quadrature step. The task
  *            runs once rather than twice, because running it twice in a row
  *            to catch up is a guess about what the caller wanted; a non zero
  *            count is the signal to lengthen a period or shorten a task.
  *
  * @note      The reload subtracts the period rather than clearing the
  *            counter, so a tick that arrives while the main loop is busy
  *            costs the run but never the phase. softtimer reloads the same
  *            way and for the same reason.
  *
  * @note      This is cooperative and single threaded. A task runs to
  *            completion and the ones after it in the table wait, so a long
  *            task delays everything behind it. There is no preemption, no
  *            priority and no stack per task, which is what keeps this a table
  *            and a counter rather than an operating system.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "sched.h"

/**
 * @brief   Initializes the scheduler over a caller owned table.
 * @param[out] driver  Scheduler state to initialize.
 * @param[in]  tasks   Caller owned array of length entries.
 * @param[in]  length  Number of entries the array can hold.
 * @return  TRUE on success, FALSE when driver or tasks is NULL, or when
 *          length is zero.
 * @note    The table starts empty. Entries are filled by schedAddTask, which
 *          is where each callback and period is checked — a table the caller
 *          filled in itself could carry a NULL callback that nothing would
 *          catch until the tick fired.
 */
uint8_t schedInit ( sched_t* driver, schedTask_t* tasks, uint32_t length )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( tasks != NULL ) && ( length != 0 ) )
    {
        driver->tasks = tasks;
        driver->length = length;
        driver->count = 0;
        driver->overrunCount = 0;

        for ( i = 0; i < length; ++i )
        {
            driver->tasks[ i ].task = NULL;
            driver->tasks[ i ].period = 0;
            driver->tasks[ i ].counter = 0;
            driver->tasks[ i ].due = FALSE;
            driver->tasks[ i ].enabled = FALSE;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds a periodic task to the next free slot.
 * @param[in,out] driver  Initialized scheduler.
 * @param[in]     task    Function to call, from the main loop.
 * @param[in]     period  Ticks between calls. One means every tick.
 * @param[out]    index   Slot the task was placed in, for schedEnable. May be
 *                        NULL when the caller does not need it.
 * @return  TRUE on success, FALSE when driver or task is NULL, when period is
 *          zero, or when the table is full.
 * @note    This returns a status although it is not an Init, for the reason
 *          pidChangeCoefficients does: it installs a new argument that would
 *          break a later invariant. A NULL task would be called from schedRun
 *          without a check, and a zero period would make the counter comparison
 *          true on every tick forever.
 * @note    The task starts enabled and its counter starts at zero, so the
 *          first run is a full period after the add rather than immediately.
 *          A caller that wants it to run at once can call it once itself,
 *          which is clearer than a flag on this function.
 */
uint8_t schedAddTask ( sched_t* driver, void ( *task )( void ), uint32_t period, uint32_t* const index )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( task != NULL ) && ( period != 0 ) &&
            ( driver->count < driver->length ) )
    {
        driver->tasks[ driver->count ].task = task;
        driver->tasks[ driver->count ].period = period;
        driver->tasks[ driver->count ].counter = 0;
        driver->tasks[ driver->count ].due = FALSE;
        driver->tasks[ driver->count ].enabled = TRUE;

        if ( index != NULL )
        {
            ( *index ) = driver->count;
        }
        else
        {
            /* Intentionally blank */
        }

        ++driver->count;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Advances every enabled task by one tick.
 * @param[in,out] driver  Initialized scheduler.
 * @note    Call from a fixed rate interrupt. This does not call any task; it
 *          only counts and marks, so the time it takes is a handful of
 *          instructions per entry whatever the tasks do.
 * @note    A task already due when its period comes round again counts an
 *          overrun and stays due once rather than twice. The counter still
 *          reloads, so the phase is kept.
 */
void schedTick ( sched_t* driver )
{
    uint32_t i = 0;

    for ( i = 0; i < driver->count; ++i )
    {
        if ( driver->tasks[ i ].enabled == TRUE )
        {
            ++driver->tasks[ i ].counter;

            if ( driver->tasks[ i ].counter >= driver->tasks[ i ].period )
            {
                driver->tasks[ i ].counter -= driver->tasks[ i ].period;

                if ( driver->tasks[ i ].due == TRUE )
                {
                    ++driver->overrunCount;
                }
                else
                {
                    driver->tasks[ i ].due = TRUE;
                }
            }
            else
            {
                /* Intentionally blank */
            }
        }
        else
        {
            /* Intentionally blank */
        }
    }
}

/**
 * @brief   Runs every task that is due.
 * @param[in,out] driver  Initialized scheduler.
 * @return  Number of tasks that ran.
 * @note    Call from the main loop. A return of zero means nothing was due,
 *          which is what a battery powered caller waits on before going back
 *          to sleep.
 * @note    Tasks run in the order they were added, each at most once per call.
 *          The due flag is cleared before the task is called rather than
 *          after, so a tick that lands during a long task marks it due again
 *          instead of having its mark erased on return.
 */
uint32_t schedRun ( sched_t* driver )
{
    uint32_t retVal = 0;
    uint32_t i = 0;

    for ( i = 0; i < driver->count; ++i )
    {
        if ( ( driver->tasks[ i ].due == TRUE ) &&
                ( driver->tasks[ i ].enabled == TRUE ) )
        {
            driver->tasks[ i ].due = FALSE;

            driver->tasks[ i ].task ( );

            ++retVal;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    return ( retVal );
}

/**
 * @brief   Enables or disables one task.
 * @param[in,out] driver   Initialized scheduler.
 * @param[in]     index    Slot reported by schedAddTask.
 * @param[in]     enabled  TRUE to run it, FALSE to stop it.
 * @return  TRUE on success, FALSE when driver is NULL or index names no task.
 * @note    Disabling stops the counter and drops a pending due flag, so a task
 *          switched off does not fire once more when it is switched back on.
 *          Enabling restarts its period from now rather than resuming a part
 *          counted one, which is the behaviour a caller turning a subsystem on
 *          expects.
 * @note    A status here rather than silence, because an index out of range is
 *          a caller mistake that would otherwise disable nothing and look like
 *          it worked.
 */
uint8_t schedEnable ( sched_t* driver, uint32_t index, uint8_t enabled )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( index < driver->count ) )
    {
        driver->tasks[ index ].counter = 0;
        driver->tasks[ index ].due = FALSE;

        if ( enabled == TRUE )
        {
            driver->tasks[ index ].enabled = TRUE;
        }
        else
        {
            driver->tasks[ index ].enabled = FALSE;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Gets the number of overruns since Init.
 * @param[in] driver  Scheduler state.
 * @return  Times a task came due while it was still due from before.
 * @note    Non zero means the main loop is not keeping up with the tick. The
 *          remedy is a longer period or a shorter task; the scheduler does not
 *          try to catch up on its own, because running a task twice in a row
 *          is a guess about what the caller wanted.
 */
uint32_t schedGetOverrunCount ( const sched_t* const driver )
{
    return ( driver->overrunCount );
}
