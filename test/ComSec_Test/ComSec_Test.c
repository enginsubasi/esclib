/*
 * Covers comsec.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The authentication function below is NOT a message authentication code and
 * must not be copied into anything. It is a deterministic mix with no key,
 * present only so that the framing, the counter and the tag comparison can be
 * exercised; a real caller installs HMAC, CMAC or whatever their part's
 * accelerator provides. comsec cannot check what it was given, which is why
 * its banner says so twice.
 */

#include <stddef.h>
#include <stdio.h>

#include "comsec.h"

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

/* Not a MAC. See the file comment. Eight bytes, dependent on every input byte. */
static void testMac ( const uint8_t* const buffer, uint32_t length, uint8_t* tag )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( j = 0; j < 8u; ++j )
    {
        tag[ j ] = ( uint8_t ) ( 0x5Au + j );
    }

    for ( i = 0; i < length; ++i )
    {
        for ( j = 0; j < 8u; ++j )
        {
            tag[ j ] = ( uint8_t ) ( ( tag[ j ] ^ buffer[ i ] ) +
                                        ( uint8_t ) ( j + 1u ) );
        }
    }
}

/* A second one, standing in for a different key. */
static void otherMac ( const uint8_t* const buffer, uint32_t length, uint8_t* tag )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( j = 0; j < 8u; ++j )
    {
        tag[ j ] = ( uint8_t ) ( 0xA5u - j );
    }

    for ( i = 0; i < length; ++i )
    {
        for ( j = 0; j < 8u; ++j )
        {
            tag[ j ] = ( uint8_t ) ( ( tag[ j ] + buffer[ i ] ) ^
                                        ( uint8_t ) ( j + 3u ) );
        }
    }
}

#define TEST_SESSION    0xBEEFu
#define TEST_TAG        8u

static const uint8_t payload[ 4 ] = { 0xDEu, 0xADu, 0x10u, 0x20u };

/* Builds a frame and copies it out of the driver's transmit buffer. */
static uint32_t buildInto ( comsec_t* driver, const uint8_t* const tx, uint8_t* out )
{
    uint32_t length = 0;
    uint32_t i = 0;

    if ( comsecBuildFrame ( driver, payload, 4u, &length ) == TRUE )
    {
        for ( i = 0; i < length; ++i )
        {
            out[ i ] = tx[ i ];
        }
    }
    else
    {
        length = 0;
    }

    return ( length );
}

/* ---------------------------------------------------------------- init */

static void initCase ( void )
{
    comsec_t driver;
    uint8_t rx[ 32 ];
    uint8_t tx[ 32 ];

    printf ( "comsecInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comsecInit ( NULL, rx, tx, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) == FALSE ) );
    check ( "a NULL receive buffer is rejected",
            ( uint8_t ) ( comsecInit ( &driver, NULL, tx, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) == FALSE ) );
    check ( "a NULL transmit buffer is rejected",
            ( uint8_t ) ( comsecInit ( &driver, rx, NULL, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) == FALSE ) );
    check ( "a zero receive size is rejected",
            ( uint8_t ) ( comsecInit ( &driver, rx, tx, 0u, 32u, TEST_SESSION, TEST_TAG, testMac ) == FALSE ) );

    /*
     * The authentication function is the whole security of this module, so a
     * NULL one is refused rather than treated as "no tag".
     */
    check ( "a NULL authentication function is rejected",
            ( uint8_t ) ( comsecInit ( &driver, rx, tx, 32u, 32u, TEST_SESSION, TEST_TAG, NULL ) == FALSE ) );

    /*
     * A very short tag is refused. Four bytes is one forgery in four thousand
     * million by chance, which on a link an attacker can hammer is not a
     * comfortable margin, and anything below that is not worth carrying.
     */
    check ( "a tag below the minimum is rejected",
            ( uint8_t ) ( comsecInit ( &driver, rx, tx, 32u, 32u, TEST_SESSION,
                            COMSEC_MIN_TAG - 1u, testMac ) == FALSE ) );
    check ( "a tag at the minimum is accepted",
            comsecInit ( &driver, rx, tx, 32u, 32u, TEST_SESSION,
                         COMSEC_MIN_TAG, testMac ) );
    check ( "a tag past the maximum is rejected",
            ( uint8_t ) ( comsecInit ( &driver, rx, tx, 32u, 32u, TEST_SESSION,
                            COMSEC_MAX_TAG + 1u, testMac ) == FALSE ) );

    check ( "a transmit buffer that cannot hold one payload byte is rejected",
            ( uint8_t ) ( comsecInit ( &driver, rx, tx, 32u,
                            COMSEC_HEADER + TEST_TAG, TEST_SESSION,
                            TEST_TAG, testMac ) == FALSE ) );

    check ( "a full init succeeds",
            comsecInit ( &driver, rx, tx, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );
    check ( "the counter starts at zero",
            ( uint8_t ) ( comsecGetCounter ( &driver ) == 0u ) );
    check ( "nothing has been rejected",
            ( uint8_t ) ( comsecGetRejectCount ( &driver ) == 0u ) );
    check ( "and no error is recorded",
            ( uint8_t ) ( comsecGetLastError ( &driver ) == ( uint8_t ) CSEC_NONE ) );
    check ( "and no payload has been accepted",
            ( uint8_t ) ( comsecGetPayloadLength ( &driver ) == 0u ) );
}

/* --------------------------------------------------------- frame layout */

static void buildCase ( void )
{
    comsec_t driver;
    uint8_t rx[ 32 ];
    uint8_t tx[ 32 ];
    uint32_t length = 0;

    printf ( "comsecBuildFrame\n" );

    check ( "Init", comsecInit ( &driver, rx, tx, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );

    check ( "a NULL payload is rejected",
            ( uint8_t ) ( comsecBuildFrame ( &driver, NULL, 4u, &length ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( comsecBuildFrame ( &driver, payload, 0u, &length ) == FALSE ) );
    check ( "a payload that does not fit is rejected",
            ( uint8_t ) ( comsecBuildFrame ( &driver, payload, 32u, &length ) == FALSE ) );
    check ( "as a length failure",
            ( uint8_t ) ( comsecGetLastError ( &driver ) == ( uint8_t ) CSEC_LENGTH ) );
    check ( "and a rejected build did not spend a counter value",
            ( uint8_t ) ( comsecGetCounter ( &driver ) == 0u ) );

    check ( "building", comsecBuildFrame ( &driver, payload, 4u, &length ) );
    check ( "the frame is the payload plus the header and the tag",
            ( uint8_t ) ( length == ( 4u + COMSEC_HEADER + TEST_TAG ) ) );
    check ( "and the counter advanced to one",
            ( uint8_t ) ( comsecGetCounter ( &driver ) == 1u ) );

    check ( "the session id leads, high byte first",
            ( uint8_t ) ( ( tx[ 0 ] == 0xBEu ) && ( tx[ 1 ] == 0xEFu ) ) );
    check ( "the counter follows in four bytes, most significant first",
            ( uint8_t ) ( ( tx[ 2 ] == 0x00u ) && ( tx[ 3 ] == 0x00u ) &&
                          ( tx[ 4 ] == 0x00u ) && ( tx[ 5 ] == 0x01u ) ) );
    check ( "then the payload",
            ( uint8_t ) ( ( tx[ 6 ] == 0xDEu ) && ( tx[ 7 ] == 0xADu ) &&
                          ( tx[ 8 ] == 0x10u ) && ( tx[ 9 ] == 0x20u ) ) );

    check ( "building again advances the counter",
            comsecBuildFrame ( &driver, payload, 4u, &length ) );
    check ( "to two", ( uint8_t ) ( comsecGetCounter ( &driver ) == 2u ) );
    check ( "and the frame carries it",
            ( uint8_t ) ( tx[ 5 ] == 0x02u ) );
}

/* ----------------------------------------------------------- round trip */

static void roundTripCase ( void )
{
    comsec_t sender;
    comsec_t receiver;
    uint8_t txS[ 32 ];
    uint8_t rxS[ 32 ];
    uint8_t txR[ 32 ];
    uint8_t rxR[ 32 ];
    uint8_t frame[ 32 ];
    uint32_t length = 0;

    printf ( "a frame across the channel\n" );

    check ( "init the sender",
            comsecInit ( &sender, rxS, txS, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );
    check ( "init the receiver",
            comsecInit ( &receiver, rxR, txR, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame was built", ( uint8_t ) ( length == 18u ) );

    check ( "and it authenticates",
            comsecCheckFrame ( &receiver, frame, length ) );
    check ( "the payload length is right",
            ( uint8_t ) ( comsecGetPayloadLength ( &receiver ) == 4u ) );
    check ( "and the payload came through unchanged",
            ( uint8_t ) ( ( comsecGetPayload ( &receiver )[ 0 ] == 0xDEu ) &&
                          ( comsecGetPayload ( &receiver )[ 3 ] == 0x20u ) ) );
    check ( "with nothing rejected",
            ( uint8_t ) ( comsecGetRejectCount ( &receiver ) == 0u ) );

    length = buildInto ( &sender, txS, frame );
    check ( "and so does the next one", comsecCheckFrame ( &receiver, frame, length ) );
}

/* -------------------------------------------------------------- forgery */

static void forgeryCase ( void )
{
    comsec_t sender;
    comsec_t receiver;
    comsec_t stranger;
    uint8_t txS[ 32 ];
    uint8_t rxS[ 32 ];
    uint8_t txR[ 32 ];
    uint8_t rxR[ 32 ];
    uint8_t txX[ 32 ];
    uint8_t rxX[ 32 ];
    uint8_t frame[ 32 ];
    uint32_t length = 0;

    printf ( "forgery and the tag comparison\n" );

    check ( "init the sender",
            comsecInit ( &sender, rxS, txS, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );
    check ( "init the receiver",
            comsecInit ( &receiver, rxR, txR, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );

    /* A payload byte changed without the tag being recomputed. */
    length = buildInto ( &sender, txS, frame );
    frame[ 7 ] = ( uint8_t ) ( frame[ 7 ] ^ 0x01u );

    check ( "an altered payload does not authenticate",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as a tag failure",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_TAG ) );
    check ( "and it is counted",
            ( uint8_t ) ( comsecGetRejectCount ( &receiver ) == 1u ) );

    /*
     * A rejection does not stop the channel. A link an attacker can reach will
     * be offered forged frames as a matter of course, and one that failed
     * closed on the first would be denied service by anyone able to send a
     * packet. This is the opposite of comsafe on purpose.
     */
    length = buildInto ( &sender, txS, frame );
    check ( "and the next genuine frame is still accepted",
            comsecCheckFrame ( &receiver, frame, length ) );

    /*
     * The tag is compared in constant time, which means every byte is read.
     * Timing cannot be asserted from here, but a difference in the very last
     * byte proves the comparison does not stop at the first one that matches.
     */
    length = buildInto ( &sender, txS, frame );
    frame[ length - 1u ] = ( uint8_t ) ( frame[ length - 1u ] ^ 0x01u );
    check ( "a tag differing only in its last byte is refused",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );

    length = buildInto ( &sender, txS, frame );
    frame[ length - ( uint32_t ) TEST_TAG ] =
            ( uint8_t ) ( frame[ length - ( uint32_t ) TEST_TAG ] ^ 0x01u );
    check ( "and one differing only in its first byte",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );

    /* The counter is covered by the tag, so it cannot be moved on its own. */
    length = buildInto ( &sender, txS, frame );
    frame[ 5 ] = ( uint8_t ) ( frame[ 5 ] + 1u );
    check ( "a counter altered in flight breaks the tag",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "and is reported as a tag failure, not a replay",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_TAG ) );

    /* The session id is covered too. */
    length = buildInto ( &sender, txS, frame );
    frame[ 0 ] = ( uint8_t ) ( frame[ 0 ] ^ 0xFFu );
    check ( "and a session id altered in flight breaks it as well",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "reported as a tag failure rather than a session one",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_TAG ) );

    /*
     * A frame that is internally consistent under a different key. The tag
     * verifies against nothing the receiver knows, so it is a tag failure —
     * which is the right answer: an attacker with the wrong key gets no
     * information about which field was wrong.
     */
    check ( "init a stranger with a different key",
            comsecInit ( &stranger, rxX, txX, 32u, 32u, TEST_SESSION, TEST_TAG, otherMac ) );

    length = buildInto ( &stranger, txX, frame );
    check ( "a well formed frame under another key does not authenticate",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as a tag failure",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_TAG ) );

    /* A frame for another session, correctly tagged for it. */
    check ( "init a sender on another session",
            comsecInit ( &stranger, rxX, txX, 32u, 32u, 0x1234u, TEST_TAG, testMac ) );

    length = buildInto ( &stranger, txX, frame );
    check ( "a correctly tagged frame for another session is refused",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "as a session failure",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_SESSION ) );
}

/* --------------------------------------------------------------- replay */

static void replayCase ( void )
{
    comsec_t sender;
    comsec_t receiver;
    uint8_t txS[ 32 ];
    uint8_t rxS[ 32 ];
    uint8_t txR[ 32 ];
    uint8_t rxR[ 32 ];
    uint8_t first[ 32 ];
    uint8_t frame[ 32 ];
    uint32_t firstLength = 0;
    uint32_t length = 0;
    uint32_t i = 0;

    printf ( "replay and the counter\n" );

    check ( "init the sender",
            comsecInit ( &sender, rxS, txS, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );
    check ( "init the receiver",
            comsecInit ( &receiver, rxR, txR, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );

    firstLength = buildInto ( &sender, txS, first );
    check ( "the first frame is accepted",
            comsecCheckFrame ( &receiver, first, firstLength ) );

    /* The same frame again, byte for byte. This is the attack. */
    check ( "the same frame replayed is refused",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, first, firstLength ) == FALSE ) );
    check ( "as a replay",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_REPLAY ) );

    length = buildInto ( &sender, txS, frame );
    check ( "the next frame is accepted",
            comsecCheckFrame ( &receiver, frame, length ) );

    check ( "and the old one is still refused after it",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, first, firstLength ) == FALSE ) );

    /*
     * A gap is accepted, and this is where comsec and comsafe part company.
     * comsafe expects the very next sequence number because a safety channel
     * treats a lost frame as a fault; here a link may lose frames
     * legitimately, and the only thing that must never happen is a frame being
     * accepted twice. So the rule is strictly greater, not next.
     */
    for ( i = 0; i < 20u; ++i )
    {
        ( void ) buildInto ( &sender, txS, frame );
    }

    length = buildInto ( &sender, txS, frame );
    check ( "a frame with twenty missing before it is accepted, unlike comsafe",
            comsecCheckFrame ( &receiver, frame, length ) );

    /* Counter exhaustion. */
    check ( "rekey to one short of the end",
            comsecRekey ( &sender, TEST_SESSION, 0xFFFFFFFEu ) );
    check ( "the last frame can still be sent",
            comsecBuildFrame ( &sender, payload, 4u, &length ) );
    check ( "and the counter is spent",
            ( uint8_t ) ( comsecGetCounter ( &sender ) == 0xFFFFFFFFu ) );

    /*
     * Refusing to send is the only safe thing left. A counter that wrapped
     * would put two frames under one key and one counter, which breaks
     * authenticity and replay protection together.
     */
    check ( "the next build is refused rather than wrapping",
            ( uint8_t ) ( comsecBuildFrame ( &sender, payload, 4u, &length ) == FALSE ) );
    check ( "as exhaustion",
            ( uint8_t ) ( comsecGetLastError ( &sender ) == ( uint8_t ) CSEC_EXHAUSTED ) );
    check ( "and the counter did not wrap",
            ( uint8_t ) ( comsecGetCounter ( &sender ) == 0xFFFFFFFFu ) );
}

/* ---------------------------------------------------------------- rekey */

static void rekeyCase ( void )
{
    comsec_t sender;
    comsec_t receiver;
    uint8_t txS[ 32 ];
    uint8_t rxS[ 32 ];
    uint8_t txR[ 32 ];
    uint8_t rxR[ 32 ];
    uint8_t frame[ 32 ];
    uint32_t length = 0;

    printf ( "comsecRekey\n" );

    check ( "init the sender",
            comsecInit ( &sender, rxS, txS, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );
    check ( "init the receiver",
            comsecInit ( &receiver, rxR, txR, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );

    length = buildInto ( &sender, txS, frame );
    check ( "a frame passes", comsecCheckFrame ( &receiver, frame, length ) );

    /* A forgery, so there is something in the count to survive the rekey. */
    frame[ 7 ] = ( uint8_t ) ( frame[ 7 ] ^ 0x01u );
    check ( "a forgery is refused",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );

    check ( "a NULL driver is rejected by rekey",
            ( uint8_t ) ( comsecRekey ( NULL, 0x0001u, 100u ) == FALSE ) );

    check ( "rekeying the sender", comsecRekey ( &sender, 0x0001u, 100u ) );
    check ( "sets the counter", ( uint8_t ) ( comsecGetCounter ( &sender ) == 100u ) );
    check ( "rekeying the receiver", comsecRekey ( &receiver, 0x0001u, 0u ) );

    /*
     * Checked here rather than at the end of the case: a rejection after the
     * rekey would push the count above zero again, so a check that ran later
     * would pass whether the rekey had cleared it or not.
     */
    check ( "and the reject count survived the rekey",
            ( uint8_t ) ( comsecGetRejectCount ( &receiver ) == 1u ) );

    /*
     * The receiver is disarmed by a rekey, so the first frame after it is
     * accepted at whatever counter it carries. The peer's counter is not
     * knowable from here; what makes this safe is that the tag still has to
     * verify.
     */
    length = buildInto ( &sender, txS, frame );
    check ( "the first frame after a rekey is accepted at its own counter",
            comsecCheckFrame ( &receiver, frame, length ) );
    check ( "under the new session id",
            ( uint8_t ) ( ( frame[ 0 ] == 0x00u ) && ( frame[ 1 ] == 0x01u ) ) );
    check ( "and the replay guard follows from there",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
    check ( "reported as a replay",
            ( uint8_t ) ( comsecGetLastError ( &receiver ) == ( uint8_t ) CSEC_REPLAY ) );

    /* A frame under the old session no longer authenticates. */
    check ( "re-init an old sender",
            comsecInit ( &sender, rxS, txS, 32u, 32u, TEST_SESSION, TEST_TAG, testMac ) );
    length = buildInto ( &sender, txS, frame );
    check ( "a frame under the old session id is refused after the rekey",
            ( uint8_t ) ( comsecCheckFrame ( &receiver, frame, length ) == FALSE ) );
}

/* --------------------------------------------------------------- length */

static void lengthCase ( void )
{
    comsec_t driver;
    uint8_t rx[ 4 ];
    uint8_t tx[ 32 ];
    uint8_t frame[ 32 ];
    uint32_t i = 0;

    printf ( "frames that cannot be frames\n" );

    check ( "Init with a small receive buffer",
            comsecInit ( &driver, rx, tx, 4u, 32u, TEST_SESSION, TEST_TAG, testMac ) );

    for ( i = 0; i < 32u; ++i )
    {
        frame[ i ] = 0u;
    }

    check ( "a frame of exactly the overhead carries no payload and is refused",
            ( uint8_t ) ( comsecCheckFrame ( &driver, frame,
                            COMSEC_HEADER + TEST_TAG ) == FALSE ) );
    check ( "as a length failure",
            ( uint8_t ) ( comsecGetLastError ( &driver ) == ( uint8_t ) CSEC_LENGTH ) );

    check ( "a shorter one too",
            ( uint8_t ) ( comsecCheckFrame ( &driver, frame, 3u ) == FALSE ) );

    check ( "and a payload longer than the receive buffer",
            ( uint8_t ) ( comsecCheckFrame ( &driver, frame,
                            COMSEC_HEADER + TEST_TAG + 5u ) == FALSE ) );
    check ( "reported as a length failure as well",
            ( uint8_t ) ( comsecGetLastError ( &driver ) == ( uint8_t ) CSEC_LENGTH ) );

    /*
     * A length failure is counted like any other. It is the cheapest thing an
     * attacker can send, so it is the first thing a reject count climbing
     * without a noisy medium would show.
     */
    check ( "and every one of them was counted",
            ( uint8_t ) ( comsecGetRejectCount ( &driver ) == 3u ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    buildCase ( );
    printf ( "\n" );
    roundTripCase ( );
    printf ( "\n" );
    forgeryCase ( );
    printf ( "\n" );
    replayCase ( );
    printf ( "\n" );
    rekeyCase ( );
    printf ( "\n" );
    lengthCase ( );

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
