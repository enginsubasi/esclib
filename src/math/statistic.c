/**
  ******************************************************************************
  *
  * @file      statistic.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      03/01/2020
  *
  * @brief     Statistic function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 03/01/2020 Created. @n
  * 03/01/2020 Standard deviation added. @n
  * 29/07/2026 Divide by zero guards added. The pending TODO notes @n
  *            about a zero length array are resolved. @n
  * 29/07/2026 statVariancei32 no longer routes an integer square through @n
  *            the double precision pow function. @n
  * 01/08/2026 The double precision sqrt calls are replaced with @n
  *            sqrtf, so the module no longer pulls in the software @n
  *            double routines on a single precision FPU. @n
  * 01/08/2026 Every function in this module carries the stat prefix @n
  *            now. The old names sat in the global namespace @n
  *            with no library marker, which invited a clash in @n
  *            any project that links other libraries. @n
  * 01/08/2026 The accumulator locals are named retVal, the way the @n
  *            rest of the library names a single exit value. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 statVarianceu32, statStandardDeviationu32 and @n
  *            statCovariancei32 are added, filling the gaps in the @n
  *            typed variants. @n
  *
  * @note      Every function returns zero for a zero length array.
  *
  ******************************************************************************
  */

#include <math.h>

#include "statistic.h"

/**
 * @brief   Calculates the population variance of the array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Population variance of the elements, or zero when length is zero.
 */
float statVariance ( const float* const array, uint32_t length )
{
    uint32_t i = 0;
    float sum = 0;
    float average = 0;
    float retVal = 0;
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
            retVal += ( difference * difference );
        }

        retVal /= length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Calculates the population variance of the signed 32-bit array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Population variance of the elements, or zero when length is zero.
 * @note    All arithmetic, including the mean, is done in int32_t with
 *          truncating integer division, so the result is less precise
 *          than statVariance().
 */
int32_t statVariancei32 ( const int32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    int32_t sum = 0;
    int32_t average = 0;
    int32_t retVal = 0;
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
            retVal += ( difference * difference );
        }

        retVal /= ( int32_t ) length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Calculates the standard deviation of the array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Standard deviation of the elements, or zero when length is zero.
 */
float statStandardDeviation ( const float* const array, uint32_t length )
{
    float retVal = 0;

    retVal = statVariance ( array, length );

    retVal = sqrtf ( retVal ); // Dep. math.h

    return ( retVal );
}

/**
 * @brief   Calculates the standard deviation of the signed 32-bit array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Standard deviation of the elements truncated to int32_t, or zero
 *          when length is zero.
 * @note    Built on statVariancei32, so it inherits that function's integer
 *          precision loss in addition to its own truncation of the sqrt
 *          result.
 */
int32_t statStandardDeviationi32 ( const int32_t* const array, uint32_t length )
{
    int32_t retVal = 0;

    retVal = statVariancei32 ( array, length );

    retVal = ( int32_t ) sqrtf ( ( float ) retVal ); // Dep. math.h

    return ( retVal );
}

/**
 * @brief   Calculates the population covariance between two arrays of the same length.
 * @param[in] array1  First array.
 * @param[in] array2  Second array, same length as array1.
 * @param[in] length  Number of elements in each array.
 * @return  Population covariance of the two arrays, or zero when length is zero.
 */
float statCovariance ( const float* const array1, const float* const array2, uint32_t length )
{
    uint32_t i = 0;
    float sum1 = 0, sum2 = 0;
    float average1 = 0, average2 = 0;
    float retVal = 0;

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
            retVal += ( ( array1[ i ] - average1 ) * ( array2[ i ] - average2 ) );
        }

        retVal /= length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Calculates the population variance of the unsigned 32-bit array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Population variance of the elements, or zero when length is zero.
 * @note    All arithmetic, including the mean, is done in uint32_t with
 *          truncating integer division, so the result is less precise than
 *          statVariance().
 * @note    Unsigned subtraction wraps instead of going negative, so each
 *          difference is taken in whichever direction is non negative. The
 *          square is the same either way.
 */
uint32_t statVarianceu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t sum = 0;
    uint32_t average = 0;
    uint32_t retVal = 0;
    uint32_t difference = 0;

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
            if ( array[ i ] > average )
            {
                difference = array[ i ] - average;
            }
            else
            {
                difference = average - array[ i ];
            }

            retVal += ( difference * difference );
        }

        retVal /= length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Calculates the standard deviation of the unsigned 32-bit array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Standard deviation of the elements truncated to uint32_t, or zero
 *          when length is zero.
 * @note    Built on statVarianceu32, so it inherits that function's integer
 *          precision loss in addition to its own truncation of the sqrt
 *          result.
 */
uint32_t statStandardDeviationu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t retVal = 0;

    retVal = statVarianceu32 ( array, length );

    retVal = ( uint32_t ) sqrtf ( ( float ) retVal ); // Dep. math.h

    return ( retVal );
}

/**
 * @brief   Calculates the population covariance between two signed 32-bit
 *          arrays of the same length.
 * @param[in] array1  First array.
 * @param[in] array2  Second array, same length as array1.
 * @param[in] length  Number of elements in each array.
 * @return  Population covariance of the two arrays, or zero when length is
 *          zero.
 * @note    All arithmetic, including both means, is done in int32_t with
 *          truncating integer division, so the result is less precise than
 *          statCovariance().
 */
int32_t statCovariancei32 ( const int32_t* const array1, const int32_t* const array2, uint32_t length )
{
    uint32_t i = 0;
    int32_t sum1 = 0, sum2 = 0;
    int32_t average1 = 0, average2 = 0;
    int32_t retVal = 0;

    if ( length != 0 )
    {
        for ( i = 0; i < length; ++i )
        {
            sum1 += array1[ i ];
            sum2 += array2[ i ];
        }

        // Average values of the arrays.
        average1 = sum1 / ( int32_t ) length;
        average2 = sum2 / ( int32_t ) length;

        for ( i = 0; i < length; ++i )
        {
            retVal += ( ( array1[ i ] - average1 ) * ( array2[ i ] - average2 ) );
        }

        retVal /= ( int32_t ) length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}
