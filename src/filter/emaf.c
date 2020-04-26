/**
  ******************************************************************************
  *
  * @file:      emaf.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 1.0.0
  * @cdate:     22/04/2020
  * @mdate:     22/04/2020
  * @history:   22/04/2020 Created
  *
  * @about:     Exponential moving average filter.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         Emaf_Init               : Initialize emaf structure.
  *         Emaf_Iteration          : Adds new data to filter.
  *         Emaf_GetOutput          : Gets current filter output.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "emaf.h"

/*
 * @about: Initialize emaf structure.
 */
int8_t Emaf_Init ( Emaf_t* driver, double alpha, double outputInit )
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
void Emaf_Iteration ( Emaf_t* driver, double newData )
{
    driver->output = ( ( newData * driver->alpha ) + ( driver->output * driver->alphan ) );
}

/*
 * @about: Gets current filter output.
 */
double Emaf_GetOutput ( Emaf_t* driver )
{
    return ( driver->output );
}

