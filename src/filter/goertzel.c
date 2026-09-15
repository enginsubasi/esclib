/**
  ******************************************************************************
  *
  * @file      goertzel.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Amplitude of one frequency, measured over a block of samples.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      This is the other half of biquad's notch. A notch removes a tone
  *            and this measures one: how much 50 Hz is riding on the load cell,
  *            whether the 1633 Hz column tone of a DTMF digit is present, how
  *            far a resonance has drifted. Nothing else in this library answers
  *            that question — a filter tells you what is left after it, not how
  *            much of one frequency went in.
  *
  * @note      The reason this rather than an FFT. An FFT gives every bin and
  *            costs a scratch buffer, a twiddle table and N log N operations to
  *            do it. Goertzel gives one bin for two multiplies and two adds a
  *            sample, with no buffer at all, which is the right shape whenever
  *            the frequency of interest is known in advance — and in an
  *            embedded system it nearly always is. A handful of these in
  *            parallel is still cheaper than one FFT, which is exactly how a
  *            DTMF receiver is built.
  *
  * @note      It is block based rather than continuous, and that is inherent
  *            rather than a simplification: the recurrence accumulates and only
  *            means anything once the whole block has gone through it. The
  *            block finishes inside goertzelIteration and the state is reloaded
  *            there, so a result the main loop fails to collect costs that
  *            result and never the phase — the same rule softtimer's periodic
  *            reload follows.
  *
  * @note      Choosing the block length is the caller's one real decision, and
  *            it is a three way trade. The block has to span at least one
  *            period of the tone for the answer to mean anything, so
  *            blockLength is never usefully below sampleRate divided by
  *            frequency. Longer blocks narrow the bin, which rejects
  *            neighbouring tones better, and they also make the answer slower
  *            and less able to follow a tone that comes and goes. And the
  *            answer is exact only when frequency times blockLength divided by
  *            sampleRate is a whole number, because the block then holds a
  *            whole number of cycles and the bin sits exactly on the tone;
  *            otherwise the usual spectral leakage of a DFT bin applies and the
  *            reading falls low by up to about 36 percent in the worst case.
  *            Picking blockLength to make that ratio an integer costs nothing
  *            and removes the whole effect.
  *
  * @note      There is no integer variant, and the reason is worth recording
  *            rather than leaving as an absence. The recurrence itself takes a
  *            fixed point coefficient as happily as biquad does, but its two
  *            state words grow with the block: for a tone at the target they
  *            reach about half the amplitude times the block length, so the
  *            usable range depends on a parameter the caller chooses at Init
  *            rather than on a fixed bound. Sizing that honestly, and finding
  *            an integer square root for the magnitude, is a design rather than
  *            a transliteration. goertzelGetPower needs no square root, so a
  *            fixed point variant that reported only power would be the natural
  *            shape if one is wanted.
  *
  ******************************************************************************
  */

#include <math.h>
#include <stddef.h>

#include "goertzel.h"

/*
 * Kept local and single precision, for the reason biquad.c gives: the double
 * M_PI would promote the design arithmetic to double and drag the software
 * double routines in with it on a single precision part.
 */
#define GOERTZEL_PI     3.14159265358979323846f

/**
 * @brief   Initializes the detector for one frequency and block length.
 * @param[out] driver       Detector state to initialize.
 * @param[in]  sampleRate   Rate goertzelIteration will be called at, in hertz.
 * @param[in]  frequency    Frequency to measure, in hertz.
 * @param[in]  blockLength  Samples per measurement.
 * @return  TRUE on success, FALSE when driver is NULL, when the frequencies do
 *          not describe a measurable tone, or when blockLength is below two.
 * @note    frequency must stay below half the sample rate, rejected here for
 *          the reason biquad rejects it: above Nyquist the tone is
 *          indistinguishable from its alias and the answer would be about a
 *          frequency the caller did not ask for.
 * @note    blockLength below two is rejected because the recurrence carries two
 *          state words and there is nothing for the second to hold. A block
 *          that is merely too short for the tone is not rejected: it gives a
 *          leaky but real answer rather than a broken one, and only the caller
 *          knows whether that is what it wants. The file banner gives the rule
 *          for choosing one.
 * @note    The coefficient is taken at the frequency asked for, not at the
 *          nearest exact bin. That keeps the reading about the tone the caller
 *          named; the price is the leakage the banner describes when the block
 *          does not hold a whole number of cycles.
 * @note    The scale normalizes the result so that a sine of amplitude one
 *          reads one, whatever the block length. Without it the raw bin grows
 *          with the block and a threshold would have to be recomputed every
 *          time the block length changed.
 */
uint8_t goertzelInit ( goertzel_t* driver, float sampleRate, float frequency, uint32_t blockLength )
{
    uint8_t retVal = FALSE;
    float omega = 0;
    float n = 0;

    if ( ( driver != NULL ) && ( sampleRate > 0 ) && ( frequency > 0 ) &&
            ( frequency < ( sampleRate / 2.0f ) ) && ( blockLength >= 2u ) )
    {
        omega = ( 2.0f * GOERTZEL_PI * frequency ) / sampleRate;

        driver->coeff = 2.0f * cosf ( omega );

        /*
         * The raw bin of a sine of amplitude A over N samples has magnitude
         * A * N / 2, so its square is A * A * N * N / 4. Dividing the square
         * by that puts the reported power at A * A.
         */
        n = ( float ) blockLength;
        driver->scale = 4.0f / ( n * n );

        driver->blockLength = blockLength;

        goertzelReset ( driver );

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Feeds one sample into the detector.
 * @param[in,out] driver   Detector state.
 * @param[in]     newData  New sample.
 * @note    Call at the sample rate the detector was initialized for. Like every
 *          other filter here it has no clock, and an irregular period moves the
 *          frequency it is actually measuring.
 * @note    The block completes inside this function: the result is computed,
 *          the ready flag raised and the state cleared for the next block, all
 *          before returning. A caller that never reads the result loses that
 *          result and nothing else, and the blocks stay on their original
 *          boundaries — the same reason softtimer reloads inside its tick
 *          rather than at the read.
 * @note    The squared magnitude is s1 squared plus s2 squared minus the
 *          coefficient times both, which is exact arithmetic that cannot be
 *          negative. In single precision it can come out very slightly below
 *          zero on a block that held almost no signal, and the square root of
 *          that is a nan which would then sit in the output forever. It is
 *          clamped at zero rather than left to propagate.
 */
void goertzelIteration ( goertzel_t* driver, float newData )
{
    float s0 = 0;
    float squared = 0;

    s0 = newData + ( driver->coeff * driver->s1 ) - driver->s2;

    driver->s2 = driver->s1;
    driver->s1 = s0;

    ++driver->count;

    if ( driver->count >= driver->blockLength )
    {
        squared = ( driver->s1 * driver->s1 ) + ( driver->s2 * driver->s2 ) -
                    ( driver->coeff * driver->s1 * driver->s2 );

        if ( squared < 0.0f )
        {
            squared = 0.0f;
        }
        else
        {
            /* Intentionally blank */
        }

        driver->power = squared * driver->scale;

        driver->s1 = 0.0f;
        driver->s2 = 0.0f;
        driver->count = 0;
        driver->ready = TRUE;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Reports whether a block has completed since this was last called,
 *          and clears the flag.
 * @param[in,out] driver  Detector state.
 * @return  TRUE when a new result is waiting, FALSE otherwise.
 * @note    This is an accessor that writes, so it takes a non const driver and
 *          is documented [in,out]. bininpGetRisingValue is the other one in
 *          this library and it is the same idea: an event flag that is not
 *          cleared by reading it would read TRUE forever after the first block.
 * @note    Reading the result is separate, so a caller can take the power, the
 *          magnitude, or both, without the flag depending on which.
 */
uint8_t goertzelIsReady ( goertzel_t* driver )
{
    uint8_t retVal = FALSE;

    retVal = driver->ready;

    driver->ready = FALSE;

    return ( retVal );
}

/**
 * @brief   Gets the power of the measured frequency in the last completed
 *          block.
 * @param[in] driver  Detector state.
 * @return  Squared amplitude, so a sine of amplitude one reads one.
 * @note    This is the one to compare a threshold against. It is the square of
 *          goertzelGetMagnitude and needs no square root, which on a part with
 *          no FPU is the difference between a few cycles and a few hundred.
 *          Square the threshold once instead.
 * @note    Zero until the first block completes.
 */
float goertzelGetPower ( const goertzel_t* const driver )
{
    return ( driver->power );
}

/**
 * @brief   Gets the amplitude of the measured frequency in the last completed
 *          block.
 * @param[in] driver  Detector state.
 * @return  Amplitude in the same units as the samples, so a sine of amplitude
 *          one reads one.
 * @note    A square root of goertzelGetPower, computed on every call rather
 *          than stored, because a caller comparing against a threshold should
 *          be using the power and never paying for this at all.
 * @note    Zero until the first block completes.
 */
float goertzelGetMagnitude ( const goertzel_t* const driver )
{
    return ( sqrtf ( driver->power ) );
}

/**
 * @brief   Abandons the block in progress and starts a new one.
 * @param[in,out] driver  Detector state.
 * @note    For a discontinuity the detector should not average across — a gain
 *          change, a channel switch, an input that was disconnected. The
 *          partial block is worthless after one and would otherwise contribute
 *          to the next result.
 * @note    The last result is cleared too, rather than left standing. A stale
 *          power that goertzelGetPower keeps returning after a reset is a trap,
 *          and the ready flag is cleared with it so nothing reads the old value
 *          as new.
 * @note    This does not re-derive the coefficient, so the frequency and block
 *          length are untouched. Changing either is what goertzelInit is for.
 */
void goertzelReset ( goertzel_t* driver )
{
    driver->s1 = 0.0f;
    driver->s2 = 0.0f;
    driver->power = 0.0f;
    driver->count = 0;
    driver->ready = FALSE;
}
