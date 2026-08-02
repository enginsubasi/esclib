/*
 * Covers comat and comstxetx.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Both modules are byte driven state machines split across three call sites:
 * Receive from an interrupt, Evaluate from the main loop and TimeoutCounter
 * from a tick. The callbacks here record what they were handed so the test can
 * check the frame that came out, not merely that something did.
 */

#include <stddef.h>
#include <stdio.h>

#include "comat.h"
#include "comstxetx.h"

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

/* --------------------------------------------------------- comat probes */

static uint32_t atProcessCalls = 0;
static uint32_t atTriggerCalls = 0;
static uint32_t atLastRxLength = 0;
static uint8_t atLastFrame[ 32 ];
static uint32_t atLastTxLength = 0;

static void atPacketProcess ( uint8_t* rxBuf, uint32_t rxInd, uint8_t* txBuf, uint32_t* txInd )
{
    uint32_t i = 0;

    ++atProcessCalls;
    atLastRxLength = rxInd;

    for ( i = 0; ( i < rxInd ) && ( i < 32u ); ++i )
    {
        atLastFrame[ i ] = rxBuf[ i ];
    }

    /* Reply with two fixed bytes so the trigger has something to report. */
    txBuf[ 0 ] = 'O';
    txBuf[ 1 ] = 'K';
    ( *txInd ) = 2u;
}

static void atTxTrigger ( uint8_t* txBuf, uint32_t txInd )
{
    ( void ) txBuf;

    ++atTriggerCalls;
    atLastTxLength = txInd;
}

static void atReset ( void )
{
    atProcessCalls = 0;
    atTriggerCalls = 0;
    atLastRxLength = 0;
    atLastTxLength = 0;
}

static void atFeed ( comat_t* driver, const char* text )
{
    uint32_t i = 0;

    for ( i = 0; text[ i ] != '\0'; ++i )
    {
        comatReceive ( driver, ( uint8_t ) text[ i ] );
    }
}

/* ------------------------------------------------------------ comatInit */

static void comatInitCase ( void )
{
    comat_t driver;
    uint8_t rxBuffer[ 16 ];
    uint8_t txBuffer[ 16 ];

    printf ( "comatInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comatInit ( NULL, rxBuffer, txBuffer, 16u, 16u, 5u,
                                      atPacketProcess, atTxTrigger ) == FALSE ) );
    check ( "a NULL rx buffer is rejected",
            ( uint8_t ) ( comatInit ( &driver, NULL, txBuffer, 16u, 16u, 5u,
                                      atPacketProcess, atTxTrigger ) == FALSE ) );
    check ( "a NULL tx buffer is rejected",
            ( uint8_t ) ( comatInit ( &driver, rxBuffer, NULL, 16u, 16u, 5u,
                                      atPacketProcess, atTxTrigger ) == FALSE ) );
    check ( "a NULL packet callback is rejected",
            ( uint8_t ) ( comatInit ( &driver, rxBuffer, txBuffer, 16u, 16u, 5u,
                                      NULL, atTxTrigger ) == FALSE ) );
    check ( "a NULL transmit callback is rejected",
            ( uint8_t ) ( comatInit ( &driver, rxBuffer, txBuffer, 16u, 16u, 5u,
                                      atPacketProcess, NULL ) == FALSE ) );
    check ( "a zero tx size is rejected",
            ( uint8_t ) ( comatInit ( &driver, rxBuffer, txBuffer, 16u, 0u, 5u,
                                      atPacketProcess, atTxTrigger ) == FALSE ) );

    /*
     * comatReceive stores a byte before it compares the index against rxSize,
     * so anything below three would take a write past the end of the buffer.
     */
    check ( "an rx size of 2 is rejected",
            ( uint8_t ) ( comatInit ( &driver, rxBuffer, txBuffer, 2u, 16u, 5u,
                                      atPacketProcess, atTxTrigger ) == FALSE ) );
    check ( "an rx size of 3 is the smallest accepted",
            comatInit ( &driver, rxBuffer, txBuffer, 3u, 16u, 5u,
                        atPacketProcess, atTxTrigger ) );

    rxBuffer[ 0 ] = 0xAAu;
    txBuffer[ 0 ] = 0xAAu;
    check ( "a full init succeeds",
            comatInit ( &driver, rxBuffer, txBuffer, 16u, 16u, 5u,
                        atPacketProcess, atTxTrigger ) );
    check ( "and zero fills both buffers",
            ( uint8_t ) ( ( rxBuffer[ 0 ] == 0u ) && ( txBuffer[ 0 ] == 0u ) ) );
}

/* -------------------------------------------------------- comat framing */

static void comatFramingCase ( void )
{
    comat_t driver;
    uint8_t rxBuffer[ 16 ];
    uint8_t txBuffer[ 16 ];

    printf ( "comat framing\n" );

    check ( "Init", comatInit ( &driver, rxBuffer, txBuffer, 16u, 16u, 5u,
                                atPacketProcess, atTxTrigger ) );
    atReset ( );

    /* Nothing should happen until a whole frame has arrived. */
    atFeed ( &driver, "AT+X" );
    comatEvaluate ( &driver );
    check ( "a partial frame does not fire the callback",
            ( uint8_t ) ( atProcessCalls == 0u ) );

    atFeed ( &driver, "\r\n" );
    check ( "and still not before Evaluate runs",
            ( uint8_t ) ( atProcessCalls == 0u ) );

    comatEvaluate ( &driver );
    check ( "the completed frame fires the callback once",
            ( uint8_t ) ( atProcessCalls == 1u ) );
    check ( "and the transmit trigger once", ( uint8_t ) ( atTriggerCalls == 1u ) );

    /* comat stores every byte of the frame, the leading AT and the CR LF too. */
    check ( "the frame length counts all six bytes",
            ( uint8_t ) ( atLastRxLength == 6u ) );
    check ( "the frame starts with A and T",
            ( uint8_t ) ( ( atLastFrame[ 0 ] == 'A' ) && ( atLastFrame[ 1 ] == 'T' ) ) );
    check ( "and ends with CR LF",
            ( uint8_t ) ( ( atLastFrame[ 4 ] == '\r' ) && ( atLastFrame[ 5 ] == '\n' ) ) );
    check ( "the reply length reaches the trigger",
            ( uint8_t ) ( atLastTxLength == 2u ) );

    /* A second Evaluate with nothing pending must do nothing. */
    comatEvaluate ( &driver );
    check ( "a second Evaluate does not fire again",
            ( uint8_t ) ( atProcessCalls == 1u ) );

    /* The driver is ready for the next frame straight away. */
    atFeed ( &driver, "AT\r\n" );
    comatEvaluate ( &driver );
    check ( "the next frame is accepted", ( uint8_t ) ( atProcessCalls == 2u ) );
    check ( "and is four bytes long", ( uint8_t ) ( atLastRxLength == 4u ) );
}

static void comatRejectCase ( void )
{
    comat_t driver;
    uint8_t rxBuffer[ 8 ];
    uint8_t txBuffer[ 8 ];

    printf ( "comat rejects what is not a frame\n" );

    check ( "Init", comatInit ( &driver, rxBuffer, txBuffer, 8u, 8u, 5u,
                                atPacketProcess, atTxTrigger ) );
    atReset ( );

    /* Noise before the leading A is discarded rather than buffered. */
    atFeed ( &driver, "xyz\r\n" );
    comatEvaluate ( &driver );
    check ( "bytes before a leading A are dropped",
            ( uint8_t ) ( atProcessCalls == 0u ) );

    /* An A that is not followed by a T restarts the search. */
    atFeed ( &driver, "AX" );
    atFeed ( &driver, "AT\r\n" );
    comatEvaluate ( &driver );
    check ( "an A without a T restarts the search",
            ( uint8_t ) ( atProcessCalls == 1u ) );
    check ( "and the frame that followed is the clean one",
            ( uint8_t ) ( atLastRxLength == 4u ) );

    atReset ( );

    /*
     * A frame longer than rxBuffer is discarded rather than overflowing it.
     * rxSize is 8, so nine payload bytes with no terminator must come to
     * nothing, and the driver must still accept the next clean frame.
     */
    atFeed ( &driver, "ATAAAAAAAAAAAA" );
    comatEvaluate ( &driver );
    check ( "an over long frame is discarded", ( uint8_t ) ( atProcessCalls == 0u ) );

    atFeed ( &driver, "AT\r\n" );
    comatEvaluate ( &driver );
    check ( "and the driver still works afterwards",
            ( uint8_t ) ( atProcessCalls == 1u ) );

    /* A lone LF without the CR does not complete a frame. */
    atReset ( );
    atFeed ( &driver, "AT\n" );
    comatEvaluate ( &driver );
    check ( "an LF with no CR before it does not complete a frame",
            ( uint8_t ) ( atProcessCalls == 0u ) );
}

static void comatTimeoutCase ( void )
{
    comat_t driver;
    uint8_t rxBuffer[ 16 ];
    uint8_t txBuffer[ 16 ];
    uint32_t i = 0;

    printf ( "comat timeout\n" );

    /* rxTimeout of 2 means the partial frame survives three ticks. */
    check ( "Init with rxTimeout 2",
            comatInit ( &driver, rxBuffer, txBuffer, 16u, 16u, 2u,
                        atPacketProcess, atTxTrigger ) );
    atReset ( );

    atFeed ( &driver, "AT+PART" );

    for ( i = 0; i < 3u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    /* Still within budget, so finishing the frame now must work. */
    atFeed ( &driver, "\r\n" );
    comatEvaluate ( &driver );
    check ( "a frame finished inside the timeout is accepted",
            ( uint8_t ) ( atProcessCalls == 1u ) );

    atReset ( );

    atFeed ( &driver, "AT+PART" );

    for ( i = 0; i < 4u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    atFeed ( &driver, "\r\n" );
    comatEvaluate ( &driver );
    check ( "a frame that ran out of time is discarded",
            ( uint8_t ) ( atProcessCalls == 0u ) );

    /*
     * rxTimeoutCounter is cleared wherever rxIndex returns to zero. The tick
     * driven discard above always cleared it, even before the July 2026 fix;
     * the paths that did not were Evaluate and the two resets inside Receive.
     * So the counter ran on across frames and the frame after a completed one
     * died early. Checking that requires completing a frame late in its budget
     * and then asking the next one for a full budget of its own.
     */
    atReset ( );
    atFeed ( &driver, "AT+A" );

    for ( i = 0; i < 3u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    atFeed ( &driver, "\r\n" );
    comatEvaluate ( &driver );
    check ( "a frame completed late in its budget still fires",
            ( uint8_t ) ( atProcessCalls == 1u ) );

    atFeed ( &driver, "AT+B" );

    for ( i = 0; i < 3u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    atFeed ( &driver, "\r\n" );
    comatEvaluate ( &driver );
    check ( "and the frame after it starts with a full budget, not the leftovers",
            ( uint8_t ) ( atProcessCalls == 2u ) );

    /* The tick must not touch a frame that is already complete and waiting. */
    atReset ( );
    atFeed ( &driver, "AT\r\n" );

    for ( i = 0; i < 50u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    comatEvaluate ( &driver );
    check ( "a completed frame is not timed out while it waits",
            ( uint8_t ) ( atProcessCalls == 1u ) );
}

static void comatOverflowTimeoutCase ( void )
{
    comat_t driver;
    uint8_t rxBuffer[ 8 ];
    uint8_t txBuffer[ 8 ];
    uint32_t i = 0;

    printf ( "comat timeout after an over long frame\n" );

    check ( "Init with rxSize 8 and rxTimeout 2",
            comatInit ( &driver, rxBuffer, txBuffer, 8u, 8u, 2u,
                        atPacketProcess, atTxTrigger ) );
    atReset ( );

    /*
     * The other path that used to leak the counter. A partial frame is aged
     * three ticks and then overruns the buffer, so Receive drops it rather than
     * the tick doing so.
     */
    atFeed ( &driver, "AT" );

    for ( i = 0; i < 3u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    atFeed ( &driver, "AAAAAA" );

    atFeed ( &driver, "AT+B" );

    for ( i = 0; i < 3u; ++i )
    {
        comatTimeoutCounter ( &driver );
    }

    atFeed ( &driver, "\r\n" );
    comatEvaluate ( &driver );
    check ( "the frame after a dropped over long one gets a full budget",
            ( uint8_t ) ( atProcessCalls == 1u ) );
}

/* ------------------------------------------------------ comstxetx probes */

static uint32_t sxProcessCalls = 0;
static uint32_t sxLastLength = 0;
static uint8_t sxLastFrame[ 32 ];

static void sxPacketProcess ( uint8_t* buffer, uint32_t index )
{
    uint32_t i = 0;

    ++sxProcessCalls;
    sxLastLength = index;

    for ( i = 0; ( i < index ) && ( i < 32u ); ++i )
    {
        sxLastFrame[ i ] = buffer[ i ];
    }
}

static void sxReset ( void )
{
    sxProcessCalls = 0;
    sxLastLength = 0;
}

static void sxFeed ( comstxetx_t* driver, const char* text )
{
    uint32_t i = 0;

    for ( i = 0; text[ i ] != '\0'; ++i )
    {
        comstxetxReceive ( driver, ( uint8_t ) text[ i ] );
    }
}

static void comstxetxCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 8 ];
    uint8_t txBuffer[ 8 ];
    uint32_t i = 0;

    printf ( "comstxetx\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comstxetxInit ( NULL, rxBuffer, txBuffer, 8u, 8u,
                                          0x02u, 0x03u, 5u, sxPacketProcess ) == FALSE ) );
    check ( "a NULL rx buffer is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, NULL, txBuffer, 8u, 8u,
                                          0x02u, 0x03u, 5u, sxPacketProcess ) == FALSE ) );
    check ( "a NULL callback is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 8u, 8u,
                                          0x02u, 0x03u, 5u, NULL ) == FALSE ) );
    check ( "a zero tx size is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 8u, 0u,
                                          0x02u, 0x03u, 5u, sxPacketProcess ) == FALSE ) );
    check ( "an rx size of 1 is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 1u, 8u,
                                          0x02u, 0x03u, 5u, sxPacketProcess ) == FALSE ) );

    /*
     * The same byte cannot both open and close a frame. The receive state
     * machine only tests for ETX once a frame is open, so every frame would
     * end empty.
     */
    check ( "an STX equal to the ETX is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 8u, 8u,
                                          0x02u, 0x02u, 5u, sxPacketProcess ) == FALSE ) );

    check ( "a full init succeeds",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 8u, 8u,
                            0x02u, 0x03u, 5u, sxPacketProcess ) );
    sxReset ( );

    /* Bytes before STX are ignored. */
    comstxetxReceive ( &driver, 'j' );
    comstxetxReceive ( &driver, 'u' );
    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "abc" );
    comstxetxEvaluate ( &driver );
    check ( "a frame with no ETX yet does not fire",
            ( uint8_t ) ( sxProcessCalls == 0u ) );

    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "ETX completes the frame", ( uint8_t ) ( sxProcessCalls == 1u ) );

    /*
     * Unlike comat, this stores STX at index 0 and does not store ETX at all,
     * so a three byte payload reports a length of four.
     */
    check ( "the length counts STX and the payload but not ETX",
            ( uint8_t ) ( sxLastLength == 4u ) );
    check ( "STX is at index 0", ( uint8_t ) ( sxLastFrame[ 0 ] == 0x02u ) );
    check ( "and the payload follows it",
            ( uint8_t ) ( ( sxLastFrame[ 1 ] == 'a' ) && ( sxLastFrame[ 3 ] == 'c' ) ) );

    comstxetxEvaluate ( &driver );
    check ( "a second Evaluate does not fire again",
            ( uint8_t ) ( sxProcessCalls == 1u ) );

    /* An over long frame is discarded rather than overflowing the buffer. */
    sxReset ( );
    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "0123456789" );
    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "an over long frame is discarded", ( uint8_t ) ( sxProcessCalls == 0u ) );

    /* And the driver recovers. */
    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "hi" );
    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "and the driver still works afterwards",
            ( uint8_t ) ( sxProcessCalls == 1u ) );

    /* The timeout behaves the same way it does in comat. */
    check ( "Init with rxTimeout 2",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 8u, 8u,
                            0x02u, 0x03u, 2u, sxPacketProcess ) );
    sxReset ( );

    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "ab" );

    for ( i = 0; i < 3u; ++i )
    {
        comstxetxTimeoutCounter ( &driver );
    }

    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "a frame finished inside the timeout is accepted",
            ( uint8_t ) ( sxProcessCalls == 1u ) );

    sxReset ( );
    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "ab" );

    for ( i = 0; i < 4u; ++i )
    {
        comstxetxTimeoutCounter ( &driver );
    }

    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "a frame that ran out of time is discarded",
            ( uint8_t ) ( sxProcessCalls == 0u ) );

    /*
     * The same counter leak comat had. A frame completed late in its budget
     * must not shorten the budget of the frame that follows it, and only
     * Evaluate clearing the counter makes that true.
     */
    sxReset ( );
    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "ab" );

    for ( i = 0; i < 3u; ++i )
    {
        comstxetxTimeoutCounter ( &driver );
    }

    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "a frame completed late in its budget still fires",
            ( uint8_t ) ( sxProcessCalls == 1u ) );

    comstxetxReceive ( &driver, 0x02u );
    sxFeed ( &driver, "cd" );

    for ( i = 0; i < 3u; ++i )
    {
        comstxetxTimeoutCounter ( &driver );
    }

    comstxetxReceive ( &driver, 0x03u );
    comstxetxEvaluate ( &driver );
    check ( "and the frame after it starts with a full budget",
            ( uint8_t ) ( sxProcessCalls == 2u ) );
}

int main ( void )
{
    comatInitCase ( );
    printf ( "\n" );
    comatFramingCase ( );
    printf ( "\n" );
    comatRejectCase ( );
    printf ( "\n" );
    comatTimeoutCase ( );
    printf ( "\n" );
    comatOverflowTimeoutCase ( );
    printf ( "\n" );
    comstxetxCase ( );

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
