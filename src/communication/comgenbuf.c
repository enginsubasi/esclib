/**
  ******************************************************************************
  *
  * @file      comgenbuf.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Queue of variable length packets in one flat buffer.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      circBuf is a queue of bytes and loses where one packet ends and
  *            the next begins. That is the right shape for a character stream
  *            and the wrong one for the layer above comstxetx or comat, where
  *            the interrupt has already assembled a whole frame and the main
  *            loop wants it back whole. This is that layer: the packet
  *            boundaries are stored with the bytes.
  *
  * @note      Each packet is held as a two byte length followed by its
  *            payload, in one ring the caller owns. Two bytes rather than one
  *            because a 255 byte cap is below what a real frame reaches, and
  *            rather than four because a queue that needs packets longer than
  *            65535 bytes needs a different design anyway.
  *
  * @note      A push is all or nothing. A packet that does not fit is refused
  *            whole and counted in comgenbufGetDropCount; there is no partial
  *            write and no packet is ever truncated, because half a frame
  *            handed to a parser is worse than no frame.
  *
  * @note      A full queue refuses the **newest** packet rather than dropping
  *            the oldest, which is the opposite of circBuf's BB_OVERWRITE and
  *            is deliberate. Overwriting a byte in a stream loses a byte;
  *            dropping a packet the queue has already accepted loses a whole
  *            message the caller was told it would get. Refusing at the door
  *            is backpressure and the caller can see it in the drop count.
  *
  * @note      Written for one producer and one consumer — the usual interrupt
  *            and main loop pair. There is no locking here, as there is none
  *            anywhere in this library, and two producers need one of their
  *            own.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "comgenbuf.h"

/* Bytes of length header in front of every packet. */
#define COMGENBUF_HEADER    2u

/**
 * @brief   Writes one byte at the head and advances it.
 * @param[in,out] driver  Initialized queue.
 * @param[in]     value   Byte to store.
 * @note    The index wraps by comparison rather than by a modulo, so the ring
 *          size does not have to be a power of two and no division runs on the
 *          per byte path.
 */
static void comgenbufPutByte ( comgenbuf_t* driver, uint8_t value )
{
    driver->buffer[ driver->head ] = value;

    ++driver->head;

    if ( driver->head >= driver->size )
    {
        driver->head = 0;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Reads the byte at the given offset from the tail, without moving it.
 * @param[in] driver  Initialized queue.
 * @param[in] offset  Bytes past the tail.
 * @return  The byte found there.
 */
static uint8_t comgenbufPeekByte ( const comgenbuf_t* const driver, uint32_t offset )
{
    uint32_t index = 0;

    index = driver->tail + offset;

    if ( index >= driver->size )
    {
        index -= driver->size;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( driver->buffer[ index ] );
}

/**
 * @brief   Moves the tail forward by the given number of bytes.
 * @param[in,out] driver  Initialized queue.
 * @param[in]     bytes   How far to move.
 */
static void comgenbufDropBytes ( comgenbuf_t* driver, uint32_t bytes )
{
    driver->tail += bytes;

    if ( driver->tail >= driver->size )
    {
        driver->tail -= driver->size;
    }
    else
    {
        /* Intentionally blank */
    }

    driver->used -= bytes;
}

/**
 * @brief   Initializes the queue over a caller owned buffer.
 * @param[out] driver  Queue state to initialize.
 * @param[in]  buffer  Caller owned storage.
 * @param[in]  size    Bytes of storage.
 * @return  TRUE on success, FALSE when driver or buffer is NULL, or when size
 *          is too small to hold one packet of one byte.
 * @note    A size below three cannot hold a two byte header and a byte of
 *          payload, so it is rejected rather than accepted as a queue that can
 *          never store anything. The usable payload is always three bytes less
 *          than the size for one packet, and two bytes less per packet after
 *          that.
 */
uint8_t comgenbufInit ( comgenbuf_t* driver, uint8_t* buffer, uint32_t size )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( buffer != NULL ) &&
            ( size > COMGENBUF_HEADER ) )
    {
        driver->buffer = buffer;
        driver->size = size;
        driver->head = 0;
        driver->tail = 0;
        driver->used = 0;
        driver->count = 0;
        driver->dropCount = 0;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Stores one packet whole, or stores nothing.
 * @param[in,out] driver  Initialized queue.
 * @param[in]     data    Packet bytes.
 * @param[in]     length  Number of bytes.
 * @return  TRUE when the packet was stored, FALSE when it was refused.
 * @note    A refusal for want of room increments the drop count. A refusal for
 *          a bad argument — a NULL pointer, a zero length, a length past
 *          COMGENBUF_MAX_PACKET — does not, because the drop count is there to
 *          tell the caller the consumer is too slow and a caller mistake is a
 *          different problem that should not be hidden in the same number.
 * @note    A zero length packet is refused rather than stored. It would occupy
 *          a header and be indistinguishable from an empty queue at the far
 *          end, and no protocol in this library sends one.
 * @note    This is the function an interrupt calls. It copies the payload once
 *          and touches nothing else, so its cost is the length of the packet
 *          and nothing more.
 */
uint8_t comgenbufPush ( comgenbuf_t* driver, const uint8_t* const data, uint32_t length )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t need = 0;

    if ( ( driver != NULL ) && ( data != NULL ) && ( length != 0 ) &&
            ( length <= COMGENBUF_MAX_PACKET ) )
    {
        need = length + COMGENBUF_HEADER;

        if ( ( driver->size - driver->used ) >= need )
        {
            comgenbufPutByte ( driver, ( uint8_t ) ( ( length >> 8 ) & 0xFFu ) );
            comgenbufPutByte ( driver, ( uint8_t ) ( length & 0xFFu ) );

            for ( i = 0; i < length; ++i )
            {
                comgenbufPutByte ( driver, data[ i ] );
            }

            driver->used += need;
            ++driver->count;

            retVal = TRUE;
        }
        else
        {
            ++driver->dropCount;

            retVal = FALSE;
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reports the length of the packet at the front of the queue.
 * @param[in] driver  Queue state.
 * @return  Bytes in the next packet, or zero when the queue is empty.
 * @note    Zero is unambiguous because a zero length packet cannot be stored.
 * @note    This is what a caller sizes its destination buffer against before
 *          calling comgenbufPop, which refuses rather than truncates.
 */
uint32_t comgenbufPeekLength ( const comgenbuf_t* const driver )
{
    uint32_t retVal = 0;

    if ( driver->count != 0 )
    {
        retVal = ( ( uint32_t ) comgenbufPeekByte ( driver, 0 ) ) << 8;
        retVal |= ( uint32_t ) comgenbufPeekByte ( driver, 1 );
    }
    else
    {
        retVal = 0;
    }

    return ( retVal );
}

/**
 * @brief   Removes the packet at the front of the queue and copies it out.
 * @param[in,out] driver    Initialized queue.
 * @param[out]    data      Destination for the payload.
 * @param[in]     capacity  Bytes the destination can hold.
 * @return  Bytes copied, or zero when the queue was empty or the destination
 *          was too small.
 * @note    A destination too small leaves the packet **in the queue**, so the
 *          caller can size a buffer from comgenbufPeekLength and come back. A
 *          pop that silently truncated would hand a parser half a frame, and
 *          one that discarded the packet would lose a message because the
 *          caller guessed a buffer size wrong.
 */
uint32_t comgenbufPop ( comgenbuf_t* driver, uint8_t* data, uint32_t capacity )
{
    uint32_t retVal = 0;
    uint32_t length = 0;
    uint32_t i = 0;

    if ( ( driver->count != 0 ) && ( data != NULL ) )
    {
        length = comgenbufPeekLength ( driver );

        if ( capacity >= length )
        {
            for ( i = 0; i < length; ++i )
            {
                data[ i ] = comgenbufPeekByte ( driver, COMGENBUF_HEADER + i );
            }

            comgenbufDropBytes ( driver, length + COMGENBUF_HEADER );
            --driver->count;

            retVal = length;
        }
        else
        {
            retVal = 0;
        }
    }
    else
    {
        retVal = 0;
    }

    return ( retVal );
}

/**
 * @brief   Gets the number of packets waiting.
 * @param[in] driver  Queue state.
 * @return  Packets stored and not yet popped.
 */
uint32_t comgenbufGetCount ( const comgenbuf_t* const driver )
{
    return ( driver->count );
}

/**
 * @brief   Gets the free space in the queue.
 * @param[in] driver  Queue state.
 * @return  Bytes free, headers included.
 * @note    A packet of this many bytes will not fit: it needs two more for its
 *          header. A caller deciding whether to push compares its length plus
 *          two against this, or simply pushes and reads the status.
 */
uint32_t comgenbufGetFree ( const comgenbuf_t* const driver )
{
    return ( driver->size - driver->used );
}

/**
 * @brief   Gets the number of packets refused for want of room.
 * @param[in] driver  Queue state.
 * @return  Packets dropped since Init or the last flush.
 * @note    Non zero means the consumer is not keeping up with the producer.
 *          The remedy is a larger buffer or a faster main loop; the queue does
 *          not make room by discarding what it already holds, for the reason
 *          the file banner gives.
 */
uint32_t comgenbufGetDropCount ( const comgenbuf_t* const driver )
{
    return ( driver->dropCount );
}

/**
 * @brief   Empties the queue.
 * @param[in,out] driver  Initialized queue.
 * @note    For a link that has gone down and come back, where everything still
 *          queued is stale. The drop count is cleared with it, because the
 *          count is about a consumer falling behind and a flush ends that
 *          episode.
 */
void comgenbufFlush ( comgenbuf_t* driver )
{
    driver->head = 0;
    driver->tail = 0;
    driver->used = 0;
    driver->count = 0;
    driver->dropCount = 0;
}
