/**
  ******************************************************************************
  *
  * @file      slew.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
  * @date      02/08/2026
  *
  * @brief     Slew rate limiter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
  *
  * @note      This is a rate constraint, not a frequency filter. The output
  *            follows the input exactly until the input moves faster than
  *            maxStep per call, and from then on it moves at maxStep until it
  *            catches up. Use it to ramp a setpoint into an actuator rather
  *            than to remove noise.
  * 06/08/2026 The u32 variants are added. This file carried float @n
  *            and i32 only, while emaf and median next to it @n
  *            carried all three widths. Both ends saturate; an @n
  *            unsigned step below zero wraps rather than clipping. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "slew.h"

/**
 * @brief   Initializes the slew rate limiter.
 * @param[out] driver      Limiter state to initialize.
 * @param[in]  maxStep     Largest change the output may make in one call.
 * @param[in]  outputInit  Value the output starts from.
 * @return  TRUE on success, FALSE when driver is NULL or maxStep is not
 *          greater than zero.
 * @note    maxStep is per call, not per second. Multiply the rate you want by
 *          the period at which slewIteration is called.
 * @note    A maxStep of zero is rejected. It would pin the output on
 *          outputInit forever while reporting a successful init.
 */
uint8_t slewInit ( slew_t* driver, float maxStep, float outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( maxStep > 0 ) )
    {
        driver->maxStep = maxStep;
        driver->output = outputInit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Moves the output towards the new sample by at most maxStep.
 * @param[in,out] driver   Limiter state.
 * @param[in]     newData  Value the output is heading for.
 */
void slewIteration ( slew_t* driver, float newData )
{
    float difference = 0;

    difference = newData - driver->output;

    if ( difference > driver->maxStep )
    {
        driver->output += driver->maxStep;
    }
    else if ( difference < -driver->maxStep )
    {
        driver->output -= driver->maxStep;
    }
    else
    {
        driver->output = newData;
    }
}

/**
 * @brief   Gets the current output of the slew rate limiter.
 * @param[in] driver  Limiter state.
 * @return  Current output value.
 */
float slewGetOutput ( const slew_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes the slew rate limiter for signed 32-bit data.
 * @param[out] driver      Limiter state to initialize.
 * @param[in]  maxStep     Largest change the output may make in one call.
 * @param[in]  outputInit  Value the output starts from.
 * @return  TRUE on success, FALSE when driver is NULL or maxStep is not
 *          greater than zero.
 * @note    maxStep is per call, not per second.
 * @note    A maxStep of zero is rejected, as in slewInit.
 */
uint8_t slewIniti32 ( slewi32_t* driver, int32_t maxStep, int32_t outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( maxStep > 0 ) )
    {
        driver->maxStep = maxStep;
        driver->output = outputInit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Moves the signed 32-bit output towards the new sample by at most
 *          maxStep.
 * @param[in,out] driver   Limiter state.
 * @param[in]     newData  Value the output is heading for.
 * @note    The step is applied by building the reachable limit and clamping
 *          to it, rather than by subtracting the two values. newData minus
 *          output overflows whenever they sit at opposite ends of the int32_t
 *          range, which is reachable with real sensor data.
 */
void slewIterationi32 ( slewi32_t* driver, int32_t newData )
{
    int32_t limit = 0;

    if ( newData > driver->output )
    {
        // Furthest the output may reach upwards on this call.
        if ( driver->output > ( INT32_MAX - driver->maxStep ) )
        {
            limit = INT32_MAX;
        }
        else
        {
            limit = driver->output + driver->maxStep;
        }

        if ( newData < limit )
        {
            driver->output = newData;
        }
        else
        {
            driver->output = limit;
        }
    }
    else if ( newData < driver->output )
    {
        // Furthest the output may reach downwards on this call.
        if ( driver->output < ( INT32_MIN + driver->maxStep ) )
        {
            limit = INT32_MIN;
        }
        else
        {
            limit = driver->output - driver->maxStep;
        }

        if ( newData > limit )
        {
            driver->output = newData;
        }
        else
        {
            driver->output = limit;
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Gets the current output of the signed 32-bit slew rate limiter.
 * @param[in] driver  Limiter state.
 * @return  Current output value.
 */
int32_t slewGetOutputi32 ( const slewi32_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes an unsigned rate limiter.
 * @param[out] driver      Limiter state to initialize.
 * @param[in]  maxStep     Largest amount the output may move on one call.
 * @param[in]  outputInit  Value the output starts at.
 * @return  TRUE on success, FALSE when driver is NULL or maxStep is zero.
 * @note    maxStep is a magnitude and is limited by the period at which
 *          slewIterationu32 is called, exactly as in the other two widths.
 * @note    A maxStep of zero is rejected. It would pin the output on
 *          outputInit forever while reporting a successful init.
 */
uint8_t slewInitu32 ( slewu32_t* driver, uint32_t maxStep, uint32_t outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( maxStep != 0 ) )
    {
        driver->maxStep = maxStep;
        driver->output = outputInit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Moves the unsigned output towards the new sample by at most maxStep.
 * @param[in,out] driver   Limiter state.
 * @param[in]     newData  Value the output is heading for.
 * @note    Both ends saturate rather than wrapping. The step is tested
 *          against the room left below UINT32_MAX and above zero before it is
 *          taken, because an unsigned overflow wraps silently in either
 *          direction and would send the output to the far end of the range.
 */
void slewIterationu32 ( slewu32_t* driver, uint32_t newData )
{
    uint32_t limit = 0;

    if ( newData > driver->output )
    {
        // Furthest the output may reach upwards on this call.
        if ( driver->output > ( UINT32_MAX - driver->maxStep ) )
        {
            limit = UINT32_MAX;
        }
        else
        {
            limit = driver->output + driver->maxStep;
        }

        if ( newData < limit )
        {
            driver->output = newData;
        }
        else
        {
            driver->output = limit;
        }
    }
    else if ( newData < driver->output )
    {
        // Furthest the output may reach downwards on this call.
        if ( driver->output < driver->maxStep )
        {
            limit = 0;
        }
        else
        {
            limit = driver->output - driver->maxStep;
        }

        if ( newData > limit )
        {
            driver->output = newData;
        }
        else
        {
            driver->output = limit;
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Returns the current limiter output.
 * @param[in]  driver  Initialized limiter.
 * @return  The rate limited value.
 */
uint32_t slewGetOutputu32 ( const slewu32_t* const driver )
{
    return ( driver->output );
}
