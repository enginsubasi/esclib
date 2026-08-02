/**
  ******************************************************************************
  *
  * @file      deadband.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
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
