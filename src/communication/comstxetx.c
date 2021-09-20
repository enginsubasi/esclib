/**
  ******************************************************************************
  *
  * @file:      comstxetx.c
  * @author:    Engin Subaşı
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     26/08/2020
  * @history:   26/08/2020 Created.
  *
  * @about:     Basic STX, ETX communication framework.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         comstxetxInit                   : Initialize comstxetx structure.
  *         comstxetxReceive                : 
  *         comstxetxEvaluate               : 
  *         comstxetxTimeoutCounter         :
  *
  * @notes:
  *
  ******************************************************************************
  */
  
#include "comstxetx.h"

/*
 * @about: Initialize comstxetx structure.
 */
void comstxetxInit ( com_stxetx_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer uint32_t rxSize, uint32_t txSize, uint8_t stx, uint8_t etx, uint32_t rxTimeout, void (*packetProcess) ( uint8_t* buffer, uint32_t index ) )
{
    uint32_t i = 0;
    
    // Function assignment.
    driver->packetProcess = packetProcess;
    
    // Parameter settings.
    driver->rxBuffer = rxBuffer;
    driver->txBuffer = txBuffer;
    
    driver->rxSize = rxSize;
    driver->txSize = txSize;
    
    driver->stx = stx;
    driver->etx = etx;
    
    driver->rxTimeoutCounter = 0;
    driver->rxTimeout = rxTimeout;
    
    // Initialize to zero and FALSE
    driver->rxIndex = 0;
    driver->rxReadyToEvaluate = FALSE;
    
    // Fill with zero
    for ( i = 0; i < driver->rxSize; ++i )
    {
        driver->rxBuffer[ i ] = 0;
    }
    
    for ( i = 0; i < driver->txSize; ++i )
    {
        driver->txBuffer[ i ] = 0;
    }
}

/*
 * @about: Run when new data taken. Run at rx interrupt or run at timer interrupt with control new data flag.
 */
void comstxetxReceive ( com_stxetx_t* driver, uint8_t data )
{
    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxIndex == 0 )
        {
            if ( data == driver->stx )
            {
                driver->rxBuffer[ driver->rxIndex ] = data;
                ++driver->rxIndex;
            }
        }
        else
        {
            if ( data == driver->etx )
            {
                driver->rxReadyToEvaluate = TRUE;
            }
            else
            {
                driver->rxBuffer[ driver->rxIndex ] = data;
                ++driver->rxIndex;
                
                if ( driver->rxIndex > driver->rxMaxLength )
                {
                    // Terminate all received bytes.
                    driver->rxIndex = 0;
                }
            }
        }
    }
}

/*
 * @about: Run at the loop or timer interrupt to evaluate received data frame.
 */
void comstxetxEvaluate ( com_stxetx_t* driver )
{
    if ( driver->rxReadyToEvaluate = TRUE )
    {
        driver->packetProcess ( driver->rxBuffer, driver->rxIndex );
        
        driver->rxIndex = 0;
        driver->rxReadyToEvaluate = FALSE;
    }
}

/*
 * @about: Run at constant timer interrupt.
 */
void comstxetxTimeoutCounter ( com_stxetx_t* driver )
{
    if ( ( driver->rxIndex != 0 ) && ( drive->rxReadyToEvaluate == FALSE ) )
    {
        if ( driver->rxTimeoutCounter > driver->rxTimeout )
        {
            // Terminate all received bytes.
            driver->rxIndex = 0;
            driver->rxTimeoutCounter = 0;
        }
        else
        {
            ++driver->rxTimeoutCounter;
        }
    }
}
