/**
  ******************************************************************************
  *
  * @file      cobs.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      16/09/2026
  *
  * @brief     Consistent overhead byte stuffing: a payload with one byte value
  *            taken out of it, so that value can delimit a frame.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 16/09/2026 Created. @n
  *
  * @note      This answers the same question comstxetx answers and gives a
  *            different answer, and the difference is a number rather than a
  *            preference. comstxetx escapes: a payload byte equal to STX, ETX
  *            or DLE travels behind a DLE, so a payload made entirely of those
  *            three **doubles** on the wire. This removes one byte value from
  *            the payload instead of escaping it, and the cost is one byte per
  *            254 — a little under half a percent, whatever the payload holds.
  *            The consequence is not elegance, it is buffer sizing: a caller
  *            over comstxetx has to allocate twice the payload or accept that
  *            a frame it could not predict is refused, and a caller here sizes
  *            from cobsEncodedSize and is done.
  *
  * @note      The name does not start with com like the rest of this
  *            directory, and that is deliberate. This is one named algorithm
  *            rather than a protocol of this library's design, the name is
  *            what a reader will search for, and crc16 and checksum sit beside
  *            their protocol users under their own names for the same reason.
  *
  * @note      **This is framing and carries no check.** An encoded frame that
  *            arrives intact decodes to the payload, and one that arrives
  *            corrupted usually decodes to something — the structure is thin
  *            enough that a flipped bit often produces a well formed frame of
  *            the wrong length. Whatever integrity the link needs goes inside
  *            the payload, where comsafe and comsec put theirs, or beside it as
  *            a check the caller appends before encoding.
  *
  * @note      The zero byte is the one removed, which is the usual choice and
  *            the reason the encoding is worth having at all: the delimiter is
  *            then a value the encoder guarantees will not appear, so a
  *            receiver resynchronizes on it after any amount of noise.
  *            cobsEncode does not write that delimiter — the caller does,
  *            because whether frames are separated by one delimiter or opened
  *            and closed by two is the caller's protocol rather than this
  *            module's.
  *
  * @note      cobsDecode validates the whole frame before it writes anything.
  *            That is one extra pass, and it is nearly free: the first pass
  *            reads only the code bytes and steps over the runs between them,
  *            so it touches a two hundred and fifty fifth of the buffer. What
  *            it buys is that a malformed frame leaves the destination alone
  *            rather than half written, which matrixInverse cannot promise and
  *            says so, and comgenbufPop promises for the same reason.
  *
  ******************************************************************************
  */

#include "cobs.h"

/**
 * @brief   Reports how long an encoded buffer can be.
 *
 * @param[in]   length  how many bytes the payload holds.
 *
 * @return  The largest number of bytes the encoding of a payload that long can
 *          come to, not counting a delimiter.
 *
 * @note    This is what a transmit buffer is sized from. The overhead is one
 *          byte to open the frame and one more for every full run of 254, and
 *          it does not depend on what the payload holds — which is the whole
 *          difference from an escaping scheme, where the same question has no
 *          answer short of twice the payload.
 */
uint32_t cobsEncodedSize ( uint32_t length )
{
    return ( length + 1u + ( length / COBS_RUN_MAX ) );
}

/**
 * @brief   Encodes a payload so that no zero byte appears in the result.
 *
 * @param[in]   source          the payload.
 * @param[in]   length          how many bytes it holds.
 * @param[out]  destination     where the encoded frame goes.
 * @param[in]   capacity        how many bytes that will hold.
 * @param[out]  encodedLength   how many bytes the frame came to.
 *
 * @return  TRUE when the frame was encoded, FALSE when it would not fit.
 *
 * @note    A payload of no bytes encodes to a single byte rather than to
 *          nothing, so an empty frame is still a frame and still arrives.
 *
 * @note    The capacity is checked against the worst case before anything is
 *          written, rather than as the writing goes. A frame refused halfway
 *          would leave the caller a buffer holding the first half of a message
 *          and a FALSE saying nothing about how much of it is there.
 */
uint8_t cobsEncode ( const uint8_t* const source, uint32_t length,
                     uint8_t* destination, uint32_t capacity,
                     uint32_t* encodedLength )
{
    uint8_t retVal = FALSE;
    uint32_t read = 0u;
    uint32_t write = 1u;
    uint32_t codeIndex = 0u;
    uint32_t code = 1u;

    if ( cobsEncodedSize ( length ) <= capacity )
    {
        for ( read = 0u; read < length; ++read )
        {
            if ( source[ read ] == COBS_DELIMITER )
            {
                destination[ codeIndex ] = ( uint8_t ) code;
                codeIndex = write;
                ++write;
                code = 1u;
            }
            else
            {
                destination[ write ] = source[ read ];
                ++write;
                ++code;

                if ( code == ( COBS_RUN_MAX + 1u ) )
                {
                    destination[ codeIndex ] = ( uint8_t ) code;
                    codeIndex = write;
                    ++write;
                    code = 1u;
                }
                else
                {
                    /* Intentionally blank */
                }
            }
        }

        destination[ codeIndex ] = ( uint8_t ) code;

        *encodedLength = write;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Decodes a frame back into the payload it was made from.
 *
 * @param[in]   source          the encoded frame, without any delimiter.
 * @param[in]   length          how many bytes it holds.
 * @param[out]  destination     where the payload goes.
 * @param[in]   capacity        how many bytes that will hold.
 * @param[out]  decodedLength   how many bytes the payload came to.
 *
 * @return  TRUE when the frame decoded, FALSE when it was malformed or the
 *          payload would not fit.
 *
 * @note    Three things make a frame malformed and all three are refused: a
 *          code byte of zero, which the encoder cannot produce; a code byte
 *          claiming a run that reaches past the end of the frame; and a frame
 *          of no bytes at all, which is not the encoding of anything.
 *
 * @note    Nothing is written unless the whole frame is sound. See the file
 *          banner for why that costs almost nothing here.
 */
uint8_t cobsDecode ( const uint8_t* const source, uint32_t length,
                     uint8_t* destination, uint32_t capacity,
                     uint32_t* decodedLength )
{
    uint8_t retVal = FALSE;
    uint8_t sound = TRUE;
    uint32_t read = 0u;
    uint32_t write = 0u;
    uint32_t i = 0u;
    uint32_t code = 0u;
    uint32_t decoded = 0u;

    if ( length == 0u )
    {
        sound = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    /*
     * The validating pass. It reads a code byte, steps over the run it
     * describes, and counts what the run would produce, so the frame is known
     * to be sound and the payload's length is known before a byte is written.
     */
    while ( ( read < length ) && ( sound == TRUE ) )
    {
        code = ( uint32_t ) source[ read ];
        ++read;

        if ( code == 0u )
        {
            sound = FALSE;
        }
        else if ( ( read + ( code - 1u ) ) > length )
        {
            sound = FALSE;
        }
        else
        {
            read = read + ( code - 1u );
            decoded = decoded + ( code - 1u );

            if ( ( code != ( COBS_RUN_MAX + 1u ) ) && ( read < length ) )
            {
                ++decoded;
            }
            else
            {
                /* Intentionally blank */
            }
        }
    }

    if ( ( sound == TRUE ) && ( decoded <= capacity ) )
    {
        read = 0u;

        while ( read < length )
        {
            code = ( uint32_t ) source[ read ];
            ++read;

            for ( i = 1u; i < code; ++i )
            {
                destination[ write ] = source[ read ];
                ++write;
                ++read;
            }

            if ( ( code != ( COBS_RUN_MAX + 1u ) ) && ( read < length ) )
            {
                destination[ write ] = COBS_DELIMITER;
                ++write;
            }
            else
            {
                /* Intentionally blank */
            }
        }

        *decodedLength = write;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}
