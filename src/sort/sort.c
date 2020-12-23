/**
  ******************************************************************************
  *
  * @file:      sort.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     24/12/2020
  * @history:   24/12/2020 Created.
  *
  * @about:     Sort function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         findMax         : Find maximum number of array elements.
  *         findMax         : Find minimum number of array elements.
  *         findMinMax      : Find max. and min. numbers of array elements.
  *
  *         calculateSum    : Calculates sum of array.
  *         calculateMean   : Calculates average/mean value of array.
  *         calculateMedian : Calculates median value of array.
  *         calculateRange  : Calculates range of array.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "sort.h"

/*
 * @about: Swaps the data of the two pointers.
 */
void swapForSort ( float* xp, float* yp )
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
    
    lengthM1 = length - 1;
        
    for ( i = 1; i < lengthM1; ++i )
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



