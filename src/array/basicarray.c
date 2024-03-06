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
  *         limitUpDw1D             : Applies limit on 1D array.
  *         limitUpDw1Du32          : Applies limit on 1D array for u32.
  *         limitUpDw1Di32          : Applies limit on 1D array for i32.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "basicarray.h"

/*
 * @about: It limits the input array according to upValue, dwValue. If exceed, the limits values are assigning.
 */
void limitUpDw1D ( float* array, float upValue, float dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            array[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            array[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}

/*
 * @about: It limits the input array according to upValue, dwValue. If exceed, the limits values are assigning.
 */
void limitUpDw1Du32 ( uint32_t* array, uint32_t upValue, uint32_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            array[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            array[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}

/*
 * @about: It limits the input array according to upValue, dwValue. If exceed, the limits values are assigning.
 */
void limitUpDw1Di32 ( int32_t* array, int32_t upValue, int32_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            array[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            array[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}
