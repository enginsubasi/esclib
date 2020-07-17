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
  * @mdate:     16/07/2020
  * @history:   16/07/2020 Created
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
 * @about:
 */
int8_t hysteresisInit ( hysteresis_t* driver, double upValue, double downValue )
{
    driver->output = FALSE;
    driver->up = upValue;
    driver->dw = downValue;
}

/*
 * @about:
 */
void hysteresisControl ( hysteresis_t* driver, double input )
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
uint8_t hysteresisGetOutput ( hysteresis_t* driver )
{
    return ( driver->output );
}


