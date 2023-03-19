/**
  ******************************************************************************
  *
  * @file:      comat.c
  * @author:    Engin Subaşı
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     20/02/2020
  * @history:   20/02/2020 Created
  *
  * @about:     AT communication framework.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         comatInit       : Brief
  *         comatReceive    : Brief
  *         comatEvaluate   :
  *         comatTimeoutCounter :
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "comat.h"

/*
 * @about: Initialize comat structure.
 */
void comatInit ( comat_t* driver, uint8_t* rxBuffer, uint8_t* txBuffer,
                                uint32_t rxSize, uint32_t txSize,
                                uint32_t rxTimeout,
                                void (*packetProcess) ( uint8_t* rxBuf, uint32_t rxInd, uint8_t* txBuf, uint32_t* txInd ),
                                void (*txTransmissionTrigger) ( uint8_t* txBuf, uint32_t txInd ) )
{
    uint32_t i = 0;
    
    // Function assignment.
    driver->packetProcess = packetProcess;
    driver->txTransmissionTrigger = txTransmissionTrigger;
    
    // Parameter settings.
    driver->rxBuffer = rxBuffer;
    driver->txBuffer = txBuffer;
    
    driver->rxSize = rxSize;
    driver->txSize = txSize;
    
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
 * @about: 
 */
void comatReceive ( comat_t* driver, uint8_t data )
{
    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( driver->rxIndex == 0 )
        {
            if ( data == 'A' )
            {
                driver->rxBuffer [ driver->rxIndex ] = data;
                ++driver->rxIndex;
            }
        }
        else if ( driver->rxIndex == 1 )
        {
            if ( data == 'T' )
            {
                driver->rxBuffer [ driver->rxIndex ] = data;
                ++driver->rxIndex;
            }
            else
            {
                // Terminate buffering
                driver->rxIndex = 0;
            }
        }
        else
        {
            driver->rxBuffer [ driver->rxIndex ] = data;
            ++driver->rxIndex;

            if ( ( driver->rxBuffer [ driver->rxIndex - 1 ] == '\n' ) &&
                ( driver->rxBuffer [ driver->rxIndex - 2 ] == '\r' ) )
            {
                driver->rxReadyToEvaluate = TRUE;
            }
            else if ( driver->rxIndex >= driver->rxSize )
            {
                // Terminate buffering
                driver->rxIndex = 0;
            }
        }
    }
}

/*
 * @about: 
 */
void comatEvaluate ( comat_t* driver )
{
    if ( driver->rxReadyToEvaluate = TRUE )
    {
        driver->packetProcess ( driver->rxBuffer, driver->rxIndex, driver->txBuffer, &driver->txIndex );
        driver->txTransmissionTrigger ( driver->txBuffer, driver->txIndex );
        
        driver->rxIndex = 0;
        driver->rxReadyToEvaluate = FALSE;
        driver->txIndex = 0;
    }
}

/*
 * @about: 
 */
void comatTimeoutCounter ( comat_t* driver )
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
