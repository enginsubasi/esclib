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
uint8_t linearSearch_i32 ( int32_t* array, uint32_t length, int32_t item, uint32_t* foundIndex )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( array[ i ] == item )
        {
            (*foundIndex) = i;
            retVal = TRUE;
            break;
        }      
    }

    return ( retVal );
}
