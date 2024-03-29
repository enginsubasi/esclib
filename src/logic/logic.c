/**
  ******************************************************************************
  *
  * @file:      logic.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     20/10/2021
  * @history:   20/10/2021 Created
  *             11/12/2021 D Flip-Flop is added
  *
  * @about:     Basic logic function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         rsff            : RS Flip-Flop implementation.
  *         dff             : D Flip-Flop implementation.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "logic.h"

/*
 * @about: RS Flip-Flop implementation. All parameters are using as a boolean.
 */
uint8_t rsff ( uint8_t r, uint8_t s, uint8_t* mem )
{
    uint8_t retVal = FALSE;

    if ( r )
    {
        ( *mem ) = FALSE;
    }
    else
    {
        if ( s )
        {
            ( *mem ) = TRUE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    retVal = ( *mem );

    return ( retVal );
}

/*
 * @about: D Flip-Flop implementation. All parameters are using as a boolean.
 */
uint8_t dff ( uint8_t d, uint8_t* mem )
{
    uint8_t retVal = FALSE;

    retVal = ( *mem );

    ( *mem ) = d;

    return ( retVal );
}
