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
  *             09/04/2022 absolute, absolutei32
  *
  * @about:     Basic mathematics function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         absolute            :
  *         absolutei32         :
  *         
  *         findMax             : Find maximum number of array elements.
  *         findMaxu32          : Find maximum number of array elements u32.
  *         findMaxi32          : Find maximum number of array elements i32.
  *         findMin             : Find minimum number of array elements.
  *         findMin32:          : Find minimum number of array elements .
  *         findMinMax          : Find max. and min. numbers of array elements.
  *         findMinMaxu32       : Find max. and min. numbers of array elements u32.
  *
  *         calculateSum        : Calculates sum of array.
  *         calculateSumu32     : Calculates sum of array u32.
  *         calculateMean       : Calculates average/mean value of array.
  *         calculateMeanu32    : Calculates average/mean value of array u32.
  *         calculateMedian     : Calculates median value of array.
  *         calculateMedianu32  : Calculates median value of array u32.
  *         calculateRange      : Calculates range of array.
  *         calculateRangeu32   : Calculates range of array u32.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "basicmath.h"

/*
 * @about:
 */
float absolute ( float inp )
{
    float retVal = 0;

    if ( inp < 0 )
    {
        retVal = -inp;
    }
    else
    {
        retVal = inp;
    }

    return ( retVal );
}

/*
 * @about:
 */
int32_t absolutei32 ( int32_t inp )
{
    int32_t retVal = 0;

    if ( inp < 0 )
    {
        retVal = -inp;
    }
    else
    {
        retVal = inp;
    }

    return ( retVal );
}

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
uint32_t findMaxu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t tempMax = 0;
    
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
int32_t findMaxi32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    int32_t tempMax = 0;
    
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
uint32_t findMinu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t tempMin = 0;
    
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
int32_t findMini32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    int32_t tempMin = 0;
    
    tempMin = array[ 0 ];
    
    for ( i = 1; i < length; ++i )
    {
        if ( tempMin < array[ i ] )
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
    
    // TODO: Div. by zero risk
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

    // TODO: Div. by zero risk
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


