/*
 * A weighing scale, end to end, using five modules of this library.
 *
 *     pack     three bytes off a 24 bit converter, sign extended
 *     median   one bad sample thrown away rather than averaged in
 *     biquad   the 50 Hz the cable picks up, notched out
 *     interp   counts turned into kilograms by a calibration table
 *
 * Nothing here is a hardware driver. The converter is faked at the bottom of
 * the file so the example builds and runs anywhere; on a real board that one
 * function is the only thing that changes.
 *
 * Build it with the five module sources it names:
 *
 *     gcc -Iinc/pack -Iinc/filter -Iinc/math sample/FilteredScale/FilteredScale.c \
 *         src/pack/pack.c src/filter/median.c src/filter/biquad.c \
 *         src/math/interp.c -lm -o scale
 */

#include <math.h>
#include <stdio.h>

#include "pack.h"
#include "median.h"
#include "biquad.h"
#include "interp.h"

/* The converter is read at a kilohertz and the mains here is 50 Hz. */
#define SAMPLE_RATE     1000.0f
#define MAINS           50.0f

/*
 * The calibration, taken with known masses on the pan. It is not a straight
 * line — no load cell is — which is the reason interp exists and mathMap would
 * not do.
 */
static const float counts[ 5 ] = { 0.0f, 50000.0f, 100000.0f, 150000.0f, 200000.0f };
static const float kilograms[ 5 ] = { 0.0f, 2.48f, 5.00f, 7.56f, 10.15f };

/*
 * Stands in for the converter. Returns three bytes, most significant first,
 * which is how an HX711 and most of its relatives hand a reading over: a
 * steady load, mains hum on top, and one wild sample now and then.
 */
static void readConverter ( uint32_t sample, uint8_t* out )
{
    int32_t value = 0;

    value = 100000;

    value += ( int32_t ) ( 900.0f * sinf ( ( 2.0f * 3.14159265f * MAINS *
                                             ( float ) sample ) / SAMPLE_RATE ) );

    if ( ( sample % 250u ) == 137u )
    {
        value = 8000000;
    }

    out[ 0 ] = ( uint8_t ) ( ( value >> 16 ) & 0xFF );
    out[ 1 ] = ( uint8_t ) ( ( value >> 8 ) & 0xFF );
    out[ 2 ] = ( uint8_t ) ( value & 0xFF );
}

int main ( void )
{
    median_t spike;
    biquad_t hum;
    interp_t scale;

    float window[ 5 ];
    float sorted[ 5 ];

    uint8_t raw[ 3 ];
    int32_t reading = 0;
    float value = 0.0f;
    float mass = 0.0f;
    uint32_t i = 0;

    /*
     * Every Init here returns a status and every one of them is checked. That
     * is the contract the whole library keeps: on FALSE the driver is left
     * untouched, so carrying on would be using a struct full of stack.
     */
    if ( medianInit ( &spike, window, sorted, 5u, 100000.0f ) != TRUE )
    {
        printf ( "median init failed\n" );
        return ( 1 );
    }

    if ( biquadInitNotch ( &hum, SAMPLE_RATE, MAINS, 30.0f ) != TRUE )
    {
        printf ( "notch init failed\n" );
        return ( 1 );
    }

    if ( interpInit ( &scale, counts, kilograms, 5u ) != TRUE )
    {
        printf ( "calibration init failed\n" );
        return ( 1 );
    }

    /*
     * The notch is settled onto the load that is already on the pan. Without
     * this it starts from zero and, at a q of 30, takes thousands of samples
     * to climb to the reading — which looks exactly like a scale that drifts.
     */
    biquadReset ( &hum, 100000.0f );

    printf ( "sample    raw      after median   after notch    kg\n" );

    for ( i = 0; i < 1000u; ++i )
    {
        readConverter ( i, raw );

        /*
         * Three bytes into a number. packGetI24be is the whole reason this
         * line is one call: there is no 24 bit type, so nothing in C sign
         * extends it, and the version that forgets reports a light negative
         * load as sixteen million.
         */
        reading = packGetI24be ( raw, 0u );

        /* One wild sample is dropped rather than averaged in. */
        medianIteration ( &spike, ( float ) reading );

        /* The mains hum the cable picked up. */
        biquadIteration ( &hum, medianGetOutput ( &spike ) );
        value = biquadGetOutput ( &hum );

        /* Counts into kilograms, along the curve rather than a straight line. */
        mass = interpCalculate ( &scale, value );

        if ( ( i % 200u ) == 199u )
        {
            printf ( "%6u  %8d   %10.1f   %10.1f  %6.3f\n",
                     ( unsigned ) i, ( int ) reading,
                     ( double ) medianGetOutput ( &spike ),
                     ( double ) value, ( double ) mass );
        }
    }

    /*
     * interpInRange is separate from the value for a reason: past either end
     * of the table the reading is held rather than extrapolated, and a caller
     * has to be able to tell a saturated scale from a broken one.
     */
    printf ( "\nfinal reading %.3f kg, inside the calibrated range: %s\n",
             ( double ) mass,
             ( interpInRange ( &scale, value ) == TRUE ) ? "yes" : "no" );

    return ( 0 );
}
