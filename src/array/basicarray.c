/**
  ******************************************************************************
  *
  * @file:      basicarray.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     03/01/2022
  * @history:   03/01/2022 Created.
  *
  *
  * @about:     Basic array function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         limit1D         : Applies limit on 1D array.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "basicarray.h"

/*
 * @about:
 */
void limitUpDw1D ( float* array, float, float upValue, float dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            matrix[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            matrix[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}
