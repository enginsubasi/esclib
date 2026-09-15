/**
  ******************************************************************************
  *
  * @file      rms.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      16/09/2026
  *
  * @brief     Mean, RMS and AC RMS of a signal, accumulated block by block as
  *            the samples arrive.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 16/09/2026 Created. @n
  *
  * @note      statistic already computes a mean and a standard deviation, and
  *            this is not a copy of it: statVariance wants the whole block in
  *            memory as an array, and this wants one sample at a time. That is
  *            the difference between basicmath's mean and maf — the same
  *            quantity, in the shape a sampling interrupt can call — and it is
  *            the whole reason for the module. Storage is a handful of words
  *            whatever the block length.
  *
  * @note      Three numbers come out of every block, because the one people
  *            mean by RMS depends on what they are measuring. The mean is the
  *            dc level. The RMS is the root of the mean square, dc included,
  *            which is what heats a resistor. The AC RMS has the dc taken out,
  *            which is what a mains meter reports, and it is the one to reach
  *            for with a converter biased to mid scale: that bias is a dc
  *            offset that is not part of the signal. The AC RMS is the
  *            population standard deviation — divided by the block length, not
  *            one less — because it describes this block of this signal rather
  *            than estimating anything, and statVariance divides the same way.
  *
  * @note      It is block based, and the block completes and reloads inside
  *            the iteration, which is goertzel's arrangement for goertzel's
  *            reason: a result the main loop fails to collect costs that result
  *            and never the phase. rmsIsReady clears the flag it reports.
  *
  * @note      The float variant subtracts the **first sample of each block**
  *            from every sample before accumulating, and that is the design
  *            decision of this file. The textbook form — the mean square less
  *            the square of the mean — subtracts two large numbers to leave a
  *            small one, and in single precision the small one is gone.
  *            Measured against an exact model: a signal of amplitude 10 riding
  *            on 2048, over 65536 samples, comes out 181 percent wrong that
  *            way, and a signal of one count on 20000 comes out wrong by a
  *            factor of 770. With the first sample subtracted the same cases
  *            are within eight parts in a million. Any sample of the block
  *            works as the offset, because what matters is only that it is
  *            within the signal's own swing of the mean, and the first is the
  *            one available without storing anything. Welford's method would
  *            also fix it, at the price of a divide on every sample.
  *
  * @note      The integer variant takes **int16_t**, and it is the first width
  *            of that name in this library. The type is the precondition: it is
  *            the range the arithmetic below is sized for, so there is nothing
  *            to document for the caller to carry and nothing to check on each
  *            sample. A twelve bit converter's reading fits as it comes; an
  *            unsigned sixteen bit one is recentred by subtracting 32768 first,
  *            which the caller knows and this module cannot.
  *
  * @note      Inside that range everything is exact. A square is below 2^30,
  *            so it is formed in 32 bits — explicitly in int32_t, because on a
  *            part whose int is sixteen bits wide the product of two int16_t is
  *            formed in int and overflows. Over at most 65536 samples the sum
  *            is below 2^31, the sum of squares below 2^46, and the block
  *            length times the sum of squares below 2^62, so N^2 times the
  *            variance is computed exactly as N times the sum of squares less
  *            the square of the sum. There is no cancellation to lose anything
  *            to, which is why this variant needs no offset. A wider sample
  *            would need 128 bit sums for the same exactness, which is why
  *            there is no i32 variant: the float one is the tool past sixteen
  *            bits.
  *
  * @note      The per sample path of the integer variant is one 32 bit
  *            multiply and two 64 bit additions, and calls no runtime helper
  *            on a Cortex-M0 — in the disassembly the first call sits after
  *            the branch that ends a block. The block end pays for 64 bit
  *            multiplies, a variable 64 bit shift and the divides, once per
  *            block.
  *
  * @note      The integer results are Q16 — a noise measurement is routinely
  *            below one count, and an integer would report it as nothing. The
  *            RMS and AC RMS are uint32_t because they are never negative and a
  *            full scale reading needs 2^31, which int32_t cannot hold. Their
  *            roots are taken after normalizing, shifting the value left in
  *            pairs of bits until its top is near 2^60 and paying for each pair
  *            with one bit on the divisor, so the root carries thirty
  *            significant bits wherever the value started and the result is
  *            within one LSB of exact. cordic normalizes its vectors for the
  *            same reason.
  *
  * @note      The square root is q16SquareRoot duplicated, which is itself
  *            rampSquareRoot duplicated, for the reason interp.c duplicates
  *            searchUpperBound's bracketing loop.
  *
  ******************************************************************************
  */

#include <stddef.h>
#include <math.h>

#include "rms.h"

/* The Q16 scale of the integer results. */
#define RMS_Q16_ONE         65536

/* Where the normalization stops: the root of a value at least this large has
   thirty significant bits. */
#define RMS_NORMAL_LIMIT    ( ( ( uint64_t ) 1 ) << 60 )

/**
 * @brief   Integer square root.
 *
 * @param[in]   value   the value.
 *
 * @return  The largest integer whose square does not exceed the value.
 */
static uint64_t rmsSquareRoot ( uint64_t value )
{
    uint64_t retVal = 0;
    uint64_t rest = 0;
    uint64_t bit = 0;

    rest = value;
    bit = ( ( uint64_t ) 1 ) << 62;

    while ( bit > rest )
    {
        bit >>= 2;
    }

    while ( bit != 0 )
    {
        if ( rest >= ( retVal + bit ) )
        {
            rest = rest - ( retVal + bit );
            retVal = ( retVal >> 1 ) + bit;
        }
        else
        {
            retVal = retVal >> 1;
        }

        bit >>= 2;
    }

    return ( retVal );
}

/**
 * @brief   Computes the square root of a value divided by a count, in Q16.
 *
 * @param[in]   numerator   the value, at most 2^62.
 * @param[in]   count       the divisor, at most 2^16.
 *
 * @return  sqrt ( numerator ) / count, in Q16, rounded.
 *
 * @note    The value is normalized before the root is taken. See the banner.
 */
static uint32_t rmsRootOver ( uint64_t numerator, uint32_t count )
{
    uint32_t retVal = 0u;
    uint64_t value = numerator;
    uint64_t root = 0u;
    uint64_t divisor = ( uint64_t ) count;
    uint32_t shift = 0u;

    if ( numerator != 0u )
    {
        while ( value < RMS_NORMAL_LIMIT )
        {
            value = value << 2;
            ++shift;
        }

        root = rmsSquareRoot ( value );
        divisor = divisor << shift;

        retVal = ( uint32_t ) ( ( ( root << 16 ) + ( divisor / 2u ) ) / divisor );
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Initializes a float RMS accumulator.
 *
 * @param[out]  driver      the accumulator.
 * @param[in]   blockLength how many samples make one result.
 *
 * @return  TRUE when the accumulator was initialized, FALSE when an argument
 *          was refused and nothing was written.
 *
 * @note    Until the first block completes, every result reads zero.
 */
uint8_t rmsInit ( rms_t* driver, uint32_t blockLength )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( blockLength > 0u ) )
    {
        driver->blockLength = blockLength;
        driver->count = 0u;

        driver->offset = 0.0f;
        driver->sum = 0.0f;
        driver->sumSquares = 0.0f;

        driver->mean = 0.0f;
        driver->rms = 0.0f;
        driver->acRms = 0.0f;

        driver->ready = FALSE;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Takes one sample. Call it at the sample rate.
 *
 * @param[in,out]   driver  the accumulator.
 * @param[in]       sample  the sample.
 *
 * @note    The first sample of a block becomes the offset every sample of that
 *          block is measured from. See the banner for why.
 *
 * @note    Rounding can leave the variance a hair below zero on a block that
 *          is very nearly constant, and it is clamped rather than handed to
 *          sqrtf as a negative.
 */
void rmsIteration ( rms_t* driver, float sample )
{
    float deviation = 0.0f;
    float n = 0.0f;
    float variance = 0.0f;

    if ( driver->count == 0u )
    {
        driver->offset = sample;
    }
    else
    {
        /* Intentionally blank */
    }

    deviation = sample - driver->offset;

    driver->sum = driver->sum + deviation;
    driver->sumSquares = driver->sumSquares + ( deviation * deviation );

    ++driver->count;

    if ( driver->count >= driver->blockLength )
    {
        n = ( float ) driver->blockLength;

        driver->mean = driver->offset + ( driver->sum / n );

        variance = ( driver->sumSquares -
                     ( ( driver->sum * driver->sum ) / n ) ) / n;

        if ( variance < 0.0f )
        {
            variance = 0.0f;
        }
        else
        {
            /* Intentionally blank */
        }

        driver->acRms = sqrtf ( variance );
        driver->rms = sqrtf ( variance + ( driver->mean * driver->mean ) );

        driver->ready = TRUE;

        driver->count = 0u;
        driver->sum = 0.0f;
        driver->sumSquares = 0.0f;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Reports whether a block has completed since the last call.
 *
 * @param[in,out]   driver  the accumulator.
 *
 * @return  TRUE once for each completed block, FALSE otherwise.
 *
 * @note    Clears the flag it reports, which is why the driver is not const.
 *          goertzelIsReady does the same.
 */
uint8_t rmsIsReady ( rms_t* driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->ready;

    driver->ready = FALSE;

    return ( retVal );
}

/**
 * @brief   Reports the mean of the last completed block.
 *
 * @param[in]   driver  the accumulator.
 *
 * @return  The mean, which is the dc level.
 */
float rmsGetMean ( const rms_t* const driver )
{
    return ( driver->mean );
}

/**
 * @brief   Reports the RMS of the last completed block, dc included.
 *
 * @param[in]   driver  the accumulator.
 *
 * @return  The root of the mean square.
 */
float rmsGetRms ( const rms_t* const driver )
{
    return ( driver->rms );
}

/**
 * @brief   Reports the RMS of the last completed block with the dc removed.
 *
 * @param[in]   driver  the accumulator.
 *
 * @return  The AC RMS, which is the population standard deviation.
 */
float rmsGetAcRms ( const rms_t* const driver )
{
    return ( driver->acRms );
}

/**
 * @brief   Initializes an integer RMS accumulator.
 *
 * @param[out]  driver      the accumulator.
 * @param[in]   blockLength how many samples make one result, from 1 to
 *                          RMS_I16_BLOCK_MAX.
 *
 * @return  TRUE when the accumulator was initialized, FALSE when an argument
 *          was refused and nothing was written.
 *
 * @note    The upper bound is where the sums stop being exact in 64 bits, so
 *          it is refused here rather than left to overflow later.
 */
uint8_t rmsIniti16 ( rmsi16_t* driver, uint32_t blockLength )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( blockLength > 0u ) &&
         ( blockLength <= RMS_I16_BLOCK_MAX ) )
    {
        driver->blockLength = blockLength;
        driver->count = 0u;

        driver->sum = 0;
        driver->sumSquares = 0u;

        driver->mean = 0;
        driver->rms = 0u;
        driver->acRms = 0u;

        driver->ready = FALSE;

        retVal = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }

    return ( retVal );
}

/**
 * @brief   Turns a completed block's sums into its three results.
 *
 * @param[in,out]   driver  the accumulator.
 *
 * @note    Kept apart from rmsIterationi16 so the per sample path is plainly
 *          the few lines it is.
 */
static void rmsFinishi16 ( rmsi16_t* driver )
{
    uint64_t n = ( uint64_t ) driver->blockLength;
    uint64_t magnitude = 0u;
    uint64_t total = 0u;
    int64_t numerator = 0;
    int64_t half = ( int64_t ) ( driver->blockLength / 2u );

    if ( driver->sum < 0 )
    {
        magnitude = ( uint64_t ) ( -driver->sum );
    }
    else
    {
        magnitude = ( uint64_t ) driver->sum;
    }

    /* N times the sum of squares, which is N^2 times the mean square. */
    total = n * driver->sumSquares;

    driver->rms = rmsRootOver ( total, driver->blockLength );

    /* Less the square of the sum, which leaves N^2 times the variance. In
       exact integers it cannot go below zero. */
    driver->acRms = rmsRootOver ( total - ( magnitude * magnitude ),
                                  driver->blockLength );

    /* A multiply rather than a shift: the sum may be negative. */
    numerator = driver->sum * RMS_Q16_ONE;

    if ( numerator >= 0 )
    {
        numerator = numerator + half;
    }
    else
    {
        numerator = numerator - half;
    }

    driver->mean = ( int32_t ) ( numerator / ( int64_t ) driver->blockLength );
}

/**
 * @brief   Takes one sample. Call it at the sample rate.
 *
 * @param[in,out]   driver  the accumulator.
 * @param[in]       sample  the sample.
 */
void rmsIterationi16 ( rmsi16_t* driver, int16_t sample )
{
    int32_t square = 0;

    /* int32_t explicitly. See the banner. */
    square = ( ( int32_t ) sample ) * ( ( int32_t ) sample );

    driver->sum = driver->sum + ( int64_t ) sample;
    driver->sumSquares = driver->sumSquares + ( uint64_t ) ( uint32_t ) square;

    ++driver->count;

    if ( driver->count >= driver->blockLength )
    {
        rmsFinishi16 ( driver );

        driver->ready = TRUE;

        driver->count = 0u;
        driver->sum = 0;
        driver->sumSquares = 0u;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Reports whether a block has completed since the last call.
 *
 * @param[in,out]   driver  the accumulator.
 *
 * @return  TRUE once for each completed block, FALSE otherwise.
 *
 * @note    Clears the flag it reports, as rmsIsReady does.
 */
uint8_t rmsIsReadyi16 ( rmsi16_t* driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->ready;

    driver->ready = FALSE;

    return ( retVal );
}

/**
 * @brief   Reports the mean of the last completed block.
 *
 * @param[in]   driver  the accumulator.
 *
 * @return  The mean in Q16, rounded half away from zero.
 */
int32_t rmsGetMeani16 ( const rmsi16_t* const driver )
{
    return ( driver->mean );
}

/**
 * @brief   Reports the RMS of the last completed block, dc included.
 *
 * @param[in]   driver  the accumulator.
 *
 * @return  The root of the mean square, in Q16.
 */
uint32_t rmsGetRmsi16 ( const rmsi16_t* const driver )
{
    return ( driver->rms );
}

/**
 * @brief   Reports the RMS of the last completed block with the dc removed.
 *
 * @param[in]   driver  the accumulator.
 *
 * @return  The AC RMS in Q16, which is the population standard deviation.
 */
uint32_t rmsGetAcRmsi16 ( const rmsi16_t* const driver )
{
    return ( driver->acRms );
}
