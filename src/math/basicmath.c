/**
  ******************************************************************************
  *
  * @file      basicmath.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      03/06/2020
  *
  * @brief     Basic mathematics function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 03/06/2020 Created. @n
  * 10/07/2020 Average named mean. @n
  * 24/08/2020 Data type changed from double to float. @n
  * 09/04/2022 mathAbsolute, mathAbsolutei32 @n
  * 29/07/2026 Bug fix. mathFindMini32 compared with < instead of >, so @n
  *            it returned the maximum element. @n
  * 29/07/2026 Bug fix. mathCalculateMedian and mathCalculateMedianu32 did @n
  *            not average the two middle elements of an even sized @n
  *            array. @n
  * 29/07/2026 Zero length guards added. The find and calculate @n
  *            functions read array[ 0 ] or divided by length without @n
  *            checking it first. @n
  * 01/08/2026 Every function in this module carries the math prefix @n
  *            now. The old names sat in the global namespace @n
  *            with no library marker, which invited a clash in @n
  *            any project that links other libraries. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The i32 variants of findMinMax, calculateSum, @n
  *            calculateMean, calculateMedian and calculateRange are @n
  *            added. Only float and u32 existed before. @n
  *
  * @note      Every function returns zero for a zero length array.
  *
  ******************************************************************************
  */

#include "basicmath.h"

/**
 * @brief   Computes the absolute value of a floating point number.
 * @param[in] inp  Value to take the absolute value of.
 * @return  inp if it is non-negative, otherwise -inp.
 */
float mathAbsolute ( float inp )
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

/**
 * @brief   Computes the absolute value of a signed 32-bit integer.
 * @param[in] inp  Value to take the absolute value of.
 * @return  inp if it is non-negative, otherwise -inp.
 */
int32_t mathAbsolutei32 ( int32_t inp )
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

/**
 * @brief   Finds the largest element of the array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The largest element, or zero when length is zero.
 */
float mathFindMax ( const float* const array, uint32_t length )
{
    uint32_t i = 0;
    float tempMax = 0;

    if ( length != 0 )
    {
        tempMax = array[ 0 ];

        for ( i = 1; i < length; ++i )
        {
            if ( tempMax < array[ i ] )
            {
                tempMax = array[ i ];
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( tempMax );
}

/**
 * @brief   Finds the largest element of the unsigned 32-bit array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The largest element, or zero when length is zero.
 */
uint32_t mathFindMaxu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t tempMax = 0;

    if ( length != 0 )
    {
        tempMax = array[ 0 ];

        for ( i = 1; i < length; ++i )
        {
            if ( tempMax < array[ i ] )
            {
                tempMax = array[ i ];
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( tempMax );
}

/**
 * @brief   Finds the largest element of the signed 32-bit array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The largest element, or zero when length is zero.
 */
int32_t mathFindMaxi32 ( const int32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    int32_t tempMax = 0;

    if ( length != 0 )
    {
        tempMax = array[ 0 ];

        for ( i = 1; i < length; ++i )
        {
            if ( tempMax < array[ i ] )
            {
                tempMax = array[ i ];
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( tempMax );
}

/**
 * @brief   Finds the smallest element of the array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The smallest element, or zero when length is zero.
 */
float mathFindMin ( const float* const array, uint32_t length )
{
    uint32_t i = 0;
    float tempMin = 0;

    if ( length != 0 )
    {
        tempMin = array[ 0 ];

        for ( i = 1; i < length; ++i )
        {
            if ( tempMin > array[ i ] )
            {
                tempMin = array[ i ];
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( tempMin );
}

/**
 * @brief   Finds the smallest element of the unsigned 32-bit array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The smallest element, or zero when length is zero.
 */
uint32_t mathFindMinu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t tempMin = 0;

    if ( length != 0 )
    {
        tempMin = array[ 0 ];

        for ( i = 1; i < length; ++i )
        {
            if ( tempMin > array[ i ] )
            {
                tempMin = array[ i ];
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( tempMin );
}

/**
 * @brief   Finds the smallest element of the signed 32-bit array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  The smallest element, or zero when length is zero.
 */
int32_t mathFindMini32 ( const int32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    int32_t tempMin = 0;

    if ( length != 0 )
    {
        tempMin = array[ 0 ];

        for ( i = 1; i < length; ++i )
        {
            if ( tempMin > array[ i ] )
            {
                tempMin = array[ i ];
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( tempMin );
}

/**
 * @brief   Finds the smallest and largest elements of the array in one pass.
 * @param[in]  array   Array to scan.
 * @param[in]  length  Number of elements in the array.
 * @param[out] min     Set to the smallest element, or zero when length is zero.
 * @param[out] max     Set to the largest element, or zero when length is zero.
 */
void mathFindMinMax ( const float* const array, uint32_t length, float* min, float* max )
{
    uint32_t i = 0;
    float tempMin = 0;
    float tempMax = 0;

    if ( length != 0 )
    {
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
    }
    else
    {
        /* Intentionally blank. */
    }

    *min = tempMin;
    *max = tempMax;
}

/**
 * @brief   Finds the smallest and largest elements of the unsigned 32-bit array,
 *          in one pass.
 * @param[in]  array   Array to scan.
 * @param[in]  length  Number of elements in the array.
 * @param[out] min     Set to the smallest element, or zero when length is zero.
 * @param[out] max     Set to the largest element, or zero when length is zero.
 */
void mathFindMinMaxu32 ( const uint32_t* const array, uint32_t length, uint32_t* min, uint32_t* max )
{
    uint32_t i = 0;
    uint32_t tempMin = 0;
    uint32_t tempMax = 0;

    if ( length != 0 )
    {
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
    }
    else
    {
        /* Intentionally blank. */
    }

    *min = tempMin;
    *max = tempMax;
}

/**
 * @brief   Calculates the sum of the array elements.
 * @param[in] array   Array to sum.
 * @param[in] length  Number of elements in the array.
 * @return  Sum of the elements, or zero when length is zero.
 */
float mathCalculateSum ( const float* const array, uint32_t length )
{
    uint32_t i = 0;
    float sum = 0;

    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }

    return ( sum );
}

/**
 * @brief   Calculates the sum of the unsigned 32-bit array elements.
 * @param[in] array   Array to sum.
 * @param[in] length  Number of elements in the array.
 * @return  Sum of the elements, or zero when length is zero.
 */
uint32_t mathCalculateSumu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t sum = 0;

    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }

    return ( sum );
}

/**
 * @brief   Calculates the arithmetic mean of the array elements.
 * @param[in] array   Array to average.
 * @param[in] length  Number of elements in the array.
 * @return  Arithmetic mean of the elements, or zero when length is zero.
 */
float mathCalculateMean ( const float* const array, uint32_t length )
{
    uint32_t i = 0;
    float sum = 0;
    float average = 0;

    if ( length != 0 )
    {
        for ( i = 0; i < length; ++i )
        {
            sum += array[ i ];
        }

        average = sum / length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( average );
}

/**
 * @brief   Calculates the arithmetic mean of the unsigned 32-bit array elements.
 * @param[in] array   Array to average.
 * @param[in] length  Number of elements in the array.
 * @return  Arithmetic mean of the elements, or zero when length is zero.
 */
uint32_t mathCalculateMeanu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t sum = 0;
    uint32_t average = 0;

    if ( length != 0 )
    {
        for ( i = 0; i < length; ++i )
        {
            sum += array[ i ];
        }

        average = sum / length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( average );
}

/**
 * @brief   Finds the median value of the array.
 * @param[in] array   Sorted array to find the median of.
 * @param[in] length  Number of elements in the array.
 * @return  The mean of the two middle elements for an even length array,
 *          the middle element for an odd length array, or zero when
 *          length is zero.
 * @note    array must be sorted in ascending order. An unsorted array
 *          produces a wrong result without any indication of error.
 */
float mathCalculateMedian ( const float* const array, uint32_t length )
{
    float retVal = 0;

    if ( length == 0 )
    {
        retVal = 0;
    }
    else if ( ( length % 2 ) == 0 )
    {
        // Even length. The median is the mean of the two middle elements.
        retVal = ( array[ ( length / 2 ) - 1 ] + array[ ( length / 2 ) ] ) / 2;
    }
    else
    {
        retVal = array[ ( length / 2 ) ];
    }

    return ( retVal );
}

/**
 * @brief   Finds the median value of the unsigned 32-bit array.
 * @param[in] array   Sorted array to find the median of.
 * @param[in] length  Number of elements in the array.
 * @return  The mean of the two middle elements for an even length array,
 *          the middle element for an odd length array, or zero when
 *          length is zero.
 * @note    array must be sorted in ascending order. An unsorted array
 *          produces a wrong result without any indication of error.
 * @note    The even length case computes low + ( ( high - low ) / 2 )
 *          instead of ( low + high ) / 2 to avoid overflowing uint32_t
 *          when low and high are both large.
 */
uint32_t mathCalculateMedianu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;

    if ( length == 0 )
    {
        retVal = 0;
    }
    else if ( ( length % 2 ) == 0 )
    {
        // Even length. The median is the mean of the two middle elements.
        low = array[ ( length / 2 ) - 1 ];
        high = array[ ( length / 2 ) ];

        // The array is sorted, so low <= high. This form cannot overflow.
        retVal = low + ( ( high - low ) / 2 );
    }
    else
    {
        retVal = array[ ( length / 2 ) ];
    }

    return ( retVal );
}

/**
 * @brief   Calculates the range (largest minus smallest element) of the array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  Difference between the largest and smallest elements, or zero
 *          when length is zero.
 */
float mathCalculateRange ( const float* const array, uint32_t length )
{
    float tempMin = 0;
    float tempMax = 0;
    uint32_t tempLength = 0;

    tempLength = length;

    mathFindMinMax ( array, tempLength, &tempMin, &tempMax );

    return ( tempMax - tempMin );
}

/**
 * @brief   Calculates the range (largest minus smallest element) of the
 *          unsigned 32-bit array.
 * @param[in] array   Array to scan.
 * @param[in] length  Number of elements in the array.
 * @return  Difference between the largest and smallest elements, or zero
 *          when length is zero.
 */
uint32_t mathCalculateRangeu32 ( const uint32_t* const array, uint32_t length )
{
    uint32_t tempMin = 0;
    uint32_t tempMax = 0;
    uint32_t tempLength = 0;

    tempLength = length;

    mathFindMinMaxu32 ( array, tempLength, &tempMin, &tempMax );

    return ( tempMax - tempMin );
}

/**
 * @brief   Finds the smallest and largest elements of the signed 32-bit array in one pass.
 * @param[in]  array   Array to scan.
 * @param[in]  length  Number of elements in the array.
 * @param[out] min     Set to the smallest element, or zero when length is zero.
 * @param[out] max     Set to the largest element, or zero when length is zero.
 */
void mathFindMinMaxi32 ( const int32_t* const array, uint32_t length, int32_t* min, int32_t* max )
{
    uint32_t i = 0;
    int32_t tempMin = 0;
    int32_t tempMax = 0;

    if ( length != 0 )
    {
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
    }
    else
    {
        /* Intentionally blank. */
    }

    *min = tempMin;
    *max = tempMax;
}

/**
 * @brief   Sums the elements of the signed 32-bit array.
 * @param[in] array   Array to sum.
 * @param[in] length  Number of elements in the array.
 * @return  Sum of the elements, or zero when length is zero.
 * @note    The sum is accumulated in int32_t and is not checked for
 *          overflow, the same way mathCalculateSumu32 works.
 */
int32_t mathCalculateSumi32 ( const int32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    int32_t sum = 0;

    for ( i = 0; i < length; ++i )
    {
        sum += array[ i ];
    }

    return ( sum );
}

/**
 * @brief   Calculates the mean of the signed 32-bit array elements.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Mean of the elements truncated towards zero, or zero when length
 *          is zero.
 */
int32_t mathCalculateMeani32 ( const int32_t* const array, uint32_t length )
{
    uint32_t i = 0;
    int32_t sum = 0;
    int32_t average = 0;

    if ( length != 0 )
    {
        for ( i = 0; i < length; ++i )
        {
            sum += array[ i ];
        }

        average = sum / ( int32_t ) length;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( average );
}

/**
 * @brief   Returns the median of the already sorted signed 32-bit array.
 * @param[in] array   Sorted array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Median of the elements, or zero when length is zero.
 * @note    The array must already be sorted. This function does not sort it.
 * @note    For an even length the two middle elements are averaged towards
 *          the lower one. The midpoint is taken as low + ( ( high - low ) / 2 )
 *          rather than ( low + high ) / 2, so the sum cannot overflow. The
 *          difference itself still overflows if the two middle elements span
 *          more than INT32_MAX.
 */
int32_t mathCalculateMediani32 ( const int32_t* const array, uint32_t length )
{
    int32_t retVal = 0;
    int32_t low = 0;
    int32_t high = 0;

    if ( length == 0 )
    {
        retVal = 0;
    }
    else if ( ( length % 2 ) == 0 )
    {
        // Even length. The median is the mean of the two middle elements.
        low = array[ ( length / 2 ) - 1 ];
        high = array[ ( length / 2 ) ];

        // The array is sorted, so low <= high and the difference is positive.
        retVal = low + ( ( high - low ) / 2 );
    }
    else
    {
        retVal = array[ ( length / 2 ) ];
    }

    return ( retVal );
}

/**
 * @brief   Calculates the span between the smallest and largest elements of
 *          the signed 32-bit array.
 * @param[in] array   Array to analyze.
 * @param[in] length  Number of elements in the array.
 * @return  Largest element minus smallest element, or zero when length is
 *          zero.
 * @note    The subtraction overflows if the array spans more than INT32_MAX,
 *          which the caller must avoid.
 */
int32_t mathCalculateRangei32 ( const int32_t* const array, uint32_t length )
{
    int32_t tempMin = 0;
    int32_t tempMax = 0;

    mathFindMinMaxi32 ( array, length, &tempMin, &tempMax );

    return ( tempMax - tempMin );
}
