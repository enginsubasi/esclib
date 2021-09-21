/**
  ******************************************************************************
  *
  * @file:      search.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     15/09/2021
  * @history:   15/09/2021 Created.
  *             20/09/2021 linearSearch_i32 is added to the library.
  *
  * @about:     Search function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         linearSearch_i32    : Linear search algorithm for signed-integer-32-bit.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "search.h"

/*
 * @about:
 */
uint8_t linearSearch_i32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
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

/*
 * @about:
 * @param: "array" should be a sorted array.
 */
uint8_t binarySearch_i32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t l = 0;
    uint32_t r = length - 1;
    uint32_t m = 0;

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
                l = m + 1;
            }
            else
            {
                r = m - 1;
            }

        }
    }

    return ( retVal );
}

