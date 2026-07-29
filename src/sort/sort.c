/**
  ******************************************************************************
  *
  * @file:      sort.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.4
  * @cdate:     24/12/2021
  * @history:   24/12/2021 Created.
  *             26/12/2021 bubbleSort added.
  *             20/01/2022 bubbleSort added.
  *             29/07/2026 Bug fix. length - 1 underflowed to 0xFFFFFFFF for a
  *                        zero length array, which drove the loops far past the
  *                        end of the buffer.
  *             29/07/2026 The insertion sort inner loop no longer relies on the
  *                        unsigned index wrapping to 0xFFFFFFFF.
  *
  * @about:     Sort function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         swapForSort     : Swaps the data of the two pointers.
  *         swapForSortu32  : Swaps the data of the two pointers for u32.
  *         swapForSorti32  : Swaps the data of the two pointers for i32.
  *
  *         selectionSort   : Sorts array elements using selection method by length.
  *         selectionSortu32: Sorts array elements using selection method by length for u32.
  *         selectionSorti32: Sorts array elements using selection method by length for i32.
  *         bubbleSort      : Sorts array elements using buble method by length.
  *         bubbleSortu32   : Sorts array elements using buble method by length for u32.
  *         bubbleSorti32   : Sorts array elements using buble method by length for i32.
  *         insertionSort   : Sorts array elements using interion method by length.
  *         insertionSortu32: Sorts array elements using interion method by length for u32.
  *         insertionSorti32: Sorts array elements using interion method by length for i32.
  *
  * @notes:
  *     A zero or one element array is already sorted, so every function below
  *     leaves it untouched.
  *
  ******************************************************************************
  */

#include "sort.h"

/*
 * @about: Swaps the data of the two pointers.
 */
static void swapForSort ( float* xp, float* yp )
{
    float temp = 0;

    temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/*
 * @about: Swaps the data of the two pointers.
 */
static void swapForSortu32 ( uint32_t* xp, uint32_t* yp )
{
    uint32_t temp = 0;

    temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/*
 * @about: Swaps the data of the two pointers.
 */
static void swapForSorti32 ( int32_t* xp, int32_t* yp )
{
    int32_t temp = 0;

    temp = *xp;
    *xp = *yp;
    *yp = temp;
}

/*
 * @about:
 */
void selectionSort ( float* array, uint32_t length )
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

/*
 * @about:
 */
void selectionSortu32 ( uint32_t* array, uint32_t length )
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

/*
 * @about:
 */
void selectionSorti32 ( int32_t* array, uint32_t length )
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

/*
 * @about:
 */
void bubbleSort ( float* array, uint32_t length )
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

/*
 * @about:
 */
void bubbleSortu32 ( uint32_t* array, uint32_t length )
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

/*
 * @about:
 */
void bubbleSorti32 ( int32_t* array, uint32_t length )
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

/*
 * @about:
 */
void insertionSort ( float* array, uint32_t length )
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

/*
 * @about:
 */
void insertionSortu32 ( uint32_t* array, uint32_t length )
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

/*
 * @about:
 */
void insertionSorti32 ( int32_t* array, uint32_t length )
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
