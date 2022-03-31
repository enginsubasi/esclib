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

/*
 * @about:
 */
uint8_t circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t capacity )
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

        // Set return value
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/*
 * @about:
 */
uint32_t circBufGetsizeu32 ( circBufu32_t* driver )
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

/*
 * @about:
 */
uint8_t circBufAddu32 ( circBufu32_t* driver, uint32_t data )
{
    uint8_t retVal = FALSE;

    if ( ( driver->wp < driver->capacity ) && ( ( driver->rp + 1 ) != driver->wp )  )
    {
        driver->buffer[ driver->wp ]
        ++driver->wp;
    }

    if ( driver->capacity == driver->wp )
    {
        if ( ( driver->rp + 1 ) != driver->wp )
        {
            driver->wp = 0;
            driver->buffer[ driver->wp ];

            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }

    

    return ( retVal );
}





