/*
 * Covers dcmotor: the bridge states, the duty cycle, and the reversal
 * interlock between them.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The whole module is pin moves through injected callbacks, so the callbacks
 * here record what they were told to do and every check reads that record. A
 * driver that called nothing would look identical to one that called the right
 * things if only the return value were checked.
 */

#include <stddef.h>
#include <stdio.h>

#include "dcmotor.h"

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

/* ----------------------------------------------------------- pin probes */

static uint8_t highState = 0xAAu;
static uint8_t lowState = 0xAAu;
static float pwmValue = -1.0f;

static uint32_t highCalls = 0;
static uint32_t lowCalls = 0;
static uint32_t pwmCalls = 0;

/*
 * Order of the first call to each probe since the last reset. Checking that
 * the duty reached zero is not enough on a reversal: it has to have reached
 * zero before the bridge pins moved, and only an order marker can tell those
 * two apart.
 */
static uint32_t sequence = 0;
static uint32_t highOrder = 0;
static uint32_t lowOrder = 0;
static uint32_t pwmOrder = 0;

static void probeHigh ( uint8_t state )
{
    highState = state;
    ++highCalls;
    ++sequence;

    if ( highOrder == 0u )
    {
        highOrder = sequence;
    }
    else
    {
        /* Intentionally blank. */
    }
}

static void probeLow ( uint8_t state )
{
    lowState = state;
    ++lowCalls;
    ++sequence;

    if ( lowOrder == 0u )
    {
        lowOrder = sequence;
    }
    else
    {
        /* Intentionally blank. */
    }
}

static void probePwm ( float duty )
{
    pwmValue = duty;
    ++pwmCalls;
    ++sequence;

    if ( pwmOrder == 0u )
    {
        pwmOrder = sequence;
    }
    else
    {
        /* Intentionally blank. */
    }
}

/* A nan without including math.h, which no test in this tree needs otherwise. */
static float makeNan ( void )
{
    volatile float zero = 0.0f;

    return ( zero / zero );
}

static void probeReset ( void )
{
    highState = 0xAAu;
    lowState = 0xAAu;
    pwmValue = -1.0f;
    highCalls = 0;
    lowCalls = 0;
    pwmCalls = 0;
    sequence = 0;
    highOrder = 0;
    lowOrder = 0;
    pwmOrder = 0;
}

/* ------------------------------------------------------------------ init */

static void initCase ( void )
{
    dcmotor_t driver;

    printf ( "dcMotorInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( dcMotorInit ( NULL, probeHigh, probeLow, probePwm ) == FALSE ) );
    check ( "a NULL high side callback is rejected",
            ( uint8_t ) ( dcMotorInit ( &driver, NULL, probeLow, probePwm ) == FALSE ) );
    check ( "a NULL low side callback is rejected",
            ( uint8_t ) ( dcMotorInit ( &driver, probeHigh, NULL, probePwm ) == FALSE ) );
    check ( "a NULL pwm callback is rejected",
            ( uint8_t ) ( dcMotorInit ( &driver, probeHigh, probeLow, NULL ) == FALSE ) );

    /*
     * A rejected Init must not have touched the pins. This is the part that
     * matters on real hardware: a half configured driver that has already
     * driven the bridge is worse than one that refused.
     */
    probeReset ( );
    ( void ) dcMotorInit ( &driver, NULL, probeLow, probePwm );
    check ( "a rejected init moves no pins",
            ( uint8_t ) ( ( highCalls == 0u ) && ( lowCalls == 0u ) &&
                          ( pwmCalls == 0u ) ) );

    probeReset ( );
    check ( "a full init succeeds",
            dcMotorInit ( &driver, probeHigh, probeLow, probePwm ) );

    /*
     * Init leaves the motor stopped and the bridge released, which is the only
     * safe state to come up in.
     */
    check ( "it sets the duty cycle to zero", ( uint8_t ) ( pwmValue == 0.0f ) );
    check ( "and calls the pwm callback exactly once", ( uint8_t ) ( pwmCalls == 1u ) );
    check ( "it releases the high side", ( uint8_t ) ( highState == FALSE ) );
    check ( "and the low side", ( uint8_t ) ( lowState == FALSE ) );
    check ( "having driven each bridge pin once",
            ( uint8_t ) ( ( highCalls == 1u ) && ( lowCalls == 1u ) ) );
}

/* ---------------------------------------------------------- bridge states */

static void bridgeCase ( void )
{
    dcmotor_t driver;

    printf ( "dcMotorBridgeState\n" );

    check ( "Init", dcMotorInit ( &driver, probeHigh, probeLow, probePwm ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_FORWARD );
    check ( "forward drives the high side only",
            ( uint8_t ) ( ( highState == TRUE ) && ( lowState == FALSE ) ) );
    check ( "and touches each pin once",
            ( uint8_t ) ( ( highCalls == 1u ) && ( lowCalls == 1u ) ) );
    check ( "without touching the pwm", ( uint8_t ) ( pwmCalls == 0u ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_BACKWARD );
    check ( "backward drives the low side only",
            ( uint8_t ) ( ( highState == FALSE ) && ( lowState == TRUE ) ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_LOCK );
    check ( "lock drives both",
            ( uint8_t ) ( ( highState == TRUE ) && ( lowState == TRUE ) ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_NO );
    check ( "no drive releases both",
            ( uint8_t ) ( ( highState == FALSE ) && ( lowState == FALSE ) ) );

    /*
     * An unrecognised state has to land somewhere, and the safe place is the
     * released bridge rather than whatever the pins happened to be holding.
     */
    dcMotorBridgeState ( &driver, BRIDGE_LOCK );
    probeReset ( );
    dcMotorBridgeState ( &driver, 99u );
    check ( "an unrecognised state falls back to releasing the bridge",
            ( uint8_t ) ( ( highState == FALSE ) && ( lowState == FALSE ) ) );
    check ( "and it does so by driving the pins, not by leaving them",
            ( uint8_t ) ( ( highCalls == 1u ) && ( lowCalls == 1u ) ) );

    /* Going straight from one direction to the other must not leave both on. */
    dcMotorBridgeState ( &driver, BRIDGE_FORWARD );
    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_BACKWARD );
    check ( "reversing direction never leaves both sides driven",
            ( uint8_t ) ( ( highState == FALSE ) && ( lowState == TRUE ) ) );
}

/* ------------------------------------------------------------------ speed */

static void speedCase ( void )
{
    dcmotor_t driver;

    printf ( "dcMotorSetSpeed\n" );

    check ( "Init", dcMotorInit ( &driver, probeHigh, probeLow, probePwm ) );
    check ( "a fresh driver reports a duty of zero",
            ( uint8_t ) ( dcMotorGetSpeed ( &driver ) == 0.0f ) );

    probeReset ( );
    dcMotorSetSpeed ( &driver, 0.5f );
    check ( "a duty inside the range reaches the callback",
            ( uint8_t ) ( pwmValue == 0.5f ) );
    check ( "and is reported back",
            ( uint8_t ) ( dcMotorGetSpeed ( &driver ) == 0.5f ) );
    check ( "with one call, and no pin moves",
            ( uint8_t ) ( ( pwmCalls == 1u ) && ( highCalls == 0u ) &&
                          ( lowCalls == 0u ) ) );

    /*
     * Clamped rather than rejected, so what the driver installed is what
     * dcMotorGetSpeed reports, not what the caller asked for.
     */
    probeReset ( );
    dcMotorSetSpeed ( &driver, 1.5f );
    check ( "a duty above one clamps to full drive",
            ( uint8_t ) ( ( pwmValue == 1.0f ) &&
                          ( dcMotorGetSpeed ( &driver ) == 1.0f ) ) );

    probeReset ( );
    dcMotorSetSpeed ( &driver, -0.25f );
    check ( "a negative duty clamps to a standstill",
            ( uint8_t ) ( ( pwmValue == 0.0f ) &&
                          ( dcMotorGetSpeed ( &driver ) == 0.0f ) ) );

    /*
     * A nan compares false against both bounds, so the obvious two sided
     * clamp would hand one straight to the hardware. This is the same defect
     * pidInit guards against by rejecting a zero ts.
     */
    probeReset ( );
    dcMotorSetSpeed ( &driver, 1.0f );
    dcMotorSetSpeed ( &driver, makeNan ( ) );
    check ( "a nan lands at a standstill rather than reaching the hardware",
            ( uint8_t ) ( ( pwmValue == 0.0f ) &&
                          ( dcMotorGetSpeed ( &driver ) == 0.0f ) ) );

    dcMotorSetSpeed ( &driver, 0.0f );
    check ( "the lower endpoint goes through unchanged",
            ( uint8_t ) ( dcMotorGetSpeed ( &driver ) == 0.0f ) );

    dcMotorSetSpeed ( &driver, 1.0f );
    check ( "and so does full drive",
            ( uint8_t ) ( dcMotorGetSpeed ( &driver ) == 1.0f ) );
}

/* -------------------------------------------------------------- reversal */

static void reversalCase ( void )
{
    dcmotor_t driver;

    printf ( "reversal\n" );

    check ( "Init", dcMotorInit ( &driver, probeHigh, probeLow, probePwm ) );

    /*
     * The hazard this guards. A motor turning at speed is a generator, and
     * throwing the bridge across it puts the supply and the back emf in
     * series through the winding. Checking the duty ended at zero is not
     * enough — it has to have reached zero before the pins moved.
     */
    dcMotorBridgeState ( &driver, BRIDGE_FORWARD );
    dcMotorSetSpeed ( &driver, 0.8f );
    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_BACKWARD );

    check ( "reversing zeroes the duty",
            ( uint8_t ) ( dcMotorGetSpeed ( &driver ) == 0.0f ) );
    check ( "and tells the hardware so",
            ( uint8_t ) ( ( pwmCalls == 1u ) && ( pwmValue == 0.0f ) ) );
    check ( "before either bridge pin moves",
            ( uint8_t ) ( ( pwmOrder != 0u ) && ( highOrder != 0u ) &&
                          ( lowOrder != 0u ) && ( pwmOrder < highOrder ) &&
                          ( pwmOrder < lowOrder ) ) );
    check ( "and the direction still changed",
            ( uint8_t ) ( ( highState == FALSE ) && ( lowState == TRUE ) ) );

    /* The other way round is the same move. */
    dcMotorSetSpeed ( &driver, 0.6f );
    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_FORWARD );
    check ( "backward to forward is the same reversal",
            ( uint8_t ) ( ( dcMotorGetSpeed ( &driver ) == 0.0f ) &&
                          ( pwmCalls == 1u ) ) );

    /*
     * Nothing else is a reversal. Releasing, locking, and picking a direction
     * up from a released bridge all leave the duty where the caller put it.
     */
    dcMotorSetSpeed ( &driver, 0.7f );
    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_LOCK );
    check ( "locking the bridge leaves the duty alone",
            ( uint8_t ) ( ( dcMotorGetSpeed ( &driver ) == 0.7f ) &&
                          ( pwmCalls == 0u ) ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_NO );
    check ( "releasing it leaves the duty alone too",
            ( uint8_t ) ( ( dcMotorGetSpeed ( &driver ) == 0.7f ) &&
                          ( pwmCalls == 0u ) ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_FORWARD );
    check ( "and taking a direction from a released bridge is not a reversal",
            ( uint8_t ) ( ( dcMotorGetSpeed ( &driver ) == 0.7f ) &&
                          ( pwmCalls == 0u ) ) );

    probeReset ( );
    dcMotorBridgeState ( &driver, BRIDGE_FORWARD );
    check ( "nor is asking for the direction already held",
            ( uint8_t ) ( ( dcMotorGetSpeed ( &driver ) == 0.7f ) &&
                          ( pwmCalls == 0u ) ) );

    /*
     * The duty is not restored after a reversal. Putting the previous torque
     * back one pwm period later is the hazard again with a delay on it.
     */
    dcMotorSetSpeed ( &driver, 0.9f );
    dcMotorBridgeState ( &driver, BRIDGE_BACKWARD );
    check ( "the duty is not restored once the reversal is done",
            ( uint8_t ) ( dcMotorGetSpeed ( &driver ) == 0.0f ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    bridgeCase ( );
    printf ( "\n" );
    speedCase ( );
    printf ( "\n" );
    reversalCase ( );

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
