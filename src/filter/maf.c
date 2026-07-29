/**
  ******************************************************************************
  *
  * @file      maf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   2.1.0
  * @date      26/04/2020
  *
  * @brief     Moving average filter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 26/04/2020 Created @n
  * 07/06/2020 Naming style changed @n
  * 24/08/2020 Data type changed from double to float. @n
  * 29/07/2026 The u32 variants declared by maf.h are implemented. @n
  *            They were missing, which broke linking for any caller @n
  *            that used them. @n
  *
  ******************************************************************************
  */

#include "maf.h"

/**
 * @brief   Initializes the moving average filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  buffer      Caller owned sample buffer of at least length entries.
 * @param[in]  length      Number of samples in the averaging window.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when length is zero or buffer is NULL.
 * @note    The buffer is not copied. It must outlive the filter.
 */
int8_t mafInit ( maf_t* driver, float* buffer, uint32_t length, float outputInit )
{
    int8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( length != 0 ) && ( buffer != 0 ) )
    {
        driver->buffer = buffer;
        driver->length = length;
        driver->output = outputInit;
        driver->sumOfArray = driver->length * driver->output;
        driver->index = 0;

        for ( i = 0; i < length; ++i )
        {
            driver->buffer[ i ] = outputInit;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds a new sample to the moving average filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample to add to the averaging window.
 */
void mafIteration ( maf_t* driver, float newData )
{
    // Add new data to buffer array and sum. of buffer array.
    driver->sumOfArray -= driver->buffer[ driver->index ];
    driver->buffer[ driver->index ] = newData;
    driver->sumOfArray += driver->buffer[ driver->index ];

    // Calculate output.
    driver->output = ( driver->sumOfArray / driver->length );

    // Index control.
    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Gets the current output of the moving average filter.
 * @param[in] driver  Filter state.
 * @return  Current filtered output value.
 */
float mafGetOutput ( maf_t* driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes the moving average filter for unsigned 32-bit data.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  buffer      Caller owned sample buffer of at least length entries.
 * @param[in]  length      Number of samples in the averaging window.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when length is zero or buffer is NULL.
 * @note    The buffer is not copied. It must outlive the filter.
 */
int8_t mafInitu32 ( mafu32_t* driver, uint32_t* buffer, uint32_t length, uint32_t outputInit )
{
    int8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( length != 0 ) && ( buffer != 0 ) )
    {
        driver->buffer = buffer;
        driver->length = length;
        driver->output = outputInit;
        driver->sumOfArray = driver->length * driver->output;
        driver->index = 0;

        for ( i = 0; i < length; ++i )
        {
            driver->buffer[ i ] = outputInit;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds a new sample to the unsigned 32-bit moving average filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample to add to the averaging window.
 */
void mafIterationu32 ( mafu32_t* driver, uint32_t newData )
{
    // Add new data to buffer array and sum. of buffer array.
    driver->sumOfArray -= driver->buffer[ driver->index ];
    driver->buffer[ driver->index ] = newData;
    driver->sumOfArray += driver->buffer[ driver->index ];

    // Calculate output.
    driver->output = ( driver->sumOfArray / driver->length );

    // Index control.
    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Gets the current output of the unsigned 32-bit moving average filter.
 * @param[in] driver  Filter state.
 * @return  Current filtered output value.
 */
uint32_t mafGetOutputu32 ( mafu32_t* driver )
{
    return ( driver->output );
}

