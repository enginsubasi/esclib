/**
  ******************************************************************************
  *
  * @file:      maf.c
  * @author:    Engin Subasi
  * @e-mail:    enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 2.0.0
  * @cdate:     26/04/2020
  * @mdate:     07/06/2020
  * @history:   26/04/2020 Created
  *             07/06/2020 Naming style changed
  *
  * @about:     Moving average filter.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         mafInit               : Initialize maf structure.
  *         mafIteration          : Adds new data to filter.
  *         mafGetOutput          : Gets current filter output.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "maf.h"

/*
 * @about: Initialize maf structure.
 */
int8_t mafInit ( maf_t* driver, uint32_t length, double outputInit )
{
    int8_t retVal = FALSE;
    uint32_t i = 0;

    if ( ( length != 0 ) && ( length <= MAX_FILTER_LENGTH ) )
    {
        driver->length = length;
        driver->output = outputInit;
        driver->sumOfArray = driver->length * driver->output;
        driver->index = 0;

        for ( i = 0; i < length; ++i )
        {
            driver->buffer[ i ] = outputInit;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/*
 * @about: Adds new data to filter.
 */
void mafIteration ( maf_t* driver, double newData )
{
    // Add new data to buffer array and sum. of buffer array.
    driver->sumOfArray -= driver->buffer[ driver->index ];
    driver->buffer[ driver->index ] = newData;
    driver->sumOfArray += driver->buffer[ driver->index ];

    // Calculate output.
    driver->output = ( driver->sumOfArray / driver->length );

    // Index control.
    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
}

/*
 * @about: Gets current filter output.
 */
double mafGetOutput ( maf_t* driver )
{
    return ( driver->output );
}

