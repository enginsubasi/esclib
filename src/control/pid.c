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
int8_t pidInit ( pid_t* driver, double upValue, double downValue )
{
    driver->output = FALSE;
    driver->up = upValue;
    driver->dw = downValue;
}

/*
 * @about:
 */
void pidControl ( pid_t* driver, double error )
{
    
    
    
    
    if ( input > double->up )
    {
        driver->output = TRUE;
    }
    else if ( input < double->dw )
    {
        driver->output = FALSE;
    }
    else
    {
        /* Intentionally blank. */
    }
}

/*
 * @about:
 */
uint8_t pidGetOutput ( pid_t* driver )
{
    return ( driver->output );
}


