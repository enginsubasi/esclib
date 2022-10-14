/**
  ******************************************************************************
  *
  * @file:      generic.c
  * @author:    Name Surname
  * @email:     mail@mail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     20/02/2020
  * @history:   20/02/2020 Created
  *
  * @about:     Generic template file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         foo1            : Brief
  *         foo2            : Brief
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
                                void (*packetProcess) ( uint8_t* rxBuf, uint32_t* rxInd, uint8_t* txBuf, uint32_t* txInd ),
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

}

/*
 * @about: 
 */
void comatEvaluate ( comat_t* driver )
{

}

/*
 * @about: 
 */
void comatTimeoutCounter ( comat_t* driver )
{

}
