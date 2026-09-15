/**
  ******************************************************************************
  *
  * @file      complex.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      09/08/2022
  *
  * @brief     Complex number library.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 09/08/2022 Created @n
  * 29/07/2026 Bug fix. complexDiv applied the division to the second @n
  *            term only, because of a misplaced parenthesis. @n
  * 29/07/2026 Bug fix. complexToPolar used atan, which divides by @n
  *            zero when re is zero and loses the quadrant when re is @n
  *            negative. Replaced with atan2. @n
  * 29/07/2026 M_PI fallback added. It is not a standard C99 macro. @n
  * 01/08/2026 The double precision sqrt, atan2, cos and sin calls are @n
  *            replaced with their float counterparts. Every operand @n
  *            here is a float, so the double versions forced a @n
  *            promotion and ran in software on a single precision FPU. @n
  * 01/08/2026 Parameters that are only read are declared const, so a @n
  *            caller can pass data it holds in flash without casting @n
  *            the qualifier away. @n
  * 15/09/2026 The Q16 fixed point width is added: the four @n
  *            arithmetic operations only. The polar pair needs a @n
  *            fixed point atan2 and sin, which is a CORDIC and a @n
  *            module of its own rather than a width of this one. @n
  *
  ******************************************************************************
  */

#include "complex.h"
#include <math.h>

// M_PI is not defined by the C99 standard. Provide it when the toolchain does not.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Single precision copy of pi. The module works in float throughout, so using
// the double M_PI directly would promote every expression that touches it back
// to double and pull in the software double routines on a single precision FPU.
#define COMPLEX_PI ( ( float ) M_PI )

/**
 * @brief   Initializes a complex number from its real and imaginary parts.
 * @param[out] cprm1  Complex number to initialize.
 * @param[in]  re     Real part.
 * @param[in]  im     Imaginary part.
 */
void complexInit ( complex_t* cprm1, float re, float im )
{
    cprm1->re = re;
    cprm1->im = im;
}

/**
 * @brief   Adds two complex numbers.
 * @param[in]  cprm1   First addend.
 * @param[in]  cprm2   Second addend.
 * @param[out] result  Sum of cprm1 and cprm2.
 */
void complexSum ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result )
{
    result->re = ( cprm1->re + cprm2->re );
    result->im = ( cprm1->im + cprm2->im );
}

/**
 * @brief   Subtracts one complex number from another.
 * @param[in]  cprm1   Minuend.
 * @param[in]  cprm2   Subtrahend.
 * @param[out] result  cprm1 minus cprm2.
 */
void complexSub ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result )
{
    result->re = ( cprm1->re - cprm2->re );
    result->im = ( cprm1->im - cprm2->im );
}

/**
 * @brief   Multiplies two complex numbers.
 * @param[in]  cprm1   First factor.
 * @param[in]  cprm2   Second factor.
 * @param[out] result  Product of cprm1 and cprm2.
 * @note    result must not alias cprm1 or cprm2. result->re is written
 *          before cprm1->re is read again to compute result->im, so
 *          calling complexMul with result equal to cprm1 or cprm2 produces
 *          a wrong result.
 */
void complexMul ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result )
{
    result->re = ( cprm1->re * cprm2->re ) - ( cprm1->im * cprm2->im );
    result->im = ( cprm1->re * cprm2->im ) + ( cprm1->im * cprm2->re ) ;
}

/**
 * @brief   Divides one complex number by another.
 * @param[in]  cprm1   Dividend.
 * @param[in]  cprm2   Divisor.
 * @param[out] result  cprm1 divided by cprm2.
 * @note    When cprm2 is zero, result is set to zero rather than
 *          reporting an error.
 * @note    result must not alias cprm1 or cprm2. result->re is written
 *          before cprm1->re is read again to compute result->im, so
 *          calling complexDiv with result equal to cprm1 or cprm2 produces
 *          a wrong result.
 */
void complexDiv ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result )
{
    float denominator = 0;

    denominator = ( cprm2->re * cprm2->re ) + ( cprm2->im * cprm2->im );

    if ( denominator != 0 )
    {
        result->re = ( ( cprm1->re * cprm2->re ) + ( cprm1->im * cprm2->im ) ) / denominator;
        result->im = ( ( cprm1->im * cprm2->re ) - ( cprm1->re * cprm2->im ) ) / denominator;
    }
    else
    {
        result->re = 0;
        result->im = 0;
    }
}

/**
 * @brief   Converts a complex number to polar form.
 * @param[in]  prm1  Complex number to convert.
 * @param[out] r     Magnitude of prm1.
 * @param[out] a     Angle of prm1.
 * @note    The angle is in degrees, not radians.
 */
void complexToPolar ( const complex_t* const prm1, float* r, float* a )
{
    // sqrtf never returns a negative value, so the magnitude needs no sign fix.
    *r = sqrtf ( ( prm1->re * prm1->re ) + ( prm1->im * prm1->im ) );

    // atan2f keeps the quadrant and tolerates a zero real part.
    *a = ( atan2f ( prm1->im, prm1->re ) * 180.0f ) / COMPLEX_PI;
}

/**
 * @brief   Converts a polar form magnitude and angle to a complex number.
 * @param[out] prm1  Complex number set from r and a.
 * @param[in]  r     Magnitude.
 * @param[in]  a     Angle.
 * @note    The angle is expected in degrees, not radians.
 */
void complexFromPolar ( complex_t* prm1, float r, float a )
{
    prm1->re = r * cosf ( ( a * COMPLEX_PI ) / 180.0f );
    prm1->im = r * sinf ( ( a * COMPLEX_PI ) / 180.0f );
}


/*
 * The Q16 fixed point width, for a part with no FPU.
 *
 * The caller this exists for is the one doing phasor arithmetic — an energy
 * meter multiplying a voltage phasor by a conjugated current to get complex
 * power, an impedance measurement dividing one by the other. That is
 * multiplies and divides of complex numbers and nothing else, which is why
 * only the four arithmetic operations are here.
 *
 * **There is no complexToPolari32 and no complexFromPolari32.** The magnitude
 * needs a square root and the angle needs an atan2, and doing those in fixed
 * point means a CORDIC rotation: a table, an iteration count and a range
 * reduction, all of which is a module of its own rather than a width of this
 * one. Leaving them out is the honest answer; a version built on a float atan2
 * would defeat the entire reason the width exists.
 *
 * Q16 throughout, so 65536 is 1.0 and the usable range is a little past plus
 * and minus 32767. Everything saturates rather than wrapping, for the reason
 * q16.c gives: a wrapped value changes sign, and a phasor that flips sign is a
 * power reading with the wrong direction of flow.
 */
#define COMPLEX_Q       16
#define COMPLEX_HALF    32768
#define COMPLEX_MAX     2147483647
#define COMPLEX_MIN     ( -2147483647 - 1 )

/**
 * @brief   Brings a 64-bit result back into int32_t, saturating at both ends.
 * @param[in] value  Result to narrow.
 * @return  value, or the nearest end of int32_t when it does not fit.
 * @note    This is q16Saturate, duplicated because no module in this library
 *          includes another's header. complex.c cannot include q16.h any more
 *          than interp.c can include search.h.
 */
static int32_t complexSaturate ( int64_t value )
{
    int32_t retVal = 0;

    if ( value > ( int64_t ) COMPLEX_MAX )
    {
        retVal = COMPLEX_MAX;
    }
    else if ( value < ( int64_t ) COMPLEX_MIN )
    {
        retVal = COMPLEX_MIN;
    }
    else
    {
        retVal = ( int32_t ) value;
    }

    return ( retVal );
}

/**
 * @brief   Brings a Q32 quantity down to Q16, rounding to nearest.
 * @param[in] value  Product of two Q16 values.
 * @return  The same quantity in Q16.
 * @note    The negative branch takes the magnitude first, so the rounding is
 *          symmetric about zero. This is the form biquadIterationi32,
 *          firIterationi32 and q16 all use.
 */
static int64_t complexShiftRound ( int64_t value )
{
    int64_t retVal = 0;

    if ( value >= 0 )
    {
        retVal = ( value + COMPLEX_HALF ) >> COMPLEX_Q;
    }
    else
    {
        retVal = - ( ( -value + COMPLEX_HALF ) >> COMPLEX_Q );
    }

    return ( retVal );
}

/**
 * @brief   Sets the real and imaginary parts of a fixed point complex number.
 * @param[out] cprm1  Number to set.
 * @param[in]  re     Real part, in Q16.
 * @param[in]  im     Imaginary part, in Q16.
 * @note    Returns void for the reason complexInit does: complexi32_t is a
 *          value type rather than a driver. It owns no caller storage and no
 *          callbacks, and giving one of these a status would be less
 *          consistent, not more.
 */
void complexIniti32 ( complexi32_t* cprm1, int32_t re, int32_t im )
{
    cprm1->re = re;
    cprm1->im = im;
}

/**
 * @brief   Adds two fixed point complex numbers.
 * @param[in]  cprm1   First operand.
 * @param[in]  cprm2   Second operand.
 * @param[out] result  Sum, which may be either operand.
 * @note    Saturating. A sum is the one operation here that cannot need a
 *          shift, because both operands are already Q16, but it can still
 *          leave the range and a wrapped phasor is a reading with the wrong
 *          sign.
 */
void complexSumi32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result )
{
    result->re = complexSaturate ( ( ( int64_t ) cprm1->re ) + ( ( int64_t ) cprm2->re ) );
    result->im = complexSaturate ( ( ( int64_t ) cprm1->im ) + ( ( int64_t ) cprm2->im ) );
}

/**
 * @brief   Subtracts one fixed point complex number from another.
 * @param[in]  cprm1   Number to subtract from.
 * @param[in]  cprm2   Number to subtract.
 * @param[out] result  Difference, which may be either operand.
 */
void complexSubi32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result )
{
    result->re = complexSaturate ( ( ( int64_t ) cprm1->re ) - ( ( int64_t ) cprm2->re ) );
    result->im = complexSaturate ( ( ( int64_t ) cprm1->im ) - ( ( int64_t ) cprm2->im ) );
}

/**
 * @brief   Multiplies two fixed point complex numbers.
 * @param[in]  cprm1   First operand.
 * @param[in]  cprm2   Second operand.
 * @param[out] result  Product. Must not be either operand.
 * @note    Every one of the four products is formed in int64_t before it comes
 *          back down, because two Q16 values make a Q32 one.
 * @note    The real part is computed into a local before either field of the
 *          result is written. That is what makes an aliased result safe here,
 *          unlike matrixMul — the imaginary part needs the operands' real
 *          parts, so writing result->re first would destroy one of them when
 *          the result is an operand.
 */
void complexMuli32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result )
{
    int64_t re = 0;
    int64_t im = 0;

    re = ( ( ( int64_t ) cprm1->re ) * ( ( int64_t ) cprm2->re ) ) -
            ( ( ( int64_t ) cprm1->im ) * ( ( int64_t ) cprm2->im ) );

    im = ( ( ( int64_t ) cprm1->re ) * ( ( int64_t ) cprm2->im ) ) +
            ( ( ( int64_t ) cprm1->im ) * ( ( int64_t ) cprm2->re ) );

    result->re = complexSaturate ( complexShiftRound ( re ) );
    result->im = complexSaturate ( complexShiftRound ( im ) );
}

/**
 * @brief   Divides one fixed point complex number by another.
 * @param[in]  cprm1   Numerator.
 * @param[in]  cprm2   Denominator.
 * @param[out] result  Quotient. Must not be either operand.
 * @note    A zero denominator gives zero, which is what complexDiv does in the
 *          float width. There is no status to return from a value function and
 *          zero keeps whatever is downstream a number.
 * @note    The sign in the imaginary part is a **minus**, and getting it wrong
 *          is the defect ComplexMath_Test pins in the float width. The same
 *          check is made here, both against the answer and by multiplying the
 *          quotient back.
 * @note    The numerator is shifted up into Q32 before the division rather
 *          than the quotient being shifted after it, for q16Div's reason:
 *          doing it the other way round makes every quotient below one come
 *          out as zero. The denominator is itself a Q16 quantity, formed from
 *          two Q32 products brought back down.
 */
void complexDivi32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result )
{
    int64_t denominator = 0;
    int64_t re = 0;
    int64_t im = 0;
    int64_t half = 0;

    denominator = complexShiftRound (
            ( ( ( int64_t ) cprm2->re ) * ( ( int64_t ) cprm2->re ) ) +
            ( ( ( int64_t ) cprm2->im ) * ( ( int64_t ) cprm2->im ) ) );

    if ( denominator != 0 )
    {
        re = ( ( ( int64_t ) cprm1->re ) * ( ( int64_t ) cprm2->re ) ) +
                ( ( ( int64_t ) cprm1->im ) * ( ( int64_t ) cprm2->im ) );

        im = ( ( ( int64_t ) cprm1->im ) * ( ( int64_t ) cprm2->re ) ) -
                ( ( ( int64_t ) cprm1->re ) * ( ( int64_t ) cprm2->im ) );

        /* Both products are already Q32, so dividing by a Q16 denominator
           leaves a Q16 quotient with no shift of its own. */
        half = denominator / 2;

        if ( half < 0 )
        {
            half = -half;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( re >= 0 )
        {
            re += half;
        }
        else
        {
            re -= half;
        }

        if ( im >= 0 )
        {
            im += half;
        }
        else
        {
            im -= half;
        }

        result->re = complexSaturate ( re / denominator );
        result->im = complexSaturate ( im / denominator );
    }
    else
    {
        result->re = 0;
        result->im = 0;
    }
}
