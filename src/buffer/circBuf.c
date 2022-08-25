/**
  ******************************************************************************
  *
  * @file:      circBuf.c
  * @author:    Engin Subaşı
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     29/03/2022
  * @history:   29/03/2022 Created.
  *
  * @about:     Circular buffer implementation.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         foo1                : Initialize comstxetx structure.
  *         foo2                : 
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "circBuf.h"

/**
 * @brief Circular buffer initialization
 * @param driver
 * @param buffer
 * @param capacity
 * @return Success
 */
uint8_t circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t capacity, uint8_t behaviour )
{
    int8_t retVal = FALSE;
    
    if ( ( driver != NULL ) && ( buffer != NULL ) && ( capacity != 0 ) )
    {
        // Assignments
        driver->buffer = buffer;
        driver->capacity = capacity;

        // Initilizations
        driver->rp = 0;
        driver->wp = 0;

        driver->behaviour = behaviour;
        driver->status = BS_FULL;

        // Set return value
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief
 */
uint32_t circBufGetLengthu32 ( circBufu32_t* driver )
{
    uint32_t retVal = 0;

    if ( wp >= rp )
    {
        retVal = ( wp - rp );
    }
    else
    {
        retVal = driver->wp + ( driver->capacity - driver->rp )
    }

    return ( retVal );
}

uint8_t circBufGetStatusu32 ( circBufu32_t* driver )
{
    return ( driver->status );
}

/**
 * @brief
 */
uint8_t circBufAddu32 ( circBufu32_t* driver, uint32_t data )
{
    uint8_t retVal = FALSE;

    if ( driver->wp < driver->capacity )
    {
        if ( driver->wp != driver->rp )
        {
            driver->buffer[ driver->wp ] = data;
            ++driver->wp;
            retVal = TRUE;
        }
        else
        {
            if ( driver->behaviour == BB_OVERWRITE )
            {
                driver->buffer[ driver->wp ] = data;
                ++driver->rp;
                ++driver->wp;
                retVal = TRUE;
            }
            else if ( driver->behaviour == BB_STOP )
            {
                retVal = FALSE;
            }
        }
    }
    else
    {
        if ( driver->rp != 0 )
        {
            driver->wp = 0;
            driver->buffer[ driver->wp ] = data;
            ++driver->wp;
            retVal = TRUE;
        }
        else
        {
            if ( driver->behaviour == BB_OVERWRITE )
            {
                driver->wp = 0;
                driver->buffer[ driver->wp ] = data;
                ++driver->rp;
                ++driver->wp;
                retVal = TRUE;
            }
            else if ( driver->behaviour == BB_STOP )
            {
                retVal = FALSE;
            }
        }
    }
    
    return ( retVal );
}





