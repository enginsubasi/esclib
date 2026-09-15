/**
  ******************************************************************************
  *
  * @file      text.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      16/09/2026
  *
  * @brief     Bytes and numbers to ASCII text and back: hexadecimal, base64
  *            and decimal, with no stdio.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 16/09/2026 Created. @n
  *
  * @note      pack turns bytes into numbers for a binary link. This turns bytes
  *            and numbers into text for an ASCII one, which is what comat
  *            speaks, what a terminal shows and what a log line is made of.
  *            Nothing in src/ may use stdio, so a project built on this library
  *            had no printf to lean on and wrote these by hand — and each of
  *            them has one specific place the hand written version goes wrong,
  *            which is the reason each one is here.
  *
  * @note      The text is **ASCII by definition**, not by assumption. Every
  *            protocol this serves defines its bytes in ASCII, so this file
  *            writes byte values — 0x30 for a zero digit, 0x41 for an A —
  *            rather than character literals, which would be in whatever
  *            execution character set the compiler happens to use. C promises
  *            only that the ten digits are contiguous there; it promises
  *            nothing at all about the letters.
  *
  * @note      Nothing here writes a terminating zero. Every buffer in this
  *            library is a pointer and a length, and every writer here reports
  *            how many bytes it wrote. A caller who wants a C string has that
  *            count and writes the zero itself, one byte past the text.
  *
  * @note      Every decoder and parser checks its whole input before it writes
  *            anything, so a refusal leaves the destination as it was. That is
  *            cobsDecode's rule, for cobsDecode's reason.
  *
  * @note      Hexadecimal is written in capitals and read in either case.
  *            Reading both costs nothing, and refusing lower case would refuse
  *            half the hexadecimal there is.
  *
  * @note      Base64 is the standard alphabet with padding, RFC 4648 section
  *            4, and it is read **strictly**: the length must be a multiple of
  *            four, padding may only close the last group, and the bits that
  *            padding leaves unused must be zero. The last rule is the one a
  *            lenient decoder skips, and it is not pedantry. Without it one
  *            payload has several encodings, so two strings that differ decode
  *            to the same bytes, and anything that compares, signs or
  *            deduplicates the text rather than the bytes is fooled by it.
  *
  * @note      The decimal parsers check for overflow **before** each multiply
  *            rather than after it. After it the value has already wrapped and
  *            the evidence is gone, which is the whole trouble with atoi: an
  *            out of range string is undefined behaviour there, and in practice
  *            a number that is wrong without saying so.
  *
  * @note      The signed writers never negate a negative value, because the
  *            most negative int32_t has no positive counterpart and negating it
  *            is undefined. The magnitude is taken as -( value + 1 ) + 1 with
  *            the last step in unsigned arithmetic, which is defined for every
  *            value there is.
  *
  * @note      textFromQ16 rounds the magnitude, so its rounding is symmetric
  *            about zero, and it carries: 0.999 at two decimals is 1.00, where
  *            a writer that rounds the fraction without carrying into the
  *            integer part prints 0.100. It prints no sign on a value that
  *            rounds to zero, so -0.001 at two decimals is 0.00 rather than
  *            -0.00, which reads as a measurement below zero that is not one.
  *
  ******************************************************************************
  */

#include "text.h"

/* The ASCII values this file reads and writes. The banner says why they are
   numbers rather than character literals. */
#define TEXT_ASCII_ZERO         0x30u
#define TEXT_ASCII_NINE         0x39u
#define TEXT_ASCII_UPPER_A      0x41u
#define TEXT_ASCII_UPPER_F      0x46u
#define TEXT_ASCII_UPPER_Z      0x5Au
#define TEXT_ASCII_LOWER_A      0x61u
#define TEXT_ASCII_LOWER_F      0x66u
#define TEXT_ASCII_LOWER_Z      0x7Au
#define TEXT_ASCII_PLUS         0x2Bu
#define TEXT_ASCII_MINUS        0x2Du
#define TEXT_ASCII_POINT        0x2Eu
#define TEXT_ASCII_SLASH        0x2Fu
#define TEXT_ASCII_EQUALS       0x3Du

/* The Q16 scale, the half of one LSB that rounds, and the fraction bits. */
#define TEXT_Q16_SHIFT          16
#define TEXT_Q16_HALF           32768u
#define TEXT_Q16_FRACTION_MASK  0xFFFFu

/* The largest integer part a Q16 value can carry, as a magnitude: the most
   negative value is exactly -32768. */
#define TEXT_Q16_INTEGER_LIMIT  32768u

/* How many fraction digits textToQ16 accumulates before it only checks. */
#define TEXT_Q16_READ_DIGITS    9u

/* The magnitude limits of int32_t, on each side of zero. */
#define TEXT_I32_POSITIVE_LIMIT 2147483647u
#define TEXT_I32_NEGATIVE_LIMIT 2147483648u

/**
 * @brief   Gives the character for one hexadecimal digit.
 *
 * @param[in]   nibble  the digit's value, 0 to 15.
 *
 * @return  The ASCII byte, in capitals.
 */
static uint8_t textHexDigit ( uint32_t nibble )
{
    uint8_t retVal = 0u;

    if ( nibble < 10u )
    {
        retVal = ( uint8_t ) ( TEXT_ASCII_ZERO + nibble );
    }
    else
    {
        retVal = ( uint8_t ) ( TEXT_ASCII_UPPER_A + ( nibble - 10u ) );
    }

    return ( retVal );
}

/**
 * @brief   Reads one hexadecimal digit.
 *
 * @param[in]   character   the ASCII byte, in either case.
 * @param[out]  nibble      its value, written only when it is a digit.
 *
 * @return  TRUE when the byte is a hexadecimal digit, FALSE otherwise.
 */
static uint8_t textHexValue ( uint8_t character, uint32_t* nibble )
{
    uint8_t retVal = TRUE;
    uint32_t c = ( uint32_t ) character;

    if ( ( c >= TEXT_ASCII_ZERO ) && ( c <= TEXT_ASCII_NINE ) )
    {
        *nibble = c - TEXT_ASCII_ZERO;
    }
    else if ( ( c >= TEXT_ASCII_UPPER_A ) && ( c <= TEXT_ASCII_UPPER_F ) )
    {
        *nibble = ( c - TEXT_ASCII_UPPER_A ) + 10u;
    }
    else if ( ( c >= TEXT_ASCII_LOWER_A ) && ( c <= TEXT_ASCII_LOWER_F ) )
    {
        *nibble = ( c - TEXT_ASCII_LOWER_A ) + 10u;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Gives the character for one base64 digit.
 *
 * @param[in]   index   the digit's value, 0 to 63.
 *
 * @return  The ASCII byte from the standard alphabet.
 *
 * @note    Computed from ranges rather than looked up, so the alphabet costs
 *          no table. It is the same trade crc8 makes: a table buys back a
 *          handful of comparisons on text that is short by construction.
 */
static uint8_t textBase64Character ( uint32_t index )
{
    uint8_t retVal = 0u;

    if ( index < 26u )
    {
        retVal = ( uint8_t ) ( TEXT_ASCII_UPPER_A + index );
    }
    else if ( index < 52u )
    {
        retVal = ( uint8_t ) ( TEXT_ASCII_LOWER_A + ( index - 26u ) );
    }
    else if ( index < 62u )
    {
        retVal = ( uint8_t ) ( TEXT_ASCII_ZERO + ( index - 52u ) );
    }
    else if ( index == 62u )
    {
        retVal = ( uint8_t ) TEXT_ASCII_PLUS;
    }
    else
    {
        retVal = ( uint8_t ) TEXT_ASCII_SLASH;
    }

    return ( retVal );
}

/**
 * @brief   Reads one base64 digit.
 *
 * @param[in]   character   the ASCII byte.
 * @param[out]  index       its value, written only when it is in the alphabet.
 *
 * @return  TRUE when the byte is in the standard alphabet, FALSE otherwise —
 *          including for the padding character, which is not a digit.
 */
static uint8_t textBase64Value ( uint8_t character, uint32_t* index )
{
    uint8_t retVal = TRUE;
    uint32_t c = ( uint32_t ) character;

    if ( ( c >= TEXT_ASCII_UPPER_A ) && ( c <= TEXT_ASCII_UPPER_Z ) )
    {
        *index = c - TEXT_ASCII_UPPER_A;
    }
    else if ( ( c >= TEXT_ASCII_LOWER_A ) && ( c <= TEXT_ASCII_LOWER_Z ) )
    {
        *index = ( c - TEXT_ASCII_LOWER_A ) + 26u;
    }
    else if ( ( c >= TEXT_ASCII_ZERO ) && ( c <= TEXT_ASCII_NINE ) )
    {
        *index = ( c - TEXT_ASCII_ZERO ) + 52u;
    }
    else if ( c == TEXT_ASCII_PLUS )
    {
        *index = 62u;
    }
    else if ( c == TEXT_ASCII_SLASH )
    {
        *index = 63u;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Counts the decimal digits of a value.
 *
 * @param[in]   value   the value.
 *
 * @return  How many digits it has, which is one for zero.
 */
static uint32_t textDigits ( uint32_t value )
{
    uint32_t retVal = 1u;
    uint32_t rest = value / 10u;

    while ( rest > 0u )
    {
        ++retVal;
        rest = rest / 10u;
    }

    return ( retVal );
}

/**
 * @brief   Writes a value as a fixed number of decimal digits.
 *
 * @param[in]   value       the value.
 * @param[in]   digits      how many digits to write. Leading positions are
 *                          filled with zeros, which is what a fraction needs.
 * @param[out]  destination the buffer.
 * @param[in]   at          where the first digit goes.
 */
static void textWriteDigits ( uint32_t value, uint32_t digits,
                              uint8_t* destination, uint32_t at )
{
    uint32_t i = 0u;
    uint32_t rest = value;

    for ( i = digits; i > 0u; --i )
    {
        destination[ ( at + i ) - 1u ] =
            ( uint8_t ) ( TEXT_ASCII_ZERO + ( rest % 10u ) );
        rest = rest / 10u;
    }
}

/**
 * @brief   Reads a run of decimal digits into a magnitude.
 *
 * @param[in]   source      the text.
 * @param[in]   start       the first byte of the run.
 * @param[in]   end         one past the last byte of the run.
 * @param[in]   limit       the largest magnitude accepted. At least 9.
 * @param[out]  magnitude   the value, written only on success.
 *
 * @return  TRUE when the run is at least one digit long, holds nothing but
 *          digits, and does not exceed the limit.
 *
 * @note    The limit is tested before each multiply, as accumulated being no
 *          larger than ( limit - digit ) / 10. That form cannot overflow, and
 *          it is exact for integers: accumulated * 10 + digit <= limit holds
 *          precisely when it does.
 */
static uint8_t textParseDigits ( const uint8_t* const source, uint32_t start,
                                 uint32_t end, uint32_t limit,
                                 uint32_t* magnitude )
{
    uint8_t retVal = TRUE;
    uint32_t i = 0u;
    uint32_t digit = 0u;
    uint32_t accumulated = 0u;

    if ( start >= end )
    {
        retVal = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    for ( i = start; ( i < end ) && ( retVal == TRUE ); ++i )
    {
        if ( ( ( uint32_t ) source[ i ] < TEXT_ASCII_ZERO ) ||
             ( ( uint32_t ) source[ i ] > TEXT_ASCII_NINE ) )
        {
            retVal = FALSE;
        }
        else
        {
            digit = ( uint32_t ) source[ i ] - TEXT_ASCII_ZERO;

            if ( accumulated > ( ( limit - digit ) / 10u ) )
            {
                retVal = FALSE;
            }
            else
            {
                accumulated = ( accumulated * 10u ) + digit;
            }
        }
    }

    if ( retVal == TRUE )
    {
        *magnitude = accumulated;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Takes the magnitude of a signed value without negating it.
 *
 * @param[in]   value   the value.
 *
 * @return  Its magnitude, which for the most negative int32_t is 2147483648.
 *
 * @note    -value would be undefined for exactly that one input. See the
 *          banner.
 */
static uint32_t textMagnitude ( int32_t value )
{
    uint32_t retVal = 0u;

    if ( value < 0 )
    {
        retVal = ( ( uint32_t ) ( -( value + 1 ) ) ) + 1u;
    }
    else
    {
        retVal = ( uint32_t ) value;
    }

    return ( retVal );
}

/**
 * @brief   Turns a magnitude and a sign back into an int32_t.
 *
 * @param[in]   magnitude   the magnitude, no more than 2147483648 when
 *                          negative and 2147483647 otherwise.
 * @param[in]   negative    TRUE for a value below zero.
 *
 * @return  The value.
 *
 * @note    The same care as textMagnitude, the other way round: casting
 *          2147483648 to int32_t is implementation defined, so the most
 *          negative value is built as -( magnitude - 1 ) - 1.
 */
static int32_t textSigned ( uint32_t magnitude, uint8_t negative )
{
    int32_t retVal = 0;

    if ( ( negative == TRUE ) && ( magnitude > 0u ) )
    {
        retVal = -( ( int32_t ) ( magnitude - 1u ) ) - 1;
    }
    else
    {
        retVal = ( int32_t ) magnitude;
    }

    return ( retVal );
}

/**
 * @brief   Computes a power of ten.
 *
 * @param[in]   exponent    the power, no more than nine.
 *
 * @return  Ten to that power.
 */
static uint32_t textPowerOfTen ( uint32_t exponent )
{
    uint32_t retVal = 1u;
    uint32_t i = 0u;

    for ( i = 0u; i < exponent; ++i )
    {
        retVal = retVal * 10u;
    }

    return ( retVal );
}

/**
 * @brief   Writes bytes as hexadecimal text, two capitals per byte.
 *
 * @param[in]   source      the bytes.
 * @param[in]   length      how many there are.
 * @param[out]  destination where the text goes.
 * @param[in]   capacity    how many bytes that holds.
 * @param[out]  written     how many bytes of text were written.
 *
 * @return  TRUE when the text was written, FALSE when it would not fit.
 *
 * @note    The capacity is compared as half of itself rather than as twice
 *          the length, so a length near the top of uint32_t cannot wrap the
 *          comparison into a pass.
 */
uint8_t textHexEncode ( const uint8_t* const source, uint32_t length,
                        uint8_t* destination, uint32_t capacity,
                        uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0u;

    if ( length <= ( capacity / 2u ) )
    {
        for ( i = 0u; i < length; ++i )
        {
            destination[ 2u * i ] =
                textHexDigit ( ( ( uint32_t ) source[ i ] >> 4 ) & 0x0Fu );
            destination[ ( 2u * i ) + 1u ] =
                textHexDigit ( ( uint32_t ) source[ i ] & 0x0Fu );
        }

        *written = 2u * length;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Reads hexadecimal text back into bytes.
 *
 * @param[in]   source      the text, in either case.
 * @param[in]   length      how many bytes of text there are.
 * @param[out]  destination where the bytes go.
 * @param[in]   capacity    how many bytes that holds.
 * @param[out]  written     how many bytes were produced.
 *
 * @return  TRUE when the text decoded, FALSE when it had an odd length, held
 *          anything but hexadecimal digits, or would not fit.
 *
 * @note    Nothing is written unless the whole text is sound.
 */
uint8_t textHexDecode ( const uint8_t* const source, uint32_t length,
                        uint8_t* destination, uint32_t capacity,
                        uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint8_t sound = TRUE;
    uint32_t i = 0u;
    uint32_t high = 0u;
    uint32_t low = 0u;

    if ( ( ( length % 2u ) != 0u ) || ( ( length / 2u ) > capacity ) )
    {
        sound = FALSE;
    }
    else
    {
        /* Intentionally blank */
    }

    for ( i = 0u; ( i < length ) && ( sound == TRUE ); ++i )
    {
        sound = textHexValue ( source[ i ], &high );
    }

    if ( sound == TRUE )
    {
        for ( i = 0u; i < ( length / 2u ); ++i )
        {
            ( void ) textHexValue ( source[ 2u * i ], &high );
            ( void ) textHexValue ( source[ ( 2u * i ) + 1u ], &low );

            destination[ i ] = ( uint8_t ) ( ( high << 4 ) | low );
        }

        *written = length / 2u;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Reports how long the base64 text for a payload is.
 *
 * @param[in]   length  how many bytes the payload holds.
 *
 * @return  How many bytes of text it encodes to, padding included.
 *
 * @note    Four characters for every three bytes or part of three. Unlike
 *          cobsEncodedSize this is exact rather than a bound, because base64
 *          does not depend on what the bytes hold.
 */
uint32_t textBase64EncodedSize ( uint32_t length )
{
    uint32_t retVal = 0u;
    uint32_t groups = length / 3u;

    if ( ( length % 3u ) != 0u )
    {
        ++groups;
    }
    else
    {
        /* Intentionally blank */
    }

    retVal = groups * 4u;

    return ( retVal );
}

/**
 * @brief   Writes bytes as base64 text, standard alphabet, padded.
 *
 * @param[in]   source      the bytes.
 * @param[in]   length      how many there are.
 * @param[out]  destination where the text goes.
 * @param[in]   capacity    how many bytes that holds.
 * @param[out]  written     how many bytes of text were written.
 *
 * @return  TRUE when the text was written, FALSE when it would not fit.
 *
 * @note    The capacity is compared in groups of four rather than in bytes,
 *          for the reason textHexEncode compares it in halves.
 */
uint8_t textBase64Encode ( const uint8_t* const source, uint32_t length,
                           uint8_t* destination, uint32_t capacity,
                           uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint32_t full = length / 3u;
    uint32_t rest = length % 3u;
    uint32_t groups = full;
    uint32_t g = 0u;
    uint32_t i = 0u;
    uint32_t w = 0u;
    uint32_t triple = 0u;

    if ( rest != 0u )
    {
        ++groups;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( groups <= ( capacity / 4u ) )
    {
        for ( g = 0u; g < full; ++g )
        {
            i = 3u * g;

            triple = ( ( ( uint32_t ) source[ i ] ) << 16 ) |
                     ( ( ( uint32_t ) source[ i + 1u ] ) << 8 ) |
                     ( ( uint32_t ) source[ i + 2u ] );

            destination[ w ] = textBase64Character ( ( triple >> 18 ) & 0x3Fu );
            destination[ w + 1u ] =
                textBase64Character ( ( triple >> 12 ) & 0x3Fu );
            destination[ w + 2u ] =
                textBase64Character ( ( triple >> 6 ) & 0x3Fu );
            destination[ w + 3u ] = textBase64Character ( triple & 0x3Fu );

            w = w + 4u;
        }

        i = 3u * full;

        if ( rest == 1u )
        {
            triple = ( ( uint32_t ) source[ i ] ) << 16;

            destination[ w ] = textBase64Character ( ( triple >> 18 ) & 0x3Fu );
            destination[ w + 1u ] =
                textBase64Character ( ( triple >> 12 ) & 0x3Fu );
            destination[ w + 2u ] = ( uint8_t ) TEXT_ASCII_EQUALS;
            destination[ w + 3u ] = ( uint8_t ) TEXT_ASCII_EQUALS;

            w = w + 4u;
        }
        else if ( rest == 2u )
        {
            triple = ( ( ( uint32_t ) source[ i ] ) << 16 ) |
                     ( ( ( uint32_t ) source[ i + 1u ] ) << 8 );

            destination[ w ] = textBase64Character ( ( triple >> 18 ) & 0x3Fu );
            destination[ w + 1u ] =
                textBase64Character ( ( triple >> 12 ) & 0x3Fu );
            destination[ w + 2u ] =
                textBase64Character ( ( triple >> 6 ) & 0x3Fu );
            destination[ w + 3u ] = ( uint8_t ) TEXT_ASCII_EQUALS;

            w = w + 4u;
        }
        else
        {
            /* Intentionally blank */
        }

        *written = w;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Reads base64 text back into bytes, strictly.
 *
 * @param[in]   source      the text.
 * @param[in]   length      how many bytes of text there are.
 * @param[out]  destination where the bytes go.
 * @param[in]   capacity    how many bytes that holds.
 * @param[out]  written     how many bytes were produced.
 *
 * @return  TRUE when the text decoded, FALSE when it was not canonical base64
 *          or would not fit.
 *
 * @note    Refused: a length that is not a multiple of four, any character
 *          outside the alphabet — whitespace and line breaks included —
 *          padding anywhere but the end, more than two padding characters,
 *          and unused bits that are not zero. The banner says why the last
 *          one matters.
 *
 * @note    Nothing is written unless the whole text is sound.
 */
uint8_t textBase64Decode ( const uint8_t* const source, uint32_t length,
                           uint8_t* destination, uint32_t capacity,
                           uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint8_t sound = TRUE;
    uint32_t groups = length / 4u;
    uint32_t padding = 0u;
    uint32_t decoded = 0u;
    uint32_t g = 0u;
    uint32_t j = 0u;
    uint32_t i = 0u;
    uint32_t value = 0u;
    uint32_t quad = 0u;
    uint32_t w = 0u;

    if ( ( length % 4u ) != 0u )
    {
        sound = FALSE;
    }
    else if ( length > 0u )
    {
        /* Padding is counted from the end, which is the only place it may
           be. A padding character anywhere else fails the alphabet below. */
        if ( ( uint32_t ) source[ length - 1u ] == TEXT_ASCII_EQUALS )
        {
            ++padding;

            if ( ( uint32_t ) source[ length - 2u ] == TEXT_ASCII_EQUALS )
            {
                ++padding;
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
    }
    else
    {
        /* Intentionally blank */
    }

    for ( i = 0u; ( i < ( length - padding ) ) && ( sound == TRUE ); ++i )
    {
        sound = textBase64Value ( source[ i ], &value );
    }

    /* The canonical form: the bits padding leaves unused are zero. */
    if ( ( sound == TRUE ) && ( padding == 2u ) )
    {
        ( void ) textBase64Value ( source[ length - 3u ], &value );

        if ( ( value & 0x0Fu ) != 0u )
        {
            sound = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }
    }
    else if ( ( sound == TRUE ) && ( padding == 1u ) )
    {
        ( void ) textBase64Value ( source[ length - 2u ], &value );

        if ( ( value & 0x03u ) != 0u )
        {
            sound = FALSE;
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

    decoded = ( groups * 3u ) - padding;

    if ( ( sound == TRUE ) && ( decoded <= capacity ) )
    {
        for ( g = 0u; g < groups; ++g )
        {
            quad = 0u;

            for ( j = 0u; j < 4u; ++j )
            {
                /* Padding is not a digit and reads as zero bits, which the
                   canonical check above has already required them to be. */
                value = 0u;
                ( void ) textBase64Value ( source[ ( 4u * g ) + j ], &value );
                quad = ( quad << 6 ) | value;
            }

            if ( w < decoded )
            {
                destination[ w ] = ( uint8_t ) ( ( quad >> 16 ) & 0xFFu );
                ++w;
            }
            else
            {
                /* Intentionally blank */
            }

            if ( w < decoded )
            {
                destination[ w ] = ( uint8_t ) ( ( quad >> 8 ) & 0xFFu );
                ++w;
            }
            else
            {
                /* Intentionally blank */
            }

            if ( w < decoded )
            {
                destination[ w ] = ( uint8_t ) ( quad & 0xFFu );
                ++w;
            }
            else
            {
                /* Intentionally blank */
            }
        }

        *written = decoded;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Writes an unsigned value as decimal text.
 *
 * @param[in]   value       the value.
 * @param[out]  destination where the text goes.
 * @param[in]   capacity    how many bytes that holds. TEXT_U32_MAX always
 *                          suffices.
 * @param[out]  written     how many bytes were written.
 *
 * @return  TRUE when the text was written, FALSE when it would not fit.
 */
uint8_t textFromU32 ( uint32_t value, uint8_t* destination, uint32_t capacity,
                      uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint32_t digits = textDigits ( value );

    if ( digits <= capacity )
    {
        textWriteDigits ( value, digits, destination, 0u );

        *written = digits;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Writes a signed value as decimal text.
 *
 * @param[in]   value       the value.
 * @param[out]  destination where the text goes.
 * @param[in]   capacity    how many bytes that holds. TEXT_I32_MAX always
 *                          suffices.
 * @param[out]  written     how many bytes were written.
 *
 * @return  TRUE when the text was written, FALSE when it would not fit.
 *
 * @note    The most negative value is written correctly, which is the one a
 *          hand written version gets wrong. See the banner.
 */
uint8_t textFromI32 ( int32_t value, uint8_t* destination, uint32_t capacity,
                      uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint32_t magnitude = textMagnitude ( value );
    uint32_t sign = 0u;
    uint32_t digits = 0u;

    if ( value < 0 )
    {
        sign = 1u;
    }
    else
    {
        /* Intentionally blank */
    }

    digits = textDigits ( magnitude );

    if ( ( digits + sign ) <= capacity )
    {
        if ( sign == 1u )
        {
            destination[ 0 ] = ( uint8_t ) TEXT_ASCII_MINUS;
        }
        else
        {
            /* Intentionally blank */
        }

        textWriteDigits ( magnitude, digits, destination, sign );

        *written = digits + sign;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Writes a Q16 value as decimal text with a fixed number of decimals.
 *
 * @param[in]   value       the value, in Q16.
 * @param[in]   decimals    how many digits after the point, up to
 *                          TEXT_Q16_DECIMALS_MAX. Zero writes no point.
 * @param[out]  destination where the text goes.
 * @param[in]   capacity    how many bytes that holds. TEXT_Q16_MAX always
 *                          suffices.
 * @param[out]  written     how many bytes were written.
 *
 * @return  TRUE when the text was written, FALSE when there were too many
 *          decimals or it would not fit.
 *
 * @note    Rounds half away from zero, carries into the integer part, and
 *          prints no sign on a value that rounds to zero. See the banner.
 *
 * @note    Five decimals is the fewest that survive a round trip through
 *          textToQ16 for every value: an LSB is 0.0000153, so five decimals
 *          lands within a third of one, and four can land more than three
 *          away.
 */
uint8_t textFromQ16 ( int32_t value, uint32_t decimals, uint8_t* destination,
                      uint32_t capacity, uint32_t* written )
{
    uint8_t retVal = FALSE;
    uint32_t magnitude = textMagnitude ( value );
    uint32_t integer = 0u;
    uint32_t scale = 0u;
    uint32_t fraction = 0u;
    uint32_t sign = 0u;
    uint32_t integerDigits = 0u;
    uint32_t total = 0u;
    uint64_t scaled = 0u;

    if ( decimals <= TEXT_Q16_DECIMALS_MAX )
    {
        integer = magnitude >> TEXT_Q16_SHIFT;
        scale = textPowerOfTen ( decimals );

        scaled = ( ( ( uint64_t ) ( magnitude & TEXT_Q16_FRACTION_MASK ) ) *
                   ( uint64_t ) scale ) + TEXT_Q16_HALF;
        fraction = ( uint32_t ) ( scaled >> TEXT_Q16_SHIFT );

        /* The carry: a fraction that rounded up to a whole one. */
        if ( fraction >= scale )
        {
            fraction = fraction - scale;
            ++integer;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( ( value < 0 ) && ( ( integer != 0u ) || ( fraction != 0u ) ) )
        {
            sign = 1u;
        }
        else
        {
            /* Intentionally blank */
        }

        integerDigits = textDigits ( integer );
        total = sign + integerDigits;

        if ( decimals > 0u )
        {
            total = total + 1u + decimals;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( total <= capacity )
        {
            if ( sign == 1u )
            {
                destination[ 0 ] = ( uint8_t ) TEXT_ASCII_MINUS;
            }
            else
            {
                /* Intentionally blank */
            }

            textWriteDigits ( integer, integerDigits, destination, sign );

            if ( decimals > 0u )
            {
                destination[ sign + integerDigits ] =
                    ( uint8_t ) TEXT_ASCII_POINT;
                textWriteDigits ( fraction, decimals, destination,
                                  sign + integerDigits + 1u );
            }
            else
            {
                /* Intentionally blank */
            }

            *written = total;

            retVal = TRUE;
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

    return ( retVal );
}

/**
 * @brief   Reads decimal text as an unsigned value.
 *
 * @param[in]   source  the text: one or more digits and nothing else.
 * @param[in]   length  how many bytes of text there are.
 * @param[out]  value   the value, written only on success.
 *
 * @return  TRUE when the text is a number that fits, FALSE otherwise.
 *
 * @note    No sign is accepted, not even a plus. Leading zeros are, because a
 *          zero padded field is ordinary in the protocols this reads.
 */
uint8_t textToU32 ( const uint8_t* const source, uint32_t length,
                    uint32_t* value )
{
    return ( textParseDigits ( source, 0u, length, 0xFFFFFFFFu, value ) );
}

/**
 * @brief   Reads decimal text as a signed value.
 *
 * @param[in]   source  the text: an optional minus and one or more digits.
 * @param[in]   length  how many bytes of text there are.
 * @param[out]  value   the value, written only on success.
 *
 * @return  TRUE when the text is a number that fits, FALSE otherwise.
 *
 * @note    The two sides of zero have different limits, and the negative one
 *          is the larger: -2147483648 is accepted and 2147483648 is not.
 */
uint8_t textToI32 ( const uint8_t* const source, uint32_t length,
                    int32_t* value )
{
    uint8_t retVal = FALSE;
    uint8_t negative = FALSE;
    uint32_t start = 0u;
    uint32_t limit = TEXT_I32_POSITIVE_LIMIT;
    uint32_t magnitude = 0u;

    if ( ( length > 0u ) && ( ( uint32_t ) source[ 0 ] == TEXT_ASCII_MINUS ) )
    {
        negative = TRUE;
        start = 1u;
        limit = TEXT_I32_NEGATIVE_LIMIT;
    }
    else
    {
        /* Intentionally blank */
    }

    if ( textParseDigits ( source, start, length, limit, &magnitude ) == TRUE )
    {
        *value = textSigned ( magnitude, negative );

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Reads decimal text as a Q16 value.
 *
 * @param[in]   source  the text: an optional minus, one or more digits, and
 *                      optionally a point followed by one or more digits.
 * @param[in]   length  how many bytes of text there are.
 * @param[out]  value   the value in Q16, written only on success.
 *
 * @return  TRUE when the text is a number the format can hold, FALSE
 *          otherwise.
 *
 * @note    The forms accepted are the ones a person writes down — 12, -3,
 *          0.5, -12.375. A point with no digit on either side of it is
 *          refused, and so is anything after the digits.
 *
 * @note    The fraction is counted by its digits, not by its value, which is
 *          the mistake worth pinning: 0.05 is five hundredths, and a parser
 *          that reads the digits after the point as the integer 5 and then
 *          scales by that integer's own length reads it as five tenths.
 *
 * @note    It rounds to the nearest Q16 value. Digits past the ninth are
 *          checked and then ignored: at that depth they move the result by
 *          less than a ten thousandth of an LSB.
 */
uint8_t textToQ16 ( const uint8_t* const source, uint32_t length,
                    int32_t* value )
{
    uint8_t retVal = FALSE;
    uint8_t sound = TRUE;
    uint8_t negative = FALSE;
    uint32_t start = 0u;
    uint32_t point = length;
    uint32_t i = 0u;
    uint32_t integer = 0u;
    uint32_t fraction = 0u;
    uint32_t scale = 1u;
    uint32_t limit = TEXT_I32_POSITIVE_LIMIT;
    uint64_t magnitude = 0u;

    if ( ( length > 0u ) && ( ( uint32_t ) source[ 0 ] == TEXT_ASCII_MINUS ) )
    {
        negative = TRUE;
        start = 1u;
        limit = TEXT_I32_NEGATIVE_LIMIT;
    }
    else
    {
        /* Intentionally blank */
    }

    /* The first point, if there is one. A second is caught below, as a
       character in the fraction that is not a digit. */
    for ( i = start; ( i < length ) && ( point == length ); ++i )
    {
        if ( ( uint32_t ) source[ i ] == TEXT_ASCII_POINT )
        {
            point = i;
        }
        else
        {
            /* Intentionally blank */
        }
    }

    sound = textParseDigits ( source, start, point, TEXT_Q16_INTEGER_LIMIT,
                              &integer );

    if ( ( sound == TRUE ) && ( point < length ) )
    {
        if ( ( point + 1u ) >= length )
        {
            sound = FALSE;
        }
        else
        {
            /* Intentionally blank */
        }

        for ( i = point + 1u; ( i < length ) && ( sound == TRUE ); ++i )
        {
            if ( ( ( uint32_t ) source[ i ] < TEXT_ASCII_ZERO ) ||
                 ( ( uint32_t ) source[ i ] > TEXT_ASCII_NINE ) )
            {
                sound = FALSE;
            }
            else if ( ( i - point ) <= TEXT_Q16_READ_DIGITS )
            {
                fraction = ( fraction * 10u ) +
                           ( ( uint32_t ) source[ i ] - TEXT_ASCII_ZERO );
                scale = scale * 10u;
            }
            else
            {
                /* Checked, and past the depth that can move the result. */
            }
        }
    }
    else
    {
        /* Intentionally blank */
    }

    if ( sound == TRUE )
    {
        magnitude = ( ( uint64_t ) integer ) << TEXT_Q16_SHIFT;
        magnitude = magnitude +
                    ( ( ( ( ( uint64_t ) fraction ) << TEXT_Q16_SHIFT ) +
                        ( ( uint64_t ) scale / 2u ) ) / ( uint64_t ) scale );

        if ( magnitude <= ( uint64_t ) limit )
        {
            *value = textSigned ( ( uint32_t ) magnitude, negative );

            retVal = TRUE;
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

    return ( retVal );
}
