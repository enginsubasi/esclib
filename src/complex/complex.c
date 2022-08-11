/**
  ******************************************************************************
  *
  * @file:      complex.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     09/08/2022
  * @history:   09/08/2022 Created
  *
  * @about:     Complex number library.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         complexInit     : Complex number initialization.
  *         complexSum      : Brief
  *         complexSub      : Brief
  *         complexMul      : Brief
  *         complexDiv      : Brief
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "complex.h"
#include <math.h>

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
    result->re = ( ( cprm1->re * cprm2->re ) + ( cprm1->im * cprm2->im ) / ( ( cprm2->re * cprm2->re ) + ( cprm2->im * cprm2->im ) ) );
    result->im = ( ( cprm1->im * cprm2->re ) - ( cprm1->re * cprm2->im ) / ( ( cprm2->re * cprm2->re ) + ( cprm2->im * cprm2->im ) ) );
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
    *r = sqrt ( ( prm1->re * prm1->re ) + ( prm1->im * prm1->im ) );

    if ( *r < 0 )
    {
        *r *= -1;
    }

    *a = ( atan ( prm1->im / prm1->re ) * 180 ) / M_PI;
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