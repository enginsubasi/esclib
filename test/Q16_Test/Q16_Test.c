/*
 * Covers q16.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Every expected value was computed from the definition of the scale — a
 * product of two Q16 values is Q32, a quotient needs the shift applied first,
 * a conversion back rounds — rather than from running the C. The module exists
 * because the hand written version of each of these gets one specific thing
 * wrong, so a test that agreed with the implementation by construction would
 * check nothing at all.
 */

#include <stdio.h>

#include "q16.h"

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

/* The ends of int32_t, written as expressions because -2147483648 is not a
 * literal in C: it is the unary minus of a value that does not fit. */
#define Q16_TEST_MAX    2147483647
#define Q16_TEST_MIN    ( -2147483647 - 1 )

/* --------------------------------------------------------- conversions */

static void conversionCase ( void )
{
    int32_t i = 0;
    uint8_t exact = TRUE;

    printf ( "q16FromInt and q16ToInt\n" );

    check ( "zero converts to zero", ( uint8_t ) ( q16FromInt ( 0 ) == 0 ) );
    check ( "one is the scale itself",
            ( uint8_t ) ( q16FromInt ( 1 ) == 65536 ) );
    check ( "and minus one is its negative",
            ( uint8_t ) ( q16FromInt ( -1 ) == -65536 ) );
    check ( "five", ( uint8_t ) ( q16FromInt ( 5 ) == 327680 ) );

    /*
     * Q16 in an int32_t reaches a little past plus or minus 32767, and beyond
     * that it saturates rather than wrapping. A wrapped value changes sign,
     * which for a number headed into a control loop is the worst answer
     * available.
     */
    check ( "the largest value that fits",
            ( uint8_t ) ( q16FromInt ( 32767 ) == 2147418112 ) );
    check ( "the smallest, which lands exactly on the end of the type",
            ( uint8_t ) ( q16FromInt ( -32768 ) == Q16_TEST_MIN ) );
    check ( "one past the top saturates rather than wrapping",
            ( uint8_t ) ( q16FromInt ( 32768 ) == Q16_TEST_MAX ) );
    check ( "and one past the bottom",
            ( uint8_t ) ( q16FromInt ( -32769 ) == Q16_TEST_MIN ) );
    check ( "and something far outside",
            ( uint8_t ) ( q16FromInt ( 100000 ) == Q16_TEST_MAX ) );

    check ( "the scale converts back to one",
            ( uint8_t ) ( q16ToInt ( 65536 ) == 1 ) );
    check ( "and its negative", ( uint8_t ) ( q16ToInt ( -65536 ) == -1 ) );

    /*
     * The conversion back rounds instead of shifting. A plain shift truncates
     * toward minus infinity, which loses half a count on average and shows up
     * as a steady drift rather than as noise on a value walked in steps.
     */
    check ( "one and a half rounds to two",
            ( uint8_t ) ( q16ToInt ( 98304 ) == 2 ) );
    check ( "minus one and a half rounds to minus two",
            ( uint8_t ) ( q16ToInt ( -98304 ) == -2 ) );
    check ( "exactly a half rounds away from zero",
            ( uint8_t ) ( q16ToInt ( 32768 ) == 1 ) );
    check ( "and so does minus a half",
            ( uint8_t ) ( q16ToInt ( -32768 ) == -1 ) );
    check ( "one short of a half rounds to zero",
            ( uint8_t ) ( q16ToInt ( 32767 ) == 0 ) );
    check ( "and so does its negative",
            ( uint8_t ) ( q16ToInt ( -32767 ) == 0 ) );

    check ( "the top of the type converts to 32768",
            ( uint8_t ) ( q16ToInt ( Q16_TEST_MAX ) == 32768 ) );
    check ( "and the bottom to minus 32768",
            ( uint8_t ) ( q16ToInt ( Q16_TEST_MIN ) == -32768 ) );

    /*
     * The property that matters more than any single case: every integer the
     * scale can hold survives a round trip. Checked over the whole range
     * rather than at a few points, because it is cheap and because an off by
     * one in the rounding would show at exactly one value.
     */
    exact = TRUE;

    for ( i = -32768; i <= 32767; ++i )
    {
        if ( q16ToInt ( q16FromInt ( i ) ) != i )
        {
            exact = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "every representable integer survives a round trip", exact );
}

/* ---------------------------------------------------------------- mul */

static void mulCase ( void )
{
    printf ( "q16Mul\n" );

    check ( "two times three is six",
            ( uint8_t ) ( q16Mul ( 131072, 196608 ) == 393216 ) );
    check ( "one is the identity",
            ( uint8_t ) ( q16Mul ( 65536, 65536 ) == 65536 ) );
    check ( "anything times zero is zero",
            ( uint8_t ) ( q16Mul ( 65536, 0 ) == 0 ) );

    /* A half squared is a quarter, which is where the shift has to happen. */
    check ( "a half times a half is a quarter",
            ( uint8_t ) ( q16Mul ( 32768, 32768 ) == 16384 ) );

    check ( "one negative argument gives a negative product",
            ( uint8_t ) ( q16Mul ( -131072, 196608 ) == -393216 ) );
    check ( "and two give a positive one",
            ( uint8_t ) ( q16Mul ( -131072, -196608 ) == 393216 ) );

    /*
     * The reason this function exists. Two Q16 values make a Q32 product, so
     * 32767.0 times 2.0 is past what thirty two bits hold; the version without
     * an int64_t intermediate wraps and comes back negative.
     */
    check ( "a product too large for the type saturates at the top",
            ( uint8_t ) ( q16Mul ( 2147418112, 131072 ) == Q16_TEST_MAX ) );
    check ( "and too small saturates at the bottom",
            ( uint8_t ) ( q16Mul ( Q16_TEST_MIN, 131072 ) == Q16_TEST_MIN ) );

    /* The smallest values, where the rounding is the whole answer. */
    check ( "two single counts multiply to below half a count, so to zero",
            ( uint8_t ) ( q16Mul ( 1, 1 ) == 0 ) );
    check ( "one count times one is one count",
            ( uint8_t ) ( q16Mul ( 1, 65536 ) == 1 ) );
    check ( "three counts times a half is one and a half, rounding to two",
            ( uint8_t ) ( q16Mul ( 3, 32768 ) == 2 ) );

    /*
     * A third times three is not one, and saying so is more useful than
     * pretending otherwise: a third is not representable in Q16, so the
     * product is one count short. Anyone building a gain out of repeated
     * multiplication needs to know the error does not cancel.
     */
    check ( "a third times three falls one count short of one",
            ( uint8_t ) ( q16Mul ( 21845, 196608 ) == 65535 ) );
}

/* ---------------------------------------------------------------- div */

static void divCase ( void )
{
    printf ( "q16Div\n" );

    check ( "one over two is a half",
            ( uint8_t ) ( q16Div ( 65536, 131072 ) == 32768 ) );
    check ( "three over two is one and a half",
            ( uint8_t ) ( q16Div ( 196608, 131072 ) == 98304 ) );
    check ( "one over one is one",
            ( uint8_t ) ( q16Div ( 65536, 65536 ) == 65536 ) );
    check ( "zero over anything is zero",
            ( uint8_t ) ( q16Div ( 0, 327680 ) == 0 ) );

    /*
     * The reason this function exists. One over three is below one, and the
     * version that divides first and shifts afterwards answers zero for every
     * quotient in that range.
     */
    check ( "one over three is a third, not zero",
            ( uint8_t ) ( q16Div ( 65536, 196608 ) == 21845 ) );

    check ( "a negative dividend",
            ( uint8_t ) ( q16Div ( -65536, 131072 ) == -32768 ) );
    check ( "a negative divisor",
            ( uint8_t ) ( q16Div ( 65536, -131072 ) == -32768 ) );
    check ( "and both negative",
            ( uint8_t ) ( q16Div ( -65536, -131072 ) == 32768 ) );

    check ( "one over a half is two",
            ( uint8_t ) ( q16Div ( 65536, 32768 ) == 131072 ) );

    /*
     * A zero divisor saturates toward the sign of the dividend rather than
     * returning a status nobody would check. The point is that a controller
     * downstream sees a rail, which its output limiter handles, instead of
     * something that passes straight through it.
     */
    check ( "a positive dividend over zero saturates at the top",
            ( uint8_t ) ( q16Div ( 65536, 0 ) == Q16_TEST_MAX ) );
    check ( "a negative one over zero saturates at the bottom",
            ( uint8_t ) ( q16Div ( -65536, 0 ) == Q16_TEST_MIN ) );
    check ( "and zero over zero is zero",
            ( uint8_t ) ( q16Div ( 0, 0 ) == 0 ) );

    /* Dividing by the smallest positive value there is. */
    check ( "five over one count saturates",
            ( uint8_t ) ( q16Div ( 327680, 1 ) == Q16_TEST_MAX ) );
}

/* --------------------------------------------------------------- sqrt */

static void sqrtCase ( void )
{
    printf ( "q16Sqrt\n" );

    check ( "the root of zero is zero", ( uint8_t ) ( q16Sqrt ( 0 ) == 0 ) );
    check ( "the root of one is one",
            ( uint8_t ) ( q16Sqrt ( 65536 ) == 65536 ) );
    check ( "the root of four is two",
            ( uint8_t ) ( q16Sqrt ( 262144 ) == 131072 ) );
    check ( "the root of nine is three",
            ( uint8_t ) ( q16Sqrt ( 589824 ) == 196608 ) );
    check ( "the root of a hundred is ten",
            ( uint8_t ) ( q16Sqrt ( 6553600 ) == 655360 ) );
    check ( "the root of a quarter is a half",
            ( uint8_t ) ( q16Sqrt ( 16384 ) == 32768 ) );

    /* 1.41421 in Q16 is 92681.9, and the root truncates rather than rounds. */
    check ( "the root of two",
            ( uint8_t ) ( q16Sqrt ( 131072 ) == 92681 ) );

    check ( "the root of the largest value the scale holds",
            ( uint8_t ) ( q16Sqrt ( 2147418112 ) == 11863102 ) );

    /*
     * A negative input is zero rather than an error, which is the same call
     * goertzelIteration makes when float rounding pushes a squared magnitude
     * just below zero.
     */
    check ( "a negative input gives zero",
            ( uint8_t ) ( q16Sqrt ( -1 ) == 0 ) );
    check ( "and so does a large negative one",
            ( uint8_t ) ( q16Sqrt ( -327680 ) == 0 ) );
}

/* ------------------------------------------------- the ramp identity */

static void rampIdentityCase ( void )
{
    int32_t acceleration = 0;
    int32_t distance = 0;
    int32_t envelope = 0;

    printf ( "the identity ramp is built on\n" );

    /*
     * rampIterationi32's braking envelope is sqrt ( 2 * a * remaining ), and
     * it survived the move to fixed point unchanged because the square root of
     * a Q32 product is a Q16 value — the scale falls out of the root for free.
     * Building the same expression out of this module has to reproduce it.
     *
     * An acceleration of 2.0 and a distance of 50.0 give sqrt ( 200 ), which
     * is 14.142136, and 14.142136 in Q16 is 926819.
     */
    acceleration = q16FromInt ( 2 );
    distance = q16FromInt ( 50 );

    envelope = q16Sqrt ( q16Mul ( q16Mul ( q16FromInt ( 2 ), acceleration ),
                                    distance ) );

    check ( "sqrt ( 2 * a * d ) comes out at 14.142136 in Q16",
            ( uint8_t ) ( envelope == 926819 ) );
    check ( "which converts back to fourteen",
            ( uint8_t ) ( q16ToInt ( envelope ) == 14 ) );
}

int main ( void )
{
    conversionCase ( );
    printf ( "\n" );
    mulCase ( );
    printf ( "\n" );
    divCase ( );
    printf ( "\n" );
    sqrtCase ( );
    printf ( "\n" );
    rampIdentityCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%lu check(s) failed\n", ( unsigned long ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
