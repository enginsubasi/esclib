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
}

/*
 * @about: Updates value.
 */
void bininpUpdate ( bininp_t* driver, uint8_t newData )
{
    uint8_t currentPhysicalValue = 0;
    
    currentPhysicalValue = newData;
    
    if ( currentPhysicalValue != driver->output )
    {
        ++driver->filterCounter;
        
        if ( driver->filterCounter > driver->filterCount )
        {
            driver->output = currentPhysicalValue;
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
