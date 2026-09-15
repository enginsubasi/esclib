/*
 * Covers comgenbuf.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The buffer is small on purpose — small enough that every case here wraps the
 * ring at least once, including one that puts a packet's two byte length
 * header across the wrap, which is the place a byte oriented implementation
 * gets it wrong.
 */

#include <stddef.h>
#include <stdio.h>

#include "comgenbuf.h"

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

static uint8_t same ( const uint8_t* const got, const uint8_t* const want, uint32_t length )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0;

    for ( i = 0; i < length; ++i )
    {
        if ( got[ i ] != want[ i ] )
        {
            retVal = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    return ( retVal );
}

static const uint8_t packetA[ 3 ] = { 0x11u, 0x22u, 0x33u };
static const uint8_t packetB[ 5 ] = { 0xA0u, 0xA1u, 0xA2u, 0xA3u, 0xA4u };
static const uint8_t packetC[ 1 ] = { 0x7Fu };

/* ---------------------------------------------------------------- init */

static void initCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 32 ];

    printf ( "comgenbufInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( comgenbufInit ( NULL, storage, 32u ) == FALSE ) );
    check ( "a NULL buffer is rejected",
            ( uint8_t ) ( comgenbufInit ( &driver, NULL, 32u ) == FALSE ) );

    /*
     * A size of two holds a header and no payload, so it could never store a
     * packet. Rejecting it is better than accepting a queue that silently
     * refuses everything.
     */
    check ( "a size of zero is rejected",
            ( uint8_t ) ( comgenbufInit ( &driver, storage, 0u ) == FALSE ) );
    check ( "a size of two, which is header and nothing else, is rejected",
            ( uint8_t ) ( comgenbufInit ( &driver, storage, 2u ) == FALSE ) );
    check ( "a size of three, the smallest that can hold anything, is accepted",
            comgenbufInit ( &driver, storage, 3u ) );

    check ( "a full init succeeds", comgenbufInit ( &driver, storage, 32u ) );
    check ( "and starts empty",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 0u ) );
    check ( "with the whole buffer free",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == 32u ) );
    check ( "no drops", ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 0u ) );
    check ( "and nothing to peek at",
            ( uint8_t ) ( comgenbufPeekLength ( &driver ) == 0u ) );
}

/* -------------------------------------------------------- push and pop */

static void roundTripCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 32 ];
    uint8_t out[ 8 ];
    uint32_t got = 0;

    printf ( "push and pop\n" );

    check ( "Init", comgenbufInit ( &driver, storage, 32u ) );

    check ( "a NULL data pointer is rejected",
            ( uint8_t ) ( comgenbufPush ( &driver, NULL, 3u ) == FALSE ) );
    check ( "a zero length is rejected",
            ( uint8_t ) ( comgenbufPush ( &driver, packetA, 0u ) == FALSE ) );

    /*
     * A rejected argument is not a drop. The drop count is there to say the
     * consumer is too slow, and hiding a caller mistake in the same number
     * would make it useless for that.
     */
    check ( "and neither counted as a drop",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 0u ) );

    check ( "pushing a three byte packet",
            comgenbufPush ( &driver, packetA, 3u ) );
    check ( "the queue holds one",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 1u ) );
    check ( "and it cost its length plus a two byte header",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == ( 32u - 5u ) ) );
    check ( "peek reports its length",
            ( uint8_t ) ( comgenbufPeekLength ( &driver ) == 3u ) );
    check ( "and peeking did not remove it",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 1u ) );

    got = comgenbufPop ( &driver, out, 8u );
    check ( "popping reports the length", ( uint8_t ) ( got == 3u ) );
    check ( "and the bytes come back unchanged",
            same ( out, packetA, 3u ) );
    check ( "the queue is empty again",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 0u ) );
    check ( "and the space came back",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == 32u ) );

    check ( "popping an empty queue reports nothing",
            ( uint8_t ) ( comgenbufPop ( &driver, out, 8u ) == 0u ) );

    /* Packet boundaries survive, which is the whole reason this is not circBuf. */
    check ( "push A", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "push B", comgenbufPush ( &driver, packetB, 5u ) );
    check ( "push C", comgenbufPush ( &driver, packetC, 1u ) );
    check ( "three packets are queued",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 3u ) );

    check ( "the first out is A, whole",
            ( uint8_t ) ( ( comgenbufPop ( &driver, out, 8u ) == 3u ) &&
                          same ( out, packetA, 3u ) ) );
    check ( "then B, whole and separate",
            ( uint8_t ) ( ( comgenbufPop ( &driver, out, 8u ) == 5u ) &&
                          same ( out, packetB, 5u ) ) );
    check ( "then C",
            ( uint8_t ) ( ( comgenbufPop ( &driver, out, 8u ) == 1u ) &&
                          same ( out, packetC, 1u ) ) );
    check ( "and the queue is empty",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 0u ) );
}

/* --------------------------------------------------------- small buffer */

static void capacityCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 32 ];
    uint8_t out[ 8 ];

    printf ( "a destination that is too small\n" );

    check ( "Init", comgenbufInit ( &driver, storage, 32u ) );
    check ( "push B", comgenbufPush ( &driver, packetB, 5u ) );

    /*
     * A destination too small leaves the packet in the queue rather than
     * truncating it or throwing it away. The caller sizes its buffer from
     * comgenbufPeekLength and comes back.
     */
    check ( "a pop into four bytes reports nothing",
            ( uint8_t ) ( comgenbufPop ( &driver, out, 4u ) == 0u ) );
    check ( "and the packet is still queued",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 1u ) );
    check ( "with its length still readable",
            ( uint8_t ) ( comgenbufPeekLength ( &driver ) == 5u ) );

    check ( "a pop into exactly five bytes works",
            ( uint8_t ) ( comgenbufPop ( &driver, out, 5u ) == 5u ) );
    check ( "and the bytes are right", same ( out, packetB, 5u ) );
}

/* ----------------------------------------------------------- full queue */

static void fullCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 16 ];
    uint8_t out[ 8 ];

    printf ( "a full queue\n" );

    /* Sixteen bytes holds three five byte entries with one byte left over. */
    check ( "Init", comgenbufInit ( &driver, storage, 16u ) );

    check ( "push one", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "push two", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "push three", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "one byte is left",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == 1u ) );

    /*
     * The fourth does not fit and is refused whole. The newest is refused
     * rather than the oldest being dropped: a packet the queue already
     * accepted is a message the caller was told it would get.
     */
    check ( "a fourth is refused",
            ( uint8_t ) ( comgenbufPush ( &driver, packetA, 3u ) == FALSE ) );
    check ( "and counted as a drop",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 1u ) );
    check ( "the three already queued are untouched",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 3u ) );
    check ( "and the free space did not move",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == 1u ) );

    /* Even a one byte packet needs three bytes, so it does not fit either. */
    check ( "a one byte packet does not fit in one free byte",
            ( uint8_t ) ( comgenbufPush ( &driver, packetC, 1u ) == FALSE ) );
    check ( "and is counted too",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 2u ) );

    /* Making room lets the next one in. */
    check ( "popping one frees five bytes",
            ( uint8_t ) ( comgenbufPop ( &driver, out, 8u ) == 3u ) );
    check ( "so the next push fits", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "and the drop count did not move",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 2u ) );
}

/* ---------------------------------------------------------------- wrap */

static void wrapCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 11 ];
    uint8_t out[ 8 ];
    uint32_t i = 0;
    uint8_t good = TRUE;

    printf ( "wrapping the ring\n" );

    /*
     * Eleven bytes is not a multiple of a five byte entry, so pushing and
     * popping repeatedly walks the head and tail through every offset in the
     * ring. Somewhere in here a packet's two byte length header lands across
     * the wrap, which is the case a length written as a single 16-bit store
     * would get wrong.
     */
    check ( "Init", comgenbufInit ( &driver, storage, 11u ) );

    good = TRUE;

    for ( i = 0; i < 50u; ++i )
    {
        if ( comgenbufPush ( &driver, packetA, 3u ) == FALSE )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( comgenbufPeekLength ( &driver ) != 3u )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( comgenbufPop ( &driver, out, 8u ) != 3u )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( same ( out, packetA, 3u ) == FALSE )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "fifty round trips through every offset in the ring", good );
    check ( "and nothing was dropped along the way",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 0u ) );

    /* Two packets in flight at once, so the tail trails the head around. */
    check ( "re-init", comgenbufInit ( &driver, storage, 11u ) );

    good = TRUE;

    for ( i = 0; i < 50u; ++i )
    {
        if ( comgenbufPush ( &driver, packetC, 1u ) == FALSE )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( comgenbufPush ( &driver, packetA, 3u ) == FALSE )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( ( comgenbufPop ( &driver, out, 8u ) != 1u ) ||
                ( out[ 0 ] != packetC[ 0 ] ) )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }

        if ( ( comgenbufPop ( &driver, out, 8u ) != 3u ) ||
                ( same ( out, packetA, 3u ) == FALSE ) )
        {
            good = FALSE;
        }
        else
        {
            /* Intentionally blank. */
        }
    }

    check ( "and fifty more with two packets in flight at a time", good );
}

/* --------------------------------------------------------------- flush */

static void flushCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 16 ];

    printf ( "comgenbufFlush\n" );

    check ( "Init", comgenbufInit ( &driver, storage, 16u ) );
    check ( "push one", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "push two", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "push three", comgenbufPush ( &driver, packetA, 3u ) );
    check ( "and refuse a fourth",
            ( uint8_t ) ( comgenbufPush ( &driver, packetA, 3u ) == FALSE ) );

    comgenbufFlush ( &driver );

    check ( "the queue is empty",
            ( uint8_t ) ( comgenbufGetCount ( &driver ) == 0u ) );
    check ( "the whole buffer is free again",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == 16u ) );
    check ( "there is nothing to peek at",
            ( uint8_t ) ( comgenbufPeekLength ( &driver ) == 0u ) );

    /*
     * The drop count goes with it. It is a record of a consumer falling
     * behind, and a flush is the end of that episode rather than a moment in
     * the middle of it.
     */
    check ( "and the drop count is cleared with it",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 0u ) );

    check ( "the queue works again afterwards",
            comgenbufPush ( &driver, packetB, 5u ) );
    check ( "and reports the right length",
            ( uint8_t ) ( comgenbufPeekLength ( &driver ) == 5u ) );
}

/* ---------------------------------------------------- a long packet */

static void longCase ( void )
{
    comgenbuf_t driver;
    uint8_t storage[ 512 ];
    uint8_t big[ 300 ];
    uint8_t out[ 300 ];
    uint32_t i = 0;

    printf ( "a packet past 255 bytes\n" );

    for ( i = 0; i < 300u; ++i )
    {
        big[ i ] = ( uint8_t ) ( i & 0xFFu );
    }

    check ( "Init", comgenbufInit ( &driver, storage, 512u ) );

    /*
     * Every other packet here is short enough that the high byte of the two
     * byte length header is zero, so a length written or read as a single byte
     * would pass all of them. Three hundred is the shortest thing that tells
     * the two apart.
     */
    check ( "pushing three hundred bytes", comgenbufPush ( &driver, big, 300u ) );
    check ( "peek reports three hundred, not forty four",
            ( uint8_t ) ( comgenbufPeekLength ( &driver ) == 300u ) );
    check ( "and it cost three hundred and two bytes",
            ( uint8_t ) ( comgenbufGetFree ( &driver ) == ( 512u - 302u ) ) );

    check ( "popping reports three hundred",
            ( uint8_t ) ( comgenbufPop ( &driver, out, 300u ) == 300u ) );
    check ( "and every byte came back unchanged", same ( out, big, 300u ) );

    /* The largest length the header can describe is refused one past. */
    check ( "a length past what the header can hold is refused",
            ( uint8_t ) ( comgenbufPush ( &driver, big,
                            COMGENBUF_MAX_PACKET + 1u ) == FALSE ) );
    check ( "and is not counted as a drop, being a caller mistake",
            ( uint8_t ) ( comgenbufGetDropCount ( &driver ) == 0u ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    roundTripCase ( );
    printf ( "\n" );
    capacityCase ( );
    printf ( "\n" );
    fullCase ( );
    printf ( "\n" );
    wrapCase ( );
    printf ( "\n" );
    longCase ( );
    printf ( "\n" );
    flushCase ( );

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
