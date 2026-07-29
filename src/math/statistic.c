/**
  ******************************************************************************
  *
  * @file:      statistic.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.3
  * @cdate:     03/01/2020
  * @history:   03/01/2020 Created.
  *             03/01/2020 Standard deviation added.
  *             29/07/2026 Divide by zero guards added. The pending TODO notes
  *                        about a zero length array are resolved.
  *             29/07/2026 variancei32 no longer routes an integer square through
  *                        the double precision pow function.
  *
  * @about:     Statistic function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         variance                : Calculates variance value of the array.
  *         variancei32             : Calculates variance value of the array i32.
  *         standardDeviation       : Calculates standard deviation value of the array.
  *         standardDeviationi32    : Calculates standard deviation value of the array i32.
  *         covariance              : Calculates covariance value of two arrays.
  *
  * @notes:
  *     Every function returns zero for a zero length array.
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
    float difference = 0;

    if ( length != 0 )
    {
        for ( i = 0; i < length; ++i )
        {
            sum += array[ i ];
        }

        // Average value of the array.
        average = sum / length;

        for ( i = 0; i < length; ++i )
        {
            difference = array[ i ] - average;
            variance += ( difference * difference );
        }

        variance /= length;
    }
    else
    {
        /* Intentionally blank. */
    }

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
    int32_t difference = 0;

    if ( length != 0 )
    {
        for ( i = 0; i < length; ++i )
        {
            sum += array[ i ];
        }

        // Average value of the array.
        average = sum / ( int32_t ) length;

        for ( i = 0; i < length; ++i )
        {
            difference = array[ i ] - average;
            variance += ( difference * difference );
        }

        variance /= ( int32_t ) length;
    }
    else
    {
        /* Intentionally blank. */
    }

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

    if ( length != 0 )
    {
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

        covariance /= length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( covariance );
}
