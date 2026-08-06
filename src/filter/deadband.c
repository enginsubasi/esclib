/**
  ******************************************************************************
  *
  * @file      deadband.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
  * @date      02/08/2026
  *
  * @brief     Deadband filter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
  *
  * @note      The output holds still until the input moves further than the
  *            threshold. Use it to stop the last digit of a display flickering,
  *            to stop sending telemetry that has not really changed, or to
  *            remove the last bit of converter noise outright rather than
  *            averaging it down.
  *
  * @note      This is not the hysteresis module. That one turns a value into a
  *            boolean with two thresholds; this one passes the value through
  *            and holds it.
  * 06/08/2026 The u32 variants are added. This file carried float @n
  *            and i32 only, while emaf and median next to it @n
  *            carried all three widths. Both ends saturate; an @n
  *            unsigned step below zero wraps rather than clipping. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "deadband.h"

/**
 * @brief   Initializes the deadband filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  threshold   How far the input must move before the output does.
 * @param[in]  mode        DB_SNAP to jump the output onto the input, DB_DRAG to
 *                         leave it trailing by the threshold.
 * @param[in]  outputInit  Value the output starts from.
 * @return  TRUE on success, FALSE when driver is NULL, threshold is negative,
 *          or mode is neither DB_SNAP nor DB_DRAG.
 * @note    DB_SNAP reacts without lag once the threshold is crossed but jumps
 *          by the whole distance at once. DB_DRAG moves only by the amount
 *          past the threshold, so a slowly drifting input produces a smooth
 *          output that stays one threshold behind. DB_DRAG is the better
 *          choice against noise, DB_SNAP against a genuine step.
 * @note    A threshold of zero is allowed and makes the filter a wire. Unlike
 *          a zero gain elsewhere in the library that would be a filter unable
 *          to respond, this one still tracks its input perfectly.
 */
uint8_t deadbandInit ( deadband_t* driver, float threshold, uint8_t mode, float outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( threshold >= 0 ) &&
            ( ( mode == DB_SNAP ) || ( mode == DB_DRAG ) ) )
    {
        driver->threshold = threshold;
        driver->mode = mode;
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
 * @brief   Updates the output only when the new sample leaves the deadband.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample.
 */
void deadbandIteration ( deadband_t* driver, float newData )
{
    if ( newData > ( driver->output + driver->threshold ) )
    {
        if ( driver->mode == DB_DRAG )
        {
            driver->output = newData - driver->threshold;
        }
        else
        {
            driver->output = newData;
        }
    }
    else if ( newData < ( driver->output - driver->threshold ) )
    {
        if ( driver->mode == DB_DRAG )
        {
            driver->output = newData + driver->threshold;
        }
        else
        {
            driver->output = newData;
        }
    }
    else
    {
        /* Inside the band. The output holds. */
    }
}

/**
 * @brief   Gets the current output of the deadband filter.
 * @param[in] driver  Filter state.
 * @return  Current output value.
 */
float deadbandGetOutput ( const deadband_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes the deadband filter for signed 32-bit data.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  threshold   How far the input must move before the output does.
 * @param[in]  mode        DB_SNAP or DB_DRAG, as in deadbandInit.
 * @param[in]  outputInit  Value the output starts from.
 * @return  TRUE on success, FALSE when driver is NULL, threshold is negative,
 *          or mode is neither DB_SNAP nor DB_DRAG.
 */
uint8_t deadbandIniti32 ( deadbandi32_t* driver, int32_t threshold, uint8_t mode, int32_t outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( threshold >= 0 ) &&
            ( ( mode == DB_SNAP ) || ( mode == DB_DRAG ) ) )
    {
        driver->threshold = threshold;
        driver->mode = mode;
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
 * @brief   Updates the signed 32-bit output only when the new sample leaves
 *          the deadband.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample.
 * @note    The band edges are built by clamping rather than by adding the
 *          threshold to the output directly, since that sum overflows once the
 *          output sits near either end of the int32_t range.
 * @note    In DB_DRAG the arithmetic that produces the new output cannot
 *          overflow: it only ever moves the sample back towards the old
 *          output, which is itself a valid int32_t.
 */
void deadbandIterationi32 ( deadbandi32_t* driver, int32_t newData )
{
    int32_t upper = 0;
    int32_t lower = 0;

    if ( driver->output > ( INT32_MAX - driver->threshold ) )
    {
        upper = INT32_MAX;
    }
    else
    {
        upper = driver->output + driver->threshold;
    }

    if ( driver->output < ( INT32_MIN + driver->threshold ) )
    {
        lower = INT32_MIN;
    }
    else
    {
        lower = driver->output - driver->threshold;
    }

    if ( newData > upper )
    {
        if ( driver->mode == DB_DRAG )
        {
            driver->output = newData - driver->threshold;
        }
        else
        {
            driver->output = newData;
        }
    }
    else if ( newData < lower )
    {
        if ( driver->mode == DB_DRAG )
        {
            driver->output = newData + driver->threshold;
        }
        else
        {
            driver->output = newData;
        }
    }
    else
    {
        /* Inside the band. The output holds. */
    }
}

/**
 * @brief   Gets the current output of the signed 32-bit deadband filter.
 * @param[in] driver  Filter state.
 * @return  Current output value.
 */
int32_t deadbandGetOutputi32 ( const deadbandi32_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes an unsigned dead band filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  threshold   How far the input must move before the output does.
 * @param[in]  mode        DB_SNAP or DB_DRAG.
 * @param[in]  outputInit  Value the output starts at.
 * @return  TRUE on success, FALSE when driver is NULL or mode is neither
 *          DB_SNAP nor DB_DRAG.
 * @note    There is no range check on the threshold, because an unsigned one
 *          cannot be negative and zero is legal here. A zero threshold gives
 *          a band of no width, which passes every sample through unchanged;
 *          deadbandInit and deadbandIniti32 accept it for the same reason and
 *          the three widths must not disagree about what a valid filter is.
 */
uint8_t deadbandInitu32 ( deadbandu32_t* driver, uint32_t threshold, uint8_t mode, uint32_t outputInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) &&
            ( ( mode == DB_SNAP ) || ( mode == DB_DRAG ) ) )
    {
        driver->threshold = threshold;
        driver->mode = mode;
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
 * @brief   Holds the unsigned output still until the input leaves the band.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  Sample to test against the band.
 * @note    Both band edges saturate rather than wrapping. An unsigned
 *          subtraction below zero wraps to near UINT32_MAX, which would put
 *          the lower edge above the upper one and make the band swallow
 *          everything.
 * @note    DB_DRAG leaves the output one threshold behind the input, so the
 *          band travels with it. DB_SNAP jumps the output onto the input.
 */
void deadbandIterationu32 ( deadbandu32_t* driver, uint32_t newData )
{
    uint32_t upper = 0;
    uint32_t lower = 0;

    if ( driver->output > ( UINT32_MAX - driver->threshold ) )
    {
        upper = UINT32_MAX;
    }
    else
    {
        upper = driver->output + driver->threshold;
    }

    if ( driver->output < driver->threshold )
    {
        lower = 0;
    }
    else
    {
        lower = driver->output - driver->threshold;
    }

    if ( newData > upper )
    {
        if ( driver->mode == DB_DRAG )
        {
            driver->output = newData - driver->threshold;
        }
        else
        {
            driver->output = newData;
        }
    }
    else if ( newData < lower )
    {
        if ( driver->mode == DB_DRAG )
        {
            /*
             * newData is below lower, which is at least zero, so it may sit
             * closer to zero than one threshold. Adding would be safe in
             * range here, but the saturation is written out so the reader
             * does not have to reconstruct that argument.
             */
            if ( newData > ( UINT32_MAX - driver->threshold ) )
            {
                driver->output = UINT32_MAX;
            }
            else
            {
                driver->output = newData + driver->threshold;
            }
        }
        else
        {
            driver->output = newData;
        }
    }
    else
    {
        /* Inside the band. The output holds. */
    }
}

/**
 * @brief   Returns the current filter output.
 * @param[in]  driver  Initialized filter.
 * @return  The dead banded value.
 */
uint32_t deadbandGetOutputu32 ( const deadbandu32_t* const driver )
{
    return ( driver->output );
}
