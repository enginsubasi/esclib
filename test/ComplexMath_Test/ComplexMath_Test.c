/*
 * Covers complex.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure. Complex_Test already exists and
 * prints; this one pins the arithmetic, including the divide whose sign the
 * July 2026 audit fixed and the polar conversions the August 2026 switch to the
 * single precision math functions moved.
 *
 * The expected values are the ones an exact calculation gives, checked against
 * a tolerance rather than compared bit for bit: after the switch to sqrtf,
 * atan2f, cosf and sinf the trailing digits belong to the target's library.
 */

#include <stdio.h>

#include "complex.h"

static uint32_t failures = 0;

static void check ( const char* what, uint8_t condition )
{
    if ( condition == TRUE )
    {
        printf ( "  PASS  %s\n", what );
    }
    else
    {
        printf ( "  FAIL  %s\n", what );
        ++failures;
    }
}

static uint8_t nearly ( float got, float wanted )
{
    uint8_t retVal = FALSE;
    float diff = 0;

    diff = got - wanted;

    if ( diff < 0 )
    {
        diff = -diff;
    }

    if ( diff < 0.001f )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

static uint8_t isComplex ( const complex_t* const value, float re, float im )
{
    uint8_t retVal = FALSE;

    if ( ( nearly ( value->re, re ) == TRUE ) && ( nearly ( value->im, im ) == TRUE ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------------ arithmetic */

static void arithmeticCase ( void )
{
    complex_t a;
    complex_t b;
    complex_t r;

    printf ( "complex arithmetic\n" );

    complexInit ( &a, 3.0f, 4.0f );
    complexInit ( &b, 1.0f, -2.0f );

    check ( "Init stores the real part", nearly ( a.re, 3.0f ) );
    check ( "and the imaginary part", nearly ( a.im, 4.0f ) );

    complexSum ( &a, &b, &r );
    check ( "sum", isComplex ( &r, 4.0f, 2.0f ) );

    complexSub ( &a, &b, &r );
    check ( "difference", isComplex ( &r, 2.0f, 6.0f ) );

    /* ( 3 + 4i )( 1 - 2i ) = 3 - 6i + 4i + 8 = 11 - 2i */
    complexMul ( &a, &b, &r );
    check ( "product", isComplex ( &r, 11.0f, -2.0f ) );

    /*
     * ( 3 + 4i ) / ( 1 - 2i ) = -1 + 2i, which multiplied back by ( 1 - 2i )
     * gives 3 + 4i again. The two cross terms carry opposite signs, so a
     * divide that got a sign wrong lands somewhere else entirely.
     */
    complexDiv ( &a, &b, &r );
    check ( "quotient", isComplex ( &r, -1.0f, 2.0f ) );

    /* And the round trip, which does not depend on knowing the answer. */
    {
        complex_t back;

        complexMul ( &r, &b, &back );
        check ( "the quotient multiplied by the divisor gives the dividend back",
                isComplex ( &back, 3.0f, 4.0f ) );
    }

    /* Dividing by zero yields zero rather than reporting an error. */
    complexInit ( &b, 0.0f, 0.0f );
    complexDiv ( &a, &b, &r );
    check ( "dividing by zero gives zero", isComplex ( &r, 0.0f, 0.0f ) );

    /* Multiplying by zero and by one, the two identities worth pinning. */
    complexMul ( &a, &b, &r );
    check ( "multiplying by zero gives zero", isComplex ( &r, 0.0f, 0.0f ) );

    complexInit ( &b, 1.0f, 0.0f );
    complexMul ( &a, &b, &r );
    check ( "multiplying by one gives the same number", isComplex ( &r, 3.0f, 4.0f ) );

    complexDiv ( &a, &b, &r );
    check ( "dividing by one does too", isComplex ( &r, 3.0f, 4.0f ) );

    /* i squared is minus one. */
    complexInit ( &a, 0.0f, 1.0f );
    complexMul ( &a, &a, &r );
    check ( "i times i is minus one", isComplex ( &r, -1.0f, 0.0f ) );
}

/* ---------------------------------------------------------------- polar */

static void polarCase ( void )
{
    complex_t value;
    float r = 0;
    float angle = 0;

    printf ( "complex polar conversions\n" );

    /* The 3, 4, 5 triangle: magnitude 5 at 53.13010 degrees. */
    complexInit ( &value, 3.0f, 4.0f );
    complexToPolar ( &value, &r, &angle );
    check ( "magnitude", nearly ( r, 5.0f ) );
    check ( "angle in degrees, not radians", nearly ( angle, 53.13010f ) );

    /* The angle must keep its quadrant, which is why atan2 is used. */
    complexInit ( &value, -1.0f, -1.0f );
    complexToPolar ( &value, &r, &angle );
    check ( "third quadrant magnitude", nearly ( r, 1.414214f ) );
    check ( "third quadrant angle is negative, not the first quadrant one",
            nearly ( angle, -135.0f ) );

    complexInit ( &value, -1.0f, 1.0f );
    complexToPolar ( &value, &r, &angle );
    check ( "second quadrant angle", nearly ( angle, 135.0f ) );

    /* A zero real part must not divide by zero. */
    complexInit ( &value, 0.0f, 2.0f );
    complexToPolar ( &value, &r, &angle );
    check ( "a zero real part gives a magnitude", nearly ( r, 2.0f ) );
    check ( "and a right angle", nearly ( angle, 90.0f ) );

    complexInit ( &value, 0.0f, 0.0f );
    complexToPolar ( &value, &r, &angle );
    check ( "the origin has zero magnitude", nearly ( r, 0.0f ) );

    complexFromPolar ( &value, 1.0f, 0.0f );
    check ( "unit magnitude at zero degrees is one", isComplex ( &value, 1.0f, 0.0f ) );

    complexFromPolar ( &value, 2.0f, 90.0f );
    check ( "two at ninety degrees is purely imaginary",
            isComplex ( &value, 0.0f, 2.0f ) );

    complexFromPolar ( &value, 1.0f, 180.0f );
    check ( "one at a hundred and eighty degrees is minus one",
            isComplex ( &value, -1.0f, 0.0f ) );

    complexFromPolar ( &value, 5.0f, 53.13010f );
    check ( "and the 3, 4, 5 triangle comes back", isComplex ( &value, 3.0f, 4.0f ) );

    /*
     * The round trip is the check that does not depend on either function
     * being right on its own, only on them agreeing about what the units are.
     */
    complexInit ( &value, -2.5f, 6.25f );
    complexToPolar ( &value, &r, &angle );
    complexFromPolar ( &value, r, angle );
    check ( "polar and back returns the original number",
            isComplex ( &value, -2.5f, 6.25f ) );
}

int main ( void )
{
    arithmeticCase ( );
    printf ( "\n" );
    polarCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%u check(s) failed\n", ( unsigned ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
