/**
  ******************************************************************************
  *
  * @file      basicmath.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
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
  * 05/08/2026 Scalar primitives: mathClamp, mathMap and mathLerp with @n
  *            their typed variants. Everything here operated on @n
  *            arrays until now, so the three lines an embedded @n
  *            project rewrites most often had to be written by hand. @n
  *
  * @note      Every array function returns zero for a zero length array.
  *            The scalar primitives at the end of this file take values
  *            rather than arrays and have no such case.
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

/**
 * @brief   Constrains a value to a range.
 * @param[in] value  Value to constrain.
 * @param[in] low    Smallest value that may be returned.
 * @param[in] high   Largest value that may be returned.
 * @return  value when it lies between low and high, otherwise whichever end
 *          it passed. The endpoints are inside the range.
 * @note    An inverted range, where low is above high, collapses onto low.
 *          The comparisons are made in that order and nothing here detects
 *          the mistake.
 */
float mathClamp ( float value, float low, float high )
{
    float retVal = 0;

    if ( value < low )
    {
        retVal = low;
    }
    else if ( value > high )
    {
        retVal = high;
    }
    else
    {
        retVal = value;
    }

    return ( retVal );
}

/**
 * @brief   Constrains an unsigned 32-bit value to a range.
 * @param[in] value  Value to constrain.
 * @param[in] low    Smallest value that may be returned.
 * @param[in] high   Largest value that may be returned.
 * @return  value when it lies between low and high, otherwise whichever end
 *          it passed.
 */
uint32_t mathClampu32 ( uint32_t value, uint32_t low, uint32_t high )
{
    uint32_t retVal = 0;

    if ( value < low )
    {
        retVal = low;
    }
    else if ( value > high )
    {
        retVal = high;
    }
    else
    {
        retVal = value;
    }

    return ( retVal );
}

/**
 * @brief   Constrains a signed 32-bit value to a range.
 * @param[in] value  Value to constrain.
 * @param[in] low    Smallest value that may be returned.
 * @param[in] high   Largest value that may be returned.
 * @return  value when it lies between low and high, otherwise whichever end
 *          it passed.
 */
int32_t mathClampi32 ( int32_t value, int32_t low, int32_t high )
{
    int32_t retVal = 0;

    if ( value < low )
    {
        retVal = low;
    }
    else if ( value > high )
    {
        retVal = high;
    }
    else
    {
        retVal = value;
    }

    return ( retVal );
}

/**
 * @brief   Rescales a value from one range to another.
 * @param[in] value    Value to rescale.
 * @param[in] inLow    Low end of the range value is measured in.
 * @param[in] inHigh   High end of that range.
 * @param[in] outLow   Low end of the range to rescale into.
 * @param[in] outHigh  High end of that range.
 * @return  The rescaled value, or outLow when the input range has zero
 *          width.
 * @note    This does not clamp. A value outside the input range is
 *          extrapolated, which is the useful behaviour and is why mathClamp
 *          is a separate function rather than folded in here.
 * @note    Either range may descend. An outHigh below outLow reverses the
 *          sense, which is what an inverted sensor needs.
 * @note    A zero width input range would divide by zero. The whole input
 *          collapses to a point, so outLow is the defensible answer and it
 *          costs one comparison to return it instead of an inf.
 */
float mathMap ( float value, float inLow, float inHigh, float outLow, float outHigh )
{
    float retVal = 0;

    if ( ( inHigh - inLow ) == 0 )
    {
        retVal = outLow;
    }
    else
    {
        retVal = outLow + ( ( ( value - inLow ) * ( outHigh - outLow ) ) /
                            ( inHigh - inLow ) );
    }

    return ( retVal );
}

/**
 * @brief   Rescales a signed 32-bit value from one range to another.
 * @param[in] value    Value to rescale.
 * @param[in] inLow    Low end of the range value is measured in.
 * @param[in] inHigh   High end of that range.
 * @param[in] outLow   Low end of the range to rescale into.
 * @param[in] outHigh  High end of that range.
 * @return  The rescaled value rounded to the nearest integer, or outLow when
 *          the input range has zero width.
 * @note    Every intermediate is int64_t, the subtractions included. This is
 *          the same exposure interp's integer path has: a twelve bit reading
 *          scaled to millivolts already reaches seven digits, and the
 *          product overflows an int32_t long before either range looks
 *          unreasonable.
 * @note    The division rounds to nearest rather than truncating toward
 *          zero, so the rounding follows the sign of the numerator. The
 *          denominator may be negative here, unlike in interp, so its
 *          magnitude is taken first.
 * @note    Like the float variant this does not clamp, and either range may
 *          descend.
 */
int32_t mathMapi32 ( int32_t value, int32_t inLow, int32_t inHigh, int32_t outLow, int32_t outHigh )
{
    int32_t retVal = 0;
    int64_t num = 0;
    int64_t den = 0;
    int64_t half = 0;

    den = ( int64_t ) inHigh - ( int64_t ) inLow;

    if ( den == 0 )
    {
        retVal = outLow;
    }
    else
    {
        num = ( ( int64_t ) value - ( int64_t ) inLow ) *
              ( ( int64_t ) outHigh - ( int64_t ) outLow );

        half = den / 2;

        if ( half < 0 )
        {
            half = -half;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( num >= 0 )
        {
            num += half;
        }
        else
        {
            num -= half;
        }

        retVal = outLow + ( int32_t ) ( num / den );
    }

    return ( retVal );
}

/**
 * @brief   Interpolates linearly between two values.
 * @param[in] from  Value returned at t of zero.
 * @param[in] to    Value returned at t of one.
 * @param[in] t     Position between the two, normally between zero and one.
 * @return  The interpolated value.
 * @note    This does not clamp t. Values outside zero to one extrapolate,
 *          which is deliberate; mathClamp is there when that is unwanted.
 * @note    Written as from + t * ( to - from ) rather than the
 *          algebraically equal ( 1 - t ) * from + t * to. The first is one
 *          multiply cheaper and the second is exact at t of one, which is
 *          the trade being made.
 */
float mathLerp ( float from, float to, float t )
{
    float retVal = 0;

    retVal = from + ( t * ( to - from ) );

    return ( retVal );
}
