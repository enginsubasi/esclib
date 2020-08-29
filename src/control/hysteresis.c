/**
  ******************************************************************************
  *
  * @file:      hysteresis.c
  * @author:    Engin Subaşı
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     16/07/2020
  * @history:   16/07/2020 Created.
  *             24/08/2020 Data type changed from double to float.
  *             24/08/2020 Naming changes. Comments added.
  *
  * @about:     Hysteresis control.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         hysteresisInit              : Initialize hysteresis structure.
  *         hysteresisControl           : Process new input data.
  *         hysteresisGetOutput         : Gets current control data. The output value is boolean.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "hysteresis.h"

/*
 * @about: Initialize hysteresis structure.
 */
void hysteresisInit ( hysteresis_t* driver, float upValue, float downValue )
{
    driver->output = FALSE;
    driver->up = upValue;
    driver->dw = downValue;
}

/*
 * @about: Control iteration.
 */
void hysteresisControl ( hysteresis_t* driver, float input )
{
    if ( input > driver->up )
    {
        driver->output = TRUE;
    }
    else if ( input < driver->dw )
    {
        driver->output = FALSE;
    }
    else
    {
        /* Intentionally blank. */
    }
}

/*
 * @about: Gets current control output.
 */
uint8_t hysteresisGetOutput ( hysteresis_t* driver )
{
    return ( driver->output );
}


