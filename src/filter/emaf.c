/**
  ******************************************************************************
  *
  * @file      emaf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   2.2.0
  * @date      22/04/2020
  *
  * @brief     Exponential moving average filter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 22/04/2020 Created @n
  * 07/06/2020 Naming style changed @n
  * 24/08/2020 Data type changed from double to float. @n
  * 01/08/2026 emafInit returns uint8_t rather than int8_t and rejects @n
  *            a NULL driver in addition to an out of range alpha. @n
  * 01/08/2026 Pointer checks use NULL from stddef.h, matching the @n
  *            wording the documentation already used. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The accessors that only read take a const driver. @n
  * 01/08/2026 The u32 variant is added, so emaf matches maf, which @n
  *            already had one. alpha stays a float and the blend runs @n
  *            in float, so a small alpha still moves the output. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "emaf.h"

/**
 * @brief   Initializes the exponential moving average filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  alpha       Smoothing factor in the range [0, 1]; higher values weight new samples more.
 * @param[in]  outputInit  Initial output value.
 * @return  TRUE on success, FALSE when driver is NULL or alpha is outside
 *          the [0, 1] range.
 */
uint8_t emafInit ( emaf_t* driver, float alpha, float outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( alpha >= 0 ) && ( alpha <= 1 ) )
    {
        driver->alpha = alpha;
        driver->alphan = 1 - alpha;
        driver->output = outputInit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds a new sample to the exponential moving average filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample to blend into the filtered output.
 */
void emafIteration ( emaf_t* driver, float newData )
{
    driver->output = ( ( newData * driver->alpha ) + ( driver->output * driver->alphan ) );
}

/**
 * @brief   Gets the current output of the exponential moving average filter.
 * @param[in] driver  Filter state.
 * @return  Current filtered output value.
 */
float emafGetOutput ( const emaf_t* const driver )
{
    return ( driver->output );
}


/**
 * @brief   Initializes the exponential moving average filter for unsigned 32-bit data.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  alpha       Smoothing factor in the range [0, 1]; higher values weight new samples more.
 * @param[in]  outputInit  Initial output value.
 * @return  TRUE on success, FALSE when driver is NULL or alpha is outside
 *          the [0, 1] range.
 * @note    alpha stays a float. The blend is computed in float and only the
 *          result is truncated back to uint32_t, so a small alpha still moves
 *          the output instead of rounding away to nothing.
 */
uint8_t emafInitu32 ( emafu32_t* driver, float alpha, uint32_t outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( alpha >= 0 ) && ( alpha <= 1 ) )
    {
        driver->alpha = alpha;
        driver->alphan = 1 - alpha;
        driver->output = outputInit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds a new sample to the unsigned 32-bit exponential moving average
 *          filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample to blend into the filtered output.
 * @note    The blend runs in float and the result is truncated towards zero on
 *          the way back into uint32_t, so the output can sit one below the
 *          exact value.
 */
void emafIterationu32 ( emafu32_t* driver, uint32_t newData )
{
    float blended = 0;

    blended = ( ( ( float ) newData * driver->alpha ) +
                ( ( float ) driver->output * driver->alphan ) );

    driver->output = ( uint32_t ) blended;
}

/**
 * @brief   Gets the current output of the unsigned 32-bit exponential moving
 *          average filter.
 * @param[in] driver  Filter state.
 * @return  Current filtered output value.
 */
uint32_t emafGetOutputu32 ( const emafu32_t* const driver )
{
    return ( driver->output );
}
