/**
  ******************************************************************************
  *
  * @file:      pid.c
  * @author:    Engin Subaşı
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     23/07/2020
  * @mdate:     23/07/2020
  * @history:   23/07/2020 Created
  *
  * @about:     PID control.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         pidInit                     : Initialize hysteresis structure.
  *         pidControl                  : Process new input data.
  *         pidGetOutput                : Gets current control data. The output value is boolean.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "pid.h"

/*
 * @about:
 */
int8_t pidInit ( pid_t* driver, double kp, double ki, double kd )
{
    driver->output = FALSE;
    driver->kp = kp;
    driver->ki = ki;
    driver->kd = kd;
}

/*
 * @about:
 */
void pidControl ( pid_t* driver, double error )
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
    driver->output = ( driver->kp * partP ) +
                        ( driver->ki * partI ) +
                        ( driver->kd * partD );
    
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
uint8_t pidGetOutput ( pid_t* driver )
{
    return ( driver->output );
}


