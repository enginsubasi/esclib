/*
 * Covers dcmotor.
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

static void probeHigh ( uint8_t state )
{
    highState = state;
    ++highCalls;
}

static void probeLow ( uint8_t state )
{
    lowState = state;
    ++lowCalls;
}

static void probePwm ( float duty )
{
    pwmValue = duty;
    ++pwmCalls;
}

static void probeReset ( void )
{
    highState = 0xAAu;
    lowState = 0xAAu;
    pwmValue = -1.0f;
    highCalls = 0;
    lowCalls = 0;
    pwmCalls = 0;
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

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    bridgeCase ( );

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
