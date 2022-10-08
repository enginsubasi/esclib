/**
  ******************************************************************************
  *
  * @file:      hc597.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     23/05/2022
  * @history:   23/05/2022 Created
  *
  * @about:     HC597 driver file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         dcmotorInit         :
  *         dcMotorBridgeState  :
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "dcmotor.h"

/*
 * @about:
 */
void dcmotorInit ( struct dcmotor_t *driver,
                    void ( *bridgeHighFnc )( uint8_t ),
                    void ( *bridgeLowFnc )( uint8_t ),
                    void ( *pwmFnc )( double ))
{
    driver->bridgeHigh = bridgeHighFnc;
    driver->bridgeLow = bridgeLowFnc;
    driver->pwm = pwmFnc;

    driver->pwm ( ( double ) 0.0 );
    dcMotorBridgeState ( driver, BRIDGE_NO );
}

/*
 * @about:
 */
void dcMotorBridgeState ( struct dcmotor_t *driver, uint8_t bridgeState )
{
    switch ( bridgeState )
    {
        case BRIDGE_NO:
            driver->bridgeHigh ( FALSE );
            driver->bridgeLow ( FALSE );
        break;

        case BRIDGE_FORWARD:
            driver->bridgeHigh ( TRUE );
            driver->bridgeLow ( FALSE );
        break;

        case BRIDGE_BACKWARD:
            driver->bridgeHigh ( FALSE );
            driver->bridgeLow ( TRUE );
        break;

        case BRIDGE_LOCK:
            driver->bridgeHigh ( TRUE );
            driver->bridgeLow ( TRUE );
        break;

        default:
            driver->bridgeHigh ( FALSE );
            driver->bridgeLow ( FALSE );
        break;
    }
}
