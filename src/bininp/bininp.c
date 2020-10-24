/**
  ******************************************************************************
  *
  * @file:      bininp.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 1.0.0
  * @cdate:     05/10/2020
  * @history:   05/10/2020 Created
  *             09/10/2020 Button name changed to bininp. It means binary input.
  *             24/10/2020 newData field added to bininpUpdate function instead of read input function pointer.
  *
  * @about:     Binary input read and filtering.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         bininpInit              :
  *         bininpUpdate            :
  *         bininpGetValue          :
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



