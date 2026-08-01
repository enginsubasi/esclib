/**
  ******************************************************************************
  *
  * @file      dcmotor.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.4
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
  * 01/08/2026 The driver struct is a typedef named after the module, @n
  *            the way every other module in the library declares it. @n
  *            Callers no longer write the struct keyword. @n
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 The pwm callback is given a float literal instead of a @n
  *            cast double zero. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "dcmotor.h"

/**
 * @brief   Initializes the DC motor driver and idles the bridge.
 * @param[out] driver         Driver state to initialize.
 * @param[in]  bridgeHighFnc  Drives the bridge high side pin.
 * @param[in]  bridgeLowFnc   Drives the bridge low side pin.
 * @param[in]  pwmFnc         Sets the motor drive PWM duty cycle.
 * @return  TRUE on success, FALSE when driver or any of the three callbacks
 *          is NULL.
 * @note    Sets the PWM duty cycle to zero and the bridge to BRIDGE_NO
 *          before returning.
 * @note    Every callback is required. This function calls all three before
 *          it returns, so a NULL here would surface as a crash rather than a
 *          status.
 */
uint8_t dcMotorInit ( dcmotor_t *driver,
                    void ( *bridgeHighFnc )( uint8_t ),
                    void ( *bridgeLowFnc )( uint8_t ),
                    void ( *pwmFnc )( float ))
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( bridgeHighFnc != NULL ) &&
            ( bridgeLowFnc != NULL ) && ( pwmFnc != NULL ) )
    {
        driver->bridgeHigh = bridgeHighFnc;
        driver->bridgeLow = bridgeLowFnc;
        driver->pwm = pwmFnc;

        driver->pwm ( 0.0f );
        dcMotorBridgeState ( driver, BRIDGE_NO );

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
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
void dcMotorBridgeState ( dcmotor_t *driver, uint8_t bridgeState )
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
