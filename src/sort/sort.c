/**
  ******************************************************************************
  *
  * @file:      sort.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     24/12/2020
  * @history:   24/12/2020 Created.
  *             26/12/2020 bubbleSort added.
  *
  * @about:     Sort function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         swapForSort     : Swaps the data of the two pointers.
  *
  *         selectionSort   : 
  *         bubbleSort      :
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
