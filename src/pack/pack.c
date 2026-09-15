/**
  ******************************************************************************
  *
  * @file      pack.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Multi byte values to and from a byte buffer, either endianness.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      This is the line every protocol in this tree leaves to its
  *            caller. comstxetx hands over a payload of bytes and comat a
  *            string of them; turning four of those into a reading is the
  *            caller's problem, and it is the problem that gets written by hand
  *            in every project and got wrong in a good half of them.
  *
  * @note      Nothing here casts a buffer pointer to a wider type. The obvious
  *            two liner, taking the address of a byte and reading it as a
  *            uint32_t, is wrong three ways at once: it assumes the machine's
  *            endianness matches the wire's, it assumes a byte four along is
  *            aligned for a word access, and it is a strict aliasing violation
  *            that a modern compiler is entitled to reorder around. A Cortex-M0
  *            faults on the second. These functions shift bytes, which has none
  *            of those properties and costs a handful of instructions.
  *
  * @note      There are no signed writers, and that is not an oversight.
  *            Converting a signed value to an unsigned one of the same width is
  *            fully defined in C — it wraps modulo two to the width — so
  *            packPutU16be with a cast at the call site is already correct and
  *            portable. The reverse is not: converting an out of range unsigned
  *            value to a signed type is implementation defined before C23, which
  *            is exactly why the signed readers exist and do the sign extension
  *            by arithmetic instead.
  *
  * @note      The 24 bit readers have no writers for the same kind of reason.
  *            Twenty four bit values come off converters — an HX711 load cell
  *            amplifier, an ADS1256, a 24 bit audio codec — and nothing sends
  *            them. Sign extending twenty four bits into thirty two is the
  *            single most reliably botched operation in this area, because
  *            there is no C type to do it for you.
  *
  * @note      No function here checks the buffer pointer or the index against a
  *            length. That matches crc16, checksum and every other stateless
  *            function in the library: they run per packet, the caller owns the
  *            buffer and knows its size, and a length argument that every call
  *            site passes correctly buys nothing. The index is a byte offset,
  *            and the value occupies index through index plus its width minus
  *            one.
  *
  ******************************************************************************
  */

#include "pack.h"

/**
 * @brief   Writes a 16 bit value into a buffer, most significant byte first.
 * @param[out] buffer  Destination buffer.
 * @param[in]  index   Byte offset of the first of the two bytes.
 * @param[in]  value   Value to write.
 * @note    Big endian is network byte order, and is what MODBUS, most sensor
 *          registers and almost every published binary protocol use.
 * @note    Pass a signed value through a cast: the conversion to uint16_t is
 *          defined to wrap, so packPutU16be ( b, 0, ( uint16_t ) reading ) is
 *          correct for a negative reading.
 */
void packPutU16be ( uint8_t* const buffer, uint32_t index, uint16_t value )
{
    buffer[ index ]         = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );
    buffer[ index + 1u ]    = ( uint8_t ) ( value & 0xFFu );
}

/**
 * @brief   Writes a 16 bit value into a buffer, least significant byte first.
 * @param[out] buffer  Destination buffer.
 * @param[in]  index   Byte offset of the first of the two bytes.
 * @param[in]  value   Value to write.
 * @note    Little endian is what a memory dump of an ARM or an x86 looks like,
 *          and what a protocol designed around one of those tends to use.
 */
void packPutU16le ( uint8_t* const buffer, uint32_t index, uint16_t value )
{
    buffer[ index ]         = ( uint8_t ) ( value & 0xFFu );
    buffer[ index + 1u ]    = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );
}

/**
 * @brief   Writes a 32 bit value into a buffer, most significant byte first.
 * @param[out] buffer  Destination buffer.
 * @param[in]  index   Byte offset of the first of the four bytes.
 * @param[in]  value   Value to write.
 */
void packPutU32be ( uint8_t* const buffer, uint32_t index, uint32_t value )
{
    buffer[ index ]         = ( uint8_t ) ( ( value >> 24 ) & 0xFFu );
    buffer[ index + 1u ]    = ( uint8_t ) ( ( value >> 16 ) & 0xFFu );
    buffer[ index + 2u ]    = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );
    buffer[ index + 3u ]    = ( uint8_t ) ( value & 0xFFu );
}

/**
 * @brief   Writes a 32 bit value into a buffer, least significant byte first.
 * @param[out] buffer  Destination buffer.
 * @param[in]  index   Byte offset of the first of the four bytes.
 * @param[in]  value   Value to write.
 */
void packPutU32le ( uint8_t* const buffer, uint32_t index, uint32_t value )
{
    buffer[ index ]         = ( uint8_t ) ( value & 0xFFu );
    buffer[ index + 1u ]    = ( uint8_t ) ( ( value >> 8 ) & 0xFFu );
    buffer[ index + 2u ]    = ( uint8_t ) ( ( value >> 16 ) & 0xFFu );
    buffer[ index + 3u ]    = ( uint8_t ) ( ( value >> 24 ) & 0xFFu );
}

/**
 * @brief   Reads a 16 bit value from a buffer, most significant byte first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the two bytes.
 * @return  The value.
 * @note    Each byte is widened before it is shifted. Shifting a uint8_t left
 *          promotes it to int first, which is fine at eight bits and is not at
 *          twenty four, so the same form is written everywhere here rather than
 *          only where it matters.
 */
uint16_t packGetU16be ( const uint8_t* const buffer, uint32_t index )
{
    uint16_t retVal = 0;

    retVal = ( uint16_t ) ( ( ( uint16_t ) buffer[ index ] ) << 8 );
    retVal = ( uint16_t ) ( retVal | ( ( uint16_t ) buffer[ index + 1u ] ) );

    return ( retVal );
}

/**
 * @brief   Reads a 16 bit value from a buffer, least significant byte first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the two bytes.
 * @return  The value.
 */
uint16_t packGetU16le ( const uint8_t* const buffer, uint32_t index )
{
    uint16_t retVal = 0;

    retVal = ( uint16_t ) ( ( ( uint16_t ) buffer[ index + 1u ] ) << 8 );
    retVal = ( uint16_t ) ( retVal | ( ( uint16_t ) buffer[ index ] ) );

    return ( retVal );
}

/**
 * @brief   Reads a 32 bit value from a buffer, most significant byte first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the four bytes.
 * @return  The value.
 */
uint32_t packGetU32be ( const uint8_t* const buffer, uint32_t index )
{
    uint32_t retVal = 0;

    retVal = ( ( uint32_t ) buffer[ index ] ) << 24;
    retVal = retVal | ( ( ( uint32_t ) buffer[ index + 1u ] ) << 16 );
    retVal = retVal | ( ( ( uint32_t ) buffer[ index + 2u ] ) << 8 );
    retVal = retVal | ( ( uint32_t ) buffer[ index + 3u ] );

    return ( retVal );
}

/**
 * @brief   Reads a 32 bit value from a buffer, least significant byte first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the four bytes.
 * @return  The value.
 */
uint32_t packGetU32le ( const uint8_t* const buffer, uint32_t index )
{
    uint32_t retVal = 0;

    retVal = ( ( uint32_t ) buffer[ index + 3u ] ) << 24;
    retVal = retVal | ( ( ( uint32_t ) buffer[ index + 2u ] ) << 16 );
    retVal = retVal | ( ( ( uint32_t ) buffer[ index + 1u ] ) << 8 );
    retVal = retVal | ( ( uint32_t ) buffer[ index ] );

    return ( retVal );
}

/**
 * @brief   Reads a signed 16 bit value from a buffer, most significant byte
 *          first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the two bytes.
 * @return  The value, sign extended.
 * @note    The sign extension is done by subtraction rather than by casting the
 *          unsigned result to int16_t. That cast is implementation defined
 *          before C23 when the value does not fit, which is every negative one,
 *          and this library does not rely on what a particular compiler happens
 *          to do there. Subtracting 65536 from anything at or above 0x8000 is
 *          two's complement by arithmetic, defined everywhere.
 */
int16_t packGetI16be ( const uint8_t* const buffer, uint32_t index )
{
    int16_t retVal = 0;
    uint16_t raw = 0;

    raw = packGetU16be ( buffer, index );

    if ( raw < 0x8000u )
    {
        retVal = ( int16_t ) raw;
    }
    else
    {
        retVal = ( int16_t ) ( ( ( int32_t ) raw ) - 65536 );
    }

    return ( retVal );
}

/**
 * @brief   Reads a signed 16 bit value from a buffer, least significant byte
 *          first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the two bytes.
 * @return  The value, sign extended.
 */
int16_t packGetI16le ( const uint8_t* const buffer, uint32_t index )
{
    int16_t retVal = 0;
    uint16_t raw = 0;

    raw = packGetU16le ( buffer, index );

    if ( raw < 0x8000u )
    {
        retVal = ( int16_t ) raw;
    }
    else
    {
        retVal = ( int16_t ) ( ( ( int32_t ) raw ) - 65536 );
    }

    return ( retVal );
}

/**
 * @brief   Reads a signed 32 bit value from a buffer, most significant byte
 *          first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the four bytes.
 * @return  The value, sign extended.
 * @note    The subtraction is carried in int64_t, because the constant it
 *          subtracts does not fit in int32_t. This is the same reason
 *          interpCalculatei32 and mathMapi32 widen their intermediates.
 */
int32_t packGetI32be ( const uint8_t* const buffer, uint32_t index )
{
    int32_t retVal = 0;
    uint32_t raw = 0;

    raw = packGetU32be ( buffer, index );

    if ( raw < 0x80000000u )
    {
        retVal = ( int32_t ) raw;
    }
    else
    {
        retVal = ( int32_t ) ( ( ( int64_t ) raw ) - ( ( ( int64_t ) 1 ) << 32 ) );
    }

    return ( retVal );
}

/**
 * @brief   Reads a signed 32 bit value from a buffer, least significant byte
 *          first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the four bytes.
 * @return  The value, sign extended.
 */
int32_t packGetI32le ( const uint8_t* const buffer, uint32_t index )
{
    int32_t retVal = 0;
    uint32_t raw = 0;

    raw = packGetU32le ( buffer, index );

    if ( raw < 0x80000000u )
    {
        retVal = ( int32_t ) raw;
    }
    else
    {
        retVal = ( int32_t ) ( ( ( int64_t ) raw ) - ( ( ( int64_t ) 1 ) << 32 ) );
    }

    return ( retVal );
}

/**
 * @brief   Reads a 24 bit value from a buffer, most significant byte first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the three bytes.
 * @return  The value, in the low twenty four bits of a uint32_t.
 * @note    Twenty four bits is a converter width rather than a protocol width,
 *          which is why this reads and nothing here writes. An HX711, an
 *          ADS1256 and a 24 bit codec all produce three bytes per sample.
 */
uint32_t packGetU24be ( const uint8_t* const buffer, uint32_t index )
{
    uint32_t retVal = 0;

    retVal = ( ( uint32_t ) buffer[ index ] ) << 16;
    retVal = retVal | ( ( ( uint32_t ) buffer[ index + 1u ] ) << 8 );
    retVal = retVal | ( ( uint32_t ) buffer[ index + 2u ] );

    return ( retVal );
}

/**
 * @brief   Reads a 24 bit value from a buffer, least significant byte first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the three bytes.
 * @return  The value, in the low twenty four bits of a uint32_t.
 */
uint32_t packGetU24le ( const uint8_t* const buffer, uint32_t index )
{
    uint32_t retVal = 0;

    retVal = ( ( uint32_t ) buffer[ index + 2u ] ) << 16;
    retVal = retVal | ( ( ( uint32_t ) buffer[ index + 1u ] ) << 8 );
    retVal = retVal | ( ( uint32_t ) buffer[ index ] );

    return ( retVal );
}

/**
 * @brief   Reads a signed 24 bit value from a buffer, most significant byte
 *          first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the three bytes.
 * @return  The value, sign extended into a full int32_t.
 * @note    This is the function the module is really for. There is no
 *          twenty four bit type, so nothing in C sign extends this for you, and
 *          the hand written version that forgets to is the classic load cell
 *          bug: readings work perfectly until the tare goes slightly negative
 *          and the value jumps to sixteen million.
 * @note    The threshold is 0x800000 and the subtraction is 0x1000000, the same
 *          two's complement arithmetic the sixteen and thirty two bit readers
 *          use. Both fit in int32_t here, so no int64_t is needed.
 */
int32_t packGetI24be ( const uint8_t* const buffer, uint32_t index )
{
    int32_t retVal = 0;
    uint32_t raw = 0;

    raw = packGetU24be ( buffer, index );

    if ( raw < 0x800000u )
    {
        retVal = ( int32_t ) raw;
    }
    else
    {
        retVal = ( int32_t ) raw - 16777216;
    }

    return ( retVal );
}

/**
 * @brief   Reads a signed 24 bit value from a buffer, least significant byte
 *          first.
 * @param[in] buffer  Source buffer.
 * @param[in] index   Byte offset of the first of the three bytes.
 * @return  The value, sign extended into a full int32_t.
 */
int32_t packGetI24le ( const uint8_t* const buffer, uint32_t index )
{
    int32_t retVal = 0;
    uint32_t raw = 0;

    raw = packGetU24le ( buffer, index );

    if ( raw < 0x800000u )
    {
        retVal = ( int32_t ) raw;
    }
    else
    {
        retVal = ( int32_t ) raw - 16777216;
    }

    return ( retVal );
}
