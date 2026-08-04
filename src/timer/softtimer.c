/**
  ******************************************************************************
  *
  * @file      softtimer.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/08/2026
  *
  * @brief     Tick counting soft timer, one shot or periodic.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 04/08/2026 Created. @n
  *
  * @note      The period is expressed in ticks, not in milliseconds. The rate
  *            at which softtimerTick is called is the unit, so a 250 ms
  *            timeout on a 1 kHz interrupt is a period of 250.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "softtimer.h"

/**
 * @brief   Initializes a soft timer and leaves it stopped.
 * @param[out] driver  Timer state to initialize.
 * @param[in]  period  Number of softtimerTick calls that make up one timeout.
 * @param[in]  mode    STM_ONESHOT or STM_PERIODIC.
 * @return  TRUE on success, FALSE when driver is NULL, period is zero or mode
 *          is neither of the two accepted values. Nothing is written to the
 *          driver when FALSE is returned.
 * @note    The timer does not start here. softtimerStart is what sets it
 *          running.
 * @note    A zero period is rejected because it would expire on every tick,
 *          and in periodic mode the subtracting reload would never converge.
 */
uint8_t softtimerInit ( softtimer_t* driver, uint32_t period, uint8_t mode )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) &&
         ( period > 0u ) &&
         ( ( mode == ( uint8_t ) STM_ONESHOT ) || ( mode == ( uint8_t ) STM_PERIODIC ) ) )
    {
        driver->period = period;
        driver->counter = 0u;
        driver->mode = mode;
        driver->state = ( uint8_t ) STS_STOPPED;
        driver->expired = FALSE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the timer is stopped, running or finished.
 * @param[in] driver  Timer state.
 * @return  STS_STOPPED, STS_RUNNING or STS_EXPIRED.
 * @note    This reports the condition and does not consume it. STS_EXPIRED is
 *          reachable in one shot mode only; a periodic timer stays
 *          STS_RUNNING across its expiries.
 */
uint8_t softtimerGetState ( const softtimer_t* const driver )
{
    uint8_t retVal = ( uint8_t ) STS_STOPPED;

    retVal = driver->state;

    return ( retVal );
}

/**
 * @brief   Starts the timer from zero.
 * @param[in,out] driver  Timer state.
 * @note    This is also the restart. The counter and the expiry flag are both
 *          cleared, so there is no separate Restart function.
 */
void softtimerStart ( softtimer_t* driver )
{
    driver->counter = 0u;
    driver->expired = FALSE;
    driver->state = ( uint8_t ) STS_RUNNING;
}

/**
 * @brief   Stops the timer without disturbing what it has counted.
 * @param[in,out] driver  Timer state.
 * @note    Stop freezes and Start resets. The counter and the expiry flag are
 *          left alone here, so softtimerGetElapsed still means something after
 *          a stop.
 */
void softtimerStop ( softtimer_t* driver )
{
    driver->state = ( uint8_t ) STS_STOPPED;
}

/**
 * @brief   Advances the timer by one tick. Call this from a fixed rate ISR.
 * @param[in,out] driver  Timer state.
 * @note    A timer that is not running ignores the call, so every timer the
 *          caller owns can be ticked unconditionally.
 * @note    A missed expiry costs the event but not the phase: the reload
 *          happens here rather than at the read, so the next expiry lands on
 *          the tick it always would have. The number of missed expiries is
 *          not counted.
 * @note    The reload subtracts the period rather than clearing the counter.
 *          The counter is always below the period on entry, so it lands
 *          exactly on the period and the two are equivalent today. The
 *          subtraction is the form that stays correct if that ever changes.
 * @note    A one shot stops counting once it expires, which is what keeps its
 *          counter from wrapping.
 */
void softtimerTick ( softtimer_t* driver )
{
    if ( driver->state == ( uint8_t ) STS_RUNNING )
    {
        ++driver->counter;

        if ( driver->counter >= driver->period )
        {
            driver->expired = TRUE;

            if ( driver->mode == ( uint8_t ) STM_PERIODIC )
            {
                driver->counter -= driver->period;
            }
            else
            {
                driver->state = ( uint8_t ) STS_EXPIRED;
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

/**
 * @brief   Reports whether the timer has expired since the last call, and
 *          clears the flag.
 * @param[in,out] driver  Timer state. The flag this reports is cleared by the
 *                        call, so the parameter is genuinely in and out.
 * @return  TRUE when the timer expired since the previous call, FALSE
 *          otherwise.
 * @note    Reading consumes the event. Two calls in a row never both return
 *          TRUE for the same expiry.
 */
uint8_t softtimerExpired ( softtimer_t* driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->expired;
    driver->expired = FALSE;

    return ( retVal );
}

/**
 * @brief   Reports how many ticks the current period has counted.
 * @param[in] driver  Timer state.
 * @return  Ticks counted since the last start or the last periodic reload.
 */
uint32_t softtimerGetElapsed ( const softtimer_t* const driver )
{
    uint32_t retVal = 0u;

    retVal = driver->counter;

    return ( retVal );
}

/**
 * @brief   Reports how many ticks are left before the timer expires.
 * @param[in] driver  Timer state.
 * @return  Ticks remaining in the current period, or zero once the counter has
 *          reached it.
 * @note    The comparison is explicit rather than an unsigned subtraction,
 *          which would wrap for an expired one shot.
 */
uint32_t softtimerGetRemaining ( const softtimer_t* const driver )
{
    uint32_t retVal = 0u;

    if ( driver->counter < driver->period )
    {
        retVal = driver->period - driver->counter;
    }
    else
    {
        retVal = 0u;
    }

    return ( retVal );
}

/**
 * @brief   Installs a new period and restarts the current interval.
 * @param[in,out] driver  Timer state.
 * @param[in]     period  New number of ticks per timeout.
 * @return  TRUE on success, FALSE when driver is NULL or period is zero.
 *          Nothing is written to the driver when FALSE is returned.
 * @note    This returns a status because it takes a new argument that would
 *          break softtimerTick, which is the same reason
 *          pidChangeCoefficients returns one.
 * @note    The counter is cleared. Without that, a new period smaller than the
 *          current count would expire on consecutive ticks until the counter
 *          drained. The state and the expiry flag are left alone, so a
 *          finished one shot stays finished until it is started again.
 */
uint8_t softtimerChangePeriod ( softtimer_t* driver, uint32_t period )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( period > 0u ) )
    {
        driver->period = period;
        driver->counter = 0u;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
