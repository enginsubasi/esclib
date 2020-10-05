/**
  ******************************************************************************
  *
  * @file:      button.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 1.0.0
  * @cdate:     05/10/2020
  * @history:   05/10/2020 Created
  *
  * @about:     Button read and filtering.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         buttonInit              :
  *         buttonUpdate            :
  *         buttonGetValue          :
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "button.h"

/*
 * @about: Initialize button structure.
 */
int8_t buttonInit ( button_t* driver, uint32_t filterCount )
{
    driver->filterCount = filterCount;
}

/*
 * @about: Updates value.
 */
void buttonUpdate ( button_t* driver )
{
    uint8_t currentPhysicalValue = 0;
    
    currentPhysicalValue = driver->readButton ( );
    
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
 * @about: Gets current button value.
 */
uint8_t buttonGetValue ( button_t* driver )
{
    return ( driver->output );
}



