/**
  ******************************************************************************
  *
  * @file      q16.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.1
  * @date      15/09/2026
  *
  * @brief     Q16 fixed point arithmetic for the caller.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  * 15/09/2026 The Q16 scaling is a multiply rather than a left @n
  *            shift. Shifting a negative signed value left is @n
  *            undefined in C, and every one of these took one @n
  *            while giving the right answer. UBSan found them. @n
  *
  * @note      Five modules in this library carry a Q16 fixed point variant —
  *            pid, ramp, alphabeta, biquad and mathLerpi32 — and by the module
  *            independence rule not one of them may include this header. This
  *            module is not for them. It is for the **caller**, who now has
  *            gains, limits and coefficients in Q16 and, until now, nothing at
  *            all to do arithmetic on them with.
  *
  * @note      Five functions, and each is here because the hand written version
  *            gets one specific thing wrong. A multiply needs an int64_t
  *            intermediate, because two Q16 values make a Q32 product and a
  *            gain of 2.0 against a value of 40000 already overflows thirty two
  *            bits. A divide needs the shift applied *before* the division, not
  *            after, or every result below one comes out as zero. A conversion
  *            back to a plain integer has to round rather than shift, or a
  *            value walked in steps drifts steadily low. And there is no square
  *            root at all without a float.
  *
  * @note      Everything saturates rather than wrapping. A wrapped Q16 value
  *            flips sign, and in the control loop these numbers are headed for
  *            that means full reverse torque from a small overshoot — the worst
  *            failure available. Saturating is what alphabetaGetVelocityi32
  *            already does at the ends of its range, for the same reason.
  *
  * @note      **No float appears anywhere here, deliberately.** The whole point
  *            is a part with no FPU, and a float function in this file would
  *            risk linking the software float library into a program that never
  *            calls it. Converting a designed constant is a compile time
  *            expression at the call site, which is what biquad.c and fir.c
  *            already tell their callers to write:
  *
  *                ( int32_t ) ( gain * 65536.0f + ( gain >= 0 ? 0.5f : -0.5f ) )
  *
  * @note      **What this costs instead.** The point of Q16 is to avoid the
  *            software float routines, and it does — but not for free. On a
  *            Cortex-M0 this module links __aeabi_lmul and __aeabi_ldivmod,
  *            because the part has neither a 64-bit multiply nor any divide
  *            instruction at all. A Q16 divide is therefore a runtime call of
  *            the same order as the float one it replaced. The multiply is
  *            much cheaper, and everything else here is shifts and compares.
  *            So the honest summary is that q16Mul, q16Sqrt and the two
  *            conversions win comfortably, while q16Div wins mostly on code
  *            size rather than on time. scripts/runtime.sh prints the helpers
  *            every module needs; the same is true of interp, biquad and the
  *            other fixed point variants, which was measured rather than
  *            assumed.
  *
  * @note      There is no q16Clamp and no q16Abs. A Q16 value is an int32_t,
  *            so mathClampi32 and mathAbsolutei32 already do exactly the right
  *            thing to one, and a renamed copy of either would be symmetry for
  *            its own sake. There is no q16Lerp either: mathLerpi32 already
  *            takes its interpolation fraction in Q16.
  *
  * @note      The integer square root below is the one rampIteration i32 uses,
  *            duplicated rather than shared because no module here includes
  *            another's header. interp.c carries a copy of searchUpperBound's
  *            bracketing loop for the same reason and says so in the same way.
  *
  ******************************************************************************
  */

#include "q16.h"

/* Half of one, for rounding to nearest. */
#define Q16_HALF        32768

#define Q16_INT_MAX     2147483647
#define Q16_INT_MIN     ( -2147483647 - 1 )

/**
 * @brief   Brings a 64-bit result back into int32_t, saturating at both ends.
 * @param[in] value  Result to narrow.
 * @return  value, or the nearest end of int32_t when it does not fit.
 * @note    Saturating rather than wrapping. A wrapped result changes sign,
 *          which for a value about to become a motor command or a controller
 *          output is the worst possible answer; the largest representable one
 *          is at least in the right direction.
 */
static int32_t q16Saturate ( int64_t value )
{
    int32_t retVal = 0;

    if ( value > ( int64_t ) Q16_INT_MAX )
    {
        retVal = Q16_INT_MAX;
    }
    else if ( value < ( int64_t ) Q16_INT_MIN )
    {
        retVal = Q16_INT_MIN;
    }
    else
    {
        retVal = ( int32_t ) value;
    }

    return ( retVal );
}

/**
 * @brief   Rounds a Q32 or Q16 quantity down by one scale, to nearest.
 * @param[in] value  Quantity to shift down by Q16_SHIFT.
 * @return  The same quantity one scale down, rounded away from zero on a half.
 * @note    The negative branch takes the magnitude first. Adding half and then
 *          shifting an arithmetic right would round a negative value the wrong
 *          way, because that shift floors toward minus infinity. This is the
 *          same form biquadIterationi32 and firIterationi32 use.
 */
static int64_t q16ShiftRound ( int64_t value )
{
    int64_t retVal = 0;

    if ( value >= 0 )
    {
        retVal = ( value + Q16_HALF ) >> Q16_SHIFT;
    }
    else
    {
        retVal = - ( ( -value + Q16_HALF ) >> Q16_SHIFT );
    }

    return ( retVal );
}

/**
 * @brief   Integer square root of a 64-bit value.
 * @param[in] value  Value to take the root of.
 * @return  The largest integer whose square does not exceed value.
 * @note    The bit by bit method, which needs no division and no floating
 *          point. This is rampSquareRoot, duplicated because no module in this
 *          library includes another's header.
 */
static uint64_t q16SquareRoot ( uint64_t value )
{
    uint64_t retVal = 0;
    uint64_t rest = 0;
    uint64_t bit = 0;

    rest = value;
    bit = ( ( uint64_t ) 1 ) << 62;

    while ( bit > rest )
    {
        bit >>= 2;
    }

    while ( bit != 0 )
    {
        if ( rest >= ( retVal + bit ) )
        {
            rest = rest - ( retVal + bit );
            retVal = ( retVal >> 1 ) + bit;
        }
        else
        {
            retVal = retVal >> 1;
        }

        bit >>= 2;
    }

    return ( retVal );
}

/**
 * @brief   Converts a plain integer to Q16.
 * @param[in] value  Plain integer.
 * @return  The same number in Q16, saturated when it will not fit.
 * @note    Q16 in an int32_t reaches a little past plus or minus 32767, so
 *          anything outside that saturates. That bound is the price of the
 *          sixteen fractional bits and is worth knowing before choosing this
 *          scale for a quantity that is naturally large — a raw converter
 *          count usually is, which is why the fixed point modules in this
 *          library keep their *signals* in plain units and scale only their
 *          coefficients.
 */
int32_t q16FromInt ( int32_t value )
{
    int32_t retVal = 0;

    retVal = q16Saturate ( ( ( int64_t ) value ) * Q16_ONE );

    return ( retVal );
}

/**
 * @brief   Converts a Q16 value back to a plain integer.
 * @param[in] value  Q16 value.
 * @return  The nearest plain integer.
 * @note    Rounds rather than shifting. A plain shift truncates toward minus
 *          infinity, which loses half a count on average and, on a value
 *          walked across a range in steps, shows up as a steady drift rather
 *          than as noise.
 */
int32_t q16ToInt ( int32_t value )
{
    int32_t retVal = 0;

    retVal = ( int32_t ) q16ShiftRound ( ( int64_t ) value );

    return ( retVal );
}

/**
 * @brief   Multiplies two Q16 values.
 * @param[in] multiplicand  First value, in Q16.
 * @param[in] multiplier    Second value, in Q16.
 * @return  The product in Q16, rounded to nearest and saturated at the ends of
 *          int32_t.
 * @note    The product of two Q16 values is Q32 and is formed in int64_t
 *          before it comes back down. This is the function that exists because
 *          the obvious version does not: a gain of 2.0 against a value of
 *          40000 is already past what thirty two bits hold, and the wrapped
 *          answer has the wrong sign.
 */
int32_t q16Mul ( int32_t multiplicand, int32_t multiplier )
{
    int32_t retVal = 0;
    int64_t product = 0;

    product = ( ( int64_t ) multiplicand ) * ( ( int64_t ) multiplier );

    retVal = q16Saturate ( q16ShiftRound ( product ) );

    return ( retVal );
}

/**
 * @brief   Divides one Q16 value by another.
 * @param[in] dividend  Value to divide, in Q16.
 * @param[in] divisor   Value to divide by, in Q16.
 * @return  The quotient in Q16, rounded to nearest and saturated at the ends
 *          of int32_t.
 * @note    The dividend is shifted up into Q32 before the division rather than
 *          the quotient being shifted after it. Doing it the other way round
 *          makes every quotient below one come out as zero, which is the
 *          second thing a hand written Q16 divide gets wrong.
 * @note    The rounding adds half the magnitude of the divisor before
 *          dividing, which is what mathMapi32 and interpCalculatei32 do, and
 *          it is written that way because the divisor here may be negative.
 * @note    A zero divisor saturates toward the sign of the dividend, and zero
 *          over zero is zero. There is no status to return — this is a
 *          stateless value function, not a driver, and a caller checking a
 *          status on every multiply and divide would check none of them. The
 *          saturated value is at least in the direction the caller was headed,
 *          and a controller that clamps its output sees it as a rail rather
 *          than as a nan passing straight through, which is the failure
 *          pidInit's zero ts guard exists to prevent.
 */
int32_t q16Div ( int32_t dividend, int32_t divisor )
{
    int32_t retVal = 0;
    int64_t numerator = 0;
    int64_t half = 0;

    if ( divisor == 0 )
    {
        if ( dividend > 0 )
        {
            retVal = Q16_INT_MAX;
        }
        else if ( dividend < 0 )
        {
            retVal = Q16_INT_MIN;
        }
        else
        {
            retVal = 0;
        }
    }
    else
    {
        numerator = ( ( int64_t ) dividend ) * Q16_ONE;

        half = ( ( int64_t ) divisor ) / 2;

        if ( half < 0 )
        {
            half = -half;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( numerator >= 0 )
        {
            numerator += half;
        }
        else
        {
            numerator -= half;
        }

        retVal = q16Saturate ( numerator / ( ( int64_t ) divisor ) );
    }

    return ( retVal );
}

/**
 * @brief   Square root of a Q16 value.
 * @param[in] value  Q16 value to take the root of.
 * @return  The square root in Q16.
 * @note    A negative input returns zero rather than an error. The square root
 *          of a negative number is not a number this type can hold, a status
 *          on a value function would go unchecked, and zero is the answer that
 *          keeps a magnitude calculation sane — the same call
 *          goertzelIteration makes when float rounding pushes a squared
 *          magnitude just below zero.
 * @note    The scale comes out of the root on its own. Shifting a Q16 value up
 *          by sixteen makes it Q32, and the square root of a Q32 value is a
 *          Q16 one, so there is no scaling either side of the root. That
 *          identity is the whole reason rampIterationi32's braking envelope
 *          survived the move to fixed point unchanged.
 * @note    The result cannot overflow. The largest input is a little over
 *          32767 in Q16 and its root is about 181, which is nowhere near the
 *          end of the type.
 */
int32_t q16Sqrt ( int32_t value )
{
    int32_t retVal = 0;

    if ( value <= 0 )
    {
        retVal = 0;
    }
    else
    {
        retVal = ( int32_t ) q16SquareRoot ( ( ( uint64_t ) value ) << Q16_SHIFT );
    }

    return ( retVal );
}
