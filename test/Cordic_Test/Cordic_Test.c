/*
 * Covers cordic.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Every expected value came from the mathematics rather than from running the
 * C: the sines and cosines are round ( sin ( x ) * 65536 ) computed in double
 * precision on a host, the angles are atan2 scaled onto the binary angle, and
 * the magnitudes are Pythagorean triples chosen so the answer is a whole
 * number. A test that took its expectations from the implementation would
 * agree with a wrong implementation.
 *
 * No float appears here either. The module's whole claim is that it needs no
 * FPU, and a test that reached for one to check it would be checking
 * something else.
 */

#include <stdio.h>

#include "cordic.h"

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

/*
 * What the measurements in cordic.c's banner allow. The sine and the cosine
 * are within one count of 65536 and the angle within 1307 of a turn; these are
 * the round numbers just above, so a change that quietly costs a bit of
 * accuracy still fails here.
 *
 * The lengths are asserted exactly wherever the answer is a whole number and
 * small, because at that size the module is exact and a tolerance there would
 * hide a defect. The two large ones carry a hundred counts, which is a tenth
 * of the error the twenty iterations are entitled to and a fiftieth of what
 * dropping to a Q16 gain constant costs.
 */
#define TRIG_TOLERANCE      2
#define ANGLE_TOLERANCE     2000u

/*
 * The round trip is allowed more, and the reason is arithmetic rather than
 * generosity. A sine and a cosine are reported in Q16, so the vector they make
 * is known to one part in 65536, and one count at the end of a unit vector is
 * an angle of atan ( 1 / 65536 ) — 10430 counts of a turn. No arc tangent can
 * recover what the rounding already threw away, so the floor here is that plus
 * the 1307 of cordicAtan2 itself.
 */
#define TRIP_TOLERANCE      16000u

/* The ends of int32_t, written as expressions because -2147483648 is not a
 * literal in C: it is the unary minus of a value that does not fit. */
#define CORDIC_TEST_MAX     2147483647
#define CORDIC_TEST_MIN     ( -2147483647 - 1 )

static uint8_t near ( int32_t got, int32_t want, int32_t tolerance )
{
    int32_t difference = got - want;

    if ( difference < 0 )
    {
        difference = -difference;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( ( uint8_t ) ( difference <= tolerance ) );
}

/*
 * The distance between two angles the short way round. Everything is unsigned,
 * so the subtraction wraps and the wrap is what makes this work at all: the
 * distance from just below a whole turn back to zero is small, and a signed
 * comparison would call it the largest difference there is.
 */
static uint8_t angleNear ( uint32_t got, uint32_t want, uint32_t tolerance )
{
    uint32_t difference = got - want;

    if ( difference > CORDIC_HALF )
    {
        difference = 0u - difference;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( ( uint8_t ) ( difference <= tolerance ) );
}

/* ------------------------------------------------------- sine and cosine */

static void sinCosCase ( void )
{
    int32_t sine = 0;
    int32_t cosine = 0;

    printf ( "cordicSinCos\n" );

    /*
     * The four quarter turns, where the answer is exact and any error in the
     * quadrant mapping shows immediately. These are also the only values in
     * the module that come out exactly, so they are asserted exactly.
     */
    cordicSinCos ( 0u, &sine, &cosine );
    check ( "zero gives sine 0 and cosine one",
            ( uint8_t ) ( ( sine == 0 ) && ( cosine == CORDIC_ONE ) ) );

    cordicSinCos ( CORDIC_QUARTER, &sine, &cosine );
    check ( "a quarter turn gives sine one and cosine 0",
            ( uint8_t ) ( ( sine == CORDIC_ONE ) && ( cosine == 0 ) ) );

    cordicSinCos ( CORDIC_HALF, &sine, &cosine );
    check ( "a half turn gives sine 0 and cosine minus one",
            ( uint8_t ) ( ( sine == 0 ) && ( cosine == -CORDIC_ONE ) ) );

    cordicSinCos ( CORDIC_THREEQUARTER, &sine, &cosine );
    check ( "three quarters gives sine minus one and cosine 0",
            ( uint8_t ) ( ( sine == -CORDIC_ONE ) && ( cosine == 0 ) ) );

    /* An eighth of a turn. 65536 / sqrt ( 2 ) is 46341 to the nearest count. */
    cordicSinCos ( 0x20000000u, &sine, &cosine );
    check ( "an eighth of a turn, sine", near ( sine, 46341, TRIG_TOLERANCE ) );
    check ( "an eighth of a turn, cosine",
            near ( cosine, 46341, TRIG_TOLERANCE ) );

    /* A twelfth, which is thirty degrees, where the sine is exactly a half. */
    cordicSinCos ( 0x15555555u, &sine, &cosine );
    check ( "a twelfth of a turn, sine", near ( sine, 32768, TRIG_TOLERANCE ) );
    check ( "a twelfth of a turn, cosine",
            near ( cosine, 56756, TRIG_TOLERANCE ) );

    /* A sixth, which is sixty degrees, the same pair the other way round. */
    cordicSinCos ( 0x2AAAAAABu, &sine, &cosine );
    check ( "a sixth of a turn, sine", near ( sine, 56756, TRIG_TOLERANCE ) );
    check ( "a sixth of a turn, cosine",
            near ( cosine, 32768, TRIG_TOLERANCE ) );

    /*
     * One in each of the other three quadrants, because the quadrant mapping
     * is four separate lines and a wrong sign in any one of them is invisible
     * from the first quadrant alone.
     */
    cordicSinCos ( 0x60000000u, &sine, &cosine );
    check ( "three eighths, second quadrant",
            ( uint8_t ) ( near ( sine, 46341, TRIG_TOLERANCE ) &&
                          near ( cosine, -46341, TRIG_TOLERANCE ) ) );

    cordicSinCos ( 0xA0000000u, &sine, &cosine );
    check ( "five eighths, third quadrant",
            ( uint8_t ) ( near ( sine, -46341, TRIG_TOLERANCE ) &&
                          near ( cosine, -46341, TRIG_TOLERANCE ) ) );

    cordicSinCos ( 0xE0000000u, &sine, &cosine );
    check ( "seven eighths, fourth quadrant",
            ( uint8_t ) ( near ( sine, -46341, TRIG_TOLERANCE ) &&
                          near ( cosine, 46341, TRIG_TOLERANCE ) ) );

    /* A few counts short of a whole turn, which is the wrap. */
    cordicSinCos ( 0xFFBE76C9u, &sine, &cosine );
    check ( "just under a whole turn",
            ( uint8_t ) ( near ( sine, -412, TRIG_TOLERANCE ) &&
                          near ( cosine, 65535, TRIG_TOLERANCE ) ) );

    cordicSinCos ( 0x12345678u, &sine, &cosine );
    check ( "cordicSin agrees with the pair",
            ( uint8_t ) ( cordicSin ( 0x12345678u ) == sine ) );
    check ( "cordicCos agrees with the pair",
            ( uint8_t ) ( cordicCos ( 0x12345678u ) == cosine ) );
}

/* -------------------------------------------------------------- identity */

static void identityCase ( void )
{
    uint32_t i = 0u;
    uint32_t angle = 0u;
    int32_t sine = 0;
    int32_t cosine = 0;
    int64_t sum = 0;
    int64_t worst = 0;
    int64_t one = ( ( int64_t ) CORDIC_ONE ) * ( ( int64_t ) CORDIC_ONE );
    uint8_t rising = TRUE;
    int32_t previous = 0;

    printf ( "the identity and the shape\n" );

    /*
     * A thousand angles spread over the whole turn by a stride that is prime
     * to it, so the sample never falls into step with the quadrants. The sum
     * of the squares must be the square of the amplitude everywhere: this is
     * the one property that holds at every angle without a table of expected
     * values, and it catches a gain left uncompensated, which a check at the
     * quarter turns alone cannot see.
     */
    for ( i = 0u; i < 1000u; ++i )
    {
        angle = angle + 4294967291u;
        cordicSinCos ( angle, &sine, &cosine );

        sum = ( ( ( int64_t ) sine ) * ( ( int64_t ) sine ) ) +
              ( ( ( int64_t ) cosine ) * ( ( int64_t ) cosine ) ) - one;

        if ( sum < 0 )
        {
            sum = -sum;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( sum > worst )
        {
            worst = sum;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    /*
     * A tenth of a percent of the square of the amplitude. The measured worst
     * is a fiftieth of that, so this fails long before the module becomes
     * useless and does not fail on the last bit of rounding.
     */
    check ( "sine squared plus cosine squared is one everywhere",
            ( uint8_t ) ( worst < ( one / 1000 ) ) );

    /* The sine rises across the whole first quadrant and nowhere turns back. */
    previous = -CORDIC_ONE;

    for ( i = 0u; i < 256u; ++i )
    {
        sine = cordicSin ( i * 4194304u );

        if ( sine < previous )
        {
            rising = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }

        previous = sine;
    }

    check ( "the sine rises across the first quadrant", rising );
}

/* ---------------------------------------------------------------- atan2 */

static void atan2Case ( void )
{
    printf ( "cordicAtan2\n" );

    /*
     * One in each quadrant, from the smallest arguments there are. Small
     * arguments are the interesting case rather than the easy one: the
     * iteration shifts its terms right, so on a component of 1 it has a single
     * bit to work with unless the module normalizes first.
     */
    check ( "first quadrant",
            angleNear ( cordicAtan2 ( 1, 1 ), 0x20000000u, ANGLE_TOLERANCE ) );
    check ( "second quadrant",
            angleNear ( cordicAtan2 ( 1, -1 ), 0x60000000u, ANGLE_TOLERANCE ) );
    check ( "third quadrant",
            angleNear ( cordicAtan2 ( -1, -1 ), 0xA0000000u, ANGLE_TOLERANCE ) );
    check ( "fourth quadrant",
            angleNear ( cordicAtan2 ( -1, 1 ), 0xE0000000u, ANGLE_TOLERANCE ) );

    /* The axes, where the half turn offset for a negative first component is
       either applied or it is not. */
    check ( "along the first axis",
            angleNear ( cordicAtan2 ( 0, 7 ), 0u, ANGLE_TOLERANCE ) );
    check ( "along the second axis",
            angleNear ( cordicAtan2 ( 9, 0 ), CORDIC_QUARTER,
                        ANGLE_TOLERANCE ) );
    check ( "back along the first axis",
            angleNear ( cordicAtan2 ( 0, -5 ), CORDIC_HALF, ANGLE_TOLERANCE ) );
    check ( "back along the second axis",
            angleNear ( cordicAtan2 ( -9, 0 ), CORDIC_THREEQUARTER,
                        ANGLE_TOLERANCE ) );

    /* Two angles that are not multiples of anything: 3-4-5 and 5-12-13. */
    check ( "the 3 4 5 angle",
            angleNear ( cordicAtan2 ( 4, 3 ), 0x25C80A3Bu, ANGLE_TOLERANCE ) );
    check ( "the 5 12 13 angle",
            angleNear ( cordicAtan2 ( 12, 5 ), 0x2FEA2DE3u, ANGLE_TOLERANCE ) );

    /* The scale of the vector must not change its angle. */
    check ( "the angle does not depend on the length",
            angleNear ( cordicAtan2 ( 4, 3 ),
                        cordicAtan2 ( 400000000, 300000000 ),
                        ANGLE_TOLERANCE ) );

    check ( "both components zero gives zero",
            ( uint8_t ) ( cordicAtan2 ( 0, 0 ) == 0u ) );
}

/* ------------------------------------------------------------ round trip */

static void roundTripCase ( void )
{
    uint32_t i = 0u;
    uint32_t angle = 0u;
    uint32_t back = 0u;
    int32_t sine = 0;
    int32_t cosine = 0;
    uint8_t ok = TRUE;

    printf ( "the round trip\n" );

    /*
     * An angle into a vector and back out of it. This is the check that pins
     * the atan table itself: a wrong entry moves the sine and the arc tangent
     * by the same amount in the same direction, so each of them on its own
     * still looks plausible against a loose expectation and only the two
     * together disagree.
     */
    for ( i = 0u; i < 500u; ++i )
    {
        angle = angle + 8589935u;
        cordicSinCos ( angle, &sine, &cosine );
        back = cordicAtan2 ( sine, cosine );

        if ( angleNear ( back, angle, TRIP_TOLERANCE ) == FALSE )
        {
            ok = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "an angle survives being turned into a vector and read back", ok );
}

/* ------------------------------------------------------------ magnitude */

static void magnitudeCase ( void )
{
    int32_t magnitude = 0;
    uint32_t angle = 0u;

    printf ( "cordicMagnitude and cordicPolar\n" );

    /*
     * The pair, which is the function the other two are written over. One run
     * of the iteration produces both, so a caller reading a phasor — which
     * wants a length and a phase together — should ask for them here.
     */
    cordicPolar ( 3, 4, &magnitude, &angle );
    check ( "the pair gives the length", ( uint8_t ) ( magnitude == 5 ) );
    check ( "and the angle with it",
            angleNear ( angle, 0x25C80A3Bu, ANGLE_TOLERANCE ) );

    cordicPolar ( 0, 0, &magnitude, &angle );
    check ( "nothing has neither",
            ( uint8_t ) ( ( magnitude == 0 ) && ( angle == 0u ) ) );

    /* Pythagorean triples, where the answer is a whole number. */
    check ( "3 4 5", ( uint8_t ) ( cordicMagnitude ( 3, 4 ) == 5 ) );
    check ( "5 12 13", ( uint8_t ) ( cordicMagnitude ( 5, 12 ) == 13 ) );
    check ( "8 15 17", ( uint8_t ) ( cordicMagnitude ( 8, 15 ) == 17 ) );
    check ( "and the same in the third quadrant",
            ( uint8_t ) ( cordicMagnitude ( -3, -4 ) == 5 ) );

    check ( "a length along one axis is the component",
            ( uint8_t ) ( cordicMagnitude ( 1000000, 0 ) == 1000000 ) );
    check ( "including a negative one",
            ( uint8_t ) ( cordicMagnitude ( 0, -1000000 ) == 1000000 ) );
    check ( "nothing has no length",
            ( uint8_t ) ( cordicMagnitude ( 0, 0 ) == 0 ) );

    /* The triple again, scaled up by a million. */
    check ( "a large triple",
            near ( cordicMagnitude ( 300000000, 400000000 ), 500000000,
                   100 ) );

    /*
     * A length whose square is past int64_t's comfort and well past int32_t's,
     * on components that are not. Nothing here squares anything, so this is an
     * answer rather than an overflow.
     */
    check ( "a component that could not be squared in 32 bits",
            near ( cordicMagnitude ( 1500000000, 0 ), 1500000000, 100 ) );

    /*
     * And a length that genuinely does not fit. It saturates: a wrapped length
     * comes back negative, and a negative length is a worse answer than a
     * clipped one.
     */
    check ( "a length past the type saturates",
            ( uint8_t ) ( cordicMagnitude ( CORDIC_TEST_MAX,
                                            CORDIC_TEST_MAX ) ==
                          CORDIC_TEST_MAX ) );
    check ( "and so does one built from the other end",
            ( uint8_t ) ( cordicMagnitude ( CORDIC_TEST_MIN,
                                            CORDIC_TEST_MIN ) ==
                          CORDIC_TEST_MAX ) );
}

/* --------------------------------------------------------------- rotate */

static void rotateCase ( void )
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t ax = 0;
    int32_t ay = 0;

    printf ( "cordicRotate\n" );

    /* The quarter turns, where a rotation is a swap and a sign. */
    x = CORDIC_ONE;
    y = 0;
    cordicRotate ( &x, &y, CORDIC_QUARTER );
    check ( "a quarter turn takes the first axis onto the second",
            ( uint8_t ) ( near ( x, 0, TRIG_TOLERANCE ) &&
                          near ( y, CORDIC_ONE, TRIG_TOLERANCE ) ) );

    cordicRotate ( &x, &y, CORDIC_QUARTER );
    check ( "and again onto the first, reversed",
            ( uint8_t ) ( near ( x, -CORDIC_ONE, TRIG_TOLERANCE ) &&
                          near ( y, 0, TRIG_TOLERANCE ) ) );

    x = 10000;
    y = 0;
    cordicRotate ( &x, &y, CORDIC_HALF );
    check ( "a half turn is a negation",
            ( uint8_t ) ( near ( x, -10000, TRIG_TOLERANCE ) &&
                          near ( y, 0, TRIG_TOLERANCE ) ) );

    /* An eighth, where both components are the length over the root of two. */
    x = 65536;
    y = 0;
    cordicRotate ( &x, &y, 0x20000000u );
    check ( "an eighth of a turn puts it on the diagonal",
            ( uint8_t ) ( near ( x, 46341, TRIG_TOLERANCE ) &&
                          near ( y, 46341, TRIG_TOLERANCE ) ) );

    /*
     * Two rotations compose into one. This is what says the quadrant handling
     * and the residual agree with each other: an error in either is an error
     * in the composed angle, and the single rotation has it in one place while
     * the pair has it in two.
     */
    x = 100000;
    y = -35000;
    ax = x;
    ay = y;
    cordicRotate ( &x, &y, 0x30000000u );
    cordicRotate ( &x, &y, 0x50000000u );
    cordicRotate ( &ax, &ay, 0x80000000u );
    check ( "two rotations compose into their sum",
            ( uint8_t ) ( near ( x, ax, 64 ) && near ( y, ay, 64 ) ) );

    /* A rotation does not change a length. */
    x = 1000000;
    y = 250000;
    cordicRotate ( &x, &y, 0x0ABCDEF0u );
    check ( "a rotation holds the length",
            near ( cordicMagnitude ( x, y ),
                   cordicMagnitude ( 1000000, 250000 ), 64 ) );

    /* Nothing stays nothing rather than acquiring a direction. */
    x = 0;
    y = 0;
    cordicRotate ( &x, &y, 0x11111111u );
    check ( "nothing rotates to nothing",
            ( uint8_t ) ( ( x == 0 ) && ( y == 0 ) ) );
}

/* ------------------------------------------------------------ the caller */

static void designCase ( void )
{
    uint32_t angle = 0u;
    int32_t sine = 0;
    int32_t cosine = 0;

    printf ( "what a caller designs with it\n" );

    /*
     * The use this module was written for: a biquad coefficient set at a
     * corner given in hertz, on a part with no FPU. A frequency as a fraction
     * of the sample rate is that fraction of a turn, so 50 Hz in a 1 kHz loop
     * is a twentieth of a turn and there is no pi anywhere in the expression.
     */
    angle = ( uint32_t ) ( ( 4294967296u / 1000u ) * 50u );
    cordicSinCos ( angle, &sine, &cosine );

    /* A twentieth of a turn is 18 degrees: sine 0.309017, cosine 0.951057. */
    check ( "a corner of a twentieth of the sample rate, sine",
            near ( sine, 20252, TRIG_TOLERANCE ) );
    check ( "a corner of a twentieth of the sample rate, cosine",
            near ( cosine, 62327, TRIG_TOLERANCE ) );

    /*
     * And the other one: the polar pair complexi32 does not carry. A phasor
     * given as a real and an imaginary part, read back as a length and a
     * phase.
     */
    check ( "a phasor's length",
            ( uint8_t ) ( cordicMagnitude ( 30000, 40000 ) == 50000 ) );
    check ( "a phasor's phase",
            angleNear ( cordicAtan2 ( 40000, 30000 ), 0x25C80A3Bu,
                        ANGLE_TOLERANCE ) );
}

int main ( void )
{
    sinCosCase ( );
    printf ( "\n" );
    identityCase ( );
    printf ( "\n" );
    atan2Case ( );
    printf ( "\n" );
    roundTripCase ( );
    printf ( "\n" );
    magnitudeCase ( );
    printf ( "\n" );
    rotateCase ( );
    printf ( "\n" );
    designCase ( );

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
