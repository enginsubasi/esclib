/*
 * A position loop, end to end, using four modules of this library.
 *
 *     ramp      the setpoint, under a velocity and an acceleration limit
 *     encoder   where the shaft actually is, from two quadrature channels
 *     pid       the error turned into a drive
 *     dcMotor   the drive turned into a direction and a duty cycle
 *
 * The four disagree about what a number means, on purpose: the ramp works in
 * units, the encoder in counts, the PID in whatever the error is, and the
 * motor in a duty from zero to one. Nothing converts between them for you.
 * The conversions in the loop below are the part worth reading.
 *
 * The motor and the shaft are faked at the bottom so this builds and runs
 * anywhere. On a real board those are the only functions that change.
 *
 *     gcc -Iinc/control -Iinc/encoder -Idrv sample/MotionLoop/MotionLoop.c \
 *         src/control/ramp.c src/control/pid.c src/encoder/encoder.c \
 *         drv/dcmotor.c -lm -o motion
 */

#include <stdio.h>

#include "ramp.h"
#include "encoder.h"
#include "pid.h"
#include "dcmotor.h"

#define TS          0.01f       /* the loop runs at a hundred hertz */

/* ------------------------------------------------- the fake hardware ----- */

/* What the bridge pins and the PWM are being told, and where the shaft is. */
static uint8_t pinHigh = 0;
static uint8_t pinLow = 0;
static float pinDuty = 0.0f;

static float shaftPosition = 0.0f;
static int32_t shaftCounts = 0;

/* Quadrature levels in the order a shaft turning forward produces them. */
static const uint8_t channelA[ 4 ] = { 0u, 0u, 1u, 1u };
static const uint8_t channelB[ 4 ] = { 0u, 1u, 1u, 0u };

static void driveHigh ( uint8_t state )
{
    pinHigh = state;
}

static void driveLow ( uint8_t state )
{
    pinLow = state;
}

static void drivePwm ( float duty )
{
    pinDuty = duty;
}

/* Turns the shaft according to the pins, and reports its quadrature. */
static void turnShaft ( encoder_t* shaft )
{
    if ( pinHigh == TRUE )
    {
        shaftPosition += ( pinDuty * 6.0f );
    }
    else if ( pinLow == TRUE )
    {
        shaftPosition -= ( pinDuty * 6.0f );
    }
    else
    {
        /* Intentionally blank. */
    }

    while ( ( float ) shaftCounts < ( shaftPosition - 1.0f ) )
    {
        ++shaftCounts;
        encoderUpdate ( shaft, channelA[ shaftCounts & 3 ], channelB[ shaftCounts & 3 ] );
    }

    while ( ( float ) shaftCounts > ( shaftPosition + 1.0f ) )
    {
        --shaftCounts;
        encoderUpdate ( shaft, channelA[ shaftCounts & 3 ], channelB[ shaftCounts & 3 ] );
    }
}

/* ------------------------------------------------------------ the loop --- */

int main ( void )
{
    ramp_t profile;
    encoder_t shaft;
    pidc_t controller;
    dcmotor_t motor;

    float setpoint = 0.0f;
    float error = 0.0f;
    float drive = 0.0f;
    float duty = 0.0f;
    uint32_t i = 0;

    /* Limits in units per second, because ramp takes ts at Init. */
    if ( rampInit ( &profile, 150.0f, 300.0f, TS, 0.0f ) != TRUE )
    {
        printf ( "ramp init failed\n" );
        return ( 1 );
    }

    /*
     * The levels the pins are sitting at, not zero. Starting from an assumed
     * zero would make the first encoderUpdate read as a transition that never
     * happened, and the loop would begin one count out.
     */
    if ( encoderInit ( &shaft, channelA[ 0 ], channelB[ 0 ] ) != TRUE )
    {
        printf ( "encoder init failed\n" );
        return ( 1 );
    }

    /*
     * The output limits are what make the next few lines work: the PID is told
     * to produce something between minus one and one, which is a signed duty,
     * and splitting that into a direction and an unsigned duty is the caller's
     * job below.
     */
    if ( pidInit ( &controller, 0.25f, 0.0f, 0.0f, TS,
                   1000.0f, -1000.0f, 1000.0f, -1000.0f,
                   1000.0f, -1000.0f, 1.0f, -1.0f ) != TRUE )
    {
        printf ( "pid init failed\n" );
        return ( 1 );
    }

    if ( dcMotorInit ( &motor, driveHigh, driveLow, drivePwm ) != TRUE )
    {
        printf ( "motor init failed\n" );
        return ( 1 );
    }

    printf ( "step   setpoint   position   drive   duty  dir\n" );

    for ( i = 0; i < 300u; ++i )
    {
        /* Where the setpoint is allowed to be this tick. */
        rampIteration ( &profile, 120.0f );
        setpoint = rampGetOutput ( &profile );

        /* Units against counts. Here they happen to be one to one. */
        error = setpoint - ( float ) encoderGetPosition ( &shaft );

        pidControl ( &controller, error );
        drive = pidGetOutput ( &controller );

        /* A signed drive into a direction and an unsigned duty. */
        duty = drive;

        if ( duty < 0.0f )
        {
            duty = -duty;
        }
        else
        {
            /* Intentionally blank. */
        }

        /*
         * Direction first, then speed. A reversal between two driven
         * directions zeroes the duty before it moves the pins, so setting the
         * speed first would have that wipe the value just written and the
         * motor would stop every time the loop changed its mind.
         */
        if ( drive > 0.0f )
        {
            dcMotorBridgeState ( &motor, BRIDGE_FORWARD );
        }
        else if ( drive < 0.0f )
        {
            dcMotorBridgeState ( &motor, BRIDGE_BACKWARD );
        }
        else
        {
            dcMotorBridgeState ( &motor, BRIDGE_NO );
        }

        dcMotorSetSpeed ( &motor, duty );

        turnShaft ( &shaft );

        if ( ( i % 40u ) == 39u )
        {
            printf ( "%4u   %8.2f   %8d  %6.3f  %5.3f   %s\n",
                     ( unsigned ) i, ( double ) setpoint,
                     ( int ) encoderGetPosition ( &shaft ),
                     ( double ) drive, ( double ) dcMotorGetSpeed ( &motor ),
                     ( pinHigh == TRUE ) ? "fwd" : ( ( pinLow == TRUE ) ? "rev" : "off" ) );
        }
    }

    printf ( "\narrived: %s, final position %d counts\n",
             ( rampIsArrived ( &profile ) == TRUE ) ? "yes" : "no",
             ( int ) encoderGetPosition ( &shaft ) );

    /*
     * A non zero error count means the loop sampled the quadrature too slowly
     * and the decoder saw both channels change at once. It refuses to guess
     * which way that went, so a count here is the signal to sample faster.
     */
    printf ( "encoder steps the decoder refused to guess: %u\n",
             ( unsigned ) encoderGetErrorCount ( &shaft ) );

    return ( 0 );
}
