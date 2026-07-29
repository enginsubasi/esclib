/**
  ******************************************************************************
  *
  * @file:      complex.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     09/08/2022
  * @history:   09/08/2022 Created
  *             29/07/2026 Bug fix. complexDiv applied the division to the second
  *                        term only, because of a misplaced parenthesis.
  *             29/07/2026 Bug fix. complexToPolar used atan, which divides by
  *                        zero when re is zero and loses the quadrant when re is
  *                        negative. Replaced with atan2.
  *             29/07/2026 M_PI fallback added. It is not a standard C99 macro.
  *
  * @about:     Complex number library.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         complexInit     : Complex number initialization.
  *         complexSum      : 
  *         complexSub      : 
  *         complexMul      : 
  *         complexDiv      : 
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "complex.h"
#include <math.h>

// M_PI is not defined by the C99 standard. Provide it when the toolchain does not.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Complex number initialization
 * @param cprm1
 * @param re
 * @param im
 * @return void
 */
void complexInit ( complex_t* cprm1, float re, float im )
{
    cprm1->re = re;
    cprm1->im = im;
}

/**
 * @brief Complex number summation
 * @param cprm1
 * @param cprm2
 * @param result
 * @return void
 */
void complexSum ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
{
    result->re = ( cprm1->re + cprm2->re );
    result->im = ( cprm1->im + cprm2->im );
}

/**
 * @brief Complex number subtraction
 * @param cprm1
 * @param cprm2
 * @param result
 * @return void
 */
void complexSub ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
{
    result->re = ( cprm1->re - cprm2->re );
    result->im = ( cprm1->im - cprm2->im );
}

/**
 * @brief Complex number multiplication
 * @param cprm1
 * @param cprm2
 * @param result
 * @return void
 */
void complexMul ( complex_t* cprm1, complex_t* cprm2, complex_t* result )
{
    result->re = ( cprm1->re * cprm2->re ) - ( cprm1->im * cprm2->im );
    result->im = ( cprm1->re * cprm2->im ) + ( cprm1->im * cprm2->re ) ;
}

/**
 * @brief Complex number divide
 * @param cprm1
 * @param cprm2
 * @param result
 * @return void
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
 * @brief Complex number to polar
 * @param cprm1
 * @param r
 * @param a
 * @return void
 */
void complexToPolar ( complex_t* prm1, float* r, float* a )
{
    // sqrt never returns a negative value, so the magnitude needs no sign fix.
    *r = sqrt ( ( prm1->re * prm1->re ) + ( prm1->im * prm1->im ) );

    // atan2 keeps the quadrant and tolerates a zero real part.
    *a = ( atan2 ( prm1->im, prm1->re ) * 180 ) / M_PI;
}

/**
 * @brief Complex number to polar
 * @param cprm1
 * @param r
 * @param a
 * @return void
 */
void complexFromPolar ( complex_t* prm1, float r, float a )
{
    prm1->re = r * cos ( ( a * M_PI ) / 180 );
    prm1->im = r * sin ( ( a * M_PI ) / 180 );
}