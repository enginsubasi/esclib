/**
  ******************************************************************************
  *
  * @file      alphabeta.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
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
  * 06/08/2026 The Q16 fixed point variant is added, for parts with @n
  *            no FPU. It carries the velocity in units per sample @n
  *            rather than per second, which takes dt out of the @n
  *            update entirely. @n
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

/*
 * Q16 fixed point. A coefficient of 1.0 is 65536, and a position of 1.0 unit
 * is 65536 in the internal state. The library's other FPU free variant,
 * emafi32, restricts its coefficient to a power of two and uses a shift; that
 * is cheaper still but cannot express an alpha of 0.3, which an alpha beta
 * filter routinely needs.
 */
#define ALPHABETA_Q          16
#define ALPHABETA_ONE        65536

/**
 * @brief   Initializes a fixed point alpha beta filter.
 * @param[out] driver        Filter state to initialize.
 * @param[in]  alpha         Position gain in Q16, so 65536 is 1.0. Must be
 *                           above zero and no greater than 1.0.
 * @param[in]  beta          Velocity gain in Q16. Must be above zero and no
 *                           greater than 4.0 - 2.0 * alpha, which is the
 *                           stability bound the float variant applies too.
 * @param[in]  positionInit  Starting position, in plain measurement units
 *                           rather than Q16.
 * @return  TRUE on success, FALSE when driver is NULL or either gain is
 *          outside its range.
 * @note    There is no dt. The float variant folds beta over dt and
 *          multiplies the velocity by dt on every iteration; this one carries
 *          the velocity in units per sample instead, which removes dt from
 *          the update entirely. Passing a period as a float would put back
 *          exactly the arithmetic this variant exists to avoid.
 * @note    The velocity therefore reads in units per sample, not per second.
 *          A caller who wants per second multiplies by the sample rate, and
 *          that conversion is the caller's because only the caller knows it
 *          in integers.
 */
uint8_t alphabetaIniti32 ( alphabetai32_t* driver, int32_t alpha, int32_t beta, int32_t positionInit )
{
    uint8_t retVal = FALSE;
    int32_t betaLimit = 0;

    // 4.0 - 2.0 * alpha, in Q16.
    betaLimit = ( 4 * ALPHABETA_ONE ) - ( 2 * alpha );

    if ( ( driver != NULL ) &&
            ( alpha > 0 ) && ( alpha <= ALPHABETA_ONE ) &&
            ( beta > 0 ) && ( beta <= betaLimit ) )
    {
        driver->alpha = alpha;
        driver->beta = beta;

        driver->position = ( ( int64_t ) positionInit ) << ALPHABETA_Q;
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
 * @brief   Feeds one measurement into the fixed point filter and updates both
 *          the position and the velocity estimate.
 * @param[in,out] driver       Filter state.
 * @param[in]     measurement  Measured position, in plain units.
 * @note    Call this at a fixed period. The filter has no clock and assumes
 *          every call is one sample apart; an irregular period skews the
 *          velocity, exactly as it does in the float variant.
 * @note    The position and velocity are held in int64_t so that Q16 does not
 *          cost sixteen bits of usable range. An encoder counting into the
 *          millions would overflow a Q16 int32_t at about 32767 counts.
 * @note    Each product is int64_t before it is shifted back down. A gain of
 *          1.0 against a residual of a few thousand units already needs more
 *          than thirty two bits once both are in Q16.
 * @note    The shift is an arithmetic one on a signed value, which the C
 *          standard leaves to the implementation but every compiler this
 *          library targets defines. emafIterationi32 relies on the same
 *          thing, so the assumption is not new here.
 */
void alphabetaIterationi32 ( alphabetai32_t* driver, int32_t measurement )
{
    int64_t predicted = 0;
    int64_t residual = 0;

    // Where the previous estimate says we should be by now. The velocity is
    // already per sample, so there is no period to multiply by.
    predicted = driver->position + driver->velocity;

    // How wrong that was.
    residual = ( ( ( int64_t ) measurement ) << ALPHABETA_Q ) - predicted;

    driver->position = predicted + ( ( driver->alpha * residual ) >> ALPHABETA_Q );
    driver->velocity = driver->velocity + ( ( driver->beta * residual ) >> ALPHABETA_Q );
}

/**
 * @brief   Gets the current position estimate.
 * @param[in] driver  Filter state.
 * @return  Filtered position in plain units, with the Q16 fraction dropped.
 */
int32_t alphabetaGetPositioni32 ( const alphabetai32_t* const driver )
{
    int32_t retVal = 0;

    retVal = ( int32_t ) ( driver->position >> ALPHABETA_Q );

    return ( retVal );
}

/**
 * @brief   Gets the current velocity estimate.
 * @param[in] driver  Filter state.
 * @return  Rate of change in Q16 units per sample, so 65536 is one unit per
 *          sample. Saturates at the ends of int32_t.
 * @note    Q16 rather than plain units because a filter tracking a slow
 *          signal spends most of its time below one unit per sample, and
 *          truncating that to an integer would report a standing zero.
 */
int32_t alphabetaGetVelocityi32 ( const alphabetai32_t* const driver )
{
    int32_t retVal = 0;

    if ( driver->velocity > ( int64_t ) 2147483647 )
    {
        retVal = 2147483647;
    }
    else if ( driver->velocity < ( int64_t ) ( -2147483647 - 1 ) )
    {
        retVal = ( -2147483647 - 1 );
    }
    else
    {
        retVal = ( int32_t ) driver->velocity;
    }

    return ( retVal );
}

/**
 * @brief   Extrapolates the position forward from the current estimate.
 * @param[in] driver  Filter state.
 * @param[in] ahead   How far ahead to project, in samples.
 * @return  Position the filter expects after that many samples, in plain
 *          units.
 * @note    A straight line projection, like the float variant. It holds while
 *          the velocity is roughly constant and degrades as soon as the
 *          signal accelerates.
 * @note    ahead counts samples rather than seconds, for the same reason the
 *          velocity is per sample: there is no period in this variant to
 *          measure seconds against.
 */
int32_t alphabetaGetPredictioni32 ( const alphabetai32_t* const driver, int32_t ahead )
{
    int32_t retVal = 0;
    int64_t projected = 0;

    projected = driver->position + ( driver->velocity * ( int64_t ) ahead );

    retVal = ( int32_t ) ( projected >> ALPHABETA_Q );

    return ( retVal );
}
