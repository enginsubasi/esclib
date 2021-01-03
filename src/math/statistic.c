/**
  ******************************************************************************
  *
  * @file:      statistic.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     03/01/2020
  * @history:   03/01/2020 Created.
  *             03/01/2020 Standard deviation added.
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

#include <math.h>

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
    
    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }
    
    // Average value of the array.
    average = sum / length;
    
    for ( i = 0; i < length; ++i )
    {
        variance += pow( tempDif, 2 ); // Dep. math.h
    }
    
    variance /= ( length - 1 );
    
    return ( variance );
}

/*
 * @about:
 */
float standardDeviation ( float* array, uint32_t length )
{
    float standardDeviation = 0;
    
    standardDeviation = variance ( array, length );
    
    standardDeviation = sqrt ( standardDeviation ); // Dep. math.h
    
    return ( standardDeviation );
}
