/**
  ******************************************************************************
  *
  * @file      dcmotor.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
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
  * 15/09/2026 dcMotorSetSpeed and dcMotorGetSpeed added. The pwm @n
  *            callback was required at Init, called once with zero @n
  *            and never again, so the driver could set a direction @n
  *            but not a speed. @n
  * 15/09/2026 A reversal between two driven directions sets the duty @n
  *            to zero first. Now that the driver owns the duty it @n
  *            also owns that hazard. @n
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

        /*
         * Both of these are set before the first dcMotorBridgeState call,
         * because that function reads them to decide whether the move it has
         * been asked for is a reversal.
         */
        driver->duty = 0.0f;
        driver->state = BRIDGE_NO;

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
 * @note    A move from one driven direction straight to the other sets the
 *          duty cycle to zero first, before any pin moves. A motor turning at
 *          speed is a generator, and reversing the bridge across it puts the
 *          supply voltage and the back emf in series through the winding. The
 *          driver owns the duty cycle now, so it owns that hazard rather than
 *          leaving it to a caller who has to remember every time.
 * @note    The duty stays at zero afterwards rather than being restored.
 *          Putting the previous duty back would command the previous torque
 *          into a reversal one pwm period later, which is the hazard again
 *          with a delay on it. The caller calls dcMotorSetSpeed when it has
 *          decided what the new direction should be doing, and
 *          dcMotorGetSpeed reports the zero in the meantime.
 * @note    Every other transition leaves the duty alone. Releasing or locking
 *          the bridge is not a reversal, and neither is picking a direction up
 *          from a released bridge.
 */
void dcMotorBridgeState ( dcmotor_t *driver, uint8_t bridgeState )
{
    uint8_t reversing = FALSE;

    if ( ( ( driver->state == BRIDGE_FORWARD ) && ( bridgeState == BRIDGE_BACKWARD ) ) ||
            ( ( driver->state == BRIDGE_BACKWARD ) && ( bridgeState == BRIDGE_FORWARD ) ) )
    {
        reversing = TRUE;
    }
    else
    {
        reversing = FALSE;
    }

    if ( reversing == TRUE )
    {
        driver->duty = 0.0f;
        driver->pwm ( 0.0f );
    }
    else
    {
        /* Intentionally blank */
    }

    driver->state = bridgeState;

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

/**
 * @brief   Sets the motor drive duty cycle.
 * @param[in,out] driver  Driver state.
 * @param[in]     duty    Fraction of full drive, zero to one. Values outside
 *                        that range are clamped rather than rejected.
 * @note    Clamped rather than rejected because there is nothing useful to
 *          report: a duty of 1.5 means full drive and a negative one means
 *          stopped, and a status the caller would have to check on every speed
 *          update would be checked by nobody. dcMotorGetSpeed reports what was
 *          actually installed.
 * @note    The clamp is written so that anything which is not above zero lands
 *          on zero, which puts a nan at a standstill rather than through to the
 *          pwm callback. A nan compares false against both bounds, so the
 *          obvious two sided clamp would pass one straight out to the hardware
 *          — the same way a nan passed through pidControl's output limiter
 *          before pidInit started rejecting a zero ts.
 * @note    This does not call mathClamp. No module in this library includes
 *          another's header, so the three lines are written out here.
 * @note    The duty is not tied to the bridge state. Setting a speed with the
 *          bridge released is allowed and does nothing until a direction is
 *          chosen, which is the order a caller starting up naturally uses.
 */
void dcMotorSetSpeed ( dcmotor_t *driver, float duty )
{
    float applied = 0.0f;

    if ( duty > 1.0f )
    {
        applied = 1.0f;
    }
    else if ( duty > 0.0f )
    {
        applied = duty;
    }
    else
    {
        applied = 0.0f;
    }

    driver->duty = applied;

    driver->pwm ( applied );
}

/**
 * @brief   Gets the duty cycle the driver last installed.
 * @param[in] driver  Driver state.
 * @return  Fraction of full drive, zero to one.
 * @note    This is what the driver put out, not what the caller asked for. A
 *          clamped request and a reversal that zeroed the duty both show up
 *          here.
 */
float dcMotorGetSpeed ( const dcmotor_t* const driver )
{
    return ( driver->duty );
}
