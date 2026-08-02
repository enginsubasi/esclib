/**
  ******************************************************************************
  *
  * @file      sort.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      24/12/2021
  *
  * @brief     Sort function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 24/12/2021 Created. @n
  * 26/12/2021 sortBubble added. @n
  * 20/01/2022 sortBubble added. @n
  * 29/07/2026 Bug fix. length - 1 underflowed to 0xFFFFFFFF for a @n
  *            zero length array, which drove the loops far past the @n
  *            end of the buffer. @n
  * 29/07/2026 The insertion sort inner loop no longer relies on the @n
  *            unsigned index wrapping to 0xFFFFFFFF. @n
  * 01/08/2026 Every function in this module carries the sort prefix @n
  *            now. The old names sat in the global namespace @n
  *            with no library marker, which invited a clash in @n
  *            any project that links other libraries. @n
  * 02/08/2026 sortHeap added for the three types. The other three @n
  *            sorts are all O(N squared), which is fine for the short @n
  *            arrays this library usually sees and is not fine past a @n
  *            few hundred elements. Heap sort is the one that suits a @n
  *            small target: in place, no recursion, and its bound is @n
  *            a guarantee rather than an average. @n
  * 02/08/2026 sortIsSorted added for the three types. searchBinary @n
  *            gives a wrong answer on an unsorted array with no @n
  *            indication of error, and there was no cheap way for a @n
  *            caller to check the precondition. @n
  * 02/08/2026 sortReverse added for the three types, so descending @n
  *            order costs one extra pass instead of a second copy of @n
  *            every sort in the file. @n
  *
  * @note      A zero or one element array is already sorted, so every function
  *            below leaves it untouched.
  *
  * @note      Every sort here is ascending. Follow one with sortReverse for
  *            descending order.
  *
  ******************************************************************************
  */

#include "sort.h"

/**
 * @brief   Swaps the values pointed to by two pointers.
 * @param[in,out] xp  First value, replaced with the second value.
 * @param[in,out] yp  Second value, replaced with the first value.
 */
static void swapForSort ( float* xp, float* yp )
{
    float temp = 0;

    temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/**
 * @brief   Swaps the values pointed to by two unsigned 32-bit pointers.
 * @param[in,out] xp  First value, replaced with the second value.
 * @param[in,out] yp  Second value, replaced with the first value.
 */
static void swapForSortu32 ( uint32_t* xp, uint32_t* yp )
{
    uint32_t temp = 0;

    temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/**
 * @brief   Swaps the values pointed to by two signed 32-bit pointers.
 * @param[in,out] xp  First value, replaced with the second value.
 * @param[in,out] yp  Second value, replaced with the first value.
 */
static void swapForSorti32 ( int32_t* xp, int32_t* yp )
{
    int32_t temp = 0;

    temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/**
 * @brief   Sorts the array into ascending order using selection sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortSelection ( float* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t lengthM1 = 0;      // Length minus 1.
    uint32_t minElmIndex = 0;   // Minimum elements index.

    if ( length != 0 )
    {
        lengthM1 = length - 1;  // Optimize loop operations.
    }
    else
    {
        // Keep lengthM1 at zero so that the loop below does not run.
        lengthM1 = 0;
    }

    for ( i = 0; i < lengthM1; ++i )
    {
        minElmIndex = i;

        for ( j = i + 1; j < length; ++j )
        {
            if ( array[ j ] < array[ minElmIndex ] )
            {
                minElmIndex = j;
            }
        }

        swapForSort ( &array[ minElmIndex ], &array[ i ] );
    }
}

/**
 * @brief   Sorts the unsigned 32-bit array into ascending order using selection sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortSelectionu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t lengthM1 = 0;      // Length minus 1.
    uint32_t minElmIndex = 0;   // Minimum elements index.

    if ( length != 0 )
    {
        lengthM1 = length - 1;  // Optimize loop operations.
    }
    else
    {
        // Keep lengthM1 at zero so that the loop below does not run.
        lengthM1 = 0;
    }

    for ( i = 0; i < lengthM1; ++i )
    {
        minElmIndex = i;

        for ( j = i + 1; j < length; ++j )
        {
            if ( array[ j ] < array[ minElmIndex ] )
            {
                minElmIndex = j;
            }
        }

        swapForSortu32 ( &array[ minElmIndex ], &array[ i ] );
    }
}

/**
 * @brief   Sorts the signed 32-bit array into ascending order using selection sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortSelectioni32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t lengthM1 = 0;      // Length minus 1.
    uint32_t minElmIndex = 0;   // Minimum elements index.

    if ( length != 0 )
    {
        lengthM1 = length - 1;  // Optimize loop operations.
    }
    else
    {
        // Keep lengthM1 at zero so that the loop below does not run.
        lengthM1 = 0;
    }

    for ( i = 0; i < lengthM1; ++i )
    {
        minElmIndex = i;

        for ( j = i + 1; j < length; ++j )
        {
            if ( array[ j ] < array[ minElmIndex ] )
            {
                minElmIndex = j;
            }
        }

        swapForSorti32 ( &array[ minElmIndex ], &array[ i ] );
    }
}

/**
 * @brief   Sorts the array into ascending order using bubble sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortBubble ( float* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t lengthM1 = 0;      // Length minus 1.

    if ( length != 0 )
    {
        lengthM1 = length - 1;  // Optimize loop operations.
    }
    else
    {
        // Keep lengthM1 at zero so that the loop below does not run.
        lengthM1 = 0;
    }

    for ( i = 0; i < lengthM1; ++i )
    {
        for ( j = 0; j < ( lengthM1 - i ); ++j )
        {
            if ( array[ j ] > array[ j + 1 ] )
            {
                swapForSort ( &array[ j ], &array[ j + 1 ] );
            }
        }
    }
}

/**
 * @brief   Sorts the unsigned 32-bit array into ascending order using bubble sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortBubbleu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t lengthM1 = 0;      // Length minus 1.

    if ( length != 0 )
    {
        lengthM1 = length - 1;  // Optimize loop operations.
    }
    else
    {
        // Keep lengthM1 at zero so that the loop below does not run.
        lengthM1 = 0;
    }

    for ( i = 0; i < lengthM1; ++i )
    {
        for ( j = 0; j < ( lengthM1 - i ); ++j )
        {
            if ( array[ j ] > array[ j + 1 ] )
            {
                swapForSortu32 ( &array[ j ], &array[ j + 1 ] );
            }
        }
    }
}

/**
 * @brief   Sorts the signed 32-bit array into ascending order using bubble sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortBubblei32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t lengthM1 = 0;      // Length minus 1.

    if ( length != 0 )
    {
        lengthM1 = length - 1;  // Optimize loop operations.
    }
    else
    {
        // Keep lengthM1 at zero so that the loop below does not run.
        lengthM1 = 0;
    }

    for ( i = 0; i < lengthM1; ++i )
    {
        for ( j = 0; j < ( lengthM1 - i ); ++j )
        {
            if ( array[ j ] > array[ j + 1 ] )
            {
                swapForSorti32 ( &array[ j ], &array[ j + 1 ] );
            }
        }
    }
}

/**
 * @brief   Sorts the array into ascending order using insertion sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortInsertion ( float* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    float key = 0;

    for ( i = 1; i < length; ++i )
    {
        key = array[ i ];

        // j is the insert position. It is compared against j - 1 so that the
        // unsigned index never has to go below zero.
        j = i;

        while ( ( j != 0 ) && ( array[ j - 1 ] > key ) )
        {
            array[ j ] = array[ j - 1 ];
            --j;
        }

        array[ j ] = key;
    }
}

/**
 * @brief   Sorts the unsigned 32-bit array into ascending order using insertion sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortInsertionu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t key = 0;

    for ( i = 1; i < length; ++i )
    {
        key = array[ i ];

        // j is the insert position. It is compared against j - 1 so that the
        // unsigned index never has to go below zero.
        j = i;

        while ( ( j != 0 ) && ( array[ j - 1 ] > key ) )
        {
            array[ j ] = array[ j - 1 ];
            --j;
        }

        array[ j ] = key;
    }
}

/**
 * @brief   Sorts the signed 32-bit array into ascending order using insertion sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortInsertioni32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;
    int32_t key = 0;

    for ( i = 1; i < length; ++i )
    {
        key = array[ i ];

        // j is the insert position. It is compared against j - 1 so that the
        // unsigned index never has to go below zero.
        j = i;

        while ( ( j != 0 ) && ( array[ j - 1 ] > key ) )
        {
            array[ j ] = array[ j - 1 ];
            --j;
        }

        array[ j ] = key;
    }
}

/**
 * @brief   Pushes the element at start down the heap until the subtree rooted
 *          there satisfies the max heap property.
 * @param[in,out] array  Array holding the heap.
 * @param[in]     start  Index of the element to push down.
 * @param[in]     end    Last index that belongs to the heap, inclusive.
 * @note    Iterative on purpose. The recursive form of this is shorter but it
 *          costs a stack frame per level, and the whole reason to reach for
 *          heap sort on a small target is that its cost is bounded.
 * @note    The child index is 2 * root + 1 and is not checked for overflow.
 *          The elements here are four bytes wide, so an array long enough to
 *          push that index past 32 bits would need more memory than the
 *          address space holds.
 */
static void siftDownForSort ( float* array, uint32_t start, uint32_t end )
{
    uint32_t root = 0;
    uint32_t child = 0;
    uint32_t largest = 0;
    uint8_t running = TRUE;

    root = start;

    while ( running == TRUE )
    {
        child = ( 2u * root ) + 1u;

        if ( child > end )
        {
            // The root has no children, so there is nothing left to push past.
            running = FALSE;
        }
        else
        {
            largest = root;

            if ( array[ child ] > array[ largest ] )
            {
                largest = child;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( ( ( child + 1u ) <= end ) && ( array[ child + 1u ] > array[ largest ] ) )
            {
                largest = child + 1u;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( largest == root )
            {
                // The root already outranks both children.
                running = FALSE;
            }
            else
            {
                swapForSort ( &array[ root ], &array[ largest ] );
                root = largest;
            }
        }
    }
}

/**
 * @brief   Pushes the element at start down the unsigned 32-bit heap until the
 *          subtree rooted there satisfies the max heap property.
 * @param[in,out] array  Array holding the heap.
 * @param[in]     start  Index of the element to push down.
 * @param[in]     end    Last index that belongs to the heap, inclusive.
 */
static void siftDownForSortu32 ( uint32_t* array, uint32_t start, uint32_t end )
{
    uint32_t root = 0;
    uint32_t child = 0;
    uint32_t largest = 0;
    uint8_t running = TRUE;

    root = start;

    while ( running == TRUE )
    {
        child = ( 2u * root ) + 1u;

        if ( child > end )
        {
            running = FALSE;
        }
        else
        {
            largest = root;

            if ( array[ child ] > array[ largest ] )
            {
                largest = child;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( ( ( child + 1u ) <= end ) && ( array[ child + 1u ] > array[ largest ] ) )
            {
                largest = child + 1u;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( largest == root )
            {
                running = FALSE;
            }
            else
            {
                swapForSortu32 ( &array[ root ], &array[ largest ] );
                root = largest;
            }
        }
    }
}

/**
 * @brief   Pushes the element at start down the signed 32-bit heap until the
 *          subtree rooted there satisfies the max heap property.
 * @param[in,out] array  Array holding the heap.
 * @param[in]     start  Index of the element to push down.
 * @param[in]     end    Last index that belongs to the heap, inclusive.
 */
static void siftDownForSorti32 ( int32_t* array, uint32_t start, uint32_t end )
{
    uint32_t root = 0;
    uint32_t child = 0;
    uint32_t largest = 0;
    uint8_t running = TRUE;

    root = start;

    while ( running == TRUE )
    {
        child = ( 2u * root ) + 1u;

        if ( child > end )
        {
            running = FALSE;
        }
        else
        {
            largest = root;

            if ( array[ child ] > array[ largest ] )
            {
                largest = child;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( ( ( child + 1u ) <= end ) && ( array[ child + 1u ] > array[ largest ] ) )
            {
                largest = child + 1u;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( largest == root )
            {
                running = FALSE;
            }
            else
            {
                swapForSorti32 ( &array[ root ], &array[ largest ] );
                root = largest;
            }
        }
    }
}

/**
 * @brief   Sorts the array into ascending order using heap sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    The only sort in this file with an O(N log N) bound, and the bound
 *          is a worst case rather than an average: no input makes it slower.
 *          Below roughly fifty elements sortInsertion is usually quicker in
 *          practice, and it is far quicker on data that is already nearly
 *          sorted, which heap sort cannot take advantage of.
 * @note    In place. It needs no scratch buffer and no recursion, so its cost
 *          in memory is a handful of locals whatever the array length.
 * @note    Not stable. Equal elements can come out in a different order than
 *          they went in. sortInsertion and sortBubble are stable.
 * @note    An array of zero or one element is left untouched.
 */
void sortHeap ( float* array, uint32_t length )
{
    uint32_t start = 0;
    uint32_t end = 0;

    if ( length > 1u )
    {
        end = length - 1u;

        /*
         * Build the heap from the last parent back to the root. start is held
         * one past the index being worked on and decremented inside the loop,
         * so the unsigned index never has to go below zero to end the walk.
         */
        start = ( ( length - 2u ) / 2u ) + 1u;

        while ( start != 0 )
        {
            --start;
            siftDownForSort ( array, start, end );
        }

        /*
         * Take the root, which is the largest remaining element, to the end of
         * the unsorted region and shrink that region by one.
         */
        while ( end != 0 )
        {
            swapForSort ( &array[ 0 ], &array[ end ] );
            --end;
            siftDownForSort ( array, 0, end );
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Sorts the unsigned 32-bit array into ascending order using heap sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    Carries the same properties as sortHeap: an O(N log N) worst case,
 *          in place, no recursion, and not stable.
 * @note    An array of zero or one element is left untouched.
 */
void sortHeapu32 ( uint32_t* array, uint32_t length )
{
    uint32_t start = 0;
    uint32_t end = 0;

    if ( length > 1u )
    {
        end = length - 1u;

        start = ( ( length - 2u ) / 2u ) + 1u;

        while ( start != 0 )
        {
            --start;
            siftDownForSortu32 ( array, start, end );
        }

        while ( end != 0 )
        {
            swapForSortu32 ( &array[ 0 ], &array[ end ] );
            --end;
            siftDownForSortu32 ( array, 0, end );
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Sorts the signed 32-bit array into ascending order using heap sort.
 * @param[in,out] array   Array to sort, sorted in place.
 * @param[in]     length  Number of elements in the array.
 * @note    Carries the same properties as sortHeap: an O(N log N) worst case,
 *          in place, no recursion, and not stable.
 * @note    An array of zero or one element is left untouched.
 */
void sortHeapi32 ( int32_t* array, uint32_t length )
{
    uint32_t start = 0;
    uint32_t end = 0;

    if ( length > 1u )
    {
        end = length - 1u;

        start = ( ( length - 2u ) / 2u ) + 1u;

        while ( start != 0 )
        {
            --start;
            siftDownForSorti32 ( array, start, end );
        }

        while ( end != 0 )
        {
            swapForSorti32 ( &array[ 0 ], &array[ end ] );
            --end;
            siftDownForSorti32 ( array, 0, end );
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Reverses the order of the elements in the array, in place.
 * @param[in,out] array   Array to reverse.
 * @param[in]     length  Number of elements in the array.
 * @note    Every sort in this file produces ascending order. Following one
 *          with this gives descending order for the cost of half a pass,
 *          which is why there is no descending variant of each sort.
 * @note    An array of zero or one element is left untouched.
 */
void sortReverse ( float* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;

    if ( length > 1u )
    {
        j = length - 1u;

        while ( i < j )
        {
            swapForSort ( &array[ i ], &array[ j ] );
            ++i;
            --j;
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Reverses the order of the elements in the unsigned 32-bit array, in place.
 * @param[in,out] array   Array to reverse.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortReverseu32 ( uint32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;

    if ( length > 1u )
    {
        j = length - 1u;

        while ( i < j )
        {
            swapForSortu32 ( &array[ i ], &array[ j ] );
            ++i;
            --j;
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Reverses the order of the elements in the signed 32-bit array, in place.
 * @param[in,out] array   Array to reverse.
 * @param[in]     length  Number of elements in the array.
 * @note    An array of zero or one element is left untouched.
 */
void sortReversei32 ( int32_t* array, uint32_t length )
{
    uint32_t i = 0;
    uint32_t j = 0;

    if ( length > 1u )
    {
        j = length - 1u;

        while ( i < j )
        {
            swapForSorti32 ( &array[ i ], &array[ j ] );
            ++i;
            --j;
        }
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Reports whether the array is already in ascending order.
 * @param[in] array   Array to inspect.
 * @param[in] length  Number of elements in the array.
 * @return  TRUE when every element is greater than or equal to the one before
 *          it, FALSE otherwise.
 * @note    Exists mainly to check the precondition of searchBinary, which
 *          reports a confident wrong answer on an unsorted array. Cheap enough
 *          to sit behind an assertion in a debug build and be compiled out of
 *          a release one.
 * @note    Stops at the first pair out of order, so the usual cost of a FALSE
 *          is far below N.
 * @note    An array of zero or one element reports TRUE. It cannot be out of
 *          order.
 */
uint8_t sortIsSorted ( const float* const array, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 1; i < length; ++i )
    {
        if ( array[ i - 1u ] > array[ i ] )
        {
            retVal = FALSE;
            break;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the unsigned 32-bit array is already in ascending order.
 * @param[in] array   Array to inspect.
 * @param[in] length  Number of elements in the array.
 * @return  TRUE when every element is greater than or equal to the one before
 *          it, FALSE otherwise.
 * @note    An array of zero or one element reports TRUE.
 */
uint8_t sortIsSortedu32 ( const uint32_t* const array, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 1; i < length; ++i )
    {
        if ( array[ i - 1u ] > array[ i ] )
        {
            retVal = FALSE;
            break;
        }
    }

    return ( retVal );
}

/**
 * @brief   Reports whether the signed 32-bit array is already in ascending order.
 * @param[in] array   Array to inspect.
 * @param[in] length  Number of elements in the array.
 * @return  TRUE when every element is greater than or equal to the one before
 *          it, FALSE otherwise.
 * @note    An array of zero or one element reports TRUE.
 */
uint8_t sortIsSortedi32 ( const int32_t* const array, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 1; i < length; ++i )
    {
        if ( array[ i - 1u ] > array[ i ] )
        {
            retVal = FALSE;
            break;
        }
    }

    return ( retVal );
}
