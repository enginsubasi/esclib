/*
 * A framed serial link, end to end, using three modules of this library.
 *
 *     comstxetx   the framing: STX, ETX, DLE escaping, and a CRC per frame
 *     crc16       the CRC, installed into comstxetx at Init
 *     comgenbuf   the queue between the interrupt and the main loop
 *
 * The point of the example is the division of labour, which is the same in
 * every project that uses these: the interrupt does comstxetxReceive one byte
 * at a time and nothing else, the main loop does comstxetxEvaluate, and the
 * finished packet lands in a queue that keeps its boundaries so the main loop
 * gets whole frames rather than a run of bytes.
 *
 * The wire is faked as an array so this builds and runs anywhere.
 *
 *     gcc -Iinc/communication -Iinc/crc sample/FramedLink/FramedLink.c \
 *         src/communication/comstxetx.c src/communication/comgenbuf.c \
 *         src/crc/crc16.c -o link
 */

#include <stdio.h>

#include "comstxetx.h"
#include "comgenbuf.h"
#include "crc16.h"

#define STX     0x02u
#define ETX     0x03u
#define DLE     0x10u

/*
 * comstxetx hands a finished packet to a callback that takes no context
 * pointer, which is the shape of every callback in this library. A real
 * interrupt reaches its queue the way this does.
 */
static comgenbuf_t* inbox = NULL;

static void packetArrived ( uint8_t* buffer, uint32_t length )
{
    if ( comgenbufPush ( inbox, buffer, length ) != TRUE )
    {
        /*
         * The queue was full. The packet is gone and comgenbufGetDropCount
         * has counted it; there is nothing useful to do about it in here,
         * which is exactly why the module counts rather than blocking.
         */
    }
}

int main ( void )
{
    comstxetx_t link;
    comgenbuf_t queue;

    uint8_t rxBuffer[ 128 ];
    uint8_t txBuffer[ 128 ];
    uint8_t queueStore[ 512 ];
    uint8_t wire[ 512 ];
    uint8_t packet[ 64 ];

    uint32_t wireLength = 0;
    uint32_t frameLength = 0;
    uint32_t packetLength = 0;
    uint32_t i = 0;

    /*
     * A payload holding the three framing bytes themselves. comstxetx escapes
     * them with DLE so any byte value crosses the link — which is the whole
     * difference between it and a protocol that reserves values.
     */
    static const uint8_t message1[ 6 ] = { 'h', 'i', STX, ETX, DLE, '!' };
    static const uint8_t message2[ 4 ] = { 0xFFu, 0x00u, 0xFFu, 0x00u };

    inbox = &queue;

    if ( comgenbufInit ( &queue, queueStore, 512u ) != TRUE )
    {
        printf ( "queue init failed\n" );
        return ( 1 );
    }

    /*
     * crc16 goes in with no wrapper because comstxetx's hook was given its
     * signature on purpose. checksumFletcher16 fits the same hole when a full
     * CRC is more than the link needs.
     */
    if ( comstxetxInit ( &link, rxBuffer, txBuffer, 128u, 128u,
                         STX, ETX, DLE, 100u, crc16, packetArrived ) != TRUE )
    {
        printf ( "link init failed\n" );
        return ( 1 );
    }

    /* ---- the sending side ---- */

    if ( comstxetxBuildFrame ( &link, message1, 6u, &frameLength ) != TRUE )
    {
        printf ( "frame build failed\n" );
        return ( 1 );
    }

    for ( i = 0; i < frameLength; ++i )
    {
        wire[ wireLength ] = txBuffer[ i ];
        ++wireLength;
    }

    printf ( "a six byte payload holding STX, ETX and DLE went out as %u bytes\n",
             ( unsigned ) frameLength );

    if ( comstxetxBuildFrame ( &link, message2, 4u, &frameLength ) != TRUE )
    {
        printf ( "frame build failed\n" );
        return ( 1 );
    }

    for ( i = 0; i < frameLength; ++i )
    {
        wire[ wireLength ] = txBuffer[ i ];
        ++wireLength;
    }

    printf ( "a four byte payload went out as %u\n\n", ( unsigned ) frameLength );

    /* ---- the interrupt side ---- */

    /*
     * Both frames arrive back to back with no main loop in between, which is
     * ordinary on a busy link. comstxetxEvaluate is called once per frame; in
     * a real program the main loop calls it whenever it goes round.
     */
    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, wire[ i ] );

        comstxetxEvaluate ( &link );
    }

    /* ---- the main loop side ---- */

    printf ( "the queue holds %u packets, %u bytes free\n",
             ( unsigned ) comgenbufGetCount ( &queue ),
             ( unsigned ) comgenbufGetFree ( &queue ) );

    while ( comgenbufGetCount ( &queue ) != 0u )
    {
        /*
         * The length comes out first so the destination can be sized against
         * it. A pop into something too small leaves the packet in the queue
         * rather than truncating it.
         */
        packetLength = comgenbufPeekLength ( &queue );

        if ( packetLength > sizeof ( packet ) )
        {
            printf ( "a %u byte packet does not fit, dropping it\n",
                     ( unsigned ) packetLength );
            ( void ) comgenbufPop ( &queue, packet, sizeof ( packet ) );
            continue;
        }

        packetLength = comgenbufPop ( &queue, packet, sizeof ( packet ) );

        printf ( "got %u bytes:", ( unsigned ) packetLength );

        for ( i = 0; i < packetLength; ++i )
        {
            printf ( " %02X", packet[ i ] );
        }

        printf ( "\n" );
    }

    /*
     * A frame that fails its CRC is dropped and counted rather than handed up.
     * Corrupting one byte of the wire shows it.
     */
    wire[ 3 ] = ( uint8_t ) ( wire[ 3 ] ^ 0x20u );

    for ( i = 0; i < wireLength; ++i )
    {
        comstxetxReceive ( &link, wire[ i ] );
        comstxetxEvaluate ( &link );
    }

    printf ( "\nafter flipping one bit on the wire: %u frames rejected, "
             "%u packets delivered\n",
             ( unsigned ) comstxetxGetRejectCount ( &link ),
             ( unsigned ) comgenbufGetCount ( &queue ) );

    printf ( "packets the queue had to refuse for want of room: %u\n",
             ( unsigned ) comgenbufGetDropCount ( &queue ) );

    return ( 0 );
}
