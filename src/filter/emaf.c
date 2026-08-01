/**
  ******************************************************************************
  *
  * @file      emaf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   3.0.1
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
  * 02/08/2026 Bug fix. emafu32 held its running value as a uint32_t, @n
  *            so every iteration truncated it and the filter stopped @n
  *            moving once the remaining difference fell below @n
  *            1 / alpha. With alpha at 0.01 it stuck 99 short of the @n
  *            input. The value is now a float accumulator and only @n
  *            GetOutput rounds it down. This changes emafu32_t. @n
  * 02/08/2026 The note claiming a small alpha still moved the output @n
  *            said the opposite of what the code did. Rewritten. @n
  * 02/08/2026 Init rejects an alpha that leaves 1 - alpha equal to 1. @n
  *            That covers zero and everything below roughly 1e-7, @n
  *            which float loses outright, leaving a filter that could @n
  *            never respond while reporting a successful init. @n
  * 02/08/2026 Bug fix. emafGetOutputu32 cast the float accumulator @n
  *            straight to uint32_t. A float carries 24 significant @n
  *            bits, so every input above 4294967167 rounds up to @n
  *            4294967296 on the way in, and casting that back is @n
  *            undefined behaviour. The read clamps now. @n
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
 * @return  TRUE on success, FALSE when driver is NULL, alpha is outside the
 *          [0, 1] range, or alpha is too small to affect the output.
 * @note    An alpha that leaves 1 - alpha equal to 1 is rejected. That covers
 *          alpha of zero and, because the weights are held as float, every
 *          alpha below roughly 1e-7 as well: the subtraction loses it and the
 *          filter would sit on its initial value forever while reporting a
 *          successful init.
 * @note    The comparison against 1 is deliberately exact. The question is
 *          whether alpha survives being stored, not whether it is close to
 *          anything.
 */
uint8_t emafInit ( emaf_t* driver, float alpha, float outputInit )
{
    uint8_t retVal = FALSE;
    float alphan = 0;

    alphan = 1.0f - alpha;

    if ( ( driver != NULL ) && ( alpha >= 0 ) && ( alpha <= 1 ) &&
            ( alphan != 1.0f ) )
    {
        driver->alpha = alpha;
        driver->alphan = alphan;
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
 * @return  TRUE on success, FALSE when driver is NULL, alpha is outside the
 *          [0, 1] range, or alpha is too small to affect the output.
 * @note    The running value is held as a float accumulator and only rounded
 *          down to uint32_t when emafGetOutputu32 reads it. Keeping uint32_t
 *          in the feedback path instead would truncate every iteration, and
 *          the filter would then stop moving as soon as the remaining
 *          difference fell below 1 / alpha, leaving it stuck that far from
 *          the input for good.
 * @note    A float holds 24 significant bits, so inputs above 16777216 lose
 *          their low bits in the accumulator.
 * @note    alpha is validated exactly as in emafInit.
 */
uint8_t emafInitu32 ( emafu32_t* driver, float alpha, uint32_t outputInit )
{
    uint8_t retVal = FALSE;
    float alphan = 0;

    alphan = 1.0f - alpha;

    if ( ( driver != NULL ) && ( alpha >= 0 ) && ( alpha <= 1 ) &&
            ( alphan != 1.0f ) )
    {
        driver->alpha = alpha;
        driver->alphan = alphan;
        driver->accumulator = ( float ) outputInit;

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
 * @note    The blend stays in the float accumulator. Nothing is truncated
 *          here, so however small alpha is, every sample still moves the
 *          filter.
 */
void emafIterationu32 ( emafu32_t* driver, uint32_t newData )
{
    driver->accumulator = ( ( ( float ) newData * driver->alpha ) +
                            ( driver->accumulator * driver->alphan ) );
}

/**
 * @brief   Gets the current output of the unsigned 32-bit exponential moving
 *          average filter.
 * @param[in] driver  Filter state.
 * @return  Current filtered value, rounded down to uint32_t and clamped to
 *          0xFFFFFFFF.
 * @note    Only this function truncates. The accumulator it reads keeps its
 *          fraction, so repeated calls do not drag the filter down.
 * @note    The clamp is not defensive padding. A float carries 24 significant
 *          bits, so every uint32_t above 4294967167 rounds up to 4294967296
 *          on its way into the accumulator, which is one past the range.
 *          Converting that back with a plain cast is undefined behaviour, and
 *          those inputs are reachable: a free running 32-bit counter or a
 *          sensor reporting 0xFFFFFFFF as its error value both land there.
 * @note    The accumulator cannot go the other way. alpha and 1 - alpha are
 *          both in [0, 1] and every input is unsigned, so a blend of
 *          non-negative terms stays non-negative and needs no lower clamp.
 */
uint32_t emafGetOutputu32 ( const emafu32_t* const driver )
{
    uint32_t retVal = 0;

    // 4294967296.0f is exactly 2^32 and is representable, unlike 0xFFFFFFFF.
    if ( driver->accumulator >= 4294967296.0f )
    {
        retVal = 0xFFFFFFFFu;
    }
    else
    {
        retVal = ( uint32_t ) driver->accumulator;
    }

    return ( retVal );
}
