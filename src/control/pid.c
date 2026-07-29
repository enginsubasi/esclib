/**
  ******************************************************************************
  *
  * @file      pid.c
  * @author    Engin Subaşı <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
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
  *
  ******************************************************************************
  */

#include "pid.h"

/**
 * @brief   Initializes the PID controller state.
 * @param[out] driver             Controller state to initialize.
 * @param[in]  kp                 Proportional gain.
 * @param[in]  ki                 Integral gain.
 * @param[in]  kd                 Derivative gain.
 * @param[in]  ts                 Sample time used by the integral and derivative terms.
 * @param[in]  pPartMaxLimit      Upper clamp for the proportional term.
 * @param[in]  pPartMinLimit      Lower clamp for the proportional term.
 * @param[in]  iPartMaxLimit      Upper clamp for the integral term.
 * @param[in]  iPartMinLimit      Lower clamp for the integral term.
 * @param[in]  dPartMaxLimit      Upper clamp for the derivative term.
 * @param[in]  dPartMinLimit      Lower clamp for the derivative term.
 * @param[in]  pidOutputMaxLimit  Upper clamp for the controller output.
 * @param[in]  pidOutputMinLimit  Lower clamp for the controller output; also the initial output value.
 */
void pidInit ( pidc_t* driver, float kp, float ki, float kd, float ts, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
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
}

/**
 * @brief   Changes the PID controller's gains and sample time.
 * @param[in,out] driver  Controller state.
 * @param[in]     kp      New proportional gain.
 * @param[in]     ki      New integral gain.
 * @param[in]     kd      New derivative gain.
 * @param[in]     ts      New sample time used by the integral and derivative terms.
 */
void pidChangeCoefficients ( pidc_t* driver, float kp, float ki, float kd, float ts )
{
    // Coefficients.
    driver->kp = kp;
    driver->ki = ki;
    driver->kd = kd;

    driver->ts = ts;
}

/**
 * @brief   Changes the PID controller's proportional, integral, derivative and output clamp limits.
 * @param[in,out] driver             Controller state.
 * @param[in]     pPartMaxLimit      New upper clamp for the proportional term.
 * @param[in]     pPartMinLimit      New lower clamp for the proportional term.
 * @param[in]     iPartMaxLimit      New upper clamp for the integral term.
 * @param[in]     iPartMinLimit      New lower clamp for the integral term.
 * @param[in]     dPartMaxLimit      New upper clamp for the derivative term.
 * @param[in]     dPartMinLimit      New lower clamp for the derivative term.
 * @param[in]     pidOutputMaxLimit  New upper clamp for the controller output.
 * @param[in]     pidOutputMinLimit  New lower clamp for the controller output.
 */
void pidChangeLimits ( pidc_t* driver, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                        float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
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
}

/**
 * @brief   Runs one PID control iteration for the given error signal.
 * @param[in,out] driver  Controller state.
 * @param[in]     error   Error signal for this iteration, i.e. setpoint minus
 *                        measurement, not the raw measurement itself.
 * @note    The result is stored in driver and read back with pidGetOutput;
 *          this function does not return it directly.
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
float pidGetOutput ( pidc_t* driver )
{
    return ( driver->output );
}


