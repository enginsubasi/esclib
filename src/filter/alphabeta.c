/**
  ******************************************************************************
  *
  * @file      alphabeta.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      02/08/2026
  *
  * @brief     Alpha beta tracking filter.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
  *
  * @note      Estimates a position and a rate of change together, from a
  *            measurement of the position alone. This is the fixed gain
  *            simplification of a Kalman filter: no matrices, no division in
  *            the update, and two state variables. On a part without a divider
  *            or an FPU that difference decides whether the filter fits at all.
  *
  * @note      The rate is what the averaging filters cannot give. Differencing
  *            the output of a moving average to get one amplifies exactly the
  *            noise the average was there to remove; this estimates the rate
  *            directly and smooths it as part of the same update.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "alphabeta.h"

/**
 * @brief   Initializes the alpha beta tracking filter.
 * @param[out] driver        Filter state to initialize.
 * @param[in]  alpha         Correction applied to the position, in (0, 1].
 * @param[in]  beta          Correction applied to the velocity, in
 *                           (0, 4 - 2 * alpha].
 * @param[in]  dt            Time between calls to alphabetaIteration, in
 *                           whatever unit the velocity should come out in.
 * @param[in]  positionInit  Position the filter starts from.
 * @return  TRUE on success, FALSE when driver is NULL, dt is not greater than
 *          zero, or the gains fall outside the stable region.
 * @note    The stable region is 0 < alpha <= 1 and 0 < beta <= 4 - 2 * alpha.
 *          Outside it the estimate oscillates and grows instead of settling, so
 *          it is rejected here rather than left to be discovered on hardware.
 * @note    Larger gains track faster and filter less. A common starting point
 *          is alpha near 0.5 with beta near 0.1, then lower both until the
 *          output is quiet enough and the lag is still acceptable.
 * @note    dt sets the unit of the velocity. Pass the period in seconds and
 *          alphabetaGetVelocity returns units per second.
 * @note    The velocity starts at zero, so the first few updates lag a signal
 *          that is already moving.
 */
uint8_t alphabetaInit ( alphabeta_t* driver, float alpha, float beta, float dt, float positionInit )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( dt > 0 ) &&
            ( alpha > 0 ) && ( alpha <= 1.0f ) &&
            ( beta > 0 ) && ( beta <= ( 4.0f - ( 2.0f * alpha ) ) ) )
    {
        driver->alpha = alpha;
        driver->dt = dt;

        // Folded here so the update needs no division.
        driver->betaOverDt = beta / dt;

        driver->position = positionInit;
        driver->velocity = 0;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Feeds one position measurement into the filter and updates both
 *          the position and the velocity estimate.
 * @param[in,out] driver       Filter state.
 * @param[in]     measurement  Measured position.
 * @note    Call this at the fixed period given to alphabetaInit as dt. The
 *          filter has no clock of its own and assumes every call is one dt
 *          apart; an irregular period skews the velocity.
 */
void alphabetaIteration ( alphabeta_t* driver, float measurement )
{
    float predicted = 0;
    float residual = 0;

    // Where the previous estimate says we should be by now.
    predicted = driver->position + ( driver->velocity * driver->dt );

    // How wrong that was.
    residual = measurement - predicted;

    driver->position = predicted + ( driver->alpha * residual );
    driver->velocity = driver->velocity + ( driver->betaOverDt * residual );
}

/**
 * @brief   Gets the current position estimate.
 * @param[in] driver  Filter state.
 * @return  Filtered position.
 */
float alphabetaGetPosition ( const alphabeta_t* const driver )
{
    return ( driver->position );
}

/**
 * @brief   Gets the current velocity estimate.
 * @param[in] driver  Filter state.
 * @return  Rate of change of the position, in units per dt as given to
 *          alphabetaInit.
 */
float alphabetaGetVelocity ( const alphabeta_t* const driver )
{
    return ( driver->velocity );
}

/**
 * @brief   Extrapolates the position forward from the current estimate.
 * @param[in] driver  Filter state.
 * @param[in] ahead   How far ahead to project, in the same unit as dt.
 * @return  Position the filter expects after that interval.
 * @note    This is the reason to prefer this filter over a smoother when the
 *          consumer runs faster than the sensor, or when a control loop has to
 *          cover the transport delay of a slow measurement.
 * @note    A straight line projection. It holds while the velocity is roughly
 *          constant and degrades as soon as the signal accelerates.
 */
float alphabetaGetPrediction ( const alphabeta_t* const driver, float ahead )
{
    return ( driver->position + ( driver->velocity * ahead ) );
}
