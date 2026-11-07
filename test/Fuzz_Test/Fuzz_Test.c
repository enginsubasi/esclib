/*
 * Fuzzes the seven functions that take bytes from a link nobody controls:
 * comstxetxReceive, comatReceive, commodbusReceive, cobsDecode,
 * comsafeCheckFrame, comsecCheckFrame and comgenbufPop.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Every other test here feeds a module the inputs its author thought of. This
 * one feeds it inputs nobody thought of, which is what a line does, and asks
 * three questions of each target that do not depend on knowing the answer in
 * advance:
 *
 *   - **Bounds.** Every buffer handed to a module is allocated at exactly the
 *     size the module was told, so a write or read one byte past it lands in
 *     the allocator's red zone. This is the question the address sanitizer
 *     answers, and CI runs this test under it; run plainly it still asks the
 *     other two.
 *   - **What comes out is well formed.** Whatever a module delivers to its
 *     caller — through a callback or a payload accessor — is checked against
 *     the module's own promise: a frame whose check really holds, a length
 *     that really fits, an address that really is ours.
 *   - **Recovery.** After any amount of noise, the documented way back — an
 *     evaluate, a silence, a reset — has to work, and the very next good frame
 *     has to arrive byte for byte. A decoder that survives noise but never
 *     resynchronizes after it is broken in the way that matters on a real link.
 *
 * The generator is biased rather than uniform. Uniform bytes almost never hit
 * the values a state machine branches on, so roughly half of every stream is
 * drawn from the target's own control bytes — STX, ETX, DLE, the AT prefix,
 * the zero COBS removes, the station address.
 *
 * Every round is seeded from the base seed, the target and the round number
 * alone, so a failure prints a line that replays exactly that round:
 *
 *     ESCLIB_FUZZ_SEED=<seed> ESCLIB_FUZZ_ROUNDS=<n> ./Fuzz_Test
 *
 * ESCLIB_FUZZ_ROUNDS raises the number of rounds per target for a long local
 * run; the default keeps the whole test to a couple of seconds so it can sit in
 * run_tests.sh and in both CI runs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comstxetx.h"
#include "comat.h"
#include "commodbus.h"
#include "cobs.h"
#include "comsafe.h"
#include "comsec.h"
#include "comgenbuf.h"
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

#define DEFAULT_SEED        0x5EED2026u
#define DEFAULT_ROUNDS      600u
#define FAULTS_SHOWN        8u
#define CHECK_SIZE          2u

static uint32_t baseSeed = DEFAULT_SEED;
static uint32_t rounds = DEFAULT_ROUNDS;

/* Which round is running, so a fault can say how to replay it. */
static const char* currentTarget = "";
static uint32_t currentRound = 0;
static uint32_t targetFaults = 0;

static void fault ( const char* what )
{
    if ( targetFaults < FAULTS_SHOWN )
    {
        printf ( "  fault %s round %lu: %s (ESCLIB_FUZZ_SEED=%lu)\n",
                 currentTarget, ( unsigned long ) currentRound, what,
                 ( unsigned long ) baseSeed );
    }
    else
    {
        /* Intentionally blank */
    }

    ++targetFaults;
}

static void expect ( uint8_t condition, const char* what )
{
    if ( condition != TRUE )
    {
        fault ( what );
    }
    else
    {
        /* Intentionally blank */
    }
}

/* ------------------------------------------------------------------------ */
/* Generator                                                                 */
/* ------------------------------------------------------------------------ */

static uint32_t rngState = 1u;

static uint32_t rngNext ( void )
{
    uint32_t x = rngState;

    x ^= ( x << 13 );
    x ^= ( x >> 17 );
    x ^= ( x << 5 );
    rngState = x;

    return ( x );
}

static uint32_t rngBelow ( uint32_t bound )
{
    uint32_t retVal = 0u;

    if ( bound != 0u )
    {
        retVal = rngNext ( ) % bound;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/* Half the time one of the target's own control bytes, otherwise any byte. */
static uint8_t rngByte ( const uint8_t* const interesting, uint32_t count )
{
    uint8_t retVal = 0u;

    if ( ( count != 0u ) && ( rngBelow ( 2u ) == 0u ) )
    {
        retVal = interesting[ rngBelow ( count ) ];
    }
    else
    {
        retVal = ( uint8_t ) rngNext ( );
    }

    return ( retVal );
}

/* Seeds one round from the base seed, the target and the round alone. */
static void rngRound ( uint32_t target, uint32_t round )
{
    uint32_t x = baseSeed ^ ( target * 0x9E3779B9u ) ^ ( round * 0x85EBCA6Bu );

    x ^= ( x >> 16 );
    x *= 0x7FEB352Du;
    x ^= ( x >> 15 );
    x *= 0x846CA68Bu;
    x ^= ( x >> 16 );

    if ( x == 0u )
    {
        x = 1u;
    }
    else
    {
        /* Intentionally blank */
    }

    rngState = x;
}

/*
 * Exactly as many bytes as asked for, so the red zone starts at the first byte
 * a module has no business touching. malloc ( 0 ) may answer NULL, which every
 * Init here rightly refuses, so a zero request is rounded up to one.
 */
static uint8_t* exact ( uint32_t size )
{
    uint8_t* retVal = ( uint8_t* ) malloc ( ( size == 0u ) ? 1u : size );

    if ( retVal == NULL )
    {
        printf ( "out of memory\n" );
        exit ( 2 );
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

static void fillRandom ( uint8_t* buffer, uint32_t length,
                         const uint8_t* const interesting, uint32_t count )
{
    uint32_t i = 0u;

    for ( i = 0u; i < length; ++i )
    {
        buffer[ i ] = rngByte ( interesting, count );
    }
}

/* A check that accepts any frame whose two check bytes are zero. */
static uint16_t permissiveCheck ( const uint8_t* const buffer, uint32_t length )
{
    ( void ) buffer;
    ( void ) length;

    return ( 0u );
}

/* ------------------------------------------------------------------------ */
/* comstxetx                                                                 */
/* ------------------------------------------------------------------------ */

static struct
{
    uint32_t rxSize;
    uint8_t permissive;
    uint32_t delivered;
    uint8_t last[ 256 ];
    uint32_t lastLength;
} stx;

static void stxPacket ( uint8_t* buffer, uint32_t index )
{
    uint16_t carried = 0u;
    uint16_t computed = 0u;

    expect ( ( uint8_t ) ( ( index + CHECK_SIZE ) <= stx.rxSize ),
             "comstxetx delivered more than its buffer holds" );

    /* The check bytes sit right after the payload, inside the buffer. */
    carried = ( uint16_t ) ( buffer[ index ] |
              ( uint16_t ) ( ( uint16_t ) buffer[ index + 1u ] << 8 ) );

    if ( stx.permissive == TRUE )
    {
        computed = 0u;
    }
    else
    {
        computed = crc16 ( buffer, index );
    }

    expect ( ( uint8_t ) ( carried == computed ),
             "comstxetx delivered a frame whose check does not hold" );

    if ( index <= sizeof ( stx.last ) )
    {
        memcpy ( stx.last, buffer, index );
    }
    else
    {
        /* Intentionally blank */
    }

    stx.lastLength = index;
    ++stx.delivered;
}

static void stxRound ( void )
{
    comstxetx_t driver;
    uint8_t* rxBuffer = NULL;
    uint8_t* txBuffer = NULL;
    uint8_t* payload = NULL;
    uint32_t txSize = 0u;
    uint32_t timeout = 0u;
    uint32_t length = 0u;
    uint32_t frameLength = 0u;
    uint32_t i = 0u;
    uint8_t control[ 5 ];

    stx.rxSize = 4u + rngBelow ( 60u );
    stx.permissive = ( uint8_t ) rngBelow ( 2u );
    stx.delivered = 0u;
    txSize = ( 2u * stx.rxSize ) + 6u;
    timeout = rngBelow ( 10u );

    /* Three distinct control bytes, chosen afresh every round. */
    control[ 0 ] = ( uint8_t ) rngNext ( );
    do
    {
        control[ 1 ] = ( uint8_t ) rngNext ( );
    }
    while ( control[ 1 ] == control[ 0 ] );
    do
    {
        control[ 2 ] = ( uint8_t ) rngNext ( );
    }
    while ( ( control[ 2 ] == control[ 0 ] ) || ( control[ 2 ] == control[ 1 ] ) );
    control[ 3 ] = 0x00u;
    control[ 4 ] = 0xFFu;

    rxBuffer = exact ( stx.rxSize );
    txBuffer = exact ( txSize );

    if ( comstxetxInit ( &driver, rxBuffer, txBuffer, stx.rxSize, txSize,
                         control[ 0 ], control[ 1 ], control[ 2 ], timeout,
                         ( stx.permissive == TRUE ) ? permissiveCheck : crc16,
                         stxPacket ) == FALSE )
    {
        fault ( "comstxetxInit refused a valid configuration" );
    }
    else
    {
        /* Noise, with the main loop and the tick interleaved at random. */
        length = rngBelow ( 400u );

        for ( i = 0u; i < length; ++i )
        {
            comstxetxReceive ( &driver, rngByte ( control, 5u ) );

            if ( rngBelow ( 8u ) == 0u )
            {
                comstxetxEvaluate ( &driver );
            }
            else
            {
                /* Intentionally blank */
            }

            if ( rngBelow ( 8u ) == 0u )
            {
                comstxetxTimeoutCounter ( &driver );
            }
            else
            {
                /* Intentionally blank */
            }
        }

        /* The way back: evaluate whatever is pending, then a silence. */
        comstxetxEvaluate ( &driver );

        for ( i = 0u; i < ( timeout + 2u ); ++i )
        {
            comstxetxTimeoutCounter ( &driver );
        }

        /* And the next good frame has to arrive whole. */
        length = rngBelow ( stx.rxSize - CHECK_SIZE );
        payload = exact ( length );
        fillRandom ( payload, length, control, 5u );

        if ( comstxetxBuildFrame ( &driver, payload, length, &frameLength ) == FALSE )
        {
            fault ( "comstxetxBuildFrame refused a payload that fits" );
        }
        else
        {
            stx.delivered = 0u;

            for ( i = 0u; i < frameLength; ++i )
            {
                comstxetxReceive ( &driver, txBuffer[ i ] );
            }

            comstxetxEvaluate ( &driver );

            expect ( ( uint8_t ) ( stx.delivered == 1u ),
                     "comstxetx did not recover after noise" );
            expect ( ( uint8_t ) ( ( stx.delivered != 1u ) ||
                     ( ( stx.lastLength == length ) &&
                       ( memcmp ( stx.last, payload, length ) == 0 ) ) ),
                     "comstxetx recovered with the wrong payload" );
        }

        free ( payload );
    }

    free ( rxBuffer );
    free ( txBuffer );
}

/* ------------------------------------------------------------------------ */
/* comat                                                                     */
/* ------------------------------------------------------------------------ */

static struct
{
    uint32_t rxSize;
    uint32_t txSize;
    uint32_t delivered;
    uint8_t last[ 64 ];
    uint32_t lastLength;
} at;

static void atPacket ( uint8_t* rxBuf, uint32_t rxInd, uint8_t* txBuf, uint32_t* txInd )
{
    uint32_t i = 0u;
    uint32_t reply = 0u;

    expect ( ( uint8_t ) ( rxInd <= at.rxSize ),
             "comat delivered more than its buffer holds" );
    expect ( ( uint8_t ) ( rxInd >= 4u ),
             "comat delivered a line shorter than AT and a line end" );

    if ( ( rxInd >= 4u ) && ( rxInd <= at.rxSize ) )
    {
        expect ( ( uint8_t ) ( ( rxBuf[ 0 ] == 'A' ) && ( rxBuf[ 1 ] == 'T' ) ),
                 "comat delivered a line that does not start with AT" );
        expect ( ( uint8_t ) ( ( rxBuf[ rxInd - 2u ] == '\r' ) &&
                               ( rxBuf[ rxInd - 1u ] == '\n' ) ),
                 "comat delivered a line that does not end in CR LF" );

        if ( rxInd <= sizeof ( at.last ) )
        {
            memcpy ( at.last, rxBuf, rxInd );
        }
        else
        {
            /* Intentionally blank */
        }
    }
    else
    {
        /* Intentionally blank */
    }

    at.lastLength = rxInd;
    ++at.delivered;

    /* A reply of any length the transmit buffer holds, written in full. */
    reply = rngBelow ( at.txSize + 1u );

    for ( i = 0u; i < reply; ++i )
    {
        txBuf[ i ] = ( uint8_t ) rngNext ( );
    }

    *txInd = reply;
}

static void atTransmit ( uint8_t* txBuf, uint32_t txInd )
{
    uint32_t i = 0u;
    uint8_t sum = 0u;

    expect ( ( uint8_t ) ( txInd <= at.txSize ),
             "comat triggered a transmission longer than its buffer" );

    /* Read every byte, so a length past the buffer is a read past it. */
    for ( i = 0u; ( i < txInd ) && ( i < at.txSize ); ++i )
    {
        sum = ( uint8_t ) ( sum + txBuf[ i ] );
    }

    ( void ) sum;
}

static void atRound ( void )
{
    static const uint8_t control[ 5 ] = { 'A', 'T', '\r', '\n', 0x00u };
    comat_t driver;
    uint8_t* rxBuffer = NULL;
    uint8_t* txBuffer = NULL;
    uint8_t line[ 64 ];
    uint32_t timeout = 0u;
    uint32_t length = 0u;
    uint32_t i = 0u;

    at.rxSize = 3u + rngBelow ( 40u );
    at.txSize = 1u + rngBelow ( 40u );
    at.delivered = 0u;
    timeout = rngBelow ( 10u );

    rxBuffer = exact ( at.rxSize );
    txBuffer = exact ( at.txSize );

    if ( comatInit ( &driver, rxBuffer, txBuffer, at.rxSize, at.txSize, timeout,
                     atPacket, atTransmit ) == FALSE )
    {
        fault ( "comatInit refused a valid configuration" );
    }
    else
    {
        length = rngBelow ( 400u );

        for ( i = 0u; i < length; ++i )
        {
            comatReceive ( &driver, rngByte ( control, 5u ) );

            if ( rngBelow ( 8u ) == 0u )
            {
                comatEvaluate ( &driver );
            }
            else
            {
                /* Intentionally blank */
            }

            if ( rngBelow ( 8u ) == 0u )
            {
                comatTimeoutCounter ( &driver );
            }
            else
            {
                /* Intentionally blank */
            }
        }

        comatEvaluate ( &driver );

        for ( i = 0u; i < ( timeout + 2u ); ++i )
        {
            comatTimeoutCounter ( &driver );
        }

        /* A command that fits: AT, a body that holds no line end, CR LF. */
        if ( at.rxSize >= 4u )
        {
            length = 4u + rngBelow ( at.rxSize - 3u );
            line[ 0 ] = 'A';
            line[ 1 ] = 'T';

            for ( i = 2u; i < ( length - 2u ); ++i )
            {
                line[ i ] = ( uint8_t ) ( 'B' + rngBelow ( 25u ) );
            }

            line[ length - 2u ] = '\r';
            line[ length - 1u ] = '\n';

            at.delivered = 0u;

            for ( i = 0u; i < length; ++i )
            {
                comatReceive ( &driver, line[ i ] );
            }

            comatEvaluate ( &driver );

            expect ( ( uint8_t ) ( at.delivered == 1u ),
                     "comat did not recover after noise" );
            expect ( ( uint8_t ) ( ( at.delivered != 1u ) ||
                     ( ( at.lastLength == length ) &&
                       ( memcmp ( at.last, line, length ) == 0 ) ) ),
                     "comat recovered with the wrong line" );
        }
        else
        {
            /* Intentionally blank */
        }
    }

    free ( rxBuffer );
    free ( txBuffer );
}

/* ------------------------------------------------------------------------ */
/* commodbus                                                                 */
/* ------------------------------------------------------------------------ */

static struct
{
    uint32_t rxSize;
    uint8_t address;
    uint8_t permissive;
    uint32_t delivered;
    uint8_t last[ 256 ];
    uint32_t lastLength;
    /*
     * Every byte the line carried since the frame began, kept independently
     * of the module. RTU has no end byte, so this is the only description of
     * a frame that does not come from the code under test.
     */
    uint8_t wire[ 512 ];
    uint32_t wireLength;
} mb;

static void mbPacket ( uint8_t* buffer, uint32_t length )
{
    uint16_t carried = 0u;
    uint16_t computed = 0u;

    expect ( ( uint8_t ) ( ( length + CHECK_SIZE ) <= mb.rxSize ),
             "commodbus delivered more than its buffer holds" );
    expect ( ( uint8_t ) ( length >= 2u ),
             "commodbus delivered a frame shorter than its minimum" );
    expect ( ( uint8_t ) ( ( buffer[ 0 ] == mb.address ) ||
                           ( buffer[ 0 ] == COMMODBUS_BROADCAST ) ),
             "commodbus delivered a frame addressed to somebody else" );

    /* Low byte first: the one little endian field in the protocol. */
    carried = ( uint16_t ) ( buffer[ length ] |
              ( uint16_t ) ( ( uint16_t ) buffer[ length + 1u ] << 8 ) );

    if ( mb.permissive == TRUE )
    {
        computed = 0u;
    }
    else
    {
        computed = crc16 ( buffer, length );
    }

    expect ( ( uint8_t ) ( carried == computed ),
             "commodbus delivered a frame whose check does not hold" );
    expect ( ( uint8_t ) ( ( ( length + CHECK_SIZE ) == mb.wireLength ) &&
                           ( memcmp ( buffer, mb.wire, length + CHECK_SIZE ) == 0 ) ),
             "commodbus delivered something other than what the line carried" );

    if ( length <= sizeof ( mb.last ) )
    {
        memcpy ( mb.last, buffer, length );
    }
    else
    {
        /* Intentionally blank */
    }

    mb.lastLength = length;
    ++mb.delivered;
}

static void mbReceive ( commodbus_t* driver, uint8_t data )
{
    /* A byte heard while a frame waits to be evaluated is not part of it. */
    if ( driver->rxReadyToEvaluate == FALSE )
    {
        if ( mb.wireLength < sizeof ( mb.wire ) )
        {
            mb.wire[ mb.wireLength ] = data;
        }
        else
        {
            /* Intentionally blank */
        }

        ++mb.wireLength;
    }
    else
    {
        /* Intentionally blank */
    }

    commodbusReceive ( driver, data );
}

static void mbEvaluate ( commodbus_t* driver )
{
    uint8_t ready = driver->rxReadyToEvaluate;

    commodbusEvaluate ( driver );

    if ( ready == TRUE )
    {
        mb.wireLength = 0u;
    }
    else
    {
        /* Intentionally blank */
    }
}

static void mbSilence ( commodbus_t* driver, uint32_t ticks )
{
    uint32_t i = 0u;

    for ( i = 0u; i < ticks; ++i )
    {
        commodbusTimeoutCounter ( driver );
    }
}

static void mbRound ( void )
{
    commodbus_t driver;
    uint8_t* rxBuffer = NULL;
    uint8_t* txBuffer = NULL;
    uint8_t* pdu = NULL;
    uint32_t silence = 0u;
    uint32_t length = 0u;
    uint32_t frameLength = 0u;
    uint32_t i = 0u;
    uint8_t to = 0u;
    uint8_t control[ 3 ];

    mb.rxSize = COMMODBUS_MIN_FRAME + rngBelow ( 60u );
    mb.address = ( uint8_t ) ( COMMODBUS_ADDRESS_MIN +
                 rngBelow ( COMMODBUS_ADDRESS_MAX - COMMODBUS_ADDRESS_MIN + 1u ) );
    mb.permissive = ( uint8_t ) rngBelow ( 2u );
    mb.delivered = 0u;
    mb.wireLength = 0u;
    silence = 1u + rngBelow ( 4u );

    control[ 0 ] = mb.address;
    control[ 1 ] = COMMODBUS_BROADCAST;
    control[ 2 ] = 0xFFu;

    rxBuffer = exact ( mb.rxSize );
    txBuffer = exact ( mb.rxSize );

    if ( commodbusInit ( &driver, rxBuffer, txBuffer, mb.rxSize, mb.rxSize,
                         mb.address, silence,
                         ( mb.permissive == TRUE ) ? permissiveCheck : crc16,
                         mbPacket ) == FALSE )
    {
        fault ( "commodbusInit refused a valid configuration" );
    }
    else
    {
        /* Bytes with gaps of every length around the silence threshold. */
        length = rngBelow ( 400u );

        for ( i = 0u; i < length; ++i )
        {
            mbReceive ( &driver, rngByte ( control, 3u ) );

            if ( rngBelow ( 4u ) == 0u )
            {
                mbSilence ( &driver, rngBelow ( silence + 3u ) );
            }
            else
            {
                /* Intentionally blank */
            }

            if ( rngBelow ( 4u ) == 0u )
            {
                mbEvaluate ( &driver );
            }
            else
            {
                /* Intentionally blank */
            }
        }

        /* The way back: a silence ends whatever was open, then evaluate. */
        mbSilence ( &driver, silence + 1u );
        mbEvaluate ( &driver );

        length = 1u + rngBelow ( mb.rxSize - 3u );
        pdu = exact ( length );
        fillRandom ( pdu, length, control, 3u );

        /* To this station or to every station; both have to be heard. */
        to = ( rngBelow ( 2u ) == 0u ) ? mb.address : ( uint8_t ) COMMODBUS_BROADCAST;

        if ( commodbusBuildFrame ( &driver, to, pdu, length,
                                   &frameLength ) == FALSE )
        {
            fault ( "commodbusBuildFrame refused a frame that fits" );
        }
        else
        {
            mb.delivered = 0u;

            /*
             * Gaps inside the frame of up to the threshold itself: a byte
             * restarts the silence, and the frame only ends one tick past it.
             */
            for ( i = 0u; i < frameLength; ++i )
            {
                mbReceive ( &driver, txBuffer[ i ] );
                mbSilence ( &driver, rngBelow ( silence + 1u ) );
            }

            mbSilence ( &driver, silence + 1u );
            mbEvaluate ( &driver );

            expect ( ( uint8_t ) ( mb.delivered == 1u ),
                     "commodbus did not recover after noise" );
            expect ( ( uint8_t ) ( ( mb.delivered != 1u ) ||
                     ( ( mb.lastLength == ( length + 1u ) ) &&
                       ( mb.last[ 0 ] == to ) &&
                       ( memcmp ( &mb.last[ 1 ], pdu, length ) == 0 ) ) ),
                     "commodbus recovered with the wrong frame" );
        }

        free ( pdu );
    }

    free ( rxBuffer );
    free ( txBuffer );
}

/* ------------------------------------------------------------------------ */
/* cobs                                                                      */
/* ------------------------------------------------------------------------ */

static void cobsRound ( void )
{
    static const uint8_t control[ 4 ] = { 0x00u, 0x01u, 0x02u, 0xFFu };
    uint8_t* source = NULL;
    uint8_t* encoded = NULL;
    uint8_t* decoded = NULL;
    uint8_t* before = NULL;
    uint32_t length = 0u;
    uint32_t capacity = 0u;
    uint32_t encodedLength = 0u;
    uint32_t decodedLength = 0u;
    uint32_t i = 0u;
    uint8_t ok = FALSE;
    uint8_t zero = FALSE;

    /*
     * Any payload goes in and comes back out, in buffers sized exactly. Half
     * of them hold no zero at all, because a run reaches the 254 where the
     * encoding changes behaviour only when nothing interrupts it.
     */
    length = rngBelow ( 700u );
    source = exact ( length );

    if ( rngBelow ( 2u ) == 0u )
    {
        fillRandom ( source, length, control, 4u );
    }
    else
    {
        for ( i = 0u; i < length; ++i )
        {
            source[ i ] = ( uint8_t ) ( 1u + rngBelow ( 255u ) );
        }
    }

    /*
     * A destination short of cobsEncodedSize is refused whatever the payload
     * holds, since the bound is what a caller sizes from and the encoder does
     * not get to decide afterwards that this payload happened to fit.
     */
    capacity = rngBelow ( cobsEncodedSize ( length ) );
    encoded = exact ( capacity );
    expect ( ( uint8_t ) ( cobsEncode ( source, length, encoded, capacity,
                                        &encodedLength ) == FALSE ),
             "cobsEncode accepted a destination short of cobsEncodedSize" );
    free ( encoded );

    capacity = cobsEncodedSize ( length );
    encoded = exact ( capacity );
    decoded = exact ( length );

    ok = cobsEncode ( source, length, encoded, capacity, &encodedLength );
    expect ( ok, "cobsEncode refused a buffer of cobsEncodedSize" );

    if ( ok == TRUE )
    {
        for ( i = 0u; i < encodedLength; ++i )
        {
            if ( encoded[ i ] == COBS_DELIMITER )
            {
                zero = TRUE;
            }
            else
            {
                /* Intentionally blank */
            }
        }

        expect ( ( uint8_t ) ( zero == FALSE ), "cobsEncode let a zero through" );

        ok = cobsDecode ( encoded, encodedLength, decoded, length, &decodedLength );
        expect ( ok, "cobsDecode refused what cobsEncode produced" );
        expect ( ( uint8_t ) ( ( ok == FALSE ) || ( ( decodedLength == length ) &&
                 ( memcmp ( decoded, source, length ) == 0 ) ) ),
                 "cobs did not round trip" );
    }
    else
    {
        /* Intentionally blank */
    }

    free ( source );
    free ( encoded );
    free ( decoded );

    /*
     * And any frame at all goes into the decoder. All or nothing: a refusal
     * leaves the destination exactly as it was, and an acceptance writes no
     * further than it says it did.
     */
    length = rngBelow ( 700u );
    capacity = rngBelow ( 700u );
    source = exact ( length );
    decoded = exact ( capacity );
    before = exact ( capacity );
    fillRandom ( source, length, control, 4u );
    fillRandom ( decoded, capacity, NULL, 0u );
    memcpy ( before, decoded, capacity );

    /* Sometimes a real frame with one byte damaged, which is the common case. */
    if ( ( rngBelow ( 2u ) == 0u ) && ( length > 0u ) )
    {
        encoded = exact ( cobsEncodedSize ( length ) );

        if ( cobsEncode ( source, length, encoded, cobsEncodedSize ( length ),
                          &encodedLength ) == TRUE )
        {
            encoded[ rngBelow ( encodedLength ) ] = rngByte ( control, 4u );
            free ( source );
            source = exact ( encodedLength );
            memcpy ( source, encoded, encodedLength );
            length = encodedLength;
        }
        else
        {
            /* Intentionally blank */
        }

        free ( encoded );
    }
    else
    {
        /* Intentionally blank */
    }

    decodedLength = 0xFFFFFFFFu;
    ok = cobsDecode ( source, length, decoded, capacity, &decodedLength );

    if ( ok == TRUE )
    {
        /*
         * The decoder is strict, so a frame it accepts is the one frame of
         * its payload: encoding what came out has to give back what went in.
         */
        encoded = exact ( cobsEncodedSize ( decodedLength ) );
        expect ( ( uint8_t ) ( ( cobsEncode ( decoded, decodedLength, encoded,
                                             cobsEncodedSize ( decodedLength ),
                                             &encodedLength ) == TRUE ) &&
                               ( encodedLength == length ) &&
                               ( memcmp ( encoded, source, length ) == 0 ) ),
                 "cobsDecode accepted a frame the encoder would not write" );
        free ( encoded );

        expect ( ( uint8_t ) ( decodedLength <= capacity ),
                 "cobsDecode wrote past its capacity" );
        expect ( ( uint8_t ) ( ( decodedLength > capacity ) ||
                 ( memcmp ( &decoded[ decodedLength ], &before[ decodedLength ],
                            capacity - decodedLength ) == 0 ) ),
                 "cobsDecode wrote past the length it reported" );
    }
    else
    {
        expect ( ( uint8_t ) ( memcmp ( decoded, before, capacity ) == 0 ),
                 "cobsDecode wrote into the destination and then refused" );
    }

    free ( source );
    free ( decoded );
    free ( before );
}

/* ------------------------------------------------------------------------ */
/* comsafe                                                                   */
/* ------------------------------------------------------------------------ */

#define SAFE_HEADER     3u
#define SAFE_HISTORY    4u

static void safeRound ( void )
{
    comsafe_t tx;
    comsafe_t foreign;
    comsafe_t rx;
    uint8_t* txBuffer = NULL;
    uint8_t* rxBuffer = NULL;
    uint8_t* txScratch = NULL;
    uint8_t* foreignBuffer = NULL;
    uint8_t* frame = NULL;
    uint8_t* payload = NULL;
    uint8_t* history[ SAFE_HISTORY ];
    uint32_t historyLength[ SAFE_HISTORY ];
    uint32_t rxSize = 0u;
    uint32_t txSize = 0u;
    uint16_t id = 0u;
    uint32_t timeout = 0u;
    uint32_t silent = 0u;
    uint32_t step = 0u;
    uint32_t length = 0u;
    uint32_t frameLength = 0u;
    uint32_t built = 0u;
    uint32_t lastAccepted = 0u;
    uint32_t errorsBefore = 0u;
    uint32_t i = 0u;
    uint8_t stateBefore = 0u;
    uint8_t armed = FALSE;
    uint8_t intact = FALSE;
    uint8_t fresh = FALSE;
    uint8_t ok = FALSE;
    uint8_t action = 0u;
    uint8_t lastSequence = 0u;
    uint16_t carried = 0u;

    rxSize = 1u + rngBelow ( 40u );
    txSize = rxSize + COMSAFE_OVERHEAD;
    id = ( uint16_t ) rngNext ( );
    timeout = 1u + rngBelow ( 30u );

    txBuffer = exact ( txSize );
    foreignBuffer = exact ( txSize );
    txScratch = exact ( COMSAFE_OVERHEAD + 1u );
    rxBuffer = exact ( rxSize );

    for ( i = 0u; i < SAFE_HISTORY; ++i )
    {
        history[ i ] = NULL;
        historyLength[ i ] = 0u;
    }

    /* The same check and a different connection id: only the id tells it apart. */
    if ( ( comsafeInit ( &tx, txScratch, txBuffer, 1u, txSize, id, 1000u, crc16 ) == FALSE ) ||
         ( comsafeInit ( &foreign, txScratch, foreignBuffer, 1u, txSize,
                         ( uint16_t ) ( id ^ ( 1u + rngBelow ( 0xFFFFu ) ) ), 1000u,
                         crc16 ) == FALSE ) ||
         ( comsafeInit ( &rx, rxBuffer, txScratch, rxSize, COMSAFE_OVERHEAD + 1u, id,
                         timeout, crc16 ) == FALSE ) )
    {
        fault ( "comsafeInit refused a valid configuration" );
    }
    else
    {
        for ( step = 0u; step < 120u; ++step )
        {
            action = ( uint8_t ) rngBelow ( 10u );
            frame = NULL;
            intact = FALSE;
            fresh = FALSE;

            if ( action <= 4u )
            {
                /* A real frame, sometimes damaged, sometimes never sent. */
                length = 1u + rngBelow ( rxSize );
                payload = exact ( length );
                fillRandom ( payload, length, NULL, 0u );

                if ( comsafeBuildFrame ( &tx, payload, length, &frameLength ) == TRUE )
                {
                    ++built;
                    frame = exact ( frameLength );
                    memcpy ( frame, txBuffer, frameLength );
                    intact = TRUE;
                    fresh = ( uint8_t ) ( ( armed == FALSE ) ||
                                          ( built == ( lastAccepted + 1u ) ) );

                    free ( history[ built % SAFE_HISTORY ] );
                    history[ built % SAFE_HISTORY ] = exact ( frameLength );
                    memcpy ( history[ built % SAFE_HISTORY ], frame, frameLength );
                    historyLength[ built % SAFE_HISTORY ] = frameLength;

                    if ( action == 1u )
                    {
                        frame[ rngBelow ( frameLength ) ] ^=
                            ( uint8_t ) ( 1u + rngBelow ( 255u ) );
                        intact = FALSE;
                    }
                    else if ( action == 2u )
                    {
                        free ( frame );
                        frame = NULL;
                    }
                    else
                    {
                        /* Intentionally blank */
                    }
                }
                else
                {
                    fault ( "comsafeBuildFrame refused a payload that fits" );
                }

                free ( payload );
            }
            else if ( action == 5u )
            {
                /* A frame from the past, replayed. */
                i = rngBelow ( SAFE_HISTORY );

                if ( history[ i ] != NULL )
                {
                    frameLength = historyLength[ i ];
                    frame = exact ( frameLength );
                    memcpy ( frame, history[ i ], frameLength );
                }
                else
                {
                    /* Intentionally blank */
                }
            }
            else if ( action == 6u )
            {
                frameLength = rngBelow ( rxSize + COMSAFE_OVERHEAD + 8u );
                frame = exact ( frameLength );
                fillRandom ( frame, frameLength, NULL, 0u );
            }
            else if ( action == 7u )
            {
                comsafeTimeoutCounter ( &rx );
                ++silent;

                if ( silent >= timeout )
                {
                    expect ( ( uint8_t ) ( comsafeGetState ( &rx ) == ( uint8_t ) CS_FAILED ),
                             "comsafe kept running past its watchdog" );
                }
                else
                {
                    /* Intentionally blank */
                }
            }
            else if ( ( action == 8u ) && ( rngBelow ( 2u ) == 0u ) )
            {
                /* A well formed frame from a sender this channel is not for. */
                length = 1u + rngBelow ( rxSize );
                payload = exact ( length );
                fillRandom ( payload, length, NULL, 0u );

                if ( comsafeBuildFrame ( &foreign, payload, length, &frameLength ) == TRUE )
                {
                    frame = exact ( frameLength );
                    memcpy ( frame, foreignBuffer, frameLength );
                }
                else
                {
                    fault ( "comsafeBuildFrame refused a payload that fits" );
                }

                free ( payload );
            }
            else
            {
                if ( comsafeGetState ( &rx ) == ( uint8_t ) CS_FAILED )
                {
                    comsafeReset ( &rx );
                    armed = FALSE;
                    silent = 0u;
                }
                else
                {
                    /* Intentionally blank */
                }
            }

            if ( frame != NULL )
            {
                stateBefore = comsafeGetState ( &rx );
                errorsBefore = comsafeGetErrorCount ( &rx );

                ok = comsafeCheckFrame ( &rx, frame, frameLength );

                if ( stateBefore == ( uint8_t ) CS_FAILED )
                {
                    expect ( ( uint8_t ) ( ok == FALSE ),
                             "comsafe accepted a frame while failed" );
                    expect ( ( uint8_t ) ( comsafeGetErrorCount ( &rx ) == errorsBefore ),
                             "comsafe counted a frame it refused while failed" );
                }
                else if ( ( intact == TRUE ) && ( fresh == TRUE ) )
                {
                    expect ( ok, "comsafe refused the next good frame" );
                }
                else
                {
                    /* Intentionally blank */
                }

                if ( ok == TRUE )
                {
                    length = frameLength - COMSAFE_OVERHEAD;
                    carried = ( uint16_t ) ( ( ( uint16_t ) frame[ SAFE_HEADER + length ] << 8 ) |
                                             frame[ SAFE_HEADER + length + 1u ] );

                    expect ( ( uint8_t ) ( comsafeGetPayloadLength ( &rx ) == length ),
                             "comsafe reported the wrong payload length" );
                    expect ( ( uint8_t ) ( memcmp ( comsafeGetPayload ( &rx ),
                             &frame[ SAFE_HEADER ], length ) == 0 ),
                             "comsafe delivered a payload the frame did not carry" );
                    expect ( ( uint8_t ) ( crc16 ( frame, SAFE_HEADER + length ) == carried ),
                             "comsafe accepted a frame whose check does not hold" );
                    expect ( ( uint8_t ) ( ( ( ( uint16_t ) frame[ 0 ] << 8 ) | frame[ 1 ] ) == id ),
                             "comsafe accepted a frame from another connection" );
                    expect ( ( uint8_t ) ( ( armed == FALSE ) ||
                             ( frame[ 2 ] == ( uint8_t ) ( lastSequence + 1u ) ) ),
                             "comsafe accepted a frame out of sequence" );

                    armed = TRUE;
                    lastSequence = frame[ 2 ];
                    silent = 0u;

                    if ( ( intact == TRUE ) && ( fresh == TRUE ) )
                    {
                        lastAccepted = built;
                    }
                    else
                    {
                        /* A replay that happened to be in sequence. */
                        lastAccepted = 0xFFFFFFFFu;
                    }
                }
                else
                {
                    /* Intentionally blank */
                }

                free ( frame );
            }
            else
            {
                /* Intentionally blank */
            }
        }
    }

    for ( i = 0u; i < SAFE_HISTORY; ++i )
    {
        free ( history[ i ] );
    }

    free ( txBuffer );
    free ( foreignBuffer );
    free ( txScratch );
    free ( rxBuffer );
}

/* ------------------------------------------------------------------------ */
/* comsec                                                                    */
/* ------------------------------------------------------------------------ */

#define SEC_HISTORY     4u

static uint8_t secTagSize = COMSEC_MIN_TAG;

/* Not a MAC, and does not need to be: it only has to depend on every byte. */
static void secMac ( const uint8_t* const buffer, uint32_t length, uint8_t* tag )
{
    uint32_t hash = 0x811C9DC5u;
    uint32_t i = 0u;

    for ( i = 0u; i < length; ++i )
    {
        hash = ( hash ^ buffer[ i ] ) * 0x01000193u;
    }

    for ( i = 0u; i < secTagSize; ++i )
    {
        hash = ( hash ^ i ) * 0x01000193u;
        tag[ i ] = ( uint8_t ) ( hash >> 24 );
    }
}

static void secRound ( void )
{
    comsec_t tx;
    comsec_t foreign;
    comsec_t rx;
    uint8_t* txBuffer = NULL;
    uint8_t* rxBuffer = NULL;
    uint8_t* txScratch = NULL;
    uint8_t* foreignBuffer = NULL;
    uint8_t* frame = NULL;
    uint8_t* payload = NULL;
    uint8_t* history[ SEC_HISTORY ];
    uint32_t historyLength[ SEC_HISTORY ];
    uint32_t rxSize = 0u;
    uint32_t txSize = 0u;
    uint32_t overhead = 0u;
    uint16_t session = 0u;
    uint32_t step = 0u;
    uint32_t length = 0u;
    uint32_t frameLength = 0u;
    uint32_t counter = 0u;
    uint32_t counterBefore = 0u;
    uint32_t lastAccepted = 0u;
    uint32_t i = 0u;
    uint8_t armed = FALSE;
    uint8_t armedBefore = FALSE;
    uint8_t foreignFrame = FALSE;
    uint8_t intact = FALSE;
    uint8_t ok = FALSE;
    uint8_t action = 0u;
    uint8_t tag[ COMSEC_MAX_TAG ];

    secTagSize = ( uint8_t ) ( COMSEC_MIN_TAG + rngBelow ( COMSEC_MAX_TAG - COMSEC_MIN_TAG + 1u ) );
    overhead = COMSEC_HEADER + secTagSize;
    rxSize = 1u + rngBelow ( 40u );
    txSize = rxSize + overhead;
    session = ( uint16_t ) rngNext ( );

    txBuffer = exact ( txSize );
    foreignBuffer = exact ( txSize );
    txScratch = exact ( overhead + 1u );
    rxBuffer = exact ( rxSize );

    for ( i = 0u; i < SEC_HISTORY; ++i )
    {
        history[ i ] = NULL;
        historyLength[ i ] = 0u;
    }

    /* The same key and a different session: only the session tells it apart. */
    if ( ( comsecInit ( &tx, txScratch, txBuffer, 1u, txSize, session,
                        secTagSize, secMac ) == FALSE ) ||
         ( comsecInit ( &foreign, txScratch, foreignBuffer, 1u, txSize,
                        ( uint16_t ) ( session ^ ( 1u + rngBelow ( 0xFFFFu ) ) ),
                        secTagSize, secMac ) == FALSE ) ||
         ( comsecInit ( &rx, rxBuffer, txScratch, rxSize, overhead + 1u, session,
                        secTagSize, secMac ) == FALSE ) )
    {
        fault ( "comsecInit refused a valid configuration" );
    }
    else
    {
        for ( step = 0u; step < 120u; ++step )
        {
            action = ( uint8_t ) rngBelow ( 9u );
            frame = NULL;
            intact = FALSE;
            foreignFrame = FALSE;

            if ( action <= 4u )
            {
                length = 1u + rngBelow ( rxSize );
                payload = exact ( length );
                fillRandom ( payload, length, NULL, 0u );

                if ( comsecBuildFrame ( &tx, payload, length, &frameLength ) == TRUE )
                {
                    frame = exact ( frameLength );
                    memcpy ( frame, txBuffer, frameLength );
                    intact = TRUE;

                    i = rngBelow ( SEC_HISTORY );
                    free ( history[ i ] );
                    history[ i ] = exact ( frameLength );
                    memcpy ( history[ i ], frame, frameLength );
                    historyLength[ i ] = frameLength;

                    if ( action == 1u )
                    {
                        frame[ rngBelow ( frameLength ) ] ^=
                            ( uint8_t ) ( 1u + rngBelow ( 255u ) );
                        intact = FALSE;
                    }
                    else if ( action == 2u )
                    {
                        /* Lost on the way, which comsec must tolerate. */
                        free ( frame );
                        frame = NULL;
                    }
                    else
                    {
                        /* Intentionally blank */
                    }
                }
                else
                {
                    fault ( "comsecBuildFrame refused a payload that fits" );
                }

                free ( payload );
            }
            else if ( action == 5u )
            {
                i = rngBelow ( SEC_HISTORY );

                if ( history[ i ] != NULL )
                {
                    frameLength = historyLength[ i ];
                    frame = exact ( frameLength );
                    memcpy ( frame, history[ i ], frameLength );
                    intact = TRUE;
                }
                else
                {
                    /* Intentionally blank */
                }
            }
            else if ( action == 8u )
            {
                /* Authentic, fresh, and for another session. */
                length = 1u + rngBelow ( rxSize );
                payload = exact ( length );
                fillRandom ( payload, length, NULL, 0u );

                if ( comsecBuildFrame ( &foreign, payload, length, &frameLength ) == TRUE )
                {
                    frame = exact ( frameLength );
                    memcpy ( frame, foreignBuffer, frameLength );
                    foreignFrame = TRUE;
                }
                else
                {
                    fault ( "comsecBuildFrame refused a payload that fits" );
                }

                free ( payload );
            }
            else
            {
                frameLength = rngBelow ( rxSize + overhead + 8u );
                frame = exact ( frameLength );
                fillRandom ( frame, frameLength, NULL, 0u );
            }

            if ( frame != NULL )
            {
                counterBefore = rx.rxCounter;
                armedBefore = rx.rxArmed;
                counter = 0u;

                if ( frameLength >= COMSEC_HEADER )
                {
                    counter = ( ( uint32_t ) frame[ 2 ] << 24 ) |
                              ( ( uint32_t ) frame[ 3 ] << 16 ) |
                              ( ( uint32_t ) frame[ 4 ] << 8 ) |
                              ( uint32_t ) frame[ 5 ];
                }
                else
                {
                    /* Intentionally blank */
                }

                ok = comsecCheckFrame ( &rx, frame, frameLength );

                /*
                 * An intact frame is accepted exactly when it is newer than
                 * everything accepted so far, which covers a fresh frame, a
                 * frame after some were lost, and a replay alike.
                 */
                if ( intact == TRUE )
                {
                    expect ( ( uint8_t ) ( ok == ( uint8_t ) ( ( armed == FALSE ) ||
                                                    ( counter > lastAccepted ) ) ),
                             "comsec judged an intact frame wrongly" );
                }
                else if ( foreignFrame == TRUE )
                {
                    expect ( ( uint8_t ) ( ok == FALSE ),
                             "comsec accepted a frame from another session" );
                }
                else
                {
                    /* Intentionally blank */
                }

                if ( ok == TRUE )
                {
                    length = frameLength - overhead;
                    secMac ( frame, COMSEC_HEADER + length, tag );

                    expect ( ( uint8_t ) ( comsecGetPayloadLength ( &rx ) == length ),
                             "comsec reported the wrong payload length" );
                    expect ( ( uint8_t ) ( memcmp ( comsecGetPayload ( &rx ),
                             &frame[ COMSEC_HEADER ], length ) == 0 ),
                             "comsec delivered a payload the frame did not carry" );
                    expect ( ( uint8_t ) ( memcmp ( tag, &frame[ COMSEC_HEADER + length ],
                                                    secTagSize ) == 0 ),
                             "comsec accepted a frame whose tag does not hold" );
                    expect ( ( uint8_t ) ( ( ( ( uint16_t ) frame[ 0 ] << 8 ) | frame[ 1 ] ) == session ),
                             "comsec accepted a frame from another session" );
                    expect ( ( uint8_t ) ( ( armedBefore == FALSE ) || ( counter > counterBefore ) ),
                             "comsec accepted a counter that was not newer" );

                    armed = TRUE;
                    lastAccepted = counter;
                }
                else
                {
                    expect ( ( uint8_t ) ( ( rx.rxCounter == counterBefore ) &&
                                           ( rx.rxArmed == armedBefore ) ),
                             "comsec moved its counter on a refusal" );
                }

                free ( frame );
            }
            else
            {
                /* Intentionally blank */
            }
        }
    }

    for ( i = 0u; i < SEC_HISTORY; ++i )
    {
        free ( history[ i ] );
    }

    free ( txBuffer );
    free ( foreignBuffer );
    free ( txScratch );
    free ( rxBuffer );
}

/* ------------------------------------------------------------------------ */
/* comgenbuf                                                                 */
/* ------------------------------------------------------------------------ */

/* More than a ring of 302 bytes can hold, at three bytes a packet. */
#define MODEL_PACKETS   128u
#define MODEL_BYTES     400u

/* The same queue, written the obvious way, to compare against. */
static struct
{
    uint8_t data[ MODEL_PACKETS ][ MODEL_BYTES ];
    uint32_t length[ MODEL_PACKETS ];
    uint32_t first;
    uint32_t count;
    uint32_t used;
    uint32_t drops;
} model;

static void genbufRound ( void )
{
    comgenbuf_t driver;
    uint8_t* ring = NULL;
    uint8_t* data = NULL;
    uint8_t* before = NULL;
    uint32_t size = 0u;
    uint32_t step = 0u;
    uint32_t length = 0u;
    uint32_t capacity = 0u;
    uint32_t popped = 0u;
    uint32_t slot = 0u;
    uint8_t ok = FALSE;
    uint8_t fits = FALSE;

    size = 3u + rngBelow ( 300u );
    ring = exact ( size );
    memset ( &model, 0, sizeof ( model ) );

    if ( comgenbufInit ( &driver, ring, size ) == FALSE )
    {
        fault ( "comgenbufInit refused a valid configuration" );
    }
    else
    {
        for ( step = 0u; step < 300u; ++step )
        {
            if ( rngBelow ( 2u ) == 0u )
            {
                length = rngBelow ( size + 3u );
                data = exact ( length );
                fillRandom ( data, length, NULL, 0u );

                fits = ( uint8_t ) ( ( length != 0u ) &&
                                     ( ( size - model.used ) >= ( length + 2u ) ) );

                ok = comgenbufPush ( &driver, data, length );
                expect ( ( uint8_t ) ( ok == fits ),
                         "comgenbufPush disagreed with the model on room" );

                if ( fits == TRUE )
                {
                    slot = ( model.first + model.count ) % MODEL_PACKETS;
                    memcpy ( model.data[ slot ], data, length );
                    model.length[ slot ] = length;
                    ++model.count;
                    model.used += length + 2u;
                }
                else if ( length != 0u )
                {
                    ++model.drops;
                }
                else
                {
                    /* Intentionally blank */
                }

                free ( data );
            }
            else
            {
                /* A destination of any size, to exercise the refusal too. */
                capacity = rngBelow ( size + 3u );
                data = exact ( capacity );
                before = exact ( capacity );
                fillRandom ( data, capacity, NULL, 0u );
                memcpy ( before, data, capacity );

                if ( model.count != 0u )
                {
                    expect ( ( uint8_t ) ( comgenbufPeekLength ( &driver ) ==
                                           model.length[ model.first ] ),
                             "comgenbufPeekLength disagreed with the model" );
                }
                else
                {
                    /* Intentionally blank */
                }

                popped = comgenbufPop ( &driver, data, capacity );

                if ( ( model.count != 0u ) && ( capacity >= model.length[ model.first ] ) )
                {
                    expect ( ( uint8_t ) ( ( popped == model.length[ model.first ] ) &&
                             ( memcmp ( data, model.data[ model.first ], popped ) == 0 ) ),
                             "comgenbufPop returned a different packet" );
                    model.used -= model.length[ model.first ] + 2u;
                    model.first = ( model.first + 1u ) % MODEL_PACKETS;
                    --model.count;
                }
                else
                {
                    expect ( ( uint8_t ) ( popped == 0u ),
                             "comgenbufPop returned a packet it could not deliver" );
                    expect ( ( uint8_t ) ( memcmp ( data, before, capacity ) == 0 ),
                             "comgenbufPop wrote into a destination it refused" );
                }

                free ( data );
                free ( before );
            }

            expect ( ( uint8_t ) ( ( comgenbufGetCount ( &driver ) == model.count ) &&
                                   ( comgenbufGetFree ( &driver ) == ( size - model.used ) ) &&
                                   ( comgenbufGetDropCount ( &driver ) == model.drops ) ),
                     "comgenbuf counters disagreed with the model" );
        }
    }

    free ( ring );
}

/* ------------------------------------------------------------------------ */

static void run ( const char* name, uint32_t target, void ( *round ) ( void ) )
{
    char line[ 96 ];

    currentTarget = name;
    targetFaults = 0u;

    for ( currentRound = 0u; currentRound < rounds; ++currentRound )
    {
        rngRound ( target, currentRound );
        round ( );
    }

    sprintf ( line, "%s: %lu rounds, no fault", name, ( unsigned long ) rounds );
    check ( line, ( uint8_t ) ( targetFaults == 0u ) );
}

static uint32_t fromEnvironment ( const char* name, uint32_t fallback )
{
    uint32_t retVal = fallback;
    const char* value = getenv ( name );

    if ( ( value != NULL ) && ( value[ 0 ] != '\0' ) )
    {
        retVal = ( uint32_t ) strtoul ( value, NULL, 0 );
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

int main ( void )
{
    baseSeed = fromEnvironment ( "ESCLIB_FUZZ_SEED", DEFAULT_SEED );
    rounds = fromEnvironment ( "ESCLIB_FUZZ_ROUNDS", DEFAULT_ROUNDS );

    printf ( "seed %lu, %lu rounds per target\n\n",
             ( unsigned long ) baseSeed, ( unsigned long ) rounds );

    run ( "comstxetx", 1u, stxRound );
    run ( "comat", 2u, atRound );
    run ( "commodbus", 3u, mbRound );
    run ( "cobs", 4u, cobsRound );
    run ( "comsafe", 5u, safeRound );
    run ( "comsec", 6u, secRound );
    run ( "comgenbuf", 7u, genbufRound );

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
