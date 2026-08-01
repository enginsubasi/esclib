/**
  ******************************************************************************
  *
  * @file      circBuf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      29/03/2022
  *
  * @brief     Circular buffer implementation.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 29/03/2022 Created. @n
  * 29/07/2026 Bug fix. NULL was used without including stddef.h. @n
  * 01/08/2026 circBufInitu32 held its status in an int8_t local while @n
  *            declaring a uint8_t return type. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The accessors that only read take a const driver. @n
  * 01/08/2026 The u8 variant is added. The module held only u32, @n
  *            while a byte buffer is what a serial project needs. @n
  *            Derived from the u32 code by width substitution; the @n
  *            indices, capacity and status stay as they were. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "circBuf.h"

/**
 * @brief   Initializes the circular buffer.
 * @param[out] driver     Buffer state to initialize.
 * @param[in]  buffer     Caller owned storage of at least capacity words.
 * @param[in]  capacity   Number of words the buffer holds.
 * @param[in]  behaviour  BB_OVERWRITE to drop the oldest word when full,
 *                        BB_STOP to reject the new word instead.
 * @return  TRUE on success, FALSE when a pointer is NULL or capacity is zero.
 */
uint8_t circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t capacity, uint8_t behaviour )
{
    uint8_t retVal = FALSE;

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
 * @brief   Gets the number of words currently stored in the circular buffer.
 * @param[in] driver  Buffer state.
 * @return  Number of words currently stored, from 0 up to capacity.
 */
uint32_t circBufGetLengthu32 ( const circBufu32_t* const driver )
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

/**
 * @brief   Gets the current fill status of the circular buffer.
 * @param[in] driver  Buffer state.
 * @return  BS_EMPTY, BS_NOTEMPTY or BS_FULL depending on the buffer's current fill state.
 */
uint8_t circBufGetStatusu32 ( const circBufu32_t* const driver )
{
    return ( driver->status );
}

/**
 * @brief   Adds one word to the circular buffer.
 * @param[in,out] driver  Buffer state.
 * @param[in]     data    Word to store.
 * @return  TRUE when the word was stored, FALSE when the buffer is
 *          full and the behaviour is BB_STOP.
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
 * @brief   Reads and removes the oldest word from the circular buffer.
 * @param[in,out] driver  Buffer state.
 * @param[out]    data    Set to the oldest stored word, or to zero when the
 *                        buffer is empty.
 * @return  TRUE when a word was read, FALSE when the buffer was empty.
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







/**
 * @brief   Initializes the circular buffer.
 * @param[out] driver     Buffer state to initialize.
 * @param[in]  buffer     Caller owned storage of at least capacity bytes.
 * @param[in]  capacity   Number of bytes the buffer holds.
 * @param[in]  behaviour  BB_OVERWRITE to drop the oldest byte when full,
 *                        BB_STOP to reject the new byte instead.
 * @return  TRUE on success, FALSE when a pointer is NULL or capacity is zero.
 */
uint8_t circBufInitu8 ( circBufu8_t* driver, uint8_t* buffer, uint32_t capacity, uint8_t behaviour )
{
    uint8_t retVal = FALSE;

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
 * @brief   Gets the number of bytes currently stored in the circular buffer.
 * @param[in] driver  Buffer state.
 * @return  Number of bytes currently stored, from 0 up to capacity.
 */
uint32_t circBufGetLengthu8 ( const circBufu8_t* const driver )
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

/**
 * @brief   Gets the current fill status of the circular buffer.
 * @param[in] driver  Buffer state.
 * @return  BS_EMPTY, BS_NOTEMPTY or BS_FULL depending on the buffer's current fill state.
 */
uint8_t circBufGetStatusu8 ( const circBufu8_t* const driver )
{
    return ( driver->status );
}

/**
 * @brief   Adds one byte to the circular buffer.
 * @param[in,out] driver  Buffer state.
 * @param[in]     data    Byte to store.
 * @return  TRUE when the byte was stored, FALSE when the buffer is
 *          full and the behaviour is BB_STOP.
 */
uint8_t circBufAddu8 ( circBufu8_t* driver, uint8_t data )
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
 * @brief   Reads and removes the oldest byte from the circular buffer.
 * @param[in,out] driver  Buffer state.
 * @param[out]    data    Set to the oldest stored byte, or to zero when the
 *                        buffer is empty.
 * @return  TRUE when a byte was read, FALSE when the buffer was empty.
 */
uint8_t circBufReadu8 ( circBufu8_t* driver, uint8_t* data )
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






