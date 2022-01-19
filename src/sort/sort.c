/**
  ******************************************************************************
  *
  * @file:      sort.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.3
  * @cdate:     24/12/2021
  * @history:   24/12/2021 Created.
  *             26/12/2021 bubbleSort added.
  *             20/01/2022 bubbleSort added.
  *
  * @about:     Sort function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         swapForSort     : Swaps the data of the two pointers.
  *         swapForSortu32  :
  *         swapForSorti32  :
  *
  *         selectionSort   :
  *         selectionSortu32:
  *         selectionSorti32:
  *         bubbleSort      :
  *         bubbleSortu32   :
  *         bubbleSorti32   :
  *         insertionSort   :
  *         insertionSortu32:
  *         insertionSorti32:
  *
  * @notes:
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
    
    lengthM1 = length - 1;      // Optimize loop operations.
        
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

    lengthM1 = length - 1;      // Optimize loop operations.

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

    lengthM1 = length - 1;      // Optimize loop operations.

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
    
    lengthM1 = length - 1;      // Optimize loop operations.
        
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

    lengthM1 = length - 1;      // Optimize loop operations.

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

    lengthM1 = length - 1;      // Optimize loop operations.

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
        key = array[i];  
        j = i - 1;  

        while ( ( j >= 0 ) && ( j != 0xFFFFFFFF ) && ( array[ j ] > key ) ) 
        {  
            array[ j + 1 ] = array[ j ];  
            j = j - 1;  
        }  
        array[ j + 1 ] = key;  
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
        key = array[i];
        j = i - 1;

        while ( ( j >= 0 ) && ( j != 0xFFFFFFFF ) && ( array[ j ] > key ) )
        {
            array[ j + 1 ] = array[ j ];
            j = j - 1;
        }
        array[ j + 1 ] = key;
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
        key = array[i];
        j = i - 1;

        while ( ( j >= 0 ) && ( j != 0xFFFFFFFF ) && ( array[ j ] > key ) )
        {
            array[ j + 1 ] = array[ j ];
            j = j - 1;
        }
        array[ j + 1 ] = key;
    }
}
