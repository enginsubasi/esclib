/**
  ******************************************************************************
  *
  * @file      hysteresis.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
  * @date      16/07/2020
  *
  * @brief     Hysteresis control.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 16/07/2020 Created. @n
  * 24/08/2020 Data type changed from double to float. @n
  * 24/08/2020 Naming changes. Comments added. @n
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 hysteresisInit rejects an upValue below downValue, @n
  *            which would leave no band for the output to hold in. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The accessors that only read take a const driver. @n
  * 06/08/2026 The i32 and u32 variants are added. This module was @n
  *            float only, and a threshold comparison needs no @n
  *            floating point at all. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "hysteresis.h"

/**
 * @brief   Initializes the hysteresis controller state.
 * @param[out] driver     Controller state to initialize.
 * @param[in]  upValue    Threshold above which the output switches TRUE.
 * @param[in]  downValue  Threshold below which the output switches FALSE.
 * @return  TRUE on success, FALSE when driver is NULL or upValue is below
 *          downValue.
 * @note    The output is initialized to FALSE.
 * @note    upValue below downValue is rejected because it leaves no band
 *          for the output to hold in. Every input would then satisfy one of
 *          the two thresholds and the controller would behave as a plain
 *          comparator rather than a hysteresis. Equal values are allowed and
 *          give exactly that comparator, which is a legitimate degenerate
 *          setting.
 */
uint8_t hysteresisInit ( hysteresis_t* driver, float upValue, float downValue )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( upValue >= downValue ) )
    {
        driver->output = FALSE;
        driver->up = upValue;
        driver->dw = downValue;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one hysteresis control iteration for the given input.
 * @param[in,out] driver  Controller state.
 * @param[in]     input   Input value to compare against the up and down thresholds.
 * @note    The output only changes when input crosses the up or down threshold;
 *          it holds its previous value inside the band between them.
 */
void hysteresisControl ( hysteresis_t* driver, float input )
{
    if ( input > driver->up )
    {
        driver->output = TRUE;
    }
    else if ( input < driver->dw )
    {
        driver->output = FALSE;
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Gets the current output of the hysteresis controller.
 * @param[in] driver  Controller state.
 * @return  TRUE or FALSE depending on which threshold was last crossed.
 */
uint8_t hysteresisGetOutput ( const hysteresis_t* const driver )
{
    return ( driver->output );
}



/**
 * @brief   Initializes a signed 32-bit hysteresis controller.
 * @param[out] driver     Controller state to initialize.
 * @param[in]  upValue    Threshold the input must exceed to set the output.
 * @param[in]  downValue  Threshold the input must fall below to clear it.
 * @return  TRUE on success, FALSE when driver is NULL or upValue is below
 *          downValue.
 * @note    The output starts FALSE whatever the thresholds are, so the first
 *          hysteresisControli32 with an input above upValue is what sets it.
 * @note    Equal thresholds are accepted and give a band of zero width, which
 *          is a plain comparator. Only an inverted pair is rejected, because
 *          it would make both branches reachable from the same input.
 */
uint8_t hysteresisIniti32 ( hysteresisi32_t* driver, int32_t upValue, int32_t downValue )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( upValue >= downValue ) )
    {
        driver->output = FALSE;
        driver->up = upValue;
        driver->dw = downValue;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one signed hysteresis iteration for the given input.
 * @param[in,out] driver  Controller state.
 * @param[in]     input   Value to compare against the two thresholds.
 * @note    The output only changes when input crosses a threshold; it holds
 *          its previous value inside the band between them. There is no
 *          arithmetic here at all, so the signed width has no edge case
 *          the float one does not: the two comparisons are the whole of it.
 */
void hysteresisControli32 ( hysteresisi32_t* driver, int32_t input )
{
    if ( input > driver->up )
    {
        driver->output = TRUE;
    }
    else if ( input < driver->dw )
    {
        driver->output = FALSE;
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Gets the current output of the signed hysteresis controller.
 * @param[in] driver  Controller state.
 * @return  TRUE or FALSE depending on which threshold was last crossed.
 */
uint8_t hysteresisGetOutputi32 ( const hysteresisi32_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Initializes an unsigned 32-bit hysteresis controller.
 * @param[out] driver     Controller state to initialize.
 * @param[in]  upValue    Threshold the input must exceed to set the output.
 * @param[in]  downValue  Threshold the input must fall below to clear it.
 * @return  TRUE on success, FALSE when driver is NULL or upValue is below
 *          downValue.
 * @note    A downValue of zero can never be crossed, since no unsigned input
 *          is below it. That is a legal configuration and means the output
 *          latches on for good once it has been set, which is a use rather
 *          than a mistake, so it is accepted.
 */
uint8_t hysteresisInitu32 ( hysteresisu32_t* driver, uint32_t upValue, uint32_t downValue )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( upValue >= downValue ) )
    {
        driver->output = FALSE;
        driver->up = upValue;
        driver->dw = downValue;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one unsigned hysteresis iteration for the given input.
 * @param[in,out] driver  Controller state.
 * @param[in]     input   Value to compare against the two thresholds.
 * @note    The output only changes when input crosses a threshold; it holds
 *          its previous value inside the band between them.
 */
void hysteresisControlu32 ( hysteresisu32_t* driver, uint32_t input )
{
    if ( input > driver->up )
    {
        driver->output = TRUE;
    }
    else if ( input < driver->dw )
    {
        driver->output = FALSE;
    }
    else
    {
        /* Intentionally blank. */
    }
}

/**
 * @brief   Gets the current output of the unsigned hysteresis controller.
 * @param[in] driver  Controller state.
 * @return  TRUE or FALSE depending on which threshold was last crossed.
 */
uint8_t hysteresisGetOutputu32 ( const hysteresisu32_t* const driver )
{
    return ( driver->output );
}
