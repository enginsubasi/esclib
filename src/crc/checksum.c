/**
  ******************************************************************************
  *
  * @file      checksum.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      05/08/2026
  *
  * @brief     Stateless checksums, for links where a full CRC is more than
  *            the job needs.
  *
  * @par Device
  * Generic
  *
  * @note      Every function here returns its own natural width rather than a
  *            common one. Only the uint16_t pair therefore fits the checksum
  *            callback comstxetxInit takes, whose type is crc16's. Widening
  *            checksumSum8 to sixteen bits would make it fit too, and would
  *            be a lie about how many bits of protection it carries.
  *
  * @note      None of these detects a reordering except Fletcher16 and
  *            Adler32. A plain sum or xor is blind to it, because neither
  *            accumulator depends on where a byte sits.
  *
  * @par History
  * 05/08/2026 Created @n
  *
  ******************************************************************************
  */

#include "checksum.h"

// Largest prime below 65536, which is what makes Adler32's modulus work.
#define CHECKSUM_ADLER_BASE     65521u

// Fletcher16 accumulates modulo 255, not 256, so a zero byte still shows up.
#define CHECKSUM_FLETCHER_BASE  255u

/**
 * @brief   Calculates the longitudinal redundancy check of a byte array.
 * @param[in] array  Bytes to run the check over.
 * @param[in] size   Number of bytes.
 * @return  Every byte exclusive ored together, zero for an empty range.
 * @note    The cheapest check there is, and the weakest. It cannot see a
 *          reordering, and two bit errors in the same column cancel.
 */
uint8_t checksumXor ( const uint8_t* const array, uint32_t size )
{
    uint8_t retVal = 0;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        retVal ^= array[ i ];
    }

    return ( retVal );
}

/**
 * @brief   Calculates the eight bit sum of a byte array.
 * @param[in] array  Bytes to run the check over.
 * @param[in] size   Number of bytes.
 * @return  The sum of every byte truncated to eight bits, zero for an empty
 *          range.
 * @note    The carry is discarded. checksumSum16 keeps it, which is the only
 *          difference between the two.
 */
uint8_t checksumSum8 ( const uint8_t* const array, uint32_t size )
{
    uint8_t retVal = 0;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        retVal += array[ i ];
    }

    return ( retVal );
}

/**
 * @brief   Calculates the sixteen bit sum of a byte array.
 * @param[in] array  Bytes to run the check over.
 * @param[in] size   Number of bytes.
 * @return  The sum of every byte truncated to sixteen bits, zero for an
 *          empty range.
 * @note    Fits the checksum callback comstxetxInit takes, so it installs
 *          there with no wrapper.
 * @note    Blind to a reordering. Use checksumFletcher16 when that matters;
 *          it costs one more accumulator and one modulo per byte.
 */
uint16_t checksumSum16 ( const uint8_t* const array, uint32_t size )
{
    uint16_t retVal = 0;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        retVal += array[ i ];
    }

    return ( retVal );
}

/**
 * @brief   Calculates the Fletcher16 checksum of a byte array.
 * @param[in] array  Bytes to run the check over.
 * @param[in] size   Number of bytes.
 * @return  The second accumulator in the high byte and the first in the low
 *          byte, zero for an empty range.
 * @note    Fits the checksum callback comstxetxInit takes, so it installs
 *          there with no wrapper.
 * @note    The second accumulator sums the running total rather than the
 *          bytes, so each byte is weighted by how far along it sits. That is
 *          what lets this see a reordering where a plain sum cannot, for
 *          nearly the cost of a plain sum.
 */
uint16_t checksumFletcher16 ( const uint8_t* const array, uint32_t size )
{
    uint16_t retVal = 0;
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        sum1 = ( uint16_t ) ( ( sum1 + array[ i ] ) % CHECKSUM_FLETCHER_BASE );
        sum2 = ( uint16_t ) ( ( sum2 + sum1 ) % CHECKSUM_FLETCHER_BASE );
    }

    retVal = ( uint16_t ) ( ( sum2 << 8 ) | sum1 );

    return ( retVal );
}

/**
 * @brief   Calculates the Adler32 checksum of a byte array.
 * @param[in] array  Bytes to run the check over.
 * @param[in] size   Number of bytes.
 * @return  The second accumulator in the high half and the first in the low
 *          half. One for an empty range, because the first accumulator
 *          starts at one rather than zero.
 * @note    Does not fit the comstxetx checksum callback; it is thirty two
 *          bits wide and that hook takes sixteen.
 * @note    Fletcher16 with a wider modulus. Stronger on long payloads and
 *          notably weak on short ones, where the first accumulator has
 *          barely moved off its seed.
 */
uint32_t checksumAdler32 ( const uint8_t* const array, uint32_t size )
{
    uint32_t retVal = 0;
    uint32_t sum1 = 1;
    uint32_t sum2 = 0;
    uint32_t i = 0;

    for ( i = 0; i < size; ++i )
    {
        sum1 = ( sum1 + array[ i ] ) % CHECKSUM_ADLER_BASE;
        sum2 = ( sum2 + sum1 ) % CHECKSUM_ADLER_BASE;
    }

    retVal = ( sum2 << 16 ) | sum1;

    return ( retVal );
}
