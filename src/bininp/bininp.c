/**
  ******************************************************************************
  *
  * @file:      bininp.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 1.0.0
  * @cdate:     05/10/2020
  * @history:   05/10/2020 Created
  *             09/10/2020 Button name changed to bininp. It means binary input.
  *             24/10/2020 newData field added to bininpUpdate function instead of read input function pointer.
  *             09/07/2025 bininpGetRisingValue function is added.
  *             29/07/2026 Bug fix. The rising field was never set, so
  *                        bininpGetRisingValue always returned FALSE. The rising
  *                        edge is now detected in bininpUpdate.
  *             29/07/2026 Bug fix. bininpInit only assigned filterCount and left
  *                        filterCounter, output and rising uninitialized.
  *
  * @about:     Binary input read and filtering.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         bininpInit              :
  *         bininpUpdate            :
  *         bininpGetValue          :
  *         bininpGetRisingValue    :
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "bininp.h"

/*
 * @about: Initialize binary input structure.
 */
void bininpInit ( bininp_t* driver, uint32_t filterCount )
{
    driver->filterCount = filterCount;
    driver->filterCounter = 0;

    driver->output = FALSE;
    driver->rising = FALSE;
}

/*
 * @about: Updates value.
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

/*
 * @about: Gets current binary input value.
 */
uint8_t bininpGetValue ( bininp_t* driver )
{
    return ( driver->output );
}

/*
 * @about: Gets rising edge value and reset internal mamory.
 */
uint8_t bininpGetRisingValue ( bininp_t* driver )
{
    uint8_t retVal = 0;

    retVal = driver->rising;

    driver->rising = FALSE;

    return ( retVal );
}
