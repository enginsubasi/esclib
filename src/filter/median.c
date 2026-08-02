/**
  ******************************************************************************
  *
  * @file      median.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      02/08/2026
  *
  * @brief     Median filter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
  *
  * @note      The moving average and the exponential moving average are linear,
  *            so a single bad sample is spread across the output rather than
  *            removed. A 1000 count spike into a five wide moving average moves
  *            the output by 200 and holds it there for five samples. The median
  *            ignores that sample outright. Reach for this against switching
  *            noise on a converter input, interference on a long cable, or a
  *            contact that drops out for one reading.
  *
  * @note      Two caller owned arrays of the same length are needed: one holds
  *            the window in arrival order, the other the same values sorted.
  *            Keeping the sorted copy up to date costs one removal and one
  *            insertion per sample, both linear, so nothing is ever sorted from
  *            scratch. The module carries its own insertion logic rather than
  *            calling the sort module, because modules here do not include one
  *            another.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "median.h"

/**
 * @brief   Initializes the median filter.
 * @param[out] driver      Filter state to initialize.
 * @param[out] buffer      Caller owned window of at least length entries.
 * @param[out] sorted      Caller owned scratch array of at least length entries.
 * @param[in]  length      Number of samples in the window. Must be odd.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when a pointer is NULL, length is zero, or
 *          length is even.
 * @note    Neither array is copied. Both must outlive the filter, and neither
 *          may be touched by the caller while the filter is in use.
 * @note    An even length is rejected. A median needs a single middle element;
 *          with an even window the two candidates would have to be averaged,
 *          which reintroduces exactly the blending this filter exists to avoid.
 *          Use 3, 5 or 7. Cost rises linearly with length, and beyond about 9
 *          the added delay usually outweighs the extra rejection.
 */
uint8_t medianInit ( median_t* driver, float* buffer, float* sorted, uint32_t length, float outputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( buffer != NULL ) && ( sorted != NULL ) &&
            ( length != 0 ) && ( ( length % 2u ) == 1u ) )
    {
        driver->buffer = buffer;
        driver->sorted = sorted;
        driver->length = length;
        driver->index = 0;
        driver->output = outputInit;

        for ( i = 0; i < length; ++i )
        {
            driver->buffer[ i ] = outputInit;
            driver->sorted[ i ] = outputInit;
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
 * @brief   Adds a new sample to the median filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample.
 * @note    The oldest value is located in the sorted array by an exact
 *          comparison. That is safe even in float: the value being searched for
 *          was copied out of the sorted array's own contents, so the bits
 *          match. Duplicates are interchangeable, so removing whichever copy is
 *          found first is correct.
 */
void medianIteration ( median_t* driver, float newData )
{
    uint32_t i = 0;
    uint32_t position = 0;
    float oldest = 0;

    oldest = driver->buffer[ driver->index ];
    driver->buffer[ driver->index ] = newData;

    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Find the outgoing value in the sorted array.
    position = 0;

    for ( i = 0; i < driver->length; ++i )
    {
        if ( driver->sorted[ i ] == oldest )
        {
            position = i;
            break;
        }
    }

    // Close the gap it leaves behind.
    for ( i = position; i < ( driver->length - 1u ); ++i )
    {
        driver->sorted[ i ] = driver->sorted[ i + 1u ];
    }

    // Find where the incoming value belongs among the remaining ones.
    position = driver->length - 1u;

    for ( i = 0; i < ( driver->length - 1u ); ++i )
    {
        if ( driver->sorted[ i ] > newData )
        {
            position = i;
            break;
        }
    }

    // Open a gap for it and drop it in.
    for ( i = driver->length - 1u; i > position; --i )
    {
        driver->sorted[ i ] = driver->sorted[ i - 1u ];
    }

    driver->sorted[ position ] = newData;

    driver->output = driver->sorted[ driver->length / 2u ];
}

/**
 * @brief   Gets the current output of the median filter.
 * @param[in] driver  Filter state.
 * @return  Middle value of the current window.
 */
float medianGetOutput ( const median_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes the median filter for unsigned 32-bit data.
 * @param[out] driver      Filter state to initialize.
 * @param[out] buffer      Caller owned window of at least length entries.
 * @param[out] sorted      Caller owned scratch array of at least length entries.
 * @param[in]  length      Number of samples in the window. Must be odd.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when a pointer is NULL, length is zero, or
 *          length is even.
 * @note    Neither array is copied. Both must outlive the filter.
 */
uint8_t medianInitu32 ( medianu32_t* driver, uint32_t* buffer, uint32_t* sorted, uint32_t length, uint32_t outputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( buffer != NULL ) && ( sorted != NULL ) &&
            ( length != 0 ) && ( ( length % 2u ) == 1u ) )
    {
        driver->buffer = buffer;
        driver->sorted = sorted;
        driver->length = length;
        driver->index = 0;
        driver->output = outputInit;

        for ( i = 0; i < length; ++i )
        {
            driver->buffer[ i ] = outputInit;
            driver->sorted[ i ] = outputInit;
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
 * @brief   Adds a new sample to the unsigned 32-bit median filter and updates
 *          its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample.
 */
void medianIterationu32 ( medianu32_t* driver, uint32_t newData )
{
    uint32_t i = 0;
    uint32_t position = 0;
    uint32_t oldest = 0;

    oldest = driver->buffer[ driver->index ];
    driver->buffer[ driver->index ] = newData;

    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank. */
    }

    position = 0;

    for ( i = 0; i < driver->length; ++i )
    {
        if ( driver->sorted[ i ] == oldest )
        {
            position = i;
            break;
        }
    }

    for ( i = position; i < ( driver->length - 1u ); ++i )
    {
        driver->sorted[ i ] = driver->sorted[ i + 1u ];
    }

    position = driver->length - 1u;

    for ( i = 0; i < ( driver->length - 1u ); ++i )
    {
        if ( driver->sorted[ i ] > newData )
        {
            position = i;
            break;
        }
    }

    for ( i = driver->length - 1u; i > position; --i )
    {
        driver->sorted[ i ] = driver->sorted[ i - 1u ];
    }

    driver->sorted[ position ] = newData;

    driver->output = driver->sorted[ driver->length / 2u ];
}

/**
 * @brief   Gets the current output of the unsigned 32-bit median filter.
 * @param[in] driver  Filter state.
 * @return  Middle value of the current window.
 */
uint32_t medianGetOutputu32 ( const medianu32_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes the median filter for signed 32-bit data.
 * @param[out] driver      Filter state to initialize.
 * @param[out] buffer      Caller owned window of at least length entries.
 * @param[out] sorted      Caller owned scratch array of at least length entries.
 * @param[in]  length      Number of samples in the window. Must be odd.
 * @param[in]  outputInit  Value the whole window is preloaded with.
 * @return  TRUE on success, FALSE when a pointer is NULL, length is zero, or
 *          length is even.
 * @note    Neither array is copied. Both must outlive the filter.
 */
uint8_t medianIniti32 ( mediani32_t* driver, int32_t* buffer, int32_t* sorted, uint32_t length, int32_t outputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( buffer != NULL ) && ( sorted != NULL ) &&
            ( length != 0 ) && ( ( length % 2u ) == 1u ) )
    {
        driver->buffer = buffer;
        driver->sorted = sorted;
        driver->length = length;
        driver->index = 0;
        driver->output = outputInit;

        for ( i = 0; i < length; ++i )
        {
            driver->buffer[ i ] = outputInit;
            driver->sorted[ i ] = outputInit;
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
 * @brief   Adds a new sample to the signed 32-bit median filter and updates
 *          its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample.
 */
void medianIterationi32 ( mediani32_t* driver, int32_t newData )
{
    uint32_t i = 0;
    uint32_t position = 0;
    int32_t oldest = 0;

    oldest = driver->buffer[ driver->index ];
    driver->buffer[ driver->index ] = newData;

    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank. */
    }

    position = 0;

    for ( i = 0; i < driver->length; ++i )
    {
        if ( driver->sorted[ i ] == oldest )
        {
            position = i;
            break;
        }
    }

    for ( i = position; i < ( driver->length - 1u ); ++i )
    {
        driver->sorted[ i ] = driver->sorted[ i + 1u ];
    }

    position = driver->length - 1u;

    for ( i = 0; i < ( driver->length - 1u ); ++i )
    {
        if ( driver->sorted[ i ] > newData )
        {
            position = i;
            break;
        }
    }

    for ( i = driver->length - 1u; i > position; --i )
    {
        driver->sorted[ i ] = driver->sorted[ i - 1u ];
    }

    driver->sorted[ position ] = newData;

    driver->output = driver->sorted[ driver->length / 2u ];
}

/**
 * @brief   Gets the current output of the signed 32-bit median filter.
 * @param[in] driver  Filter state.
 * @return  Middle value of the current window.
 */
int32_t medianGetOutputi32 ( const mediani32_t* const driver )
{
    return ( driver->output );
}
