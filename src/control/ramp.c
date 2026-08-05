/**
  ******************************************************************************
  *
  * @file      ramp.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
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
  * 05/08/2026 The profile itself: rampIteration, with the square root @n
  *            velocity envelope and the final step clamp. @n
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
 * @brief   Advances the setpoint one step toward the target, under both the
 *          velocity and the acceleration limit.
 * @param[in,out] driver  Initialized ramp.
 * @param[in]     target  Where the setpoint is heading. May change between
 *                        calls; the profile re-plans from where it is.
 * @note    The velocity envelope is the whole module. sqrtf ( 2 * a * d ) is
 *          the fastest the ramp could be going and still stop exactly on the
 *          target, so braking begins by itself the moment the velocity meets
 *          it and no separate brake point has to be computed.
 * @note    Nothing here divides. A square root and several products, which
 *          is why rampInit rejects a limit of zero to stop the ramp doing
 *          nothing rather than to keep a nan out of the output.
 * @note    The velocity needs no clamp of its own. vDesired is already
 *          bounded by maxVelocity and the step below never carries the
 *          velocity past vDesired, so the bound is preserved from the zero
 *          rampInit starts it at.
 * @note    The final clamp is discrete time's only correction. Without it
 *          the last step passes the target and the ramp oscillates about it.
 *          It also covers the degenerate case at no extra cost: a target
 *          equal to the position gives a distance and a step of zero, and
 *          zero is not less than zero, so the ramp arrives at once.
 * @note    A target moved backwards while the ramp is running will be
 *          overshot before the ramp turns around. That is physics, not a
 *          defect; decelerating at maxAcceleration is the fastest stop the
 *          limits permit.
 */
void rampIteration ( ramp_t* driver, float target )
{
    float remaining = 0;
    float distance = 0;
    float vEnvelope = 0;
    float vDesired = 0;
    float maxDv = 0;
    float step = 0;
    float stepMagnitude = 0;

    remaining = target - driver->position;

    distance = remaining;

    if ( distance < 0 )
    {
        distance = -distance;
    }
    else
    {
        /* Intentionally blank */
    }

    vEnvelope = sqrtf ( 2.0f * driver->maxAcceleration * distance );

    if ( vEnvelope > driver->maxVelocity )
    {
        vDesired = driver->maxVelocity;
    }
    else
    {
        vDesired = vEnvelope;
    }

    if ( remaining < 0 )
    {
        vDesired = -vDesired;
    }
    else
    {
        /* Intentionally blank */
    }

    maxDv = driver->maxAcceleration * driver->ts;

    if ( ( vDesired - driver->velocity ) > maxDv )
    {
        driver->velocity += maxDv;
    }
    else if ( ( vDesired - driver->velocity ) < -maxDv )
    {
        driver->velocity -= maxDv;
    }
    else
    {
        driver->velocity = vDesired;
    }

    step = driver->velocity * driver->ts;

    stepMagnitude = step;

    if ( stepMagnitude < 0 )
    {
        stepMagnitude = -stepMagnitude;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( stepMagnitude >= distance )
    {
        driver->position = target;
        driver->velocity = 0;
        driver->arrived = TRUE;
    }
    else
    {
        driver->position += step;
        driver->arrived = FALSE;
    }
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
