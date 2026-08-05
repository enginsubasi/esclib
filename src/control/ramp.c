/**
  ******************************************************************************
  *
  * @file      ramp.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      05/08/2026
  *
  * @brief     Setpoint profile under a velocity and an acceleration limit.
  *
  * @par Device
  * Generic
  *
  * @note      slew bounds one derivative and has no target it is trying to
  *            arrive at; it only ever chases the last sample it was handed.
  *            This module bounds two and comes to rest exactly on the target,
  *            which is the whole difference between them.
  *
  * @par History
  * 05/08/2026 Created @n
  *
  ******************************************************************************
  */

#include <stddef.h>
#include <math.h>

#include "ramp.h"

/**
 * @brief   Initializes a setpoint ramp.
 * @param[out] driver           Ramp state to initialize.
 * @param[in]  maxVelocity      Largest velocity the profile may reach, in
 *                              units per second.
 * @param[in]  maxAcceleration  Largest rate of change of velocity, in units
 *                              per second squared.
 * @param[in]  ts               Seconds between two rampIteration calls. The
 *                              caller is responsible for calling at that
 *                              rate; nothing here measures time.
 * @param[in]  positionInit     Where the ramp starts. Unconstrained.
 * @return  TRUE on success, FALSE when driver is NULL or any of the three
 *          limits is not strictly positive.
 * @note    The ramp starts at rest and reports itself arrived, because no
 *          move is pending until rampIteration is handed a target.
 * @note    A limit of zero is rejected even though rampIteration never
 *          divides. It would leave the ramp reporting a successful init
 *          while never moving, which is the same reason slewInit rejects a
 *          maxStep of zero.
 */
uint8_t rampInit ( ramp_t* driver, float maxVelocity, float maxAcceleration, float ts, float positionInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( maxVelocity > 0 ) && ( maxAcceleration > 0 ) &&
         ( ts > 0 ) )
    {
        driver->maxVelocity = maxVelocity;
        driver->maxAcceleration = maxAcceleration;
        driver->ts = ts;

        driver->position = positionInit;
        driver->velocity = 0;
        driver->arrived = TRUE;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Returns the current setpoint.
 * @param[in]  driver  Initialized ramp.
 * @return  The position the profile has reached.
 */
float rampGetOutput ( const ramp_t* const driver )
{
    float retVal = 0;

    retVal = driver->position;

    return ( retVal );
}

/**
 * @brief   Returns the current velocity of the setpoint.
 * @param[in]  driver  Initialized ramp.
 * @return  Units per second, signed. Zero once the target is reached.
 */
float rampGetVelocity ( const ramp_t* const driver )
{
    float retVal = 0;

    retVal = driver->velocity;

    return ( retVal );
}

/**
 * @brief   Reports whether the ramp has come to rest on its target.
 * @param[in]  driver  Initialized ramp.
 * @return  TRUE once the position sits exactly on the target with zero
 *          velocity, FALSE while the profile is still running.
 * @note    Arrival is a level rather than an event, so this reads without
 *          clearing anything and takes a const driver. It is not the
 *          consuming read bininpGetRisingValue is.
 */
uint8_t rampIsArrived ( const ramp_t* const driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->arrived;

    return ( retVal );
}
