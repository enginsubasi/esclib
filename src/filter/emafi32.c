/**
  ******************************************************************************
  *
  * @file      emafi32.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      02/08/2026
  *
  * @brief     Exponential moving average filter, integer only.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
  *
  * @note      Same filter as emaf, reachable on a part with no floating point
  *            unit. The smoothing factor is fixed at 1 / 2^shift so the whole
  *            update is one subtraction, one addition and two shifts. No
  *            multiply, no divide, no float anywhere.
  *
  * @note      Deliberately a separate module rather than another variant inside
  *            emaf.c. Modules here are consumed by copying a source pair into a
  *            project, and a project without an FPU should not have to take the
  *            float code along to get this. The parameterisation differs too:
  *            emaf takes an arbitrary alpha, this takes a shift count.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "emafi32.h"

/**
 * @brief   Initializes the integer exponential moving average filter.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  shift       Smoothing factor, as alpha = 1 / 2^shift. Must be
 *                         between 1 and 30.
 * @param[in]  outputInit  Value the filter starts from.
 * @return  TRUE on success, FALSE when driver is NULL, shift is outside
 *          1 to 30, or outputInit is too large for the chosen shift.
 * @note    A larger shift filters harder and responds more slowly. shift of 1
 *          is alpha of 0.5, shift of 4 is 0.0625, shift of 8 is about 0.004.
 * @note    Every input must satisfy the same bound as outputInit, which is
 *          INT32_MAX >> shift, and nothing checks it on the iteration path.
 *          The accumulator holds the output scaled up by 2^shift, so an input
 *          past that bound overflows it. With shift of 4 the limit is about
 *          134 million, with shift of 8 about 8.4 million. A converter reading
 *          is nowhere near either; a raw 32-bit counter is.
 * @note    A shift of zero is rejected. It would leave the filter a wire, and
 *          it is also the one setting where the subtraction in the update could
 *          overflow.
 */
uint8_t emafi32Init ( emafi32_t* driver, uint8_t shift, int32_t outputInit )
{
    uint8_t retVal = FALSE;
    int32_t bound = 0;

    if ( ( driver != NULL ) && ( shift >= 1u ) && ( shift <= 30u ) )
    {
        bound = INT32_MAX >> shift;

        if ( ( outputInit <= bound ) && ( outputInit >= -bound ) )
        {
            driver->shift = shift;
            driver->output = outputInit;

            /*
             * The accumulator carries the output scaled up by 2^shift, which is
             * where the fraction lives. Built with a multiply rather than a left
             * shift: shifting a negative value left is undefined in C, and
             * outputInit is signed.
             */
            driver->accumulator = outputInit * ( int32_t ) ( 1u << shift );

            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds a new sample to the integer filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample. Must satisfy the bound documented on
 *                         emafi32Init.
 * @note    The fraction is kept in the accumulator and only the shifted copy is
 *          published, so a sample that moves the output by less than one count
 *          still moves the accumulator and is not lost.
 * @note    Relies on the right shift of a negative value being arithmetic. The
 *          standard leaves that implementation defined, but every compiler this
 *          library targets does it, and the alternative, dividing by 2^shift,
 *          rounds towards zero and would bias the filter upward on negative
 *          signals.
 */
void emafi32Iteration ( emafi32_t* driver, int32_t newData )
{
    driver->accumulator += ( newData - ( driver->accumulator >> driver->shift ) );
    driver->output = driver->accumulator >> driver->shift;
}

/**
 * @brief   Gets the current output of the integer filter.
 * @param[in] driver  Filter state.
 * @return  Current filtered value.
 */
int32_t emafi32GetOutput ( const emafi32_t* const driver )
{
    return ( driver->output );
}
