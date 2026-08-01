/**
  ******************************************************************************
  *
  * @file      maf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   2.3.0
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
  * 01/08/2026 mafInit and mafInitu32 return uint8_t rather than int8_t @n
  *            and reject a NULL driver, not only a NULL buffer. @n
  * 01/08/2026 Pointer checks use NULL from stddef.h, matching the @n
  *            wording the documentation already used. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The accessors that only read take a const driver. @n
  * 02/08/2026 The float window sum is rebuilt from the buffer on every @n
  *            wrap. Carrying it incrementally is not exact in float and @n
  *            the error had no bound; it is now capped at one window @n
  *            and costs one extra add per sample. @n
  * 02/08/2026 mafInitu32 rejects a length and outputInit whose product @n
  *            does not fit the uint32_t window sum. It used to wrap @n
  *            silently and start the filter on a garbage sum. @n
  * 02/08/2026 The uint32_t sample bound and the truncating division are @n
  *            documented on the functions that carry them. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "maf.h"

/**
 * @brief   Initializes the moving average filter.
 * @param[out] driver      Filter state to initialize.
 * @param[out] buffer      Caller owned sample buffer of at least length entries.
 * @param[in]  length      Number of samples in the averaging window.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when a pointer is NULL or length is zero.
 * @note    The buffer is not copied. It must outlive the filter.
 */
uint8_t mafInit ( maf_t* driver, float* buffer, uint32_t length, float outputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( buffer != NULL ) && ( length != 0 ) )
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
 * @note    The window sum is carried from one call to the next rather than
 *          rebuilt, which costs one add and one subtract per sample instead
 *          of one per window entry. In float that is not exact, so the sum is
 *          rebuilt from the buffer every time the index wraps. Without that
 *          the rounding error grows without bound; with it the error can
 *          never exceed one window's worth, and the rebuild costs one extra
 *          add per sample once it is spread over the window.
 */
void mafIteration ( maf_t* driver, float newData )
{
    uint32_t i = 0;

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

        // Drop the accumulated rounding error, see the note above.
        driver->sumOfArray = 0;

        for ( i = 0; i < driver->length; ++i )
        {
            driver->sumOfArray += driver->buffer[ i ];
        }
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
float mafGetOutput ( const maf_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes the moving average filter for unsigned 32-bit data.
 * @param[out] driver      Filter state to initialize.
 * @param[out] buffer      Caller owned sample buffer of at least length entries.
 * @param[in]  length      Number of samples in the averaging window.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when a pointer is NULL, length is zero, or
 *          length multiplied by outputInit does not fit in uint32_t.
 * @note    The buffer is not copied. It must outlive the filter.
 * @note    The window sum is a uint32_t, so the preloaded sum has to fit and
 *          it is checked here. The same bound applies to every sample fed in
 *          later, which mafIterationu32 does not check: keep each sample
 *          below 0xFFFFFFFF / length. With a 16 entry window that is roughly
 *          268 million, with a 256 entry window roughly 16.7 million. A 12 bit
 *          converter is nowhere near it; a 24 bit one or a raw counter can be.
 */
uint8_t mafInitu32 ( mafu32_t* driver, uint32_t* buffer, uint32_t length, uint32_t outputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint8_t sumFits = FALSE;

    // length * outputInit, without performing the multiply that would wrap.
    if ( outputInit == 0 )
    {
        sumFits = TRUE;
    }
    else if ( length <= ( 0xFFFFFFFFu / outputInit ) )
    {
        sumFits = TRUE;
    }
    else
    {
        sumFits = FALSE;
    }

    if ( ( driver != NULL ) && ( buffer != NULL ) && ( length != 0 ) &&
            ( sumFits == TRUE ) )
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
 * @note    The window sum is exact here, unlike the float variant, so it is
 *          never rebuilt. What it can do instead is overflow: the sum is a
 *          uint32_t and nothing checks it on this path. Keep every sample
 *          below 0xFFFFFFFF / length, the bound mafInitu32 documents.
 * @note    The output is an integer division of the sum by the window length,
 *          so it is truncated towards zero rather than rounded.
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
 * @return  Current filtered output value, truncated towards zero by the
 *          integer division in mafIterationu32.
 */
uint32_t mafGetOutputu32 ( const mafu32_t* const driver )
{
    return ( driver->output );
}

