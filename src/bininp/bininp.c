/**
  ******************************************************************************
  *
  * @file      bininp.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   1.0.0
  * @date      05/10/2020
  *
  * @brief     Binary input read and filtering.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 05/10/2020 Created @n
  * 09/10/2020 Button name changed to bininp. It means binary input. @n
  * 24/10/2020 newData field added to bininpUpdate function instead of read input function pointer. @n
  * 09/07/2025 bininpGetRisingValue function is added. @n
  * 29/07/2026 Bug fix. The rising field was never set, so @n
  *            bininpGetRisingValue always returned FALSE. The rising @n
  *            edge is now detected in bininpUpdate. @n
  * 29/07/2026 Bug fix. bininpInit only assigned filterCount and left @n
  *            filterCounter, output and rising uninitialized. @n
  *
  ******************************************************************************
  */

#include "bininp.h"

/**
 * @brief   Initializes the binary input filter state.
 * @param[out] driver       Binary input state to initialize.
 * @param[in]  filterCount  Debounce threshold. A new physical value is
 *                          accepted as the filtered output only once it has
 *                          persisted for more than filterCount consecutive
 *                          bininpUpdate calls, i.e. filterCount + 1 calls.
 */
void bininpInit ( bininp_t* driver, uint32_t filterCount )
{
    driver->filterCount = filterCount;
    driver->filterCounter = 0;

    driver->output = FALSE;
    driver->rising = FALSE;
}

/**
 * @brief   Filters a new raw sample and updates the binary input's output and rising edge flag.
 * @param[in,out] driver   Binary input state.
 * @param[in]     newData  New raw sample; any nonzero value is treated as TRUE.
 */
void bininpUpdate ( bininp_t* driver, uint8_t newData )
{
    uint8_t currentPhysicalValue = 0;

    // Normalize the input so that any non zero value is handled as TRUE.
    if ( newData != FALSE )
    {
        currentPhysicalValue = TRUE;
    }
    else
    {
        currentPhysicalValue = FALSE;
    }

    if ( currentPhysicalValue != driver->output )
    {
        ++driver->filterCounter;

        if ( driver->filterCounter > driver->filterCount )
        {
            // The filtered value changed. A FALSE to TRUE change is a rising edge.
            if ( driver->output == FALSE )
            {
                driver->rising = TRUE;
            }
            else
            {
                /* Intentionally blank. */
            }

            driver->output = currentPhysicalValue;
            driver->filterCounter = 0;
        }
    }
    else
    {
        driver->filterCounter = 0;
    }
}

/**
 * @brief   Gets the current filtered value of the binary input.
 * @param[in] driver  Binary input state.
 * @return  TRUE or FALSE, the current debounced output value.
 */
uint8_t bininpGetValue ( bininp_t* driver )
{
    return ( driver->output );
}

/**
 * @brief   Gets whether a rising edge occurred on the binary input and clears the flag.
 * @param[in,out] driver  Binary input state.
 * @return  TRUE when a rising edge occurred since the last call, FALSE otherwise.
 * @note    Reading the flag clears it, so an immediate second call returns FALSE.
 */
uint8_t bininpGetRisingValue ( bininp_t* driver )
{
    uint8_t retVal = 0;

    retVal = driver->rising;

    driver->rising = FALSE;

    return ( retVal );
}
