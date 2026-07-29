/**
  ******************************************************************************
  *
  * @file      hysteresis.c
  * @author    Engin Subaşı <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
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
  *
  ******************************************************************************
  */

#include "hysteresis.h"

/**
 * @brief   Initializes the hysteresis controller state.
 * @param[out] driver     Controller state to initialize.
 * @param[in]  upValue    Threshold above which the output switches TRUE.
 * @param[in]  downValue  Threshold below which the output switches FALSE.
 * @note    The output is initialized to FALSE.
 */
void hysteresisInit ( hysteresis_t* driver, float upValue, float downValue )
{
    driver->output = FALSE;
    driver->up = upValue;
    driver->dw = downValue;
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
uint8_t hysteresisGetOutput ( hysteresis_t* driver )
{
    return ( driver->output );
}


