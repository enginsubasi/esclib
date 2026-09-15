/*
 * Covers nothing, and that is the point.
 *
 * Every other test in this tree exercises one module. The library forbids a
 * module from including another module's header, so the only place modules
 * ever meet is in caller code — and until this file there was no caller code
 * in the repository at all. Each module being correct on its own says nothing
 * about whether the units, the buffer sizes and the callback shapes line up
 * when they are stacked, which is the first thing a consumer finds out and the
 * last thing anything here checked.
 *
 * Three stacks, each one a real shape rather than a demonstration:
 *
 *   the receive path     comstxetx -> comgenbuf -> comsafe
 *   the motion loop      ramp -> encoder -> pid -> dcMotor
 *   the measurement      pack -> median -> biquad -> interp
 *
 * Asserts rather than printing, so it needs no output.txt and returns non zero
 * on failure.
 */

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "comstxetx.h"
#include "comgenbuf.h"
#include "comsafe.h"
#include "crc16.h"
#include "ramp.h"
#include "encoder.h"
#include "pid.h"
#include "dcmotor.h"
#include "pack.h"
#include "median.h"
#include "biquad.h"
#include "interp.h"

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
    float diff = 0;

    diff = got - want;

    if ( diff < 0 )
    {
        diff = -diff;
    }
    else
    {
        /* Intentionally blank. */
    }

    if ( diff <= tolerance )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* =================================================== the receive path ===== */

/*
 * comstxetx hands a finished payload to a callback that carries no context
 * pointer, which is the shape every callback in this library has. A real ISR
 * reaches the queue the same way this does: through a file scope pointer.
 */
static comgenbuf_t* deliveryQueue = NULL;
static uint32_t deliveredCount = 0;

static void deliverToQueue ( uint8_t* buffer, uint32_t index )
{
    ++deliveredCount;

    ( void ) comgenbufPush ( deliveryQueue, buffer, index );
}

static void receivePathCase ( void )
{
    comstxetx_t link;
    comgenbuf_t queue;
    comsafe_t sender;
    comsafe_t receiver;

    uint8_t linkRx[ 128 ];
    uint8_t linkTx[ 128 ];
    uint8_t queueStore[ 256 ];
    uint8_t safeTx[ 64 ];
    uint8_t safeRx[ 64 ];
    uint8_t safeTxR[ 64 ];
    uint8_t safeRxR[ 64 ];

    uint8_t wire[ 256 ];
    uint8_t popped[ 64 ];

    uint32_t safeLength = 0;
    uint32_t wireLength = 0;
    uint32_t popLength = 0;
    uint32_t i = 0;

    /*
     * A payload holding all three framing bytes. A comsafe frame carries a
     * two byte check whose value the caller does not choose, so a real link
     * will meet this case by itself sooner or later; putting the bytes in on
     * purpose makes it happen on the first frame instead.
     */
    static const uint8_t payload[ 5 ] = { 0x02u, 0x03u, 0x10u, 0x41u, 0x42u };

    printf ( "the receive path: comstxetx -> comgenbuf -> comsafe\n" );

    deliveryQueue = &queue;
    deliveredCount = 0;

    check ( "the packet queue initializes",
            comgenbufInit ( &queue, queueStore, 256u ) );
    check ( "the transport initializes",
            comstxetxInit ( &link, linkRx, linkTx, 128u, 128u,
                            0x02u, 0x03u, 0x10u, 100u, crc16, deliverToQueue ) );
    check ( "the safety sender initializes",
            comsafeInit ( &sender, safeRx, safeTx, 64u, 64u, 0x0A0Bu, 50u, crc16 ) );
    check ( "and the safety receiver",
            comsafeInit ( &receiver, safeRxR, safeTxR, 64u, 64u, 0x0A0Bu, 50u, crc16 ) );

    /*
     * The sending half of the stack. The safety layer wraps the payload, the
     * transport wraps that, and what comes out is what goes on the wire.
     */
    check ( "the safety layer wraps the payload",
            comsafeBuildFrame ( &sender, payload, 5u, &safeLength ) );
    check ( "the transport wraps that",
            comstxetxBuildFrame ( &link, safeTx, safeLength, &wireLength ) );

    for ( i = 0; i < wireLength; ++i )
    {
        wire[ i ] = linkTx[ i ];
    }

    /*
     * The transport had to escape at least one byte, or this stack would not
     * be testing transparency at all: the payload it was handed contains STX,
     * ETX and DLE, so the frame on the wire must be longer than the naive sum.
     */
    check ( "and the wire frame is longer than the payload plus the framing, "
            "because bytes were escaped",
            ( uint8_t ) ( wireLength > ( safeLength + 4u ) ) );

    /* The receiving half, one byte at a time as an interrupt would. */
    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, wire[ i ] );
    }

    comstxetxEvaluate ( &link );

    check ( "the transport delivered exactly one packet",
            ( uint8_t ) ( deliveredCount == 1u ) );
    check ( "and it reached the queue",
            ( uint8_t ) ( comgenbufGetCount ( &queue ) == 1u ) );
    check ( "with the length the safety layer produced",
            ( uint8_t ) ( comgenbufPeekLength ( &queue ) == safeLength ) );

    popLength = comgenbufPop ( &queue, popped, 64u );
    check ( "the main loop pops it whole",
            ( uint8_t ) ( popLength == safeLength ) );

    check ( "and the safety layer accepts it",
            comsafeCheckFrame ( &receiver, popped, popLength ) );
    check ( "the channel is in service",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );
    check ( "and the payload survived all three layers unchanged",
            ( uint8_t ) ( ( comsafeGetPayloadLength ( &receiver ) == 5u ) &&
                          ( comsafeGetPayload ( &receiver )[ 0 ] == 0x02u ) &&
                          ( comsafeGetPayload ( &receiver )[ 1 ] == 0x03u ) &&
                          ( comsafeGetPayload ( &receiver )[ 2 ] == 0x10u ) &&
                          ( comsafeGetPayload ( &receiver )[ 4 ] == 0x42u ) ) );

    /*
     * Three frames arriving back to back with no main loop in between. This is
     * what the queue is for and what circBuf could not do: the boundaries have
     * to survive so the safety layer gets three separate frames rather than
     * one run of bytes.
     */
    deliveredCount = 0;

    for ( i = 0; i < 3u; ++i )
    {
        ( void ) comsafeBuildFrame ( &sender, payload, 5u, &safeLength );
        ( void ) comstxetxBuildFrame ( &link, safeTx, safeLength, &wireLength );

        for ( popLength = 0; popLength < wireLength; ++popLength )
        {
            comstxetxReceive ( &link, linkTx[ popLength ] );
        }

        comstxetxEvaluate ( &link );
    }

    check ( "three frames arrive with no main loop between them",
            ( uint8_t ) ( deliveredCount == 3u ) );
    check ( "and the queue holds three, not one run of bytes",
            ( uint8_t ) ( comgenbufGetCount ( &queue ) == 3u ) );

    for ( i = 0; i < 3u; ++i )
    {
        popLength = comgenbufPop ( &queue, popped, 64u );

        check ( "each pops whole", ( uint8_t ) ( popLength == safeLength ) );
        check ( "and each is in sequence for the safety layer",
                comsafeCheckFrame ( &receiver, popped, popLength ) );
    }

    check ( "the queue is empty", ( uint8_t ) ( comgenbufGetCount ( &queue ) == 0u ) );
    check ( "and nothing was rejected anywhere in the stack",
            ( uint8_t ) ( ( comstxetxGetRejectCount ( &link ) == 0u ) &&
                          ( comsafeGetErrorCount ( &receiver ) == 0u ) &&
                          ( comgenbufGetDropCount ( &queue ) == 0u ) ) );

    /*
     * A bit flipped on the wire. The transport's own check catches it, so the
     * safety layer never sees the frame at all — which is the division of
     * labour the two are supposed to have.
     */
    ( void ) comsafeBuildFrame ( &sender, payload, 5u, &safeLength );
    ( void ) comstxetxBuildFrame ( &link, safeTx, safeLength, &wireLength );

    for ( i = 0; i < wireLength; ++i )
    {
        wire[ i ] = linkTx[ i ];
    }

    wire[ 4 ] = ( uint8_t ) ( wire[ 4 ] ^ 0x40u );

    deliveredCount = 0;

    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, wire[ i ] );
    }

    comstxetxEvaluate ( &link );

    check ( "a corrupted frame is stopped by the transport",
            ( uint8_t ) ( comstxetxGetRejectCount ( &link ) == 1u ) );
    check ( "so nothing reaches the queue",
            ( uint8_t ) ( ( deliveredCount == 0u ) &&
                          ( comgenbufGetCount ( &queue ) == 0u ) ) );
    check ( "and the safety layer is untouched by it",
            ( uint8_t ) ( comsafeGetState ( &receiver ) == ( uint8_t ) CS_OK ) );

    /*
     * And here is what that costs, which is the kind of thing only a stack can
     * show. The transport ate the corrupted frame, so from the safety layer's
     * side a frame simply never arrived — which is a *lost* frame, and comsafe
     * treats a gap in the sequence as a fault by design. The next good frame
     * is therefore refused too, and the caller has to decide the channel is
     * trustworthy again. That is not a defect in either module; it is the
     * contract of the one above meeting the behaviour of the one below.
     */
    ( void ) comsafeBuildFrame ( &sender, payload, 5u, &safeLength );
    ( void ) comstxetxBuildFrame ( &link, safeTx, safeLength, &wireLength );

    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, linkTx[ i ] );
    }
    comstxetxEvaluate ( &link );

    popLength = comgenbufPop ( &queue, popped, 64u );

    check ( "the frame after a transport level drop is refused by the safety layer",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, popped, popLength ) == FALSE ) );
    check ( "because a frame the transport ate is a frame the safety layer lost",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_SEQUENCE ) );

    comsafeReset ( &receiver );

    /*
     * A frame the transport is perfectly happy with, delivered twice. Only the
     * safety layer can see that, because only it counts frames.
     */
    ( void ) comsafeBuildFrame ( &sender, payload, 5u, &safeLength );
    ( void ) comstxetxBuildFrame ( &link, safeTx, safeLength, &wireLength );

    for ( i = 0; i < wireLength; ++i )
    {
        wire[ i ] = linkTx[ i ];
    }

    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, wire[ i ] );
    }
    comstxetxEvaluate ( &link );

    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, wire[ i ] );
    }
    comstxetxEvaluate ( &link );

    check ( "a duplicated frame passes the transport twice",
            ( uint8_t ) ( comgenbufGetCount ( &queue ) == 2u ) );

    popLength = comgenbufPop ( &queue, popped, 64u );
    check ( "the first copy is accepted",
            comsafeCheckFrame ( &receiver, popped, popLength ) );

    popLength = comgenbufPop ( &queue, popped, 64u );
    check ( "and the second is caught by the layer that counts frames",
            ( uint8_t ) ( comsafeCheckFrame ( &receiver, popped, popLength ) == FALSE ) );
    check ( "as a sequence failure",
            ( uint8_t ) ( comsafeGetLastError ( &receiver ) == ( uint8_t ) CS_SEQUENCE ) );
}

/* ===================================================== the motion loop ==== */

/* Quadrature levels for a position, in the order a shaft turning forward
   produces them: 00, 01, 11, 10. */
static const uint8_t quadA[ 4 ] = { 0u, 0u, 1u, 1u };
static const uint8_t quadB[ 4 ] = { 0u, 1u, 1u, 0u };

/* The bridge pins, recorded rather than driven. */
static uint8_t bridgeHighLevel = 0;
static uint8_t bridgeLowLevel = 0;
static float lastDuty = -1.0f;

static void probeHigh ( uint8_t state )
{
    bridgeHighLevel = state;
}

static void probeLow ( uint8_t state )
{
    bridgeLowLevel = state;
}

static void probePwm ( float duty )
{
    lastDuty = duty;
}

static void motionLoopCase ( void )
{
    ramp_t profile;
    encoder_t shaft;
    pidc_t controller;
    dcmotor_t motor;

    int32_t plantCounts = 0;
    float plantFraction = 0.0f;
    float setpoint = 0.0f;
    float error = 0.0f;
    float drive = 0.0f;
    float magnitude = 0.0f;
    uint32_t i = 0;
    uint8_t dutyInRange = TRUE;
    uint8_t everReversed = FALSE;

    printf ( "the motion loop: ramp -> encoder -> pid -> dcMotor\n" );

    check ( "the profile initializes",
            rampInit ( &profile, 200.0f, 400.0f, 0.01f, 0.0f ) );
    check ( "the encoder initializes at the levels the pins are sitting at",
            encoderInit ( &shaft, quadA[ 0 ], quadB[ 0 ] ) );
    check ( "the controller initializes",
            pidInit ( &controller, 0.30f, 0.0f, 0.0f, 0.01f,
                      100.0f, -100.0f, 100.0f, -100.0f,
                      100.0f, -100.0f, 1.0f, -1.0f ) );
    check ( "and the motor",
            dcMotorInit ( &motor, probeHigh, probeLow, probePwm ) );

    /*
     * Four modules, four different ideas of what a number means: the ramp
     * works in units, the encoder in counts, the PID in whatever the error is
     * and the motor in a duty from zero to one. Nothing converts between them
     * for the caller, and the conversions below are the whole reason a test
     * like this exists.
     */
    for ( i = 0; i < 400u; ++i )
    {
        rampIteration ( &profile, 100.0f );
        setpoint = rampGetOutput ( &profile );

        error = setpoint - ( float ) encoderGetPosition ( &shaft );

        pidControl ( &controller, error );
        drive = pidGetOutput ( &controller );

        /*
         * The PID's output is signed and its limits were set to plus and minus
         * one; the motor takes a direction and an unsigned duty. Splitting one
         * into the other two is the caller's job, and it is where a sign
         * mistake would put full reverse torque on the shaft.
         */
        magnitude = drive;

        if ( magnitude < 0.0f )
        {
            magnitude = -magnitude;
        }
        else
        {
            /* Intentionally blank. */
        }

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

        dcMotorSetSpeed ( &motor, magnitude );

        if ( ( lastDuty < 0.0f ) || ( lastDuty > 1.0f ) )
        {
            dutyInRange = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( drive < 0.0f )
        {
            everReversed = TRUE;
        }
        else
        {
            /* Intentionally blank. */
        }

        /*
         * A plant crude enough to be obviously not the point: the shaft moves
         * at a rate proportional to the duty, in the direction of the bridge,
         * and the encoder is driven from where it ends up.
         */
        if ( bridgeHighLevel == TRUE )
        {
            plantFraction += ( lastDuty * 8.0f );
        }
        else if ( bridgeLowLevel == TRUE )
        {
            plantFraction -= ( lastDuty * 8.0f );
        }
        else
        {
            /* Intentionally blank. */
        }

        while ( plantFraction >= 1.0f )
        {
            plantFraction -= 1.0f;
            ++plantCounts;
            encoderUpdate ( &shaft, quadA[ plantCounts & 3 ], quadB[ plantCounts & 3 ] );
        }

        while ( plantFraction <= -1.0f )
        {
            plantFraction += 1.0f;
            --plantCounts;
            encoderUpdate ( &shaft, quadA[ plantCounts & 3 ], quadB[ plantCounts & 3 ] );
        }
    }

    check ( "the duty handed to the motor never left zero to one", dutyInRange );

    /*
     * The encoder must agree with the plant. If it did not, either the
     * quadrature order above is wrong or the decoder is, and the loop would be
     * closing on a position that is not where the shaft is.
     */
    check ( "the encoder tracked the shaft exactly",
            ( uint8_t ) ( encoderGetPosition ( &shaft ) == plantCounts ) );
    check ( "and never had to guess a missed step",
            ( uint8_t ) ( encoderGetErrorCount ( &shaft ) == 0u ) );

    check ( "the profile reached its target",
            rampIsArrived ( &profile ) );
    check ( "and the loop closed on it",
            near ( ( float ) encoderGetPosition ( &shaft ), 100.0f, 3.0f ) );

    /*
     * Reversing at all is what makes the interlock matter. If the loop never
     * overshot, the dcMotor reversal guard would never have been exercised
     * here and this stack would be proving less than it looks.
     */
    check ( "the loop reversed at least once on the way",
            everReversed );
}

/* ================================================== the measurement ====== */

static void measurementCase ( void )
{
    median_t spike;
    biquad_t hum;
    interp_t calibration;

    float window[ 5 ];
    float sorted[ 5 ];

    /* Counts to kilograms, from a two point calibration with a curve in it. */
    static const float countsTable[ 4 ] = { 0.0f, 100000.0f, 200000.0f, 300000.0f };
    static const float kgTable[ 4 ]     = { 0.0f, 5.0f, 10.2f, 15.0f };

    uint8_t raw[ 3 ];
    int32_t counts = 0;
    float filtered = 0.0f;
    float kilograms = 0.0f;
    float peak = 0.0f;
    uint32_t i = 0;

    printf ( "the measurement chain: pack -> median -> biquad -> interp\n" );

    check ( "the spike filter initializes",
            medianInit ( &spike, window, sorted, 5u, 0.0f ) );
    check ( "the mains notch initializes",
            biquadInitNotch ( &hum, 1000.0f, 50.0f, 30.0f ) );
    check ( "and the calibration table",
            interpInit ( &calibration, countsTable, kgTable, 4u ) );

    /*
     * The notch is settled onto the reading it is about to see. biquad's own
     * banner says to do this and says why: a filter attached to a signal
     * already sitting at a hundred thousand otherwise spends its whole
     * settling time climbing there from zero, and at a q of thirty that
     * settling time is very long indeed. Leaving it out is the first thing
     * this stack got wrong.
     */
    biquadReset ( &hum, 100000.0f );

    /*
     * A converter reading of minus ten counts, as three bytes. This is the
     * pattern a hand written reader turns into sixteen million, and it is the
     * first thing in the chain, so everything after it would be wrong too.
     */
    raw[ 0 ] = 0xFFu;
    raw[ 1 ] = 0xFFu;
    raw[ 2 ] = 0xF6u;

    check ( "a negative converter reading arrives as a negative number",
            ( uint8_t ) ( packGetI24be ( raw, 0u ) == -10 ) );

    /* A steady reading with one spike in it. */
    for ( i = 0; i < 40u; ++i )
    {
        raw[ 0 ] = 0x01u;
        raw[ 1 ] = 0x86u;
        raw[ 2 ] = 0xA0u;                   /* 100000 counts */

        counts = packGetI24be ( raw, 0u );

        if ( i == 20u )
        {
            counts = 300000;                /* one sample of nonsense */
        }
        else
        {
            /* Intentionally blank. */
        }

        medianIteration ( &spike, ( float ) counts );
        filtered = medianGetOutput ( &spike );

        if ( i > 10u )
        {
            if ( near ( filtered, 100000.0f, 1.0f ) == FALSE )
            {
                peak = filtered;
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

    check ( "a single spike never reaches the output of the median",
            near ( peak, 0.0f, 0.5f ) );

    kilograms = interpCalculate ( &calibration, filtered );
    check ( "and the calibration turns the counts into kilograms",
            near ( kilograms, 5.0f, 0.01f ) );

    /*
     * Between two table points the curve interpolates rather than snapping to
     * an entry, which is the whole difference between interp and
     * searchClosest.
     */
    kilograms = interpCalculate ( &calibration, 150000.0f );
    check ( "a reading between two table points interpolates",
            near ( kilograms, 7.6f, 0.01f ) );

    /* Mains hum on top of the reading, and the notch that removes it. */
    peak = 0.0f;

    for ( i = 0; i < 2000u; ++i )
    {
        filtered = 100000.0f +
                    ( 500.0f * sinf ( ( 2.0f * 3.14159265f * 50.0f *
                                        ( float ) i ) / 1000.0f ) );

        biquadIteration ( &hum, filtered );

        if ( i > 1000u )
        {
            filtered = biquadGetOutput ( &hum ) - 100000.0f;

            if ( filtered < 0.0f )
            {
                filtered = -filtered;
            }
            else
            {
                /* Intentionally blank. */
            }

            if ( filtered > peak )
            {
                peak = filtered;
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

    check ( "five hundred counts of mains hum come out below ten",
            ( uint8_t ) ( peak < 10.0f ) );

    /*
     * And the measurement underneath the hum is still there. A filter that
     * removed the tone by removing everything would pass the check above and
     * be useless.
     */
    check ( "while the reading underneath it is untouched",
            near ( biquadGetOutput ( &hum ), 100000.0f, 20.0f ) );

    kilograms = interpCalculate ( &calibration, biquadGetOutput ( &hum ) );
    check ( "so the calibrated result is still five kilograms",
            near ( kilograms, 5.0f, 0.01f ) );
}

/* ------------------------------------------- the order those two go in ---- */

static void bridgeOrderCase ( void )
{
    dcmotor_t motor;

    printf ( "the order dcMotorBridgeState and dcMotorSetSpeed go in\n" );

    /*
     * The loop above calls dcMotorBridgeState and then dcMotorSetSpeed, every
     * iteration, and that order is not arbitrary. A reversal between two driven
     * directions zeroes the duty before it moves the pins — which is the
     * interlock doing its job — so a caller that sets the speed first and the
     * direction second has the interlock wipe the value it just wrote, and the
     * motor stops every time the loop changes its mind.
     *
     * Both modules are behaving exactly as documented. The bug only exists
     * where they meet, which is why it is here and not in DcMotor_Test.
     */
    check ( "Init", dcMotorInit ( &motor, probeHigh, probeLow, probePwm ) );

    dcMotorBridgeState ( &motor, BRIDGE_FORWARD );
    dcMotorSetSpeed ( &motor, 0.6f );

    /* Direction first, then speed: the speed survives. */
    dcMotorBridgeState ( &motor, BRIDGE_BACKWARD );
    dcMotorSetSpeed ( &motor, 0.7f );

    check ( "direction then speed leaves the motor driving",
            near ( dcMotorGetSpeed ( &motor ), 0.7f, 0.0001f ) );

    /* Speed first, then direction: the reversal wipes it. */
    dcMotorSetSpeed ( &motor, 0.7f );
    dcMotorBridgeState ( &motor, BRIDGE_FORWARD );

    check ( "speed then direction leaves it stopped, every reversal",
            near ( dcMotorGetSpeed ( &motor ), 0.0f, 0.0001f ) );
}

int main ( void )
{
    receivePathCase ( );
    printf ( "\n" );
    motionLoopCase ( );
    printf ( "\n" );
    measurementCase ( );
    printf ( "\n" );
    bridgeOrderCase ( );

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
