/**
  ******************************************************************************
  *
  * @file:      emaf.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 2.0.0
  * @cdate:     22/04/2020
  * @history:   22/04/2020 Created
  *             07/06/2020 Naming style changed
  *             24/08/2020 Data type changed from double to float.
  *
  * @about:     Exponential moving average filter.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         emafInit               : Initialize emaf structure.
  *         emafIteration          : Adds new data to filter.
  *         emafGetOutput          : Gets current filter output.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "emaf.h"

/*
 * @about: Initialize emaf structure.
 */
int8_t emafInit ( emaf_t* driver, float alpha, float outputInit )
{
    int8_t retVal = FALSE;

    if ( ( alpha >= 0 ) && ( alpha <= 1 ) )
    {
        driver->alpha = alpha;
        driver->alphan = 1 - alpha;
        driver->output = outputInit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/*
 * @about: Adds new data to filter.
 */
void emafIteration ( emaf_t* driver, float newData )
{
    driver->output = ( ( newData * driver->alpha ) + ( driver->output * driver->alphan ) );
}

/*
 * @about: Gets current filter output.
 */
float emafGetOutput ( emaf_t* driver )
{
    return ( driver->output );
}

