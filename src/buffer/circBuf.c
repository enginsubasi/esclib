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
        driver->status = BS_EMPTY;

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

    if ( driver->status == BS_FULL )
    {
        retVal = driver->capacity;
    }
    else if ( driver->wp >= driver->rp )
    {
        retVal = ( driver->wp - driver->rp );
    }
    else
    {
        retVal = driver->wp + ( driver->capacity - driver->rp );
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


    if ( ( driver->wp != driver->rp ) || ( driver->status == BS_EMPTY ) )
    {
        driver->buffer[ driver->wp ] = data;
        ++driver->wp;
        if ( driver->wp >= driver->capacity )
        {
            driver->wp = 0;
        }

        driver->status = BS_NOTEMPTY;

        if ( driver->wp == driver->rp)
        {
            driver->status = BS_FULL;
        }

        retVal = TRUE;
    }
    else
    {
        if ( driver->behaviour == BB_OVERWRITE )
        {
            driver->buffer[ driver->wp ] = data;
            ++driver->rp;
            ++driver->wp;
            driver->status = BS_FULL;
            retVal = TRUE;
        }
        else if ( driver->behaviour == BB_STOP )
        {
            driver->status = BS_FULL;
            retVal = FALSE;
        }
    }

    if ( driver->wp >= driver->capacity )
    {
        driver->wp = 0;
    }

    if ( driver->rp >= driver->capacity )
    {
        driver->rp = 0;
    }

    return ( retVal );
}

/**
 * @brief
 */
uint8_t circBufReadu32 ( circBufu32_t* driver, uint32_t* data )
{
    uint8_t retVal = FALSE;

    if ( driver->status != BS_EMPTY )
    {
        if ( driver->rp < driver->capacity )
        {
            *data = driver->buffer[ driver->rp ];
            ++driver->rp;

            if ( driver->rp >= driver->capacity )
            {
                driver->rp = 0;

                if ( driver->status == BS_FULL )
                {
                    driver->status = BS_NOTEMPTY;
                }

                if ( driver->rp == driver->wp )
                {
                    driver->status = BS_EMPTY;
                }
            }
            else
            {
                if ( driver->status == BS_FULL )
                {
                    driver->status = BS_NOTEMPTY;
                }

                if ( driver->rp == driver->wp )
                {
                    driver->status = BS_EMPTY;
                }
            }
        }

        retVal = TRUE;
    }
    else
    {
        *data = 0;
        retVal = FALSE;
    }

    return ( retVal );
}






