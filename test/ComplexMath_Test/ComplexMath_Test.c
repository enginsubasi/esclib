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

/* ------------------------------------------------------ the i32 width --- */

#define CQ_ONE      65536

static uint8_t sameI ( const complexi32_t* const got, int32_t re, int32_t im )
{
    uint8_t retVal = FALSE;

    if ( ( got->re == re ) && ( got->im == im ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

static void i32Case ( void )
{
    complexi32_t a;
    complexi32_t b;
    complexi32_t r;

    printf ( "complex i32 variant\n" );

    /* Q16 throughout: 65536 is 1.0. */
    complexIniti32 ( &a, 1 * CQ_ONE, 2 * CQ_ONE );
    complexIniti32 ( &b, 3 * CQ_ONE, 4 * CQ_ONE );

    check ( "init sets both parts",
            ( uint8_t ) ( sameI ( &a, 65536, 131072 ) ) );

    complexSumi32 ( &a, &b, &r );
    check ( "(1+2i) + (3+4i) is 4+6i", sameI ( &r, 262144, 393216 ) );

    complexSubi32 ( &b, &a, &r );
    check ( "(3+4i) - (1+2i) is 2+2i", sameI ( &r, 131072, 131072 ) );

    /*
     * The product needs an int64_t intermediate: two Q16 values make a Q32
     * one, and 2 times 4 in Q16 is already past what thirty two bits hold.
     */
    complexMuli32 ( &a, &b, &r );
    check ( "(1+2i) * (3+4i) is -5+10i", sameI ( &r, -327680, 655360 ) );

    /* And dividing the product back gives each operand. */
    {
        complexi32_t product;
        complexi32_t back;

        complexMuli32 ( &a, &b, &product );

        complexDivi32 ( &product, &b, &back );
        check ( "the product divided by one operand gives the other",
                sameI ( &back, 65536, 131072 ) );

        complexDivi32 ( &product, &a, &back );
        check ( "and the other way round", sameI ( &back, 196608, 262144 ) );
    }

    /*
     * The sign in the imaginary part of the division. A plus instead of a
     * minus gives +i here, which is the defect this test pins in the float
     * width too.
     */
    complexIniti32 ( &a, CQ_ONE, 0 );
    complexIniti32 ( &b, 0, CQ_ONE );

    complexDivi32 ( &a, &b, &r );
    check ( "one divided by i is minus i, not plus i",
            sameI ( &r, 0, -CQ_ONE ) );

    complexMuli32 ( &b, &b, &r );
    check ( "i times i is minus one", sameI ( &r, -CQ_ONE, 0 ) );

    complexIniti32 ( &a, CQ_ONE, 2 * CQ_ONE );
    complexIniti32 ( &b, CQ_ONE, 0 );

    complexMuli32 ( &a, &b, &r );
    check ( "one is the multiplicative identity", sameI ( &r, 65536, 131072 ) );

    complexDivi32 ( &a, &b, &r );
    check ( "and the divisive one", sameI ( &r, 65536, 131072 ) );

    complexDivi32 ( &a, &a, &r );
    check ( "anything over itself is one", sameI ( &r, CQ_ONE, 0 ) );

    /* A zero denominator gives zero, as the float width does. */
    complexIniti32 ( &b, 0, 0 );
    complexDivi32 ( &a, &b, &r );
    check ( "division by zero gives zero rather than an infinity",
            sameI ( &r, 0, 0 ) );

    /* Rounding, at exactly half a count. */
    complexIniti32 ( &a, CQ_ONE / 2, CQ_ONE / 2 );
    complexMuli32 ( &a, &a, &r );
    check ( "(0.5+0.5i) squared is 0.5i", sameI ( &r, 0, 32768 ) );

    complexIniti32 ( &a, 1, 1 );
    complexMuli32 ( &a, &a, &r );
    check ( "two single counts multiply to below half a count, so to zero",
            sameI ( &r, 0, 0 ) );

    complexIniti32 ( &a, 3 * CQ_ONE, 0 );
    complexIniti32 ( &b, 2 * CQ_ONE, 0 );
    complexDivi32 ( &a, &b, &r );
    check ( "three over two is one and a half", sameI ( &r, 98304, 0 ) );

    complexIniti32 ( &a, CQ_ONE, 0 );
    complexIniti32 ( &b, 3 * CQ_ONE, 0 );
    complexDivi32 ( &a, &b, &r );
    check ( "one over three is a third, not zero", sameI ( &r, 21845, 0 ) );

    /*
     * Saturation rather than wrapping. A wrapped phasor changes sign, which
     * for a power reading is the wrong direction of flow.
     */
    complexIniti32 ( &a, 30000 * CQ_ONE, 0 );
    complexIniti32 ( &b, 2 * CQ_ONE, 0 );

    complexMuli32 ( &a, &b, &r );
    check ( "a product past the range saturates at the top",
            sameI ( &r, 2147483647, 0 ) );

    complexIniti32 ( &a, -30000 * CQ_ONE, 0 );
    complexMuli32 ( &a, &b, &r );
    check ( "and at the bottom", sameI ( &r, -2147483647 - 1, 0 ) );

    complexIniti32 ( &a, 30000 * CQ_ONE, 0 );
    complexSumi32 ( &a, &a, &r );
    check ( "a sum past the range saturates too",
            sameI ( &r, 2147483647, 0 ) );

    /*
     * The multiply is safe with an aliased result, unlike matrixMul, because
     * both parts are computed into locals before either field is written.
     */
    complexIniti32 ( &a, CQ_ONE, 2 * CQ_ONE );
    complexIniti32 ( &b, 3 * CQ_ONE, 4 * CQ_ONE );

    complexMuli32 ( &a, &b, &a );
    check ( "a product written over one of its operands is still right",
            sameI ( &a, -327680, 655360 ) );
}

int main ( void )
{
    arithmeticCase ( );
    printf ( "\n" );
    polarCase ( );
    printf ( "\n" );
    i32Case ( );

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
