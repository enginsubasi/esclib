/*
 * Covers fir, its i32 variant, and goertzel.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The two modules are paired here for the reason ArrayMatrix_Test pairs its
 * two: they answer the two halves of one question. fir shapes what frequencies
 * survive; goertzel measures how much of one frequency was there.
 *
 * Every expected value was worked out from the algorithm before the C was run.
 * The fir ones are exact convolutions of small integers, so they are asserted
 * exactly; the goertzel ones come from the normalization the module documents,
 * where a sine of amplitude one reads one whatever the block length.
 */

#include <math.h>
#include <stdio.h>

#include "fir.h"
#include "goertzel.h"

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

static uint8_t near ( float got, float want, float tolerance )
{
    uint8_t retVal = FALSE;
    float difference = 0;

    difference = got - want;

    if ( difference < 0 )
    {
        difference = -difference;
    }
    else
    {
        /* Intentionally blank. */
    }

    if ( difference <= tolerance )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------------------- fir */

/*
 * Asymmetric on purpose. A symmetric tap set reads the same whichever end of
 * the history it is applied from, so it cannot tell a correct implementation
 * from one that walks the history the wrong way.
 */
static const float rampTaps[ 3 ] = { 1.0f, 2.0f, 3.0f };

/* A pure one sample delay: nothing of the current sample, all of the previous. */
static const float delayTaps[ 2 ] = { 0.0f, 1.0f };

/* The rectangular window maf implements, written out as taps. */
static const float boxTaps[ 4 ] = { 0.25f, 0.25f, 0.25f, 0.25f };

/*
 * A triangular window, unity gain at dc and 1/9 at Nyquist. Both of those are
 * exact sums of the taps, so both can be asserted rather than measured.
 */
static const float triangleTaps[ 5 ] =
        { 1.0f / 9.0f, 2.0f / 9.0f, 3.0f / 9.0f, 2.0f / 9.0f, 1.0f / 9.0f };

/* Taps that sum to zero, so the settled output is zero at any input. */
static const float diffTaps[ 2 ] = { 1.0f, -1.0f };

static void firCase ( void )
{
    fir_t driver;
    float history[ 5 ];
    uint32_t i = 0;
    uint8_t flat = TRUE;

    printf ( "fir\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( firInit ( NULL, rampTaps, history, 3u, 0.0f ) == FALSE ) );
    check ( "NULL taps are rejected",
            ( uint8_t ) ( firInit ( &driver, NULL, history, 3u, 0.0f ) == FALSE ) );
    check ( "a NULL history is rejected",
            ( uint8_t ) ( firInit ( &driver, rampTaps, NULL, 3u, 0.0f ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( firInit ( &driver, rampTaps, history, 0u, 0.0f ) == FALSE ) );

    /*
     * y[n] = 1*x[n] + 2*x[n-1] + 3*x[n-2], with the history starting at zero.
     * This is the check that pins the tap order: an implementation that walked
     * the history forward would compute 3*x[n] + 2*x[n-1] + 1*x[n-2] and give
     * 30, 80, 140 where this gives 10, 40, 100. It also runs past the end of
     * the history buffer twice, so it covers the index wrap.
     */
    check ( "Init", firInit ( &driver, rampTaps, history, 3u, 0.0f ) );

    firIteration ( &driver, 10.0f );
    check ( "taps[0] meets the newest sample",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 10.0f, 0.001f ) ) );

    firIteration ( &driver, 20.0f );
    check ( "and taps[1] the one before it",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 40.0f, 0.001f ) ) );

    firIteration ( &driver, 30.0f );
    check ( "and taps[2] the one before that",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 100.0f, 0.001f ) ) );

    firIteration ( &driver, 40.0f );
    check ( "the history wraps without disturbing the order",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 160.0f, 0.001f ) ) );

    firIteration ( &driver, 50.0f );
    check ( "and keeps wrapping",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 220.0f, 0.001f ) ) );

    /* A single delay tap, which is the tap order stated the other way round. */
    check ( "Init a delay", firInit ( &driver, delayTaps, history, 2u, 0.0f ) );

    firIteration ( &driver, 1.0f );
    check ( "a delay filter holds the first sample back",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 0.0f, 0.001f ) ) );

    firIteration ( &driver, 2.0f );
    check ( "and then reports the previous sample",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 1.0f, 0.001f ) ) );

    firIteration ( &driver, 3.0f );
    check ( "one sample behind, forever",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 2.0f, 0.001f ) ) );

    /*
     * The rectangular window written out as taps has to agree with what maf
     * computes, which is the clearest statement of what this module is: maf
     * is this filter with the taps fixed and a running sum instead of a loop.
     */
    check ( "Init a box", firInit ( &driver, boxTaps, history, 4u, 0.0f ) );

    firIteration ( &driver, 1.0f );
    check ( "a four tap box averages one sample against three zeros",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 0.25f, 0.0001f ) ) );

    firIteration ( &driver, 2.0f );
    firIteration ( &driver, 3.0f );
    firIteration ( &driver, 4.0f );
    check ( "and the four sample mean once it is full",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 2.5f, 0.0001f ) ) );

    firIteration ( &driver, 5.0f );
    check ( "and follows the window along",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 3.5f, 0.0001f ) ) );

    /*
     * Init fills the history with inputInit rather than zero, so a filter
     * switched onto a live signal starts settled instead of climbing from
     * nothing. For a unity gain tap set the settled output is inputInit.
     */
    check ( "Init settled on 1000",
            firInit ( &driver, triangleTaps, history, 5u, 1000.0f ) );
    check ( "the output is settled before the first sample",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 1000.0f, 0.01f ) ) );

    flat = TRUE;

    for ( i = 0; i < 50u; ++i )
    {
        firIteration ( &driver, 1000.0f );

        if ( near ( firGetOutput ( &driver ), 1000.0f, 0.01f ) == FALSE )
        {
            flat = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and there is no settling transient at all", flat );

    /*
     * The same tap set at Nyquist. An alternating input meets alternating tap
     * signs, so the gain is the alternating sum of the taps, 1/9, which is an
     * exact statement about this filter rather than a measurement of it.
     */
    check ( "re-init the triangle",
            firInit ( &driver, triangleTaps, history, 5u, 0.0f ) );

    flat = TRUE;

    for ( i = 0; i < 40u; ++i )
    {
        firIteration ( &driver, ( ( i % 2u ) == 0u ) ? 1.0f : -1.0f );

        if ( i >= 10u )
        {
            /* The sign alternates with the input, so it is checked too. */
            if ( near ( firGetOutput ( &driver ),
                        ( ( i % 2u ) == 0u ) ? ( 1.0f / 9.0f ) : ( -1.0f / 9.0f ),
                        0.0001f ) == FALSE )
            {
                flat = FALSE;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "a triangular window cuts Nyquist to a ninth, on every sample", flat );

    /*
     * Taps that sum to zero. The settled output is inputInit times that sum,
     * so it is zero whatever the input was, and firInit has to say so rather
     * than reporting inputInit.
     */
    check ( "Init a differentiator settled on 500",
            firInit ( &driver, diffTaps, history, 2u, 500.0f ) );
    check ( "taps summing to zero settle on zero, not on the input",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 0.0f, 0.001f ) ) );

    firIteration ( &driver, 500.0f );
    check ( "a steady input stays at zero",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 0.0f, 0.001f ) ) );

    firIteration ( &driver, 600.0f );
    check ( "and a step shows up as its own size",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 100.0f, 0.001f ) ) );

    firIteration ( &driver, 600.0f );
    check ( "then falls back to zero",
            ( uint8_t ) ( near ( firGetOutput ( &driver ), 0.0f, 0.001f ) ) );
}

/* ------------------------------------------------------- fir, i32 variant */

/* The same {1,2,3} in Q16, so the outputs must be the same integers. */
static const int32_t rampTapsQ16[ 3 ] = { 65536, 131072, 196608 };

/* One tap of exactly a half, which is where rounding and truncation differ. */
static const int32_t halfTapQ16[ 1 ] = { 32768 };

/* Three unity taps, to overflow a 32 bit accumulator on a large input. */
static const int32_t unityTapsQ16[ 3 ] = { 65536, 65536, 65536 };

static void firi32Case ( void )
{
    firi32_t driver;
    int32_t history[ 3 ];

    printf ( "fir i32 variant\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( firIniti32 ( NULL, rampTapsQ16, history, 3u, 0 ) == FALSE ) );
    check ( "NULL taps are rejected",
            ( uint8_t ) ( firIniti32 ( &driver, NULL, history, 3u, 0 ) == FALSE ) );
    check ( "a NULL history is rejected",
            ( uint8_t ) ( firIniti32 ( &driver, rampTapsQ16, NULL, 3u, 0 ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( firIniti32 ( &driver, rampTapsQ16, history, 0u, 0 ) == FALSE ) );

    /* The float case again, exactly, because these taps and samples are whole. */
    check ( "Init", firIniti32 ( &driver, rampTapsQ16, history, 3u, 0 ) );

    firIterationi32 ( &driver, 10 );
    check ( "it agrees with the float variant on the first sample",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 10 ) );

    firIterationi32 ( &driver, 20 );
    check ( "and the second",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 40 ) );

    firIterationi32 ( &driver, 30 );
    check ( "and the third",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 100 ) );

    firIterationi32 ( &driver, 40 );
    check ( "and across the history wrap",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 160 ) );

    /*
     * The shift rounds rather than truncating. A single tap of a half against
     * an input of one is exactly half a count, which a truncating shift reports
     * as zero and a rounding one as one; against three it is one and a half,
     * truncating to one and rounding to two. Both signs are checked, because a
     * rounded negative is where a naive add-then-shift gets it backwards.
     */
    check ( "Init a half tap", firIniti32 ( &driver, halfTapQ16, history, 1u, 0 ) );

    firIterationi32 ( &driver, 1 );
    check ( "half a count rounds up rather than truncating to zero",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 1 ) );

    firIterationi32 ( &driver, -1 );
    check ( "and rounds away from zero on the negative side",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == -1 ) );

    firIterationi32 ( &driver, 3 );
    check ( "one and a half rounds to two",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 2 ) );

    firIterationi32 ( &driver, -3 );
    check ( "and minus one and a half to minus two",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == -2 ) );

    /*
     * Three unity taps against a million. Each product is 65536000000 and the
     * sum reaches 196608000000, which is ninety times what an int32_t holds,
     * so this fails outright on an accumulator that is not int64_t.
     */
    check ( "Init unity taps", firIniti32 ( &driver, unityTapsQ16, history, 3u, 0 ) );

    firIterationi32 ( &driver, 1000000 );
    check ( "a million through one unity tap",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 1000000 ) );

    firIterationi32 ( &driver, 1000000 );
    firIterationi32 ( &driver, 1000000 );
    check ( "and three of them, which overflows a 32 bit accumulator",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 3000000 ) );

    /* Init settles the integer variant the same way the float one does. */
    check ( "Init settled on 1234",
            firIniti32 ( &driver, halfTapQ16, history, 1u, 1234 ) );
    check ( "a half tap settles on half the input, rounded",
            ( uint8_t ) ( firGetOutputi32 ( &driver ) == 617 ) );
}

/* -------------------------------------------------------------- goertzel */

#define GZ_RATE         1000.0f
#define GZ_TONE         100.0f
#define GZ_BLOCK        100u

/* Feeds one block of a sine of the given frequency and amplitude. */
static void feedTone ( goertzel_t* driver, float frequency, float amplitude,
                        float phase, uint32_t samples )
{
    uint32_t i = 0;

    for ( i = 0; i < samples; ++i )
    {
        goertzelIteration ( driver,
                            amplitude * sinf ( ( ( 2.0f * 3.14159265f * frequency *
                                                    ( float ) i ) / GZ_RATE ) + phase ) );
    }
}

static void goertzelInitCase ( void )
{
    goertzel_t driver;

    printf ( "goertzelInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( goertzelInit ( NULL, GZ_RATE, GZ_TONE, GZ_BLOCK ) == FALSE ) );
    check ( "a zero sample rate is rejected",
            ( uint8_t ) ( goertzelInit ( &driver, 0.0f, GZ_TONE, GZ_BLOCK ) == FALSE ) );
    check ( "a zero frequency is rejected",
            ( uint8_t ) ( goertzelInit ( &driver, GZ_RATE, 0.0f, GZ_BLOCK ) == FALSE ) );
    check ( "a frequency at Nyquist is rejected",
            ( uint8_t ) ( goertzelInit ( &driver, GZ_RATE, 500.0f, GZ_BLOCK ) == FALSE ) );
    check ( "a frequency above Nyquist is rejected",
            ( uint8_t ) ( goertzelInit ( &driver, GZ_RATE, 600.0f, GZ_BLOCK ) == FALSE ) );

    /*
     * The recurrence carries two state words, so a block of one has nothing
     * for the second to hold. A block merely too short for the tone is not
     * rejected — it gives a leaky answer rather than a broken one.
     */
    check ( "a zero block length is rejected",
            ( uint8_t ) ( goertzelInit ( &driver, GZ_RATE, GZ_TONE, 0u ) == FALSE ) );
    check ( "a block length of one is rejected",
            ( uint8_t ) ( goertzelInit ( &driver, GZ_RATE, GZ_TONE, 1u ) == FALSE ) );
    check ( "a block length of two is accepted",
            goertzelInit ( &driver, GZ_RATE, GZ_TONE, 2u ) );

    check ( "a full init succeeds",
            goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    check ( "and starts with nothing to report",
            ( uint8_t ) ( ( goertzelIsReady ( &driver ) == FALSE ) &&
                          ( goertzelGetPower ( &driver ) == 0.0f ) &&
                          ( goertzelGetMagnitude ( &driver ) == 0.0f ) ) );
}

static void goertzelMeasureCase ( void )
{
    goertzel_t driver;

    printf ( "goertzel measurement\n" );

    /*
     * A hundred samples at a thousand hertz holds exactly ten cycles of a
     * hundred hertz tone, so the bin sits on the tone and there is no leakage
     * to allow for. The module normalizes by the block length, so the answer
     * is the amplitude itself rather than something proportional to it.
     */
    check ( "Init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 1.0f, 0.0f, GZ_BLOCK );
    check ( "a unit amplitude tone reads one",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 1.0f, 0.005f ) );
    check ( "and its power reads one too",
            ( uint8_t ) near ( goertzelGetPower ( &driver ), 1.0f, 0.01f ) );

    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 5.0f, 0.0f, GZ_BLOCK );
    check ( "amplitude five reads five",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 5.0f, 0.02f ) );
    check ( "and its power is the square of that",
            ( uint8_t ) near ( goertzelGetPower ( &driver ), 25.0f, 0.2f ) );

    /*
     * Phase independence. The magnitude of a bin does not depend on where the
     * block happened to start, and an implementation that dropped the cross
     * term would lose exactly that property.
     */
    check ( "re-init for phase", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 1.0f, 1.0f, GZ_BLOCK );
    check ( "a tone starting at a different phase reads the same",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 1.0f, 0.005f ) );

    check ( "re-init for quarter phase", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 1.0f, 1.5707963f, GZ_BLOCK );
    check ( "and a cosine reads the same as a sine",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 1.0f, 0.005f ) );

    /* Nothing there at all. */
    check ( "re-init for silence", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 0.0f, 0.0f, GZ_BLOCK );
    check ( "silence reads zero",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 0.0f, 0.001f ) );
}

static void goertzelRejectCase ( void )
{
    goertzel_t driver;
    uint32_t i = 0;

    printf ( "goertzel selectivity\n" );

    /*
     * Every one of these also holds a whole number of cycles in the block, so
     * each sits exactly on another bin and is orthogonal to the one being
     * measured. Anything above the float noise floor here would mean the
     * cross term or the coefficient is wrong.
     */
    check ( "Init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, 200.0f, 1.0f, 0.0f, GZ_BLOCK );
    check ( "an octave above reads nothing",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 0.0f, 0.001f ) );

    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, 50.0f, 1.0f, 0.0f, GZ_BLOCK );
    check ( "an octave below reads nothing",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 0.0f, 0.001f ) );

    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, 110.0f, 1.0f, 0.0f, GZ_BLOCK );
    check ( "the very next bin, ten hertz away, reads nothing",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 0.0f, 0.001f ) );

    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );

    for ( i = 0; i < GZ_BLOCK; ++i )
    {
        goertzelIteration ( &driver, 1.0f );
    }

    check ( "and a steady dc input reads nothing either",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 0.0f, 0.001f ) );

    /*
     * The coefficient is taken at the frequency asked for rather than snapped
     * to the nearest exact bin, so a tone between bins is measured low rather
     * than being quietly rounded onto one. Five hertz off centre, half a bin,
     * reads about 0.65 — which is the leakage the file banner describes and
     * the reason it tells the caller how to choose a block length.
     */
    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, 105.0f, 1.0f, 0.0f, GZ_BLOCK );
    check ( "a tone half a bin off centre leaks, and is not snapped onto the bin",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 0.65f, 0.03f ) );
}

static void goertzelBlockCase ( void )
{
    goertzel_t driver;
    uint32_t i = 0;

    printf ( "goertzel blocks\n" );

    check ( "Init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );

    /* Nothing is ready until the block is full. */
    for ( i = 0; i < ( GZ_BLOCK - 1u ); ++i )
    {
        goertzelIteration ( &driver, 1.0f );
    }

    check ( "one sample short of a block, nothing is ready",
            ( uint8_t ) ( goertzelIsReady ( &driver ) == FALSE ) );

    goertzelIteration ( &driver, 1.0f );
    check ( "the last sample of the block completes it",
            goertzelIsReady ( &driver ) );
    check ( "and reading it clears the flag, as bininpGetRisingValue does",
            ( uint8_t ) ( goertzelIsReady ( &driver ) == FALSE ) );

    /*
     * The block reloads inside the iteration, so a caller that never collects
     * a result loses that result and nothing else: the boundaries stay where
     * they were. Two blocks go by here with no read between them, the first at
     * amplitude one and the second at amplitude two, and what comes out has to
     * be the second.
     */
    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 1.0f, 0.0f, GZ_BLOCK );
    feedTone ( &driver, GZ_TONE, 2.0f, 0.0f, GZ_BLOCK );

    check ( "a missed result leaves the next one ready",
            goertzelIsReady ( &driver ) );
    check ( "and it is the second block, not a mixture of the two",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 2.0f, 0.01f ) );

    /* Power is the square of magnitude, by construction rather than by luck. */
    check ( "power is the square of magnitude",
            ( uint8_t ) near ( goertzelGetPower ( &driver ),
                                goertzelGetMagnitude ( &driver ) *
                                goertzelGetMagnitude ( &driver ), 0.001f ) );
}

static void goertzelResetCase ( void )
{
    goertzel_t driver;

    printf ( "goertzelReset\n" );

    check ( "Init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );

    /*
     * Half a block of the wrong thing, abandoned. Without the reset those
     * fifty samples would sit in the state and the next block would be a
     * mixture of the two.
     */
    feedTone ( &driver, 300.0f, 9.0f, 0.0f, GZ_BLOCK / 2u );
    goertzelReset ( &driver );
    feedTone ( &driver, GZ_TONE, 1.0f, 0.0f, GZ_BLOCK );

    check ( "an abandoned part block does not pollute the next one",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 1.0f, 0.005f ) );
    check ( "and the block that followed the reset completed on time",
            goertzelIsReady ( &driver ) );

    /*
     * A reset also drops the last result rather than leaving it standing. A
     * stale power that survives a reset is a trap: the caller asked to start
     * again and would read the old answer as the new one.
     */
    check ( "re-init", goertzelInit ( &driver, GZ_RATE, GZ_TONE, GZ_BLOCK ) );
    feedTone ( &driver, GZ_TONE, 3.0f, 0.0f, GZ_BLOCK );
    check ( "a result is standing before the reset",
            ( uint8_t ) near ( goertzelGetMagnitude ( &driver ), 3.0f, 0.02f ) );

    goertzelReset ( &driver );
    check ( "and the reset clears it",
            ( uint8_t ) ( goertzelGetPower ( &driver ) == 0.0f ) );
    check ( "along with the ready flag",
            ( uint8_t ) ( goertzelIsReady ( &driver ) == FALSE ) );
}

int main ( void )
{
    firCase ( );
    printf ( "\n" );
    firi32Case ( );
    printf ( "\n" );
    goertzelInitCase ( );
    printf ( "\n" );
    goertzelMeasureCase ( );
    printf ( "\n" );
    goertzelRejectCase ( );
    printf ( "\n" );
    goertzelBlockCase ( );
    printf ( "\n" );
    goertzelResetCase ( );

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
