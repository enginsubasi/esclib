/**
  ******************************************************************************
  *
  * @file      pid.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.3
  * @date      23/07/2020
  *
  * @brief     PID control.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 23/07/2020 Created. @n
  * 24/08/2020 Data type changed from double to float. @n
  * 29/07/2026 Bug fix. pidInit left error, lastError, partP, partI @n
  *            and partD uninitialized. @n
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 pidInit and pidChangeCoefficients reject a zero ts. @n
  *            pidControl divides the error difference by ts, and a @n
  *            zero there produced a nan that passed straight through @n
  *            the output limiter, since nan compares false against @n
  *            both bounds. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The accessors that only read take a const driver. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "pid.h"

/**
 * @brief   Initializes the PID controller state.
 * @param[out] driver             Controller state to initialize.
 * @param[in]  kp                 Proportional gain.
 * @param[in]  ki                 Integral gain.
 * @param[in]  kd                 Derivative gain.
 * @param[in]  ts                 Sample time used by the integral and derivative terms.
 * @param[in]  pPartMaxLimit      Upper clamp for partP, in error units, applied before the kp multiply.
 * @param[in]  pPartMinLimit      Lower clamp for partP, in error units, applied before the kp multiply.
 * @param[in]  iPartMaxLimit      Upper clamp for partI, in error units, applied before the ki multiply.
 * @param[in]  iPartMinLimit      Lower clamp for partI, in error units, applied before the ki multiply.
 * @param[in]  dPartMaxLimit      Upper clamp for partD, in error units, applied before the kd multiply.
 * @param[in]  dPartMinLimit      Lower clamp for partD, in error units, applied before the kd multiply.
 * @param[in]  pidOutputMaxLimit  Upper clamp for the controller output.
 * @param[in]  pidOutputMinLimit  Lower clamp for the controller output; also the initial output value.
 * @return  TRUE on success, FALSE when driver is NULL or ts is zero.
 * @note    ts is rejected when zero because pidControl divides the error
 *          difference by it. Nothing else in the module guards that divide,
 *          so this check and the one in pidChangeCoefficients are what keep
 *          the derivative term finite.
 */
uint8_t pidInit ( pidc_t* driver, float kp, float ki, float kd, float ts, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( ts != 0 ) )
    {
        driver->output = pidOutputMinLimit;

        // Error memory. Without this the first derivative term would use garbage.
        driver->error = 0;
        driver->lastError = 0;

        // Term accumulators. partI integrates, so it must start from a known value.
        driver->partP = 0;
        driver->partI = 0;
        driver->partD = 0;

        // Coefficients.
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;

        driver->ts = ts;

        // Limits of proportional part.
        driver->pMax = pPartMaxLimit;
        driver->pMin = pPartMinLimit;

        // Limits of integral part.
        driver->iMax = iPartMaxLimit;
        driver->iMin = iPartMinLimit;

        // Limits of derivative part.
        driver->dMax = dPartMaxLimit;
        driver->dMin = dPartMinLimit;

        // Limits of PID output value.
        driver->pidMax = pidOutputMaxLimit;
        driver->pidMin = pidOutputMinLimit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Changes the PID controller's gains and sample time.
 * @param[in,out] driver  Controller state.
 * @param[in]     kp      New proportional gain.
 * @param[in]     ki      New integral gain.
 * @param[in]     kd      New derivative gain.
 * @param[in]     ts      New sample time used by the integral and derivative terms.
 * @return  TRUE on success, FALSE when driver is NULL or ts is zero.
 * @note    On FALSE nothing is written, so the controller keeps the gains
 *          and the sample time it already had.
 */
uint8_t pidChangeCoefficients ( pidc_t* driver, float kp, float ki, float kd, float ts )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( ts != 0 ) )
    {
        // Coefficients.
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;

        driver->ts = ts;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Changes the PID controller's proportional, integral, derivative and output clamp limits.
 * @param[in,out] driver             Controller state.
 * @param[in]     pPartMaxLimit      New upper clamp for partP, in error units, applied before the kp multiply.
 * @param[in]     pPartMinLimit      New lower clamp for partP, in error units, applied before the kp multiply.
 * @param[in]     iPartMaxLimit      New upper clamp for partI, in error units, applied before the ki multiply.
 * @param[in]     iPartMinLimit      New lower clamp for partI, in error units, applied before the ki multiply.
 * @param[in]     dPartMaxLimit      New upper clamp for partD, in error units, applied before the kd multiply.
 * @param[in]     dPartMinLimit      New lower clamp for partD, in error units, applied before the kd multiply.
 * @param[in]     pidOutputMaxLimit  New upper clamp for the controller output.
 * @param[in]     pidOutputMinLimit  New lower clamp for the controller output.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    On FALSE nothing is written, so the controller keeps the limits
 *          it already had.
 */
uint8_t pidChangeLimits ( pidc_t* driver, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                        float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        // Limits of proportional part.
        driver->pMax = pPartMaxLimit;
        driver->pMin = pPartMinLimit;

        // Limits of integral part.
        driver->iMax = iPartMaxLimit;
        driver->iMin = iPartMinLimit;

        // Limits of derivative part.
        driver->dMax = dPartMaxLimit;
        driver->dMin = dPartMinLimit;

        // Limits of PID output value.
        driver->pidMax = pidOutputMaxLimit;
        driver->pidMin = pidOutputMinLimit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one PID control iteration for the given error signal.
 * @param[in,out] driver  Controller state.
 * @param[in]     error   Error signal for this iteration, i.e. setpoint minus
 *                        measurement, not the raw measurement itself.
 * @note    The result is stored in driver and read back with pidGetOutput;
 *          this function does not return it directly.
 * @note    The derivative term divides by driver->ts. That divide is safe
 *          because pidInit and pidChangeCoefficients both refuse a zero ts,
 *          so ts cannot be zero on a controller that was initialized
 *          successfully.
 */
void pidControl ( pidc_t* driver, float error )
{
    driver->error = error;

    // Calculate proportional part
    driver->partP = driver->error;

    // Control proportional range
    if ( driver->partP > driver->pMax )
    {
        driver->partP = driver->pMax;
    }
    else if ( driver->partP < driver->pMin )
    {
        driver->partP = driver->pMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate integral part
    driver->partI += ( driver->error * driver->ts );

    // Control integral range
    if ( driver->partI > driver->iMax )
    {
        driver->partI = driver->iMax;
    }
    else if ( driver->partI < driver->iMin )
    {
        driver->partI = driver->iMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate derivative part
    driver->partD = ( ( driver->error - driver->lastError ) / driver->ts );
    
    // Control derivative range
    if ( driver->partD > driver->dMax )
    {
        driver->partD = driver->dMax;
    }
    else if ( driver->partD < driver->dMin )
    {
        driver->partD = driver->dMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate PID output value
    driver->output = ( driver->kp * driver->partP ) +
                        ( driver->ki * driver->partI ) +
                        ( driver->kd * driver->partD );

    // Control PID range
    if ( driver->output > driver->pidMax )
    {
        driver->output = driver->pidMax;
    }
    else if ( driver->output < driver->pidMin )
    {
        driver->output = driver->pidMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Save current error for next iteration over lastError
    driver->lastError = driver->error;
}

/**
 * @brief   Gets the most recently computed PID controller output.
 * @param[in] driver  Controller state.
 * @return  Current PID output value, already clamped to the configured limits.
 */
float pidGetOutput ( const pidc_t* const driver )
{
    return ( driver->output );
}


