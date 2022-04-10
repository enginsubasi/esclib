/**
  ******************************************************************************
  *
  * @file:      statistic.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
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
  *         variance                : Calculates variance value of the array.
  *         variancei32             :
  *         standardDeviation       :
  *         standardDeviationi32    : 
  *         covariance              :
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
    // TODO: Prevent from divide by zero.
    average = sum / length;

    for ( i = 0; i < length; ++i )
    {
        variance += pow ( ( array[ i ] - average ), 2 );
    }

    // TODO: Prevent from divide by zero.
    variance /= ( length - 1 );

    return ( variance );
}

/*
 * @about:
 */
int32_t variancei32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    int32_t sum = 0;
    int32_t average = 0;
    int32_t variance = 0;

    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }

    // Average value of the array.
    // TODO: Prevent from divide by zero.
    average = sum / length;

    for ( i = 0; i < length; ++i )
    {
        variance += pow ( ( array[ i ] - average ), 2 );
    }

    // TODO: Prevent from divide by zero.
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

/*
 * @about:
 */
int32_t standardDeviationi32 ( int32_t* array, uint32_t length )
{
    int32_t standardDeviation = 0;
    
    standardDeviation = variancei32 ( array, length );
    
    standardDeviation = sqrt ( standardDeviation ); // Dep. math.h
    
    return ( standardDeviation );
}

/*
 * @about:
 */
float covariance ( float* array1, float* array2, uint32_t length )
{
    uint32_t i = 0;
    float sum1 = 0, sum2 = 0;
    float average1 = 0, average2 = 0;
    float covariance = 0;
    
    for ( i = 0; i < length; ++i )
    {
        sum1 += array1[ i ];
        sum2 += array2[ i ];
    }
    
    // Average values of the arrays.
    average1 = sum1 / length;
    average2 = sum2 / length;
    
    for ( i = 0; i < length; ++i )
    {
        covariance += ( ( array1[ i ] - average1 ) * ( array2[ i ] - average2 ) );
    }

    // TODO: Prevent from divide by zero.
    covariance /= length;

    return ( covariance );
}
