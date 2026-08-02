/**
  ******************************************************************************
  *
  * @file      search.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
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
  *            searchLinear made the body run unconditionally, so the @n
  *            function always reported a match at index 0. @n
  * 29/07/2026 Bug fix. length - 1 underflowed to 0xFFFFFFFF in the @n
  *            binary search functions for a zero length array. @n
  * 01/08/2026 Every function in this module carries the search prefix @n
  *            now. The old names sat in the global namespace @n
  *            with no library marker, which invited a clash in @n
  *            any project that links other libraries. @n
  * 02/08/2026 searchLowerBound and searchUpperBound added for the @n
  *            three types. They answer where an item belongs rather @n
  *            than whether it is present, which is what a caller @n
  *            keeping an array sorted by hand needs, and what @n
  *            searchBinary could not say. @n
  * 02/08/2026 searchClosest added for the three types. A calibration @n
  *            or linearisation table wants the nearest entry, not an @n
  *            exact one, and on float data an exact match is close to @n
  *            useless in the first place. @n
  *
  * @note      Every function that reports a status returns FALSE for a zero
  *            length array and leaves foundIndex untouched.
  *
  * @note      searchLowerBound and searchUpperBound return an index rather
  *            than a status, because their answer always exists. For a zero
  *            length array that answer is 0, which is at once one past the
  *            end and the only position an insert can go.
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
uint8_t searchLinear ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
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
uint8_t searchLinearu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex )
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
uint8_t searchLineari32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
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
uint8_t searchBinary ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
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
uint8_t searchBinaryu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex )
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
uint8_t searchBinaryi32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
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
 * @brief   Finds the position of the first element that is not below the item.
 * @param[in] array   Array to search, sorted in ascending order.
 * @param[in] length  Number of elements in the array.
 * @param[in] item    Value to place.
 * @return  Index of the first element greater than or equal to item, or length
 *          when every element is below it.
 * @note    No status to check. The answer always exists, and length is a
 *          meaningful part of the range rather than an error: it says the item
 *          belongs past the last element.
 * @note    The returned index is where an insert has to go to leave the array
 *          sorted, whether or not the item is already present. That makes this
 *          the function to reach for when a sorted table is built up a value at
 *          a time.
 * @note    With duplicates this lands on the first of them, which searchBinary
 *          does not promise. searchUpperBound lands one past the last, so the
 *          difference between the two is how many times the item occurs.
 * @note    The window is half open, low inclusive and high exclusive, so high
 *          never has to step below low and the unsigned indices cannot
 *          underflow.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 * @note    epsilon plays no part here, unlike searchBinary. The question is
 *          which side of the item an element falls on, not whether it is near
 *          enough to count as equal.
 */
uint32_t searchLowerBound ( const float* const array, uint32_t length, float item )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( array[ mid ] < item )
        {
            low = mid + 1u;
        }
        else
        {
            high = mid;
        }
    }

    retVal = low;

    return ( retVal );
}

/**
 * @brief   Finds the position of the first unsigned 32-bit element that is not
 *          below the item.
 * @param[in] array   Array to search, sorted in ascending order.
 * @param[in] length  Number of elements in the array.
 * @param[in] item    Value to place.
 * @return  Index of the first element greater than or equal to item, or length
 *          when every element is below it.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint32_t searchLowerBoundu32 ( const uint32_t* const array, uint32_t length, uint32_t item )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( array[ mid ] < item )
        {
            low = mid + 1u;
        }
        else
        {
            high = mid;
        }
    }

    retVal = low;

    return ( retVal );
}

/**
 * @brief   Finds the position of the first signed 32-bit element that is not
 *          below the item.
 * @param[in] array   Array to search, sorted in ascending order.
 * @param[in] length  Number of elements in the array.
 * @param[in] item    Value to place.
 * @return  Index of the first element greater than or equal to item, or length
 *          when every element is below it.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint32_t searchLowerBoundi32 ( const int32_t* const array, uint32_t length, int32_t item )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( array[ mid ] < item )
        {
            low = mid + 1u;
        }
        else
        {
            high = mid;
        }
    }

    retVal = low;

    return ( retVal );
}

/**
 * @brief   Finds the position one past the last element that is not above the item.
 * @param[in] array   Array to search, sorted in ascending order.
 * @param[in] length  Number of elements in the array.
 * @param[in] item    Value to place.
 * @return  Index of the first element strictly greater than item, or length
 *          when no element is.
 * @note    Differs from searchLowerBound in one comparison: this one steps past
 *          an element equal to the item, the other stops on it. So for an item
 *          that is not present the two agree, and for one that is they bracket
 *          its run.
 * @note    searchUpperBound minus searchLowerBound is how many times the item
 *          occurs, and costs two binary searches rather than a scan.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint32_t searchUpperBound ( const float* const array, uint32_t length, float item )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( array[ mid ] > item )
        {
            high = mid;
        }
        else
        {
            low = mid + 1u;
        }
    }

    retVal = low;

    return ( retVal );
}

/**
 * @brief   Finds the position one past the last unsigned 32-bit element that is
 *          not above the item.
 * @param[in] array   Array to search, sorted in ascending order.
 * @param[in] length  Number of elements in the array.
 * @param[in] item    Value to place.
 * @return  Index of the first element strictly greater than item, or length
 *          when no element is.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint32_t searchUpperBoundu32 ( const uint32_t* const array, uint32_t length, uint32_t item )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( array[ mid ] > item )
        {
            high = mid;
        }
        else
        {
            low = mid + 1u;
        }
    }

    retVal = low;

    return ( retVal );
}

/**
 * @brief   Finds the position one past the last signed 32-bit element that is
 *          not above the item.
 * @param[in] array   Array to search, sorted in ascending order.
 * @param[in] length  Number of elements in the array.
 * @param[in] item    Value to place.
 * @return  Index of the first element strictly greater than item, or length
 *          when no element is.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint32_t searchUpperBoundi32 ( const int32_t* const array, uint32_t length, int32_t item )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( array[ mid ] > item )
        {
            high = mid;
        }
        else
        {
            low = mid + 1u;
        }
    }

    retVal = low;

    return ( retVal );
}

/**
 * @brief   Finds the element nearest to the item, whether or not it matches.
 * @param[in]  array       Array to search, sorted in ascending order.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the nearest element. Untouched when the
 *                         array is empty.
 * @return  TRUE when an index was produced, FALSE only when length is zero.
 * @note    This is the lookup a calibration or linearisation table actually
 *          wants. searchBinary answers whether a value is present, which for
 *          float data is rarely the question and rarely yes; this answers which
 *          entry to read, which always has a sensible answer.
 * @note    An item outside the table clamps to the nearer end rather than
 *          failing, so a reading past the ends of a calibration table gives the
 *          first or last entry instead of nothing.
 * @note    Where two entries are equally near, the lower index wins, which
 *          keeps the result steady as a reading dithers around the midpoint
 *          between them. Where several entries hold that same nearest value,
 *          the first of them is reported, so the answer depends on the contents
 *          of the table and not on the shape of the search that walked it.
 * @note    Costs one binary search, or two when the answer is at or below the
 *          item, since the second one walks back to the first entry holding the
 *          value the first search landed on. Still O(log N).
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint8_t searchClosest ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t pos = 0;
    float distBelow = 0;
    float distAbove = 0;

    if ( length != 0 )
    {
        pos = searchLowerBound ( array, length, item );

        if ( pos == 0 )
        {
            // The item sits at or below the first element, and index 0 is
            // already the first entry holding that value.
            ( *foundIndex ) = 0;
        }
        else if ( pos == length )
        {
            // The item sits above every element, so the last one is nearest.
            // It can be the tail of a run of equal values, so step back to the
            // head of that run.
            ( *foundIndex ) = searchLowerBound ( array, length, array[ length - 1u ] );
        }
        else
        {
            // The item falls between the two neighbours of pos, so that
            // array[pos - 1] is below it and array[pos] is at or above it.
            distBelow = item - array[ pos - 1u ];
            distAbove = array[ pos ] - item;

            if ( distAbove < distBelow )
            {
                // pos is where searchLowerBound stopped, so no earlier element
                // can hold this value and pos is already the first of its run.
                ( *foundIndex ) = pos;
            }
            else
            {
                ( *foundIndex ) = searchLowerBound ( array, length, array[ pos - 1u ] );
            }
        }

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Finds the unsigned 32-bit element nearest to the item, whether or
 *          not it matches.
 * @param[in]  array       Array to search, sorted in ascending order.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the nearest element. Untouched when the
 *                         array is empty.
 * @return  TRUE when an index was produced, FALSE only when length is zero.
 * @note    Where two entries are equally near, the lower index wins, and where
 *          several hold that nearest value, the first of them is reported.
 * @note    Neither subtraction can underflow. array[pos - 1] is strictly below
 *          the item and array[pos] is at or above it, which is exactly what
 *          searchLowerBound guarantees about the position it returns.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint8_t searchClosestu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t pos = 0;
    uint32_t distBelow = 0;
    uint32_t distAbove = 0;

    if ( length != 0 )
    {
        pos = searchLowerBoundu32 ( array, length, item );

        if ( pos == 0 )
        {
            ( *foundIndex ) = 0;
        }
        else if ( pos == length )
        {
            ( *foundIndex ) = searchLowerBoundu32 ( array, length, array[ length - 1u ] );
        }
        else
        {
            distBelow = item - array[ pos - 1u ];
            distAbove = array[ pos ] - item;

            if ( distAbove < distBelow )
            {
                ( *foundIndex ) = pos;
            }
            else
            {
                ( *foundIndex ) = searchLowerBoundu32 ( array, length, array[ pos - 1u ] );
            }
        }

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}

/**
 * @brief   Finds the signed 32-bit element nearest to the item, whether or not
 *          it matches.
 * @param[in]  array       Array to search, sorted in ascending order.
 * @param[in]  length      Number of elements in the array.
 * @param[in]  item        Value to look for.
 * @param[out] foundIndex  Index of the nearest element. Untouched when the
 *                         array is empty.
 * @return  TRUE when an index was produced, FALSE only when length is zero.
 * @note    Where two entries are equally near, the lower index wins, and where
 *          several hold that nearest value, the first of them is reported.
 * @note    The two distances are computed on the values cast to uint32_t, not
 *          subtracted as signed. The gap between two int32_t values can reach
 *          just under 2 to the 32nd, which overflows a signed subtraction and
 *          is undefined; an unsigned one wraps by definition and lands on the
 *          exact distance. An item of INT32_MAX against an element of INT32_MIN
 *          is the case that breaks the naive form.
 * @note    array must be sorted in ascending order. An unsorted array produces
 *          a wrong result without any indication of error.
 */
uint8_t searchClosesti32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t pos = 0;
    uint32_t distBelow = 0;
    uint32_t distAbove = 0;

    if ( length != 0 )
    {
        pos = searchLowerBoundi32 ( array, length, item );

        if ( pos == 0 )
        {
            ( *foundIndex ) = 0;
        }
        else if ( pos == length )
        {
            ( *foundIndex ) = searchLowerBoundi32 ( array, length, array[ length - 1u ] );
        }
        else
        {
            distBelow = ( uint32_t ) item - ( uint32_t ) array[ pos - 1u ];
            distAbove = ( uint32_t ) array[ pos ] - ( uint32_t ) item;

            if ( distAbove < distBelow )
            {
                ( *foundIndex ) = pos;
            }
            else
            {
                ( *foundIndex ) = searchLowerBoundi32 ( array, length, array[ pos - 1u ] );
            }
        }

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank. */
    }

    return ( retVal );
}
