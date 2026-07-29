/**
  ******************************************************************************
  *
  * @file      search.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.4
  * @date      15/09/2021
  *
  * @brief     Search function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2021 Created. @n
  * 20/09/2021 linearSearch_i32 is added to the library. @n
  * 20/09/2021 binarySearch_i32 is added to the library. @n
  * 29/07/2026 Bug fix. A stray semicolon after the if statement of @n
  *            linearSearch made the body run unconditionally, so the @n
  *            function always reported a match at index 0. @n
  * 29/07/2026 Bug fix. length - 1 underflowed to 0xFFFFFFFF in the @n
  *            binary search functions for a zero length array. @n
  *
  * @note      Every function reports FALSE for a zero length array and leaves
  *            foundIndex untouched.
  *
  ******************************************************************************
  */

#include "search.h"

/**
 * @brief   Compares two floating point numbers for equality within a tolerance.
 * @param[in] f1       First value.
 * @param[in] f2       Second value.
 * @param[in] epsilon  Largest absolute difference still counted as equal.
 * @return  TRUE when the absolute difference between f1 and f2 is less than
 *          or equal to epsilon, FALSE otherwise.
 */
static uint8_t isEqualf ( float f1, float f2, float epsilon )
{
    uint8_t retVal = FALSE;

    if ( f1 > f2 )
    {
        retVal = ( ( f1 - f2 ) <= epsilon );
    }
    else
    {
        retVal = ( ( f2 - f1 ) <= epsilon );
    }

    return ( retVal );
}

/**
 * @brief   Searches the array element by element for a matching item.
 * @param[in]  array       Array to search.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the first match. Untouched when no match.
 * @param[in]  epsilon     Largest difference still counted as equal.
 * @return  TRUE when a match was found, FALSE otherwise, including when
 *          length is zero.
 */
uint8_t linearSearch ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( isEqualf ( array[ i ], item, epsilon ) == TRUE )
        {
            ( *foundIndex ) = i;
            retVal = TRUE;
            break;
        }
    }

    return ( retVal );
}

/**
 * @brief   Searches the unsigned 32-bit array element by element for a matching item.
 * @param[in]  array       Array to search.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the first match. Untouched when no match.
 * @return  TRUE when a match was found, FALSE otherwise, including when
 *          length is zero.
 */
uint8_t linearSearchu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( array[ i ] == item )
        {
            ( *foundIndex ) = i;
            retVal = TRUE;
            break;
        }
    }

    return ( retVal );
}

/**
 * @brief   Searches the signed 32-bit array element by element for a matching item.
 * @param[in]  array       Array to search.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the first match. Untouched when no match.
 * @return  TRUE when a match was found, FALSE otherwise, including when
 *          length is zero.
 */
uint8_t linearSearchi32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( array[ i ] == item )
        {
            ( *foundIndex ) = i;
            retVal = TRUE;
            break;
        }
    }

    return ( retVal );
}

/**
 * @brief   Searches a sorted array for a matching item using binary search.
 * @param[in]  array       Array to search, sorted in ascending order.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of a match. Untouched when no match.
 * @param[in]  epsilon     Largest difference still counted as equal.
 * @return  TRUE when a match was found, FALSE otherwise, including when
 *          length is zero.
 * @note    array must be sorted in ascending order. An unsorted array
 *          produces a wrong result without any indication of error.
 */
uint8_t binarySearch ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
{
    uint8_t retVal = FALSE;
    uint32_t l = 0;
    uint32_t r = 0;
    uint32_t m = 0;

    if ( length != 0 )
    {
        r = length - 1;

        while ( l <= r )
        {
            m = l + ( ( r - l ) >> 1 ); // divide by 2

            if ( isEqualf ( array[ m ], item, epsilon ) == TRUE )
            {
                ( *foundIndex ) = m;
                retVal = TRUE;
                break;
            }

            if ( array[ m ] < item )
            {
                l = m + 1;
            }
            else
            {
                if ( m == 0 )
                {
                    // The item is below the first element. Ending the search
                    // this way keeps the unsigned right bound from underflowing.
                    l = m + 1;
                }
                else
                {
                    r = m - 1;
                }
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Searches a sorted unsigned 32-bit array for a matching item using binary search.
 * @param[in]  array       Array to search, sorted in ascending order.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of a match. Untouched when no match.
 * @return  TRUE when a match was found, FALSE otherwise, including when
 *          length is zero.
 * @note    array must be sorted in ascending order. An unsorted array
 *          produces a wrong result without any indication of error.
 */
uint8_t binarySearchu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t l = 0;
    uint32_t r = 0;
    uint32_t m = 0;

    if ( length != 0 )
    {
        r = length - 1;

        while ( l <= r )
        {
            m = l + ( ( r - l ) >> 1 ); // divide by 2

            if ( array[ m ] == item )
            {
                ( *foundIndex ) = m;
                retVal = TRUE;
                break;
            }

            if ( array[ m ] < item )
            {
                l = m + 1;
            }
            else
            {
                if ( m == 0 )
                {
                    // The item is below the first element. Ending the search
                    // this way keeps the unsigned right bound from underflowing.
                    l = m + 1;
                }
                else
                {
                    r = m - 1;
                }
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Searches a sorted signed 32-bit array for a matching item using binary search.
 * @param[in]  array       Array to search, sorted in ascending order.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of a match. Untouched when no match.
 * @return  TRUE when a match was found, FALSE otherwise, including when
 *          length is zero.
 * @note    array must be sorted in ascending order. An unsorted array
 *          produces a wrong result without any indication of error.
 */
uint8_t binarySearchi32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t l = 0;
    uint32_t r = 0;
    uint32_t m = 0;

    if ( length != 0 )
    {
        r = length - 1;

        while ( l <= r )
        {
            m = l + ( ( r - l ) >> 1 ); // divide by 2

            if ( array[ m ] == item )
            {
                ( *foundIndex ) = m;
                retVal = TRUE;
                break;
            }

            if ( array[ m ] < item )
            {
                l = m + 1;
            }
            else
            {
                if ( m == 0 )
                {
                    // The item is below the first element. Ending the search
                    // this way keeps the unsigned right bound from underflowing.
                    l = m + 1;
                }
                else
                {
                    r = m - 1;
                }
            }
        }
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}
