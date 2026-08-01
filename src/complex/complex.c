/**
  ******************************************************************************
  *
  * @file      complex.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.3
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
void complexSum ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
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
void complexSub ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
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
void complexMul ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
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
void complexDiv ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
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
void complexToPolar ( complex_t* prm1, float* r, float* a )
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