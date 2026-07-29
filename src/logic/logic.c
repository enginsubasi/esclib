/**
  ******************************************************************************
  *
  * @file      logic.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      20/10/2021
  *
  * @brief     Basic logic function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 20/10/2021 Created @n
  * 11/12/2021 D Flip-Flop is added @n
  *
  ******************************************************************************
  */

#include "logic.h"

/**
 * @brief   Evaluates one step of an RS flip-flop; reset is dominant over set.
 * @param[in]     r    Reset input, treated as a boolean.
 * @param[in]     s    Set input, treated as a boolean.
 * @param[in,out] mem  Flip-flop's stored state; read and then updated.
 * @return  New state of mem, i.e. TRUE or FALSE.
 * @note    When r is TRUE the output is cleared regardless of s.
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

/**
 * @brief   Evaluates one step of a D flip-flop.
 * @param[in]     d    Data input, treated as a boolean.
 * @param[in,out] mem  Flip-flop's stored state; read and then updated.
 * @return  Previous state of mem, captured before it is updated to d.
 */
uint8_t dff ( uint8_t d, uint8_t* mem )
{
    uint8_t retVal = FALSE;

    retVal = ( *mem );

    ( *mem ) = d;

    return ( retVal );
}
