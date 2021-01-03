/**
  ******************************************************************************
  *
  * @file:      statistic.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     03/01/2020
  * @history:   03/01/2020 Created.
  *
  * @about:     Statistic function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         variance        : Calculates variance value of the array.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "statistic.h"

/*
 * @about:
 */
float variance ( float* array, uint32_t length )
{
    uint32_t i = 0;
    float sum = 0;
    float average = 0;
    float variance = 0;
    float tempDif = 0;
    
    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }
    
    // Average value of the array.
    average = sum / length;
    
    for ( i = 0; i < length; ++i )
    {
        // To do not use pow function. Because, It causes dependency to math library.
        tempDif = ( array[ i ] - average );
        variance += ( tempDif * tempDif );
    }
    
    variance /= ( length - 1 );
    
    return ( variance );
}
