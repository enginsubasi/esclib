/**
  ******************************************************************************
  *
  * @file      pid.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.3.0
  * @date      23/07/2020
  *
  * @brief     PID control.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 23/07/2020 Created. @n
  * 24/08/2020 Data type changed from double to float. @n
  * 29/07/2026 Bug fix. pidInit left error, lastError, partP, partI @n
  *            and partD uninitialized. @n
  * 01/08/2026 Init reports its outcome as a uint8_t status instead of @n
  *            returning void, and validates its arguments. The @n
  *            library used three different conventions for this. @n
  * 01/08/2026 pidInit and pidChangeCoefficients reject a zero ts. @n
  *            pidControl divides the error difference by ts, and a @n
  *            zero there produced a nan that passed straight through @n
  *            the output limiter, since nan compares false against @n
  *            both bounds. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 01/08/2026 The accessors that only read take a const driver. @n
  * 06/08/2026 The Q16 fixed point variant is added, for parts with @n
  *            no FPU. It carries no ts: the integral is a running @n
  *            sum of errors and the derivative a plain difference, @n
  *            so the caller folds the period into the gains. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "pid.h"

/**
 * @brief   Initializes the PID controller state.
 * @param[out] driver             Controller state to initialize.
 * @param[in]  kp                 Proportional gain.
 * @param[in]  ki                 Integral gain.
 * @param[in]  kd                 Derivative gain.
 * @param[in]  ts                 Sample time used by the integral and derivative terms.
 * @param[in]  pPartMaxLimit      Upper clamp for partP, in error units, applied before the kp multiply.
 * @param[in]  pPartMinLimit      Lower clamp for partP, in error units, applied before the kp multiply.
 * @param[in]  iPartMaxLimit      Upper clamp for partI, in error units, applied before the ki multiply.
 * @param[in]  iPartMinLimit      Lower clamp for partI, in error units, applied before the ki multiply.
 * @param[in]  dPartMaxLimit      Upper clamp for partD, in error units, applied before the kd multiply.
 * @param[in]  dPartMinLimit      Lower clamp for partD, in error units, applied before the kd multiply.
 * @param[in]  pidOutputMaxLimit  Upper clamp for the controller output.
 * @param[in]  pidOutputMinLimit  Lower clamp for the controller output; also the initial output value.
 * @return  TRUE on success, FALSE when driver is NULL or ts is zero.
 * @note    ts is rejected when zero because pidControl divides the error
 *          difference by it. Nothing else in the module guards that divide,
 *          so this check and the one in pidChangeCoefficients are what keep
 *          the derivative term finite.
 */
uint8_t pidInit ( pidc_t* driver, float kp, float ki, float kd, float ts, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( ts != 0 ) )
    {
        driver->output = pidOutputMinLimit;

        // Error memory. Without this the first derivative term would use garbage.
        driver->error = 0;
        driver->lastError = 0;

        // Term accumulators. partI integrates, so it must start from a known value.
        driver->partP = 0;
        driver->partI = 0;
        driver->partD = 0;

        // Coefficients.
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;

        driver->ts = ts;

        // Limits of proportional part.
        driver->pMax = pPartMaxLimit;
        driver->pMin = pPartMinLimit;

        // Limits of integral part.
        driver->iMax = iPartMaxLimit;
        driver->iMin = iPartMinLimit;

        // Limits of derivative part.
        driver->dMax = dPartMaxLimit;
        driver->dMin = dPartMinLimit;

        // Limits of PID output value.
        driver->pidMax = pidOutputMaxLimit;
        driver->pidMin = pidOutputMinLimit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Changes the PID controller's gains and sample time.
 * @param[in,out] driver  Controller state.
 * @param[in]     kp      New proportional gain.
 * @param[in]     ki      New integral gain.
 * @param[in]     kd      New derivative gain.
 * @param[in]     ts      New sample time used by the integral and derivative terms.
 * @return  TRUE on success, FALSE when driver is NULL or ts is zero.
 * @note    On FALSE nothing is written, so the controller keeps the gains
 *          and the sample time it already had.
 */
uint8_t pidChangeCoefficients ( pidc_t* driver, float kp, float ki, float kd, float ts )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( ts != 0 ) )
    {
        // Coefficients.
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;

        driver->ts = ts;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Changes the PID controller's proportional, integral, derivative and output clamp limits.
 * @param[in,out] driver             Controller state.
 * @param[in]     pPartMaxLimit      New upper clamp for partP, in error units, applied before the kp multiply.
 * @param[in]     pPartMinLimit      New lower clamp for partP, in error units, applied before the kp multiply.
 * @param[in]     iPartMaxLimit      New upper clamp for partI, in error units, applied before the ki multiply.
 * @param[in]     iPartMinLimit      New lower clamp for partI, in error units, applied before the ki multiply.
 * @param[in]     dPartMaxLimit      New upper clamp for partD, in error units, applied before the kd multiply.
 * @param[in]     dPartMinLimit      New lower clamp for partD, in error units, applied before the kd multiply.
 * @param[in]     pidOutputMaxLimit  New upper clamp for the controller output.
 * @param[in]     pidOutputMinLimit  New lower clamp for the controller output.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    On FALSE nothing is written, so the controller keeps the limits
 *          it already had.
 */
uint8_t pidChangeLimits ( pidc_t* driver, float pPartMaxLimit, float pPartMinLimit, float iPartMaxLimit, float iPartMinLimit,
                        float dPartMaxLimit, float dPartMinLimit, float pidOutputMaxLimit, float pidOutputMinLimit )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        // Limits of proportional part.
        driver->pMax = pPartMaxLimit;
        driver->pMin = pPartMinLimit;

        // Limits of integral part.
        driver->iMax = iPartMaxLimit;
        driver->iMin = iPartMinLimit;

        // Limits of derivative part.
        driver->dMax = dPartMaxLimit;
        driver->dMin = dPartMinLimit;

        // Limits of PID output value.
        driver->pidMax = pidOutputMaxLimit;
        driver->pidMin = pidOutputMinLimit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one PID control iteration for the given error signal.
 * @param[in,out] driver  Controller state.
 * @param[in]     error   Error signal for this iteration, i.e. setpoint minus
 *                        measurement, not the raw measurement itself.
 * @note    The result is stored in driver and read back with pidGetOutput;
 *          this function does not return it directly.
 * @note    The derivative term divides by driver->ts. That divide is safe
 *          because pidInit and pidChangeCoefficients both refuse a zero ts,
 *          so ts cannot be zero on a controller that was initialized
 *          successfully.
 */
void pidControl ( pidc_t* driver, float error )
{
    driver->error = error;

    // Calculate proportional part
    driver->partP = driver->error;

    // Control proportional range
    if ( driver->partP > driver->pMax )
    {
        driver->partP = driver->pMax;
    }
    else if ( driver->partP < driver->pMin )
    {
        driver->partP = driver->pMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate integral part
    driver->partI += ( driver->error * driver->ts );

    // Control integral range
    if ( driver->partI > driver->iMax )
    {
        driver->partI = driver->iMax;
    }
    else if ( driver->partI < driver->iMin )
    {
        driver->partI = driver->iMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate derivative part
    driver->partD = ( ( driver->error - driver->lastError ) / driver->ts );
    
    // Control derivative range
    if ( driver->partD > driver->dMax )
    {
        driver->partD = driver->dMax;
    }
    else if ( driver->partD < driver->dMin )
    {
        driver->partD = driver->dMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate PID output value
    driver->output = ( driver->kp * driver->partP ) +
                        ( driver->ki * driver->partI ) +
                        ( driver->kd * driver->partD );

    // Control PID range
    if ( driver->output > driver->pidMax )
    {
        driver->output = driver->pidMax;
    }
    else if ( driver->output < driver->pidMin )
    {
        driver->output = driver->pidMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Save current error for next iteration over lastError
    driver->lastError = driver->error;
}

/**
 * @brief   Gets the most recently computed PID controller output.
 * @param[in] driver  Controller state.
 * @return  Current PID output value, already clamped to the configured limits.
 */
float pidGetOutput ( const pidc_t* const driver )
{
    return ( driver->output );
}



/*
 * Q16 fixed point. A gain of 1.0 is 65536.
 *
 * There is no ts. The float variant multiplies the error by ts to integrate
 * and divides by ts to differentiate; this one works per sample, so the
 * integral is a running sum of errors and the derivative is a plain
 * difference. The caller folds the period into the gains once, when it
 * converts continuous gains to this form: ki here is the float ki times ts,
 * and kd here is the float kd divided by ts. alphabetaIniti32 and rampIniti32
 * drop their periods the same way and for the same reason, that a floating
 * point period would put back the arithmetic this variant exists to avoid.
 */
#define PID_Q       16

/**
 * @brief   Initializes a fixed point PID controller.
 * @param[out] driver             Controller state to initialize.
 * @param[in]  kp                 Proportional gain in Q16, so 65536 is 1.0.
 * @param[in]  ki                 Integral gain in Q16, already multiplied by
 *                                the sample period.
 * @param[in]  kd                 Derivative gain in Q16, already divided by
 *                                the sample period.
 * @param[in]  pPartMaxLimit      Upper limit of the proportional term.
 * @param[in]  pPartMinLimit      Lower limit of the proportional term.
 * @param[in]  iPartMaxLimit      Upper limit of the integral accumulator.
 * @param[in]  iPartMinLimit      Lower limit of the integral accumulator.
 * @param[in]  dPartMaxLimit      Upper limit of the derivative term.
 * @param[in]  dPartMinLimit      Lower limit of the derivative term.
 * @param[in]  pidOutputMaxLimit  Upper limit of the output.
 * @param[in]  pidOutputMinLimit  Lower limit of the output.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    Only the pointer is checked, and that is deliberate. pidInit's
 *          other guard rejects a ts of zero because pidControl divides by it;
 *          there is no ts here and pidControli32 never divides, so the guard
 *          has nothing left to check. Adding a limit ordering test that the
 *          float variant does not apply would make the two widths disagree
 *          about what a valid controller is.
 * @note    The three term limits are in error units rather than output units.
 *          They bound each part before its gain is applied, which is where
 *          the float variant clamps too.
 * @note    lastError and partI are cleared here. Leaving them unset was a
 *          real defect in the float variant, fixed in July 2026, and the same
 *          clearing matters just as much in this one.
 */
uint8_t pidIniti32 ( pidci32_t* driver, int32_t kp, int32_t ki, int32_t kd, int32_t pPartMaxLimit, int32_t pPartMinLimit, int32_t iPartMaxLimit, int32_t iPartMinLimit,
                int32_t dPartMaxLimit, int32_t dPartMinLimit, int32_t pidOutputMaxLimit, int32_t pidOutputMinLimit )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        driver->output = pidOutputMinLimit;

        // Error memory. Without this the first derivative term would use garbage.
        driver->error = 0;
        driver->lastError = 0;

        // Term accumulators. partI integrates, so it must start from a known value.
        driver->partP = 0;
        driver->partI = 0;
        driver->partD = 0;

        // Coefficients, in Q16.
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;

        // Limits of proportional part.
        driver->pMax = pPartMaxLimit;
        driver->pMin = pPartMinLimit;

        // Limits of integral part.
        driver->iMax = iPartMaxLimit;
        driver->iMin = iPartMinLimit;

        // Limits of derivative part.
        driver->dMax = dPartMaxLimit;
        driver->dMin = dPartMinLimit;

        // Limits of PID output value.
        driver->pidMax = pidOutputMaxLimit;
        driver->pidMin = pidOutputMinLimit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Installs new fixed point gains without disturbing the state.
 * @param[in,out] driver  Controller state.
 * @param[in]     kp      Proportional gain in Q16.
 * @param[in]     ki      Integral gain in Q16.
 * @param[in]     kd      Derivative gain in Q16.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    The integral accumulator is left alone, so a gain change does not
 *          discard what has been integrated so far. pidChangeCoefficients
 *          makes the same choice.
 * @note    This takes no period, unlike its float counterpart, so it cannot
 *          break the invariant that function returns a status to guard. The
 *          status is here because the driver pointer can still be NULL,
 *          which is pidChangeLimits' reason for carrying one.
 */
uint8_t pidChangeCoefficientsi32 ( pidci32_t* driver, int32_t kp, int32_t ki, int32_t kd )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        driver->kp = kp;
        driver->ki = ki;
        driver->kd = kd;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Installs new fixed point limits without disturbing the state.
 * @param[in,out] driver             Controller state.
 * @param[in]     pPartMaxLimit      Upper limit of the proportional term.
 * @param[in]     pPartMinLimit      Lower limit of the proportional term.
 * @param[in]     iPartMaxLimit      Upper limit of the integral accumulator.
 * @param[in]     iPartMinLimit      Lower limit of the integral accumulator.
 * @param[in]     dPartMaxLimit      Upper limit of the derivative term.
 * @param[in]     dPartMinLimit      Lower limit of the derivative term.
 * @param[in]     pidOutputMaxLimit  Upper limit of the output.
 * @param[in]     pidOutputMinLimit  Lower limit of the output.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    A tightened integral limit does not take effect until the next
 *          pidControli32, which is when the accumulator is clamped again.
 */
uint8_t pidChangeLimitsi32 ( pidci32_t* driver, int32_t pPartMaxLimit, int32_t pPartMinLimit, int32_t iPartMaxLimit, int32_t iPartMinLimit,
                        int32_t dPartMaxLimit, int32_t dPartMinLimit, int32_t pidOutputMaxLimit, int32_t pidOutputMinLimit )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        driver->pMax = pPartMaxLimit;
        driver->pMin = pPartMinLimit;

        driver->iMax = iPartMaxLimit;
        driver->iMin = iPartMinLimit;

        driver->dMax = dPartMaxLimit;
        driver->dMin = dPartMinLimit;

        driver->pidMax = pidOutputMaxLimit;
        driver->pidMin = pidOutputMinLimit;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Runs one fixed point control iteration for the given error.
 * @param[in,out] driver  Controller state.
 * @param[in]     error   Setpoint minus measurement, in the caller's units.
 * @note    Each of the three terms is limited on its own before the gains are
 *          applied, and the sum is limited again. Clamping the integral
 *          accumulator rather than the term after its gain is what makes that
 *          limit an anti windup bound: the accumulator itself cannot run
 *          away, so the controller recovers as soon as the error changes
 *          sign. The float variant clamps in the same place.
 * @note    partI is int64_t, and the reason is narrower than it looks. The
 *          clamp below runs every call and iPartMaxLimit is an int32_t, so
 *          the accumulator can never end a call outside int32 range. What it
 *          can do is leave that range for the moment between the addition and
 *          the clamp: with a limit near the top of int32, partI plus one more
 *          error overflows before anything bounds it, and an int32_t
 *          accumulator would wrap to a large negative value and hand the
 *          clamp a number from the wrong end.
 * @note    Each gain product is formed in int64_t before it is shifted back
 *          down. A gain of ten against an error of ten thousand already
 *          exceeds thirty two bits once the gain is in Q16.
 * @note    The shift is an arithmetic one on a signed value, which the C
 *          standard leaves to the implementation and every compiler this
 *          library targets defines. emafIterationi32 relies on it too.
 * @note    Nothing here divides, which is the whole point of dropping ts.
 */
void pidControli32 ( pidci32_t* driver, int32_t error )
{
    int64_t sum = 0;

    driver->error = error;

    // Calculate proportional part.
    driver->partP = driver->error;

    // Control proportional range.
    if ( driver->partP > driver->pMax )
    {
        driver->partP = driver->pMax;
    }
    else if ( driver->partP < driver->pMin )
    {
        driver->partP = driver->pMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate integral part. One call is one period, so there is no ts.
    driver->partI += ( int64_t ) driver->error;

    // Control integral range.
    if ( driver->partI > ( int64_t ) driver->iMax )
    {
        driver->partI = ( int64_t ) driver->iMax;
    }
    else if ( driver->partI < ( int64_t ) driver->iMin )
    {
        driver->partI = ( int64_t ) driver->iMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate derivative part. Again one call is one period.
    driver->partD = driver->error - driver->lastError;

    // Control derivative range.
    if ( driver->partD > driver->dMax )
    {
        driver->partD = driver->dMax;
    }
    else if ( driver->partD < driver->dMin )
    {
        driver->partD = driver->dMin;
    }
    else
    {
        /* Intentionally blank. */
    }

    // Calculate PID output value.
    sum = ( ( ( int64_t ) driver->kp ) * ( ( int64_t ) driver->partP ) ) +
          ( ( ( int64_t ) driver->ki ) * driver->partI ) +
          ( ( ( int64_t ) driver->kd ) * ( ( int64_t ) driver->partD ) );

    sum = sum >> PID_Q;

    // Control PID range.
    if ( sum > ( int64_t ) driver->pidMax )
    {
        driver->output = driver->pidMax;
    }
    else if ( sum < ( int64_t ) driver->pidMin )
    {
        driver->output = driver->pidMin;
    }
    else
    {
        driver->output = ( int32_t ) sum;
    }

    // Save current error for next iteration over lastError.
    driver->lastError = driver->error;
}

/**
 * @brief   Gets the current output of the fixed point controller.
 * @param[in] driver  Controller state.
 * @return  The limited controller output, in the caller's output units.
 */
int32_t pidGetOutputi32 ( const pidci32_t* const driver )
{
    int32_t retVal = 0;

    retVal = driver->output;

    return ( retVal );
}
