/**
  ******************************************************************************
  *
  * @file      NotchNoFpu.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      16/09/2026
  *
  * @brief     A mains notch designed at boot on a part with no FPU, using
  *            cordic for the cosine and q16 for the arithmetic.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 16/09/2026 Created. @n
  *
  ******************************************************************************
  *
  * biquad's four designers take a corner in hertz and are float only, and the
  * module says why: computing a cosine at boot pulls the whole software float
  * library onto the part the fixed point variant exists to serve. So the
  * fixed point variant takes coefficients, and the caller is told to work them
  * out on a host.
  *
  * That answer fails for exactly one case, which is the case the notch exists
  * for: the mains. A product that ships to a 50 Hz country and a 60 Hz one
  * cannot carry one compile time constant, and neither can one that samples at
  * a rate its own clock trims. Those coefficients have to be worked out on the
  * part, at boot, and cordic is the cosine that does it.
  *
  * Not one float appears in the design below. The reference values printed
  * beside it were computed on a host and written down.
  *
  * Build and run it with the other samples:
  *     sh scripts/samples.sh
  *
  ******************************************************************************
  */

#include <stdio.h>

#include "cordic.h"
#include "q16.h"
#include "biquad.h"

/* The loop this runs in, and the hum it is there to remove. */
#define SAMPLE_RATE     1000u
#define MAINS           50u

/* The width of the notch: q = 4.0 in Q16. Higher is narrower. */
#define NOTCH_Q         262144

/* The signal the converter sees: a slow measurement with mains hum on it. */
#define SIGNAL_HZ       3u
#define SIGNAL_COUNTS   400
#define HUM_COUNTS      300

#define RUN_LENGTH      4000u
#define SETTLED         2000u

/*
 * A frequency as a fraction of the sample rate is that fraction of a turn, so
 * the whole expression is integer and there is no pi anywhere in it. This is
 * the one line that makes a binary angle worth having.
 */
static uint32_t stepAngle ( uint32_t frequency )
{
    return ( ( uint32_t ) ( ( ( ( uint64_t ) frequency ) << 32 ) /
                            ( uint64_t ) SAMPLE_RATE ) );
}

/* A tone of the given amplitude in plain counts, from the binary angle. */
static int32_t tone ( uint32_t phase, int32_t amplitude )
{
    return ( ( int32_t ) ( ( ( ( int64_t ) cordicSin ( phase ) ) *
                             amplitude ) >> 16 ) );
}

/* Prints a Q16 value as a signed decimal with four places, without a float. */
static void printQ16 ( const char* name, int32_t value )
{
    int32_t whole = 0;
    int32_t fraction = 0;
    const char* sign = "";

    if ( value < 0 )
    {
        sign = "-";
        value = -value;
    }

    whole = value >> 16;
    fraction = ( ( ( value & 0xFFFF ) * 10000 ) + 32768 ) >> 16;

    if ( fraction >= 10000 )
    {
        fraction = fraction - 10000;
        whole = whole + 1;
    }

    printf ( "  %-4s %s%ld.%04ld\n", name, sign, ( long ) whole,
             ( long ) fraction );
}

int main ( void )
{
    biquadi32_t notch;
    uint32_t w0 = 0;
    uint32_t phase = 0;
    uint32_t humPhase = 0;
    uint32_t i = 0;
    int32_t sine = 0;
    int32_t cosine = 0;
    int32_t alpha = 0;
    int32_t a0 = 0;
    int32_t b0 = 0;
    int32_t b1 = 0;
    int32_t a2 = 0;
    int32_t sample = 0;
    int32_t output = 0;
    int32_t inputPeak = 0;
    int32_t outputPeak = 0;

    printf ( "A %lu Hz notch designed at boot, in a %lu Hz loop, with no FPU\n\n",
             ( unsigned long ) MAINS, ( unsigned long ) SAMPLE_RATE );

    /* ---------------------------------------------------- the design */

    w0 = stepAngle ( MAINS );

    cordicSinCos ( w0, &sine, &cosine );

    /* alpha = sin ( w0 ) / ( 2q ), and then the notch's five coefficients,
       every one of them in Q16. */
    alpha = q16Div ( sine, 2 * NOTCH_Q );

    a0 = 65536 + alpha;

    b0 = q16Div ( 65536, a0 );
    b1 = q16Div ( -2 * cosine, a0 );
    a2 = q16Div ( 65536 - alpha, a0 );

    if ( biquadIniti32 ( &notch, b0, b1, b0, b1, a2 ) == FALSE )
    {
        printf ( "the coefficients were refused\n" );
        return ( 1 );
    }

    printf ( "designed:\n" );
    printQ16 ( "b0", b0 );
    printQ16 ( "b1", b1 );
    printQ16 ( "b2", b0 );
    printQ16 ( "a1", b1 );
    printQ16 ( "a2", a2 );
    printf ( "  (a host computes 0.9628, -1.8314, 0.9628, -1.8314, 0.9256)\n\n" );

    /* ------------------------------------------------- what it does */

    biquadReseti32 ( &notch, 0 );

    for ( i = 0; i < RUN_LENGTH; ++i )
    {
        phase = phase + stepAngle ( SIGNAL_HZ );
        humPhase = humPhase + w0;

        sample = tone ( phase, SIGNAL_COUNTS ) + tone ( humPhase, HUM_COUNTS );

        biquadIterationi32 ( &notch, sample );
        output = biquadGetOutputi32 ( &notch );

        /* Measure once the filter has settled, over a whole signal cycle. */
        if ( i >= SETTLED )
        {
            if ( sample > inputPeak )
            {
                inputPeak = sample;
            }

            if ( output > outputPeak )
            {
                outputPeak = output;
            }
        }
    }

    printf ( "a %ld count measurement with %ld counts of hum on it:\n",
             ( long ) SIGNAL_COUNTS, ( long ) HUM_COUNTS );
    printf ( "  peak in   %ld counts\n", ( long ) inputPeak );
    printf ( "  peak out  %ld counts\n", ( long ) outputPeak );
    printf ( "\nthe hum is gone and the measurement is not.\n" );

    return ( 0 );
}
