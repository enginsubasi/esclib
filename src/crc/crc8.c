/**
  ******************************************************************************
  *
  * @file      crc8.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.0
  * @date      15/09/2026
  *
  * @brief     CRC8 Calculation functions.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  * 16/09/2026 crc8Update and crc8DallasUpdate added. A 1-Wire ROM @n
  *            read arrives a byte at a time and an SMBus transfer @n
  *            is built a byte at a time, so both polynomials get @n
  *            the streaming form. @n
  *
  * @note      Two functions rather than one, because the two CRC8s an embedded
  *            project actually meets are different polynomials and neither can
  *            stand in for the other. crc8 is the SMBus packet error code, which
  *            a temperature sensor, a fuel gauge or a power monitor on an I2C
  *            bus appends to every transaction. crc8Dallas is the 1-Wire one,
  *            which every DS18B20 and DS2431 checks its ROM and scratchpad with.
  *            Handing an SMBus device a 1-Wire CRC produces a byte that is
  *            wrong in a way nothing reports.
  *
  * @note      This is the same choice crc16 offers in the other axis. crc16 and
  *            crc16Alt are one polynomial computed two ways, a table for speed
  *            and a loop for size; crc8 and crc8Dallas are two polynomials
  *            computed the one way.
  *
  * @note      Neither carries a table. A 256 byte table buys eight shifts a byte
  *            back, and the payloads an 8 bit CRC protects are short by
  *            construction — a three byte SMBus transaction, an eight byte
  *            1-Wire ROM. Spending a quarter of a kilobyte of flash to speed
  *            that up is the wrong trade on the parts these functions exist for.
  *            crc16 is where the table lives, because a MODBUS frame is long
  *            enough to earn it.
  *
  * @note      Both seed with zero and neither inverts the result, which is what
  *            makes the self check work: running the CRC over a buffer that
  *            already carries its own CRC as the last byte gives zero. A 1-Wire
  *            ROM read is normally verified that way rather than by comparing
  *            against a recomputed value.
  *
  * @note      The price of that zero seed is the one weakness worth knowing:
  *            neither function can tell a run of zero bytes from a longer run of
  *            zero bytes, because a zero byte leaves a zero register alone. Both
  *            standards are specified this way and changing the seed would make
  *            these compute something no device on either bus agrees with, so it
  *            is recorded rather than fixed. A protocol whose payload can be all
  *            zeros of varying length needs its length in the CRC, which is
  *            where comstxetx puts its own.
  *
  ******************************************************************************
  */

#include "crc8.h"

/**
 * @brief   Calculates the SMBus packet error code of a byte array.
 * @param[in] array  Bytes to run the CRC over.
 * @param[in] size   Number of bytes.
 * @return  The CRC8 value, seeded with 0x00.
 * @note    Polynomial 0x07, most significant bit first, no reflection and no
 *          final inversion. This is the CRC-8 of the SMBus specification, also
 *          catalogued as CRC-8/SMBUS and CRC-8/ATM. Its check value, the CRC of
 *          the nine ASCII bytes "123456789", is 0xF4.
 * @note    A size of zero returns the seed, which is 0x00. There is no error
 *          case to report: the CRC of nothing is well defined, and a caller
 *          passing a zero size has already decided there is nothing to protect.
 * @note    The array pointer is not checked, as in crc16 and every other
 *          stateless function in this library. These run per packet, and the
 *          caller holds the buffer.
 */
uint8_t crc8 ( const uint8_t* const array, uint32_t size )
{
    uint32_t i = 0;     // Array index counter.
    uint8_t j = 0;      // Bit shift counter.

    uint8_t crc = 0x00; // CRC of array.

    /* Loop until size. */
    for ( i = 0 ; i < size ; ++i )
    {
        crc = crc ^ array[ i ];

        for ( j = 0 ; j < 8 ; ++j )
        {
            if ( crc & 0x80 )
            {
                crc = ( uint8_t ) ( ( crc << 1 ) ^ 0x07 );
            }
            else
            {
                crc = ( uint8_t ) ( crc << 1 );
            }
        }
    }

    return ( crc );
}

/**
 * @brief   Calculates the Dallas 1-Wire CRC8 of a byte array.
 * @param[in] array  Bytes to run the CRC over.
 * @param[in] size   Number of bytes.
 * @return  The CRC8 value, seeded with 0x00.
 * @note    Polynomial 0x31 reflected to 0x8C, least significant bit first, no
 *          final inversion. This is the CRC-8 of the Dallas and Maxim 1-Wire
 *          devices, catalogued as CRC-8/MAXIM-DOW. Its check value, the CRC of
 *          the nine ASCII bytes "123456789", is 0xA1.
 * @note    Running this over a whole 1-Wire ROM code, the eight bytes including
 *          the CRC the device itself supplies, gives zero. That is the way a
 *          ROM read is normally verified, and it is cheaper than recomputing
 *          over seven bytes and comparing.
 * @note    The shift direction is the only difference from crc8 that matters,
 *          and it is not cosmetic. The two polynomials are genuinely different
 *          and their results share nothing.
 */
uint8_t crc8Dallas ( const uint8_t* const array, uint32_t size )
{
    uint32_t i = 0;     // Array index counter.
    uint8_t j = 0;      // Bit shift counter.

    uint8_t crc = 0x00; // CRC of array.

    /* Loop until size. */
    for ( i = 0 ; i < size ; ++i )
    {
        crc = crc ^ array[ i ];

        for ( j = 0 ; j < 8 ; ++j )
        {
            if ( crc & 0x01 )
            {
                crc = ( uint8_t ) ( ( crc >> 1 ) ^ 0x8C );
            }
            else
            {
                crc = ( uint8_t ) ( crc >> 1 );
            }
        }
    }

    return ( crc );
}

/**
 * @brief   Takes one byte into an SMBus CRC8 that is being computed a piece at
 *          a time.
 *
 * @param[in]   crc     the value so far, CRC8_SEED before the first byte.
 * @param[in]   data    the byte.
 *
 * @return  The value including that byte.
 *
 * @note    CRC8_SEED run through this once per byte gives exactly what crc8
 *          gives over the same bytes. An SMBus packet error code is computed
 *          over the address and command bytes as they go out, which is this
 *          shape rather than the whole buffer one.
 */
uint8_t crc8Update ( uint8_t crc, uint8_t data )
{
    uint8_t retVal = 0;
    uint8_t j = 0;

    retVal = ( uint8_t ) ( crc ^ data );

    for ( j = 0; j < 8; ++j )
    {
        if ( ( retVal & 0x80u ) != 0u )
        {
            retVal = ( uint8_t ) ( ( retVal << 1 ) ^ 0x07u );
        }
        else
        {
            retVal = ( uint8_t ) ( retVal << 1 );
        }
    }

    return ( retVal );
}

/**
 * @brief   Takes one byte into a Dallas 1-Wire CRC8 that is being computed a
 *          piece at a time.
 *
 * @param[in]   crc     the value so far, CRC8_DALLAS_SEED before the first
 *                      byte.
 * @param[in]   data    the byte.
 *
 * @return  The value including that byte.
 *
 * @note    CRC8_DALLAS_SEED run through this once per byte gives exactly what
 *          crc8Dallas gives over the same bytes. A ROM read arrives eight
 *          bytes at a time over a one wire bus, and checking it as it arrives
 *          is why the zero seed is worth having: the whole read including its
 *          CRC checks to zero.
 */
uint8_t crc8DallasUpdate ( uint8_t crc, uint8_t data )
{
    uint8_t retVal = 0;
    uint8_t j = 0;

    retVal = ( uint8_t ) ( crc ^ data );

    for ( j = 0; j < 8; ++j )
    {
        if ( ( retVal & 0x01u ) != 0u )
        {
            retVal = ( uint8_t ) ( ( retVal >> 1 ) ^ 0x8Cu );
        }
        else
        {
            retVal = ( uint8_t ) ( retVal >> 1 );
        }
    }

    return ( retVal );
}
