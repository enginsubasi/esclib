/**
  ******************************************************************************
  *
  * @file      sort.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.5
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
  *
  * @note      A zero or one element array is already sorted, so every function
  *            below leaves it untouched.
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
