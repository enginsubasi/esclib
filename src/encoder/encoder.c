/**
  ******************************************************************************
  *
  * @file      encoder.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      06/08/2026
  *
  * @brief     Quadrature encoder decoding.
  *
  * @par Device
  * Generic
  *
  * @note      Counts at four times the cycle rate. The state machine sees all
  *            four transitions of a quadrature cycle anyway, so throwing
  *            three of them away would be a deliberate loss of resolution. A
  *            caller who wants the cycle count divides the position by four.
  *
  * @note      The feedback counterpart to dcMotor, and the natural consumer
  *            of two bininp outputs. It includes neither header: module
  *            independence forbids it, and the caller passes the two levels
  *            in from wherever it reads them.
  *
  * @par History
  * 06/08/2026 Created @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "encoder.h"

// Both channels differing between two samples. See encoderUpdate.
#define ENCODER_DIAGONAL    0x03u

/**
 * @brief   Initializes a quadrature decoder.
 * @param[out] driver    Decoder state to initialize.
 * @param[in]  channelA  Level channel A is sitting at right now.
 * @param[in]  channelB  Level channel B is sitting at right now.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    The two levels are not optional. They are the state the first
 *          encoderUpdate compares against, and starting from an assumed zero
 *          instead would make that first call read as a transition that never
 *          happened.
 * @note    Any non zero value means a high level. A real caller passes a
 *          masked register read rather than a clean one, so nothing here
 *          expects the value to be exactly one.
 * @note    The position starts at zero wherever the shaft happens to be.
 *          Homing is the caller's business; encoderSetPosition is how the
 *          answer gets installed.
 */
uint8_t encoderInit ( encoder_t* driver, uint8_t channelA, uint8_t channelB )
{
    uint8_t retVal = FALSE;
    uint8_t state = 0;

    if ( driver != NULL )
    {
        if ( channelA != 0 )
        {
            state = 2u;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( channelB != 0 )
        {
            state |= 1u;
        }
        else
        {
            /* Intentionally blank */
        }

        driver->state = state;
        driver->position = 0;
        driver->direction = ( int8_t ) ENC_STOPPED;
        driver->errorCount = 0;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Called with a fresh reading of both channels; advances the
 *          position by one count when the pair moved by one step.
 * @param[in,out] driver    Decoder state.
 * @param[in]     channelA  Level of channel A.
 * @param[in]     channelB  Level of channel B.
 * @note    The state is ( A << 1 ) | B, so the forward Gray sequence
 *          00 01 11 10 reads 0 1 3 2 and the table below is indexed by the
 *          old state and the new one together.
 * @note    A transition where both channels changed means a step was missed,
 *          either because the sampling is too slow for the shaft or because
 *          the line is noisy. Which way it went cannot be recovered, so the
 *          position is left alone and encoderGetErrorCount records it. A
 *          table answering plus or minus two there would look right on a
 *          clean signal and drift silently on a dirty one, which is the worst
 *          failure this module can have.
 * @note    The new state is adopted either way, so one missed step costs one
 *          count rather than desynchronising every sample after it.
 * @note    The direction is not cleared when the shaft stops. It reports
 *          which way the last counted step went, which is something the
 *          caller cannot reconstruct from the position if it reads more
 *          slowly than the shaft turns.
 */
void encoderUpdate ( encoder_t* driver, uint8_t channelA, uint8_t channelB )
{
    static const int8_t stepTable[ 16 ] =
    {
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0
    };

    uint8_t newState = 0;
    uint8_t oldState = 0;
    int8_t step = 0;

    if ( channelA != 0 )
    {
        newState = 2u;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( channelB != 0 )
    {
        newState |= 1u;
    }
    else
    {
        /* Intentionally blank */
    }

    oldState = driver->state;

    if ( ( uint8_t ) ( oldState ^ newState ) == ENCODER_DIAGONAL )
    {
        ++driver->errorCount;
    }
    else
    {
        step = stepTable[ ( oldState << 2 ) | newState ];

        if ( step != 0 )
        {
            driver->position += step;
            driver->direction = step;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    driver->state = newState;
}

/**
 * @brief   Returns the accumulated position.
 * @param[in]  driver  Initialized decoder.
 * @return  Counts since Init or since the last encoderSetPosition, four per
 *          quadrature cycle, negative when the shaft has turned backwards.
 * @note    Wraps at the ends of int32_t. At four counts per cycle and a
 *          thousand cycles per revolution that is over half a million
 *          revolutions, so a caller that can reach it knows it can.
 */
int32_t encoderGetPosition ( const encoder_t* const driver )
{
    int32_t retVal = 0;

    retVal = driver->position;

    return ( retVal );
}

/**
 * @brief   Installs a position, without disturbing the decoding.
 * @param[in,out] driver    Decoder state.
 * @param[in]     position  Count to continue from.
 * @note    This is how homing lands. The channel state is untouched, so the
 *          next encoderUpdate still sees the transition it would have seen.
 */
void encoderSetPosition ( encoder_t* driver, int32_t position )
{
    driver->position = position;
}

/**
 * @brief   Returns the direction of the last counted step.
 * @param[in]  driver  Initialized decoder.
 * @return  ENC_FORWARD, ENC_BACKWARD, or ENC_STOPPED when nothing has been
 *          counted yet.
 * @note    Not cleared when the shaft stops. Standing still leaves the last
 *          direction reported rather than replacing it with ENC_STOPPED,
 *          which only ever means nothing has moved since Init.
 */
int8_t encoderGetDirection ( const encoder_t* const driver )
{
    int8_t retVal = 0;

    retVal = driver->direction;

    return ( retVal );
}

/**
 * @brief   Returns the number of transitions that could not be decoded.
 * @param[in]  driver  Initialized decoder.
 * @return  How many times both channels changed between two samples.
 * @note    A non zero count means steps are being missed, and the position
 *          is short by at least that many. It is the signal to sample faster
 *          or to filter the lines, and it is why this module reports the
 *          problem rather than hiding it in a guess.
 */
uint32_t encoderGetErrorCount ( const encoder_t* const driver )
{
    uint32_t retVal = 0;

    retVal = driver->errorCount;

    return ( retVal );
}
