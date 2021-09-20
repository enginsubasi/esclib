/**
  ******************************************************************************
  *
  * @file:      pid.c
  * @author:    Engin Subaşı
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     23/07/2020
  * @history:   23/07/2020 Created.
  *             24/08/2020 Data type changed from double to float.
  *
  * @about:     PID control.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         pidInit                     : Initialize hysteresis structure.
  *         pidControl                  : Process new input data.
  *         pidGetOutput                : Gets current control data. The output value is float.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "pid.h"

/*
 * @about: Initialize pid structure.
 */
void pidInit ( pidc_t* driver, float kp, float ki, float kd, float iPartMaxLimit, float iPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
{
    driver->output = pidOutputMinLimit;

    // Coefficients.
    driver->kp = kp;
    driver->ki = ki;
    driver->kd = kd;

    // Limits of integral part.
    driver->iMax = iPartMaxLimit;
    driver->iMin = iPartMinLimit;

    // Limits of PID output value.
    driver->pidMax = pidOutputMaxLimit;
    driver->pidMin = pidOutputMinLimit;
}

/*
 * @about: Changes PID coefficients.
 */
void pidChangeCoefficients ( pidc_t* driver, float kp, float ki, float kd )
{
    // Coefficients.
    driver->kp = kp;
    driver->ki = ki;
    driver->kd = kd;
}

/*
 * @about: Control iteration.
 */
void pidControl ( pidc_t* driver, float error )
{
    driver->error = error;

    // Calculate proportional part
    driver->partP = driver->error * driver->kp;

    // Calculate integral part
    driver->partI += driver->error;

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
    driver->partD = ( driver->error - driver->lastError );

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

/*
 * @about:
 */
float pidGetOutput ( pidc_t* driver )
{
    return ( driver->output );
}


