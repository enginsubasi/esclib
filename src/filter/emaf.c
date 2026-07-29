/**
  ******************************************************************************
  *
  * @file      emaf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   2.0.0
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
  *
  ******************************************************************************
  */

#include "emaf.h"

/**
 * @brief   Initializes the exponential moving average filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  alpha       Smoothing factor in the range [0, 1]; higher values weight new samples more.
 * @param[in]  outputInit  Initial output value.
 * @return  TRUE on success, FALSE when alpha is outside the [0, 1] range.
 */
int8_t emafInit ( emaf_t* driver, float alpha, float outputInit )
{
    int8_t retVal = FALSE;

    if ( ( alpha >= 0 ) && ( alpha <= 1 ) )
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
float emafGetOutput ( emaf_t* driver )
{
    return ( driver->output );
}

