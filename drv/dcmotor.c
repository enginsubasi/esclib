/**
  ******************************************************************************
  *
  * @file:      dcmotor.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     23/05/2022
  * @history:   23/05/2022 Created
  *             29/07/2026 The file banner named the wrong file and device. It
  *                        was copied from the hc597 driver.
  *             29/07/2026 The pwm callback takes float instead of double, to
  *                        match the rest of the library. This changes the public
  *                        API.
  *
  * @about:     DC motor driver file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         dcMotorInit         :
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
void dcMotorInit ( struct dcmotor_t *driver,
                    void ( *bridgeHighFnc )( uint8_t ),
                    void ( *bridgeLowFnc )( uint8_t ),
                    void ( *pwmFnc )( float ))
{
    driver->bridgeHigh = bridgeHighFnc;
    driver->bridgeLow = bridgeLowFnc;
    driver->pwm = pwmFnc;

    driver->pwm ( ( float ) 0.0 );
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
