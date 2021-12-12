/**
  ******************************************************************************
  *
  * @file:      basicmath.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     03/06/2020
  * @history:   03/06/2020 Created.
  *             10/07/2020 Average named mean.
  *             24/08/2020 Data type changed from double to float.
  *
  * @about:     Basic mathematic function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         findMax         : Find maximum number of array elements.
  *         findMax         : Find minimum number of array elements.
  *         findMinMax      : Find max. and min. numbers of array elements.
  *
  *         calculateSum    : Calculates sum of array.
  *         calculateMean   : Calculates average/mean value of array.
  *         calculateMedian : Calculates median value of array.
  *         calculateRange  : Calculates range of array.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "basicmath.h"

/*
 * @about:
 */
float findMax ( float* array, uint32_t length )
{
    uint32_t i = 0;
    float tempMax = 0;
    
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
float findMin ( float* array, uint32_t length )
{
    uint32_t i = 0;
    float tempMin = 0;
    
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
void findMinMax ( float* array, uint32_t length, float* min, float* max )
{
    uint32_t i = 0;
    float tempMin = 0;
    float tempMax = 0;
    
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
void findMinMaxu32 ( uint32_t* array, uint32_t length, uint32_t* min, uint32_t* max )
{
    uint32_t i = 0;
    uint32_t tempMin = 0;
    uint32_t tempMax = 0;

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
float calculateSum ( float* array, uint32_t length )
{
    uint32_t i = 0;
    float sum = 0;
    
    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }
    
    return ( sum );
}

/*
 * @about:
 */
uint32_t calculateSumu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t sum = 0;

    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }

    return ( sum );
}

/*
 * @about:
 */
float calculateMean ( float* array, uint32_t length )
{
    uint32_t i = 0;
    float sum = 0;
    float average = 0;
    
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
uint32_t calculateMeanu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t sum = 0;
    uint32_t average = 0;

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
float calculateMedian ( float* array, uint32_t length )
{
    return ( array[ ( length / 2 ) ] );
}

/*
 * @about:
 */
uint32_t calculateMedianu32 ( uint32_t* array, uint32_t length )
{
    return ( array[ ( length / 2 ) ] );
}

/*
 * @about:
 */
float calculateRange ( float* array, uint32_t length )
{
    float tempMin = 0;
    float tempMax = 0;
    uint32_t tempLength = 0;
    
    tempLength = length;
    
    findMinMax ( array, tempLength, &tempMin, &tempMax );
    
    return ( tempMax - tempMin );
}

/*
 * @about:
 */
uint32_t calculateRangeu32 ( uint32_t* array, uint32_t length )
{
    uint32_t tempMin = 0;
    uint32_t tempMax = 0;
    uint32_t tempLength = 0;

    tempLength = length;

    findMinMaxu32 ( array, tempLength, &tempMin, &tempMax );

    return ( tempMax - tempMin );
}


