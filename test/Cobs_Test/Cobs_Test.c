/*
 * Covers cobs.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Two properties carry most of this file and neither is checked by comparing
 * against a table of expected bytes. The first is that no zero byte survives
 * the encoding, which is the entire reason the encoding exists — so every case
 * here checks it, not just the ones that obviously contain zeros. The second is
 * that whatever goes in comes back out, checked over payload shapes chosen for
 * where the algorithm changes behaviour rather than for variety: at the run
 * length of 254, on either side of it, all zeros, no zeros, and the zeros at
 * the ends where an off-by-one lives.
 */

#include <stdio.h>

#include "cobs.h"

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

#define BUFFER_SIZE     1024u

static uint8_t payload [ BUFFER_SIZE ];
static uint8_t encoded [ BUFFER_SIZE ];
static uint8_t decoded [ BUFFER_SIZE ];

/*
 * Encodes, checks the shape of what came out, decodes, and checks it is what
 * went in. Everything a payload has to satisfy is in here, so a new payload
 * shape is one line.
 */
static uint8_t roundTrip ( const uint8_t* const source, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t encodedLength = 0u;
    uint32_t decodedLength = 0u;
    uint32_t i = 0u;

    if ( cobsEncode ( source, length, encoded, BUFFER_SIZE,
                      &encodedLength ) == FALSE )
    {
        retVal = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    /* The property the whole encoding exists for. */
    for ( i = 0u; i < encodedLength; ++i )
    {
        if ( encoded[ i ] == COBS_DELIMITER )
        {
            retVal = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    /* And the bound a caller sizes its buffer from. */
    if ( encodedLength > cobsEncodedSize ( length ) )
    {
        retVal = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( cobsDecode ( encoded, encodedLength, decoded, BUFFER_SIZE,
                      &decodedLength ) == FALSE )
    {
        retVal = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( decodedLength != length )
    {
        retVal = FALSE;
    }
    else
    {
        for ( i = 0u; i < length; ++i )
        {
            if ( decoded[ i ] != source[ i ] )
            {
                retVal = FALSE;
            }
            else
            {
                /* Intentionally blank */
            }
        }
    }

    return ( retVal );
}

/* ----------------------------------------------------------- round trips */

static void roundTripCase ( void )
{
    uint32_t i = 0u;

    printf ( "what goes in comes out\n" );

    check ( "nothing at all", roundTrip ( payload, 0u ) );

    payload[ 0 ] = 0x00u;
    check ( "one zero", roundTrip ( payload, 1u ) );

    payload[ 0 ] = 0x41u;
    check ( "one byte that is not zero", roundTrip ( payload, 1u ) );

    /* Zeros at the ends, where an off-by-one in the run length lives. */
    payload[ 0 ] = 0x00u;
    payload[ 1 ] = 0x11u;
    payload[ 2 ] = 0x22u;
    payload[ 3 ] = 0x00u;
    check ( "a zero at each end", roundTrip ( payload, 4u ) );

    for ( i = 0u; i < 16u; ++i )
    {
        payload[ i ] = 0x00u;
    }

    check ( "nothing but zeros", roundTrip ( payload, 16u ) );

    for ( i = 0u; i < 300u; ++i )
    {
        payload[ i ] = ( uint8_t ) ( ( i % 255u ) + 1u );
    }

    check ( "three hundred bytes with no zero among them",
            roundTrip ( payload, 300u ) );

    /*
     * The run length itself and its neighbours. A code byte describes at most
     * 254 payload bytes, so this is where the encoder has to start a second
     * group with no zero to hang it on, and where the decoder has to know not
     * to put a zero back.
     */
    for ( i = 0u; i < 512u; ++i )
    {
        payload[ i ] = 0xAAu;
    }

    check ( "one short of a full run", roundTrip ( payload, 253u ) );
    check ( "exactly a full run", roundTrip ( payload, 254u ) );
    check ( "one past a full run", roundTrip ( payload, 255u ) );
    check ( "two full runs", roundTrip ( payload, 508u ) );

    /* A full run with a zero immediately after it. */
    payload[ 254 ] = 0x00u;
    check ( "a full run and then a zero", roundTrip ( payload, 256u ) );

    /* Every byte value there is, in order, several times over. */
    for ( i = 0u; i < 1000u; ++i )
    {
        payload[ i ] = ( uint8_t ) ( i % 256u );
    }

    check ( "every byte value, repeatedly", roundTrip ( payload, 1000u ) );
}

/* ------------------------------------------------------------- the bound */

static void sizeCase ( void )
{
    uint32_t i = 0u;
    uint32_t encodedLength = 0u;
    uint8_t exact = TRUE;

    printf ( "cobsEncodedSize\n" );

    check ( "nothing still needs one byte",
            ( uint8_t ) ( cobsEncodedSize ( 0u ) == 1u ) );
    check ( "one short of a run costs one byte",
            ( uint8_t ) ( cobsEncodedSize ( 253u ) == 254u ) );
    check ( "a full run costs two",
            ( uint8_t ) ( cobsEncodedSize ( 254u ) == 256u ) );
    check ( "and so does one past it",
            ( uint8_t ) ( cobsEncodedSize ( 255u ) == 257u ) );

    /*
     * The bound is reached, not merely respected. A payload with no zero in it
     * is the worst case, so the encoding of one must come to exactly what the
     * sizing function promised — a bound that is never reached would be a
     * caller allocating for nothing.
     */
    for ( i = 0u; i < 600u; ++i )
    {
        payload[ i ] = 0x7Fu;
    }

    for ( i = 0u; i <= 600u; ++i )
    {
        if ( cobsEncode ( payload, i, encoded, BUFFER_SIZE,
                          &encodedLength ) == FALSE )
        {
            exact = FALSE;
        }
        else if ( encodedLength != cobsEncodedSize ( i ) )
        {
            exact = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "a payload with no zero in it reaches the bound exactly", exact );

    /*
     * And the overhead is what the file claims: one byte per 254, which is
     * where the argument against an escaping scheme lives. A payload of a
     * thousand bytes costs four.
     */
    check ( "a thousand bytes cost four",
            ( uint8_t ) ( cobsEncodedSize ( 1000u ) == 1004u ) );
}

/* ------------------------------------------------------------- refusals */

static void refusalCase ( void )
{
    uint32_t encodedLength = 0u;
    uint32_t decodedLength = 0u;
    uint32_t i = 0u;
    uint8_t untouched = TRUE;
    uint8_t frame [ 8 ];

    printf ( "what is refused\n" );

    for ( i = 0u; i < 16u; ++i )
    {
        payload[ i ] = 0x33u;
    }

    /* A destination one byte short of the bound. */
    for ( i = 0u; i < BUFFER_SIZE; ++i )
    {
        encoded[ i ] = 0xA5u;
    }

    check ( "a destination too small is refused",
            ( uint8_t ) ( cobsEncode ( payload, 16u, encoded,
                                       cobsEncodedSize ( 16u ) - 1u,
                                       &encodedLength ) == FALSE ) );

    /* And refused before anything was written into it. */
    for ( i = 0u; i < BUFFER_SIZE; ++i )
    {
        if ( encoded[ i ] != 0xA5u )
        {
            untouched = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "and nothing was written into it", untouched );

    check ( "a destination exactly the size of the bound is enough",
            cobsEncode ( payload, 16u, encoded, cobsEncodedSize ( 16u ),
                         &encodedLength ) );

    /* A frame of nothing is not the encoding of anything. */
    check ( "an empty frame is refused",
            ( uint8_t ) ( cobsDecode ( encoded, 0u, decoded, BUFFER_SIZE,
                                       &decodedLength ) == FALSE ) );

    /*
     * A code byte of zero. The encoder cannot produce one — that is the whole
     * point of it — so a frame carrying one did not come from an encoder, and
     * following it would walk the length backwards.
     */
    frame[ 0 ] = 0x03u;
    frame[ 1 ] = 0x11u;
    frame[ 2 ] = 0x22u;
    frame[ 3 ] = 0x00u;
    frame[ 4 ] = 0x44u;

    check ( "a zero code byte is refused",
            ( uint8_t ) ( cobsDecode ( frame, 5u, decoded, BUFFER_SIZE,
                                       &decodedLength ) == FALSE ) );

    /* A code byte claiming a run that runs off the end of the frame. */
    frame[ 0 ] = 0x08u;
    frame[ 1 ] = 0x11u;
    frame[ 2 ] = 0x22u;

    check ( "a run reaching past the end is refused",
            ( uint8_t ) ( cobsDecode ( frame, 3u, decoded, BUFFER_SIZE,
                                       &decodedLength ) == FALSE ) );

    /* A sound frame whose payload will not fit where it is going. */
    frame[ 0 ] = 0x04u;
    frame[ 1 ] = 0x11u;
    frame[ 2 ] = 0x22u;
    frame[ 3 ] = 0x33u;

    for ( i = 0u; i < BUFFER_SIZE; ++i )
    {
        decoded[ i ] = 0xC3u;
    }

    check ( "a payload that will not fit is refused",
            ( uint8_t ) ( cobsDecode ( frame, 4u, decoded, 2u,
                                       &decodedLength ) == FALSE ) );

    untouched = TRUE;

    for ( i = 0u; i < BUFFER_SIZE; ++i )
    {
        if ( decoded[ i ] != 0xC3u )
        {
            untouched = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "and the destination was left alone", untouched );

    check ( "the same frame into a destination that fits",
            ( uint8_t ) ( ( cobsDecode ( frame, 4u, decoded, 3u,
                                         &decodedLength ) == TRUE ) &&
                          ( decodedLength == 3u ) ) );
}

/* ------------------------------------------------------------ the frame */

static void framingCase ( void )
{
    uint32_t encodedLength = 0u;
    uint32_t decodedLength = 0u;
    uint32_t i = 0u;
    uint32_t j = 0u;
    uint32_t start = 0u;
    uint32_t count = 0u;
    uint32_t frames = 0u;
    uint32_t delimiters = 0u;
    uint8_t wire [ 64 ];
    uint8_t chunk [ 64 ];
    uint8_t first [ 8 ];
    uint8_t second [ 8 ];
    uint32_t firstLength = 0u;
    uint32_t secondLength = 0u;
    uint8_t ok = TRUE;

    printf ( "two frames on a wire\n" );

    /*
     * What the module is for, written out: two payloads encoded into one
     * stream with a zero after each, then recovered by splitting on the zero.
     * This is the half a caller does, which is why cobsEncode does not write
     * the delimiter itself — one delimiter between frames or two around each
     * is the caller's protocol rather than this module's.
     *
     * Both payloads carry a zero of their own, which is the point: after the
     * encoding they do not, so the only zeros left in the stream are the two
     * the caller put there.
     */
    payload[ 0 ] = 0x01u;
    payload[ 1 ] = 0x00u;
    payload[ 2 ] = 0x02u;

    check ( "the first frame encodes",
            cobsEncode ( payload, 3u, encoded, BUFFER_SIZE, &encodedLength ) );

    for ( i = 0u; i < encodedLength; ++i )
    {
        wire[ count ] = encoded[ i ];
        ++count;
    }

    wire[ count ] = COBS_DELIMITER;
    ++count;

    payload[ 0 ] = 0xFFu;
    payload[ 1 ] = 0x00u;

    check ( "the second frame encodes",
            cobsEncode ( payload, 2u, encoded, BUFFER_SIZE, &encodedLength ) );

    for ( i = 0u; i < encodedLength; ++i )
    {
        wire[ count ] = encoded[ i ];
        ++count;
    }

    wire[ count ] = COBS_DELIMITER;
    ++count;

    /* A receiver's whole job: what lies between two delimiters is a frame. */
    start = 0u;

    for ( i = 0u; i < count; ++i )
    {
        if ( wire[ i ] == COBS_DELIMITER )
        {
            ++delimiters;

            for ( j = start; j < i; ++j )
            {
                chunk[ j - start ] = wire[ j ];
            }

            if ( cobsDecode ( chunk, i - start, decoded, BUFFER_SIZE,
                              &decodedLength ) == FALSE )
            {
                ok = FALSE;
            }
            else if ( frames == 0u )
            {
                firstLength = decodedLength;

                for ( j = 0u; j < decodedLength; ++j )
                {
                    first[ j ] = decoded[ j ];
                }

                ++frames;
            }
            else
            {
                secondLength = decodedLength;

                for ( j = 0u; j < decodedLength; ++j )
                {
                    second[ j ] = decoded[ j ];
                }

                ++frames;
            }

            start = i + 1u;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    check ( "every frame on the wire decoded", ok );
    check ( "and there were two of them",
            ( uint8_t ) ( ( frames == 2u ) && ( delimiters == 2u ) ) );

    check ( "the first is what was put in",
            ( uint8_t ) ( ( firstLength == 3u ) && ( first[ 0 ] == 0x01u ) &&
                          ( first[ 1 ] == 0x00u ) && ( first[ 2 ] == 0x02u ) ) );
    check ( "and so is the second",
            ( uint8_t ) ( ( secondLength == 2u ) && ( second[ 0 ] == 0xFFu ) &&
                          ( second[ 1 ] == 0x00u ) ) );

    /*
     * And the only zeros in the whole stream are the two delimiters, although
     * both payloads contained one. That is the property the framing rests on:
     * a receiver may resynchronize on a zero after any amount of noise and
     * know it is at a frame boundary.
     */
    check ( "the only zeros left are the delimiters",
            ( uint8_t ) ( delimiters == 2u ) );
}

int main ( void )
{
    roundTripCase ( );
    printf ( "\n" );
    sizeCase ( );
    printf ( "\n" );
    refusalCase ( );
    printf ( "\n" );
    framingCase ( );

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
