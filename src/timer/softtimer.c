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
