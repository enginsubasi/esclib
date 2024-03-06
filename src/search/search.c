/**
  ******************************************************************************
  *
  * @file:      search.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.3
  * @cdate:     15/09/2021
  * @history:   15/09/2021 Created.
  *             20/09/2021 linearSearch_i32 is added to the library.
  *             20/09/2021 binarySearch_i32 is added to the library.
  *
  * @about:     Search function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         isEqual_f           : It is a help function for floating point search functions.
  *
  *         linearSearch        : Linear search algorithm for float.
  *         linearSearchu32     : Linear search algorithm for u32.
  *         linearSearchi32     : Linear search algorithm for i32.
  *         binarySearch        : Binary search algorithm for float.
  *         binarySearchu32     : Binary search algorithm for u32.
  *         binarySearchi32     : Binary search algorithm for i32.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "search.h"

/*
 * @about: isEqual function for floating point data type.
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

/*
 * @about:
 */
uint8_t linearSearch ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( isEqualf ( array[ i ], item, epsilon ) == TRUE );
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

/*
 * @about:
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

/*
 * @about:
 * @param: "array" should be a sorted array.
 */
uint8_t binarySearch ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon )
{
    uint8_t retVal = FALSE;
    uint32_t l = 0;
    uint32_t r = length - 1;
    uint32_t m = 0;

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

/*
 * @about:
 * @param: "array" should be a sorted array.
 */
uint8_t binarySearchu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex )
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

/*
 * @about:
 * @param: "array" should be a sorted array.
 */
uint8_t binarySearchi32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex )
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
