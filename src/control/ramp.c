/**
  ******************************************************************************
  *
  * @file      ramp.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.3.1
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
  * 06/08/2026 The Q16 fixed point variant is added, for parts with @n
  *            no FPU. sqrtf is replaced by an integer square root, @n
  *            and the limits are per sample so no period is @n
  *            carried. @n
  * 15/09/2026 The Q16 scaling is a multiply rather than a left @n
  *            shift. Shifting a negative signed value left is @n
  *            undefined in C, and every one of these took one @n
  *            while giving the right answer. UBSan found them. @n
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

/*
 * Q16 fixed point. A limit of 1.0 unit is 65536.
 *
 * There is no ts here. The float variant takes a period and writes its limits
 * in units per second; this one carries them per sample, which removes the
 * two multiplications by ts from the update and, more to the point, keeps a
 * floating point period out of a variant whose whole purpose is to avoid one.
 * alphabetaIniti32 drops dt for the same reason.
 */
#define RAMP_Q               16
#define RAMP_ONE             65536

/**
 * @brief   Integer square root of a 64-bit value.
 * @param[in] value  Value to take the root of.
 * @return  The largest integer whose square does not exceed value.
 * @note    The bit by bit method, which needs no division and no floating
 *          point. The float variant calls sqrtf here instead, and replacing
 *          that call is the only real work in this whole variant.
 * @note    The velocity envelope feeds this a Q32 product, and the square
 *          root of a Q32 value is a Q16 one, so the fixed point scale comes
 *          out right with no shifting of its own.
 */
static uint64_t rampSquareRoot ( uint64_t value )
{
    uint64_t retVal = 0;
    uint64_t rest = 0;
    uint64_t bit = 0;

    rest = value;
    bit = ( ( uint64_t ) 1 ) << 62;

    while ( bit > rest )
    {
        bit >>= 2;
    }

    while ( bit != 0 )
    {
        if ( rest >= ( retVal + bit ) )
        {
            rest = rest - ( retVal + bit );
            retVal = ( retVal >> 1 ) + bit;
        }
        else
        {
            retVal = retVal >> 1;
        }

        bit >>= 2;
    }

    return ( retVal );
}

/**
 * @brief   Initializes a fixed point setpoint ramp.
 * @param[out] driver           Ramp state to initialize.
 * @param[in]  maxVelocity      Largest velocity in Q16 units per sample, so
 *                              65536 is one unit per sample.
 * @param[in]  maxAcceleration  Largest change of velocity in Q16 units per
 *                              sample squared. This is the step the velocity
 *                              may take on one call.
 * @param[in]  positionInit     Where the ramp starts, in plain units.
 * @return  TRUE on success, FALSE when driver is NULL or either limit is not
 *          strictly positive.
 * @note    Both limits are per sample rather than per second, so the caller
 *          converts once, at Init, in whatever arithmetic it likes, instead
 *          of the module carrying a floating point period.
 * @note    The ramp starts at rest and reports itself arrived, because no
 *          move is pending until rampIterationi32 is handed a target.
 */
uint8_t rampIniti32 ( rampi32_t* driver, int32_t maxVelocity, int32_t maxAcceleration, int32_t positionInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( maxVelocity > 0 ) && ( maxAcceleration > 0 ) )
    {
        driver->maxVelocity = ( int64_t ) maxVelocity;
        driver->maxAcceleration = ( int64_t ) maxAcceleration;

        /* Multiplied rather than shifted: a negative positionInit
           shifted left is undefined in C. */
        driver->position = ( ( int64_t ) positionInit ) * RAMP_ONE;
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
 * @brief   Advances the fixed point setpoint one step toward the target.
 * @param[in,out] driver  Initialized ramp.
 * @param[in]     target  Where the setpoint is heading, in plain units. May
 *                        change between calls; the profile re-plans from
 *                        where it is.
 * @note    The velocity envelope is the whole module, exactly as in the float
 *          variant. Two times the acceleration times the remaining distance,
 *          both in Q16, is a Q32 product, and its integer square root is the
 *          Q16 velocity from which the target is still reachable at rest.
 * @note    That product is the range limit of this variant. It is formed in
 *          int64_t and overflows once twice the acceleration times the
 *          distance passes about 9.2e18 in Q32, which at an acceleration of
 *          one unit per sample squared is a move of roughly 1e9 units.
 * @note    The final clamp is discrete time's only correction, the same one
 *          the float variant carries. Without it the last step passes the
 *          target and the ramp oscillates about it.
 */
void rampIterationi32 ( rampi32_t* driver, int32_t target )
{
    int64_t targetQ = 0;
    int64_t remaining = 0;
    int64_t distance = 0;
    int64_t envelope = 0;
    int64_t desired = 0;
    int64_t step = 0;
    int64_t stepMagnitude = 0;

    /* Multiplied rather than shifted, for the reason rampIniti32 gives. */
    targetQ = ( ( int64_t ) target ) * RAMP_ONE;

    remaining = targetQ - driver->position;

    distance = remaining;

    if ( distance < 0 )
    {
        distance = -distance;
    }
    else
    {
        /* Intentionally blank */
    }

    envelope = ( int64_t ) rampSquareRoot ( ( uint64_t ) ( 2 * driver->maxAcceleration * distance ) );

    if ( envelope > driver->maxVelocity )
    {
        desired = driver->maxVelocity;
    }
    else
    {
        desired = envelope;
    }

    if ( remaining < 0 )
    {
        desired = -desired;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( ( desired - driver->velocity ) > driver->maxAcceleration )
    {
        driver->velocity += driver->maxAcceleration;
    }
    else if ( ( desired - driver->velocity ) < -driver->maxAcceleration )
    {
        driver->velocity -= driver->maxAcceleration;
    }
    else
    {
        driver->velocity = desired;
    }

    // The velocity is already per sample, so the step is the velocity itself.
    step = driver->velocity;

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
        driver->position = targetQ;
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
 * @return  The position the profile has reached, in plain units with the Q16
 *          fraction dropped.
 */
int32_t rampGetOutputi32 ( const rampi32_t* const driver )
{
    int32_t retVal = 0;

    retVal = ( int32_t ) ( driver->position >> RAMP_Q );

    return ( retVal );
}

/**
 * @brief   Returns the current velocity of the setpoint.
 * @param[in]  driver  Initialized ramp.
 * @return  Q16 units per sample, signed. Zero once the target is reached.
 * @note    Q16 rather than plain units, for the reason
 *          alphabetaGetVelocityi32 reports Q16 too: a ramp spends the
 *          beginning and the end of every move below one unit per sample, and
 *          truncating that would report a standing zero while it moved.
 */
int32_t rampGetVelocityi32 ( const rampi32_t* const driver )
{
    int32_t retVal = 0;

    retVal = ( int32_t ) driver->velocity;

    return ( retVal );
}

/**
 * @brief   Reports whether the ramp has come to rest on its target.
 * @param[in]  driver  Initialized ramp.
 * @return  TRUE once the position sits exactly on the target with zero
 *          velocity, FALSE while the profile is still running.
 */
uint8_t rampIsArrivedi32 ( const rampi32_t* const driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->arrived;

    return ( retVal );
}
