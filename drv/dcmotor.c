/**
  ******************************************************************************
  *
  * @file      dcmotor.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
  * @date      23/05/2022
  *
  * @brief     DC motor driver file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 23/05/2022 Created @n
  * 29/07/2026 The file banner named the wrong file and device. It @n
  *            was copied from the hc597 driver. @n
  * 29/07/2026 The pwm callback takes float instead of double, to @n
  *            match the rest of the library. This changes the public @n
  *            API. @n
  *
  ******************************************************************************
  */

#include "dcmotor.h"

/**
 * @brief   Initializes the DC motor driver and idles the bridge.
 * @param[out] driver         Driver state to initialize.
 * @param[in]  bridgeHighFnc  Drives the bridge high side pin.
 * @param[in]  bridgeLowFnc   Drives the bridge low side pin.
 * @param[in]  pwmFnc         Sets the motor drive PWM duty cycle.
 * @note    Sets the PWM duty cycle to zero and the bridge to BRIDGE_NO
 *          before returning.
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

/**
 * @brief   Drives the bridge pins to match the requested state.
 * @param[in,out] driver       Driver state.
 * @param[in]     bridgeState  BRIDGE_NO to release the bridge, BRIDGE_FORWARD
 *                             or BRIDGE_BACKWARD to drive one direction each,
 *                             or BRIDGE_LOCK to drive both pins high. An
 *                             unrecognised value falls back to the BRIDGE_NO
 *                             behaviour.
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
