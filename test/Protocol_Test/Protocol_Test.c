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
#include "crc16.h"

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

/*
 * A byte sum rather than a real CRC, so every expected value in this file can
 * be worked out by hand. crc16 is exercised separately in sxBuildGuardCase to
 * prove the callback type takes it with no wrapper.
 */
static void sxFeedBytes ( comstxetx_t* driver, const uint8_t* const bytes, uint32_t length )
{
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        comstxetxReceive ( driver, bytes[ i ] );
    }
}

static uint16_t sxSumChecksum ( const uint8_t* const buffer, uint32_t length )
{
    uint16_t retVal = 0;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        retVal = ( uint16_t ) ( retVal + buffer[ i ] );
    }

    return ( retVal );
}

static void sxInitCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];

    printf ( "comstxetx init contract\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comstxetxInit ( NULL, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "stx equal to etx is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x02u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "dle equal to stx is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x02u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "dle equal to etx is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x03u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "a NULL checksum is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          NULL, sxPacketProcess ) == FALSE ) );

    check ( "a NULL packetProcess is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, NULL ) == FALSE ) );

    check ( "an rxSize below two is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 1u, 32u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "a txSize below four is rejected",
            ( uint8_t ) ( comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 3u,
                                          0x02u, 0x03u, 0x10u, 10u,
                                          sxSumChecksum, sxPacketProcess ) == FALSE ) );

    check ( "a well formed init succeeds",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                            0x02u, 0x03u, 0x10u, 10u,
                            sxSumChecksum, sxPacketProcess ) );

    check ( "init leaves no frame open",
            ( uint8_t ) ( driver.rxFrameOpen == FALSE ) );
    check ( "init leaves no escape pending",
            ( uint8_t ) ( driver.rxEscape == FALSE ) );
    check ( "init clears the reject count",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 0u ) );
}

/*
 * Framing with stx 0x02, etx 0x03, dle 0x10.
 *
 * The payload is 0x41 0x03 0x10, chosen because the second byte is ETX and the
 * third is DLE. Without escaping the frame would end at the second byte, which
 * is the hole this work exists to close.
 *
 * Sum checksum over { 0x41, 0x03, 0x10 } is 0x54, so the frame carries 0x54
 * then 0x00, low byte first. Neither collides with a framing byte, so neither
 * needs an escape.
 */
static void sxEscapeCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t wire[ ] =
    {
        0x02u,                  /* STX                        */
        0x41u,                  /* letter A, no escape needed */
        0x10u, 0x03u,           /* escaped ETX in the payload */
        0x10u, 0x10u,           /* escaped DLE in the payload */
        0x54u, 0x00u,           /* checksum, low byte first   */
        0x03u                   /* ETX                        */
    };

    printf ( "comstxetx escaping\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    sxFeedBytes ( &driver, wire, ( uint32_t ) sizeof ( wire ) );
    comstxetxEvaluate ( &driver );

    check ( "the frame reached packetProcess", ( uint8_t ) ( sxProcessCalls == 1u ) );
    check ( "the payload is three bytes, checksum stripped",
            ( uint8_t ) ( sxLastLength == 3u ) );
    check ( "the escaped ETX arrived as data",
            ( uint8_t ) ( sxLastFrame[ 1 ] == 0x03u ) );
    check ( "the escaped DLE arrived as data",
            ( uint8_t ) ( sxLastFrame[ 2 ] == 0x10u ) );
    check ( "the unescaped byte is unchanged",
            ( uint8_t ) ( sxLastFrame[ 0 ] == 0x41u ) );
    check ( "nothing was rejected",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 0u ) );
}

/*
 * A single corrupted payload byte must be rejected rather than delivered, and
 * a frame that closes with fewer than two stored bytes cannot carry a checksum
 * at all.
 */
static void sxRejectCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t corrupt[ ] =
    {
        /*
         * A valid single byte frame carrying 0x41 would read
         * 0x02 0x41 0x41 0x00 0x03, the check being the sum of one byte. Here
         * the payload byte alone was flipped to 0x42, so the check still
         * claims 0x41 while the payload now sums to 0x42.
         */
        0x02u, 0x42u, 0x41u, 0x00u, 0x03u
    };
    static const uint8_t tooShort[ ] =
    {
        0x02u, 0x41u, 0x03u                 /* one stored byte, then ETX */
    };

    printf ( "comstxetx rejection\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    sxFeedBytes ( &driver, corrupt, ( uint32_t ) sizeof ( corrupt ) );
    comstxetxEvaluate ( &driver );

    check ( "a corrupted frame does not reach packetProcess",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
    check ( "and is counted",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 1u ) );

    sxFeedBytes ( &driver, tooShort, ( uint32_t ) sizeof ( tooShort ) );
    comstxetxEvaluate ( &driver );

    check ( "a frame too short to carry a checksum does not reach packetProcess",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
    check ( "and is counted too",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 2u ) );
}

/*
 * An unescaped STX inside an open frame can only mean the sender restarted,
 * because a payload STX always arrives escaped. The bytes before it are
 * abandoned and the frame begins again.
 */
static void sxResyncCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t wire[ ] =
    {
        0x02u, 0x99u, 0x99u,    /* abandoned by the STX that follows */
        0x02u,                  /* restart                           */
        0x41u, 0x41u, 0x00u,    /* payload 0x41, checksum 0x41 0x00  */
        0x03u
    };

    printf ( "comstxetx resynchronisation\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    sxFeedBytes ( &driver, wire, ( uint32_t ) sizeof ( wire ) );
    comstxetxEvaluate ( &driver );

    check ( "the restarted frame reached packetProcess",
            ( uint8_t ) ( sxProcessCalls == 1u ) );
    check ( "only the bytes after the second STX are payload",
            ( uint8_t ) ( sxLastLength == 1u ) );
    check ( "and the payload is the byte that followed it",
            ( uint8_t ) ( sxLastFrame[ 0 ] == 0x41u ) );
}

/*
 * An escape sequence straddling the end of the buffer must not write past it.
 * rxSize is deliberately smaller than the array, and the byte just beyond it
 * carries a sentinel that Init did not touch, so an overrun is visible.
 */
static void sxBoundaryCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 8 ];
    uint8_t txBuffer[ 32 ];
    static const uint8_t wire[ ] =
    {
        0x02u,                  /* STX                          */
        0x10u, 0x02u,           /* escaped STX, stores one byte */
        0x11u,                  /* stores one byte              */
        0x12u,                  /* stores one byte              */
        0x10u, 0x03u            /* escaped ETX, the fourth store fills rxSize */
    };

    printf ( "comstxetx escape at the buffer boundary\n" );

    sxReset ( );

    check ( "init with an rxSize of four",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 4u, 32u,
                            0x02u, 0x03u, 0x10u, 10u,
                            sxSumChecksum, sxPacketProcess ) );

    rxBuffer[ 4 ] = 0xEEu;

    sxFeedBytes ( &driver, wire, ( uint32_t ) sizeof ( wire ) );

    check ( "the byte past rxSize is untouched",
            ( uint8_t ) ( rxBuffer[ 4 ] == 0xEEu ) );
    check ( "the overrun frame was discarded",
            ( uint8_t ) ( driver.rxFrameOpen == FALSE ) );
    check ( "and no escape stayed pending",
            ( uint8_t ) ( driver.rxEscape == FALSE ) );

    comstxetxEvaluate ( &driver );

    check ( "nothing reached packetProcess",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
}

/*
 * The pinned regression.
 *
 * With STX no longer stored, rxIndex == 0 no longer means no frame is open. A
 * frame that has opened on STX but received no payload byte yet must still
 * time out. Keying comstxetxTimeoutCounter off rxIndex leaves it pending for
 * good, and every other case in this file passes with that bug present.
 */
static void sxOpenEmptyTimeoutCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 32 ];
    uint8_t txBuffer[ 32 ];
    uint32_t i = 0;
    static const uint8_t rest[ ] = { 0x41u, 0x41u, 0x00u, 0x03u };

    printf ( "comstxetx timeout on an opened but empty frame\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 32u, 32u,
                                    0x02u, 0x03u, 0x10u, 3u,
                                    sxSumChecksum, sxPacketProcess ) );

    comstxetxReceive ( &driver, 0x02u );

    check ( "the frame is open", ( uint8_t ) ( driver.rxFrameOpen == TRUE ) );

    for ( i = 0; i < 5u; ++i )
    {
        comstxetxTimeoutCounter ( &driver );
    }

    check ( "the empty open frame timed out",
            ( uint8_t ) ( driver.rxFrameOpen == FALSE ) );

    /*
     * The bytes that would have completed the abandoned frame must not be
     * taken as a frame of their own, because no STX opened them.
     */
    sxFeedBytes ( &driver, rest, ( uint32_t ) sizeof ( rest ) );
    comstxetxEvaluate ( &driver );

    check ( "the orphaned tail did not become a frame",
            ( uint8_t ) ( sxProcessCalls == 0u ) );
}

/*
 * The decisive case. Build a frame whose payload holds a byte equal to each
 * framing byte, then feed the wire bytes back one at a time and confirm the
 * payload survives unchanged. An encoder that disagrees with the decoder is
 * the defect this design is most exposed to, and nothing else catches it.
 */
static void sxRoundTripCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 64 ];
    uint8_t txBuffer[ 64 ];
    uint8_t wire[ 64 ];
    uint32_t wireLength = 0;
    uint32_t i = 0;
    uint8_t same = TRUE;
    static const uint8_t payload[ ] =
    {
        0x02u, 0x03u, 0x10u, 0x00u, 0xFFu, 0x41u
    };

    printf ( "comstxetx round trip\n" );

    sxReset ( );

    check ( "init", comstxetxInit ( &driver, rxBuffer, txBuffer, 64u, 64u,
                                    0x02u, 0x03u, 0x10u, 10u,
                                    sxSumChecksum, sxPacketProcess ) );

    check ( "the frame was built",
            comstxetxBuildFrame ( &driver, payload,
                                  ( uint32_t ) sizeof ( payload ), &wireLength ) );

    /*
     * Three payload bytes need an escape and none of the checksum bytes do:
     * the sum of the payload is 0x0155, so the check bytes are 0x55 and 0x01.
     * STX + 6 payload bytes + 3 escapes + 2 check bytes + ETX is 13.
     */
    check ( "the wire length accounts for the escapes",
            ( uint8_t ) ( wireLength == 13u ) );
    check ( "the frame opens with STX", ( uint8_t ) ( txBuffer[ 0 ] == 0x02u ) );
    check ( "the frame closes with ETX",
            ( uint8_t ) ( txBuffer[ wireLength - 1u ] == 0x03u ) );

    for ( i = 0; i < wireLength; ++i )
    {
        wire[ i ] = txBuffer[ i ];
    }

    sxFeedBytes ( &driver, wire, wireLength );
    comstxetxEvaluate ( &driver );

    check ( "the frame reached packetProcess", ( uint8_t ) ( sxProcessCalls == 1u ) );
    check ( "the payload length survived",
            ( uint8_t ) ( sxLastLength == ( uint32_t ) sizeof ( payload ) ) );

    for ( i = 0; i < ( uint32_t ) sizeof ( payload ); ++i )
    {
        if ( sxLastFrame[ i ] != payload[ i ] )
        {
            same = FALSE;
        }
    }

    check ( "and every payload byte survived", same );
    check ( "nothing was rejected",
            ( uint8_t ) ( comstxetxGetRejectCount ( &driver ) == 0u ) );
}

/*
 * BuildFrame validates its own arguments because they are new and can break a
 * later invariant, and it refuses rather than writing past the buffer it was
 * given. crc16 is used here to prove the callback type takes the library own
 * CRC with no wrapper.
 */
static void sxBuildGuardCase ( void )
{
    comstxetx_t driver;
    uint8_t rxBuffer[ 64 ];
    uint8_t txBuffer[ 8 ];
    uint32_t wireLength = 99u;
    static const uint8_t payload[ ] =
    {
        0x02u, 0x02u, 0x02u, 0x02u, 0x02u, 0x02u
    };

    printf ( "comstxetx build guards\n" );

    check ( "init takes crc16 directly",
            comstxetxInit ( &driver, rxBuffer, txBuffer, 64u, 8u,
                            0x02u, 0x03u, 0x10u, 10u,
                            crc16, sxPacketProcess ) );

    check ( "a NULL payload is rejected",
            ( uint8_t ) ( comstxetxBuildFrame ( &driver, NULL, 1u, &wireLength ) == FALSE ) );
    check ( "a NULL length pointer is rejected",
            ( uint8_t ) ( comstxetxBuildFrame ( &driver, payload, 1u, NULL ) == FALSE ) );

    /*
     * Every payload byte equals STX, so each one costs two wire bytes. The
     * frame would need 1 + 12 + at least 2 + 1 bytes and the buffer holds 8.
     */
    check ( "a frame that would overflow txBuffer is refused",
            ( uint8_t ) ( comstxetxBuildFrame ( &driver, payload,
                                                ( uint32_t ) sizeof ( payload ),
                                                &wireLength ) == FALSE ) );
    check ( "and the reported length is zeroed",
            ( uint8_t ) ( wireLength == 0u ) );
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
    sxInitCase ( );
    printf ( "\n" );
    sxEscapeCase ( );
    printf ( "\n" );
    sxRejectCase ( );
    printf ( "\n" );
    sxResyncCase ( );
    printf ( "\n" );
    sxBoundaryCase ( );
    printf ( "\n" );
    sxOpenEmptyTimeoutCase ( );
    printf ( "\n" );
    sxRoundTripCase ( );
    printf ( "\n" );
    sxBuildGuardCase ( );

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
