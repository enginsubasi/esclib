/**
  ******************************************************************************
  *
  * @file:      basicmath.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     03/06/2020
  * @mdate:     03/06/2020
  * @history:   03/06/2020 Created
  *
  * @about:     Basic mathematic function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         findMax         : Find maximum number of array elements.
  *         findMax         : Find minimum number of array elements.
  *
  *         calculateSum    : Calculates sum of array.
  *         calculateAverage: Calculates average value of array.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "basicmath.h"

/*
 * @about:
 */
double findMax ( double* array, uint32_t length )
{
    uint32_t i = 0;
    double tempMax = 0;
    
    tempMax = array[ 0 ];
    
    for ( i = 1; i < length; ++i )
    {
        if ( tempMax < array[ i ] )
        {
            tempMax = array[ i ];
        }
    }
    
    return ( tempMax );
}

/*
 * @about:
 */
double findMin ( double* array, uint32_t length )
{
    uint32_t i = 0;
    double tempMin = 0;
    
    tempMin = array[ 0 ];
    
    for ( i = 1; i < length; ++i )
    {
        if ( tempMin > array[ i ] )
        {
            tempMin = array[ i ];
        }
    }
    
    return ( tempMin );
}

/*
 * @about:
 */
double calculateSum ( double* array, uint32_t length )
{
    uint32_t i = 0;
    double sum = 0;
    
    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }
    
    return ( sum );
}

/*
 * @about:
 */
double calculateAverage ( double* array, uint32_t length )
{
    uint32_t i = 0;
    double sum = 0;
    double average = 0;
    
    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }
    
    average = sum / length;
    
    return ( average );
}
