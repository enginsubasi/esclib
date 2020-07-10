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
  * @history:   03/06/2020 Created.
  *             10/07/2020 Average named mean.
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
  *         calculateMean   : Calculates average/mean value of array.
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
void findMinMax ( double* array, uint32_t length, double* min, double* max )
{
    uint32_t i = 0;
    double tempMin = 0;
    double tempMax = 0;
    
    tempMin = array[ 0 ];
    tempMax = array[ 0 ];
    
    for ( i = 1; i < length; ++i )
    {
        if ( tempMin > array[ i ] )
        {
            tempMin = array[ i ];
        }
        
        if ( tempMax < array[ i ] )
        {
            tempMax = array[ i ];
        }
    }
    
    *min = tempMin;
    *max = tempMax;
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
double calculateMean ( double* array, uint32_t length )
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

/*
 * @about:
 */
double calculateMedian ( double* array, uint32_t length )
{
    return ( array[ ( length / 2 ) ] );
}

/*
 * @about:
 */
double calculateMode ( double* array, uint32_t length )
{
    
}

/*
 * @about:
 */
double calculateRange ( double* array, uint32_t length )
{
    double tempMin = 0;
    double tempMax = 0;
    uint32_t tempLength = 0;
    
    tempLength = length;
    
    findMinMax ( array, tempLength, &tempMin, &tempMax );
    
    return ( tempMax - tempMin );
}


