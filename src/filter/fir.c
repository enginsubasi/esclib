/**
  ******************************************************************************
  *
  * @file      fir.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Finite impulse response filter with caller supplied taps.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      maf is already an FIR filter — a rectangular window, every tap
  *            equal. This is the same structure with the taps left to the
  *            caller, which is the whole difference and it is a large one: a
  *            rectangular window has a first sidelobe only 13 dB down and no
  *            way to move it, while a designed window puts the stopband where
  *            the caller wants it.
  *
  * @note      What is not the difference is cost. maf keeps a running sum and
  *            costs one add and one subtract a sample whatever its length;
  *            this multiplies every tap every sample. So maf is not a special
  *            case to be replaced by this one — it is the right filter
  *            whenever a rectangular window will do, and it stays O(1) where
  *            this is O(N).
  *
  * @note      The reason to reach for this over biquad is phase. A symmetric
  *            tap set has exactly linear phase, so every frequency is delayed
  *            by the same time and the shape of a pulse survives the filter.
  *            An IIR cannot do that at all. When the measurement is the shape
  *            of a waveform — a pulse width, an ECG complex, an edge arrival —
  *            that matters more than the sharper rolloff biquad gets for far
  *            fewer operations.
  *
  * @note      The taps are held as a pointer to const, because a designed tap
  *            set is a compile time constant and belongs in flash. Nothing
  *            here writes to them.
  *
  * @note      There is no designer. Designing an FIR is a windowed sinc or a
  *            Parks-McClellan run, neither of which belongs on the target: the
  *            first needs a sinc and a window function evaluated at boot and
  *            the second is an iterative fit. Taps are worked out on a host
  *            and compiled in, which is also why they are const.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "fir.h"

/*
 * Q16 fixed point for the integer variant, the scale pid, ramp, alphabeta and
 * biquad use. A tap of 1.0 is 65536, and the sample going in and the sample
 * coming out are in plain units.
 */
#define FIR_Q       16
#define FIR_HALF    32768

/**
 * @brief   Initializes the filter and settles it on a steady input.
 * @param[out] driver     Filter state to initialize.
 * @param[in]  taps       Coefficients. taps[ 0 ] multiplies the newest sample.
 * @param[in]  history    Caller owned buffer of length samples.
 * @param[in]  length     Number of taps, and the length of history.
 * @param[in]  inputInit  Steady input the filter is being switched onto.
 * @return  TRUE on success, FALSE when driver, taps or history is NULL, or
 *          when length is zero.
 * @note    taps[ 0 ] multiplies the newest sample and taps[ length - 1 ] the
 *          oldest, which is the ordinary convolution order. A symmetric tap
 *          set — every linear phase design is one — reads the same either way,
 *          so this only matters for an asymmetric one such as a differentiator
 *          or a Hilbert transformer, where getting it backwards inverts the
 *          output.
 * @note    The history is filled with inputInit rather than with zero, so a
 *          filter attached to a signal already sitting at 1000 does not spend
 *          its whole settling time climbing from nothing. The settled output
 *          is inputInit multiplied by the sum of the taps, which is what
 *          firGetOutput returns before the first iteration. For a filter
 *          normalized to unity gain at dc that is inputInit itself; for a
 *          differentiator, whose taps sum to zero, it is zero, and correctly
 *          so.
 * @note    Calling this again is how the filter is re-settled. There is no
 *          separate Reset, because filling the history is all a reset would
 *          do and Init already does it.
 * @note    Nothing here checks that the taps describe a sensible filter. There
 *          is no cheap test for that, and a caller supplying raw coefficients
 *          is assumed to have designed them — the same position biquadInit
 *          takes.
 */
uint8_t firInit ( fir_t* driver, const float* const taps, float* history, uint32_t length, float inputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    float sumOfTaps = 0;

    if ( ( driver != NULL ) && ( taps != NULL ) && ( history != NULL ) &&
            ( length != 0 ) )
    {
        driver->taps = taps;
        driver->history = history;
        driver->length = length;
        driver->index = 0;

        for ( i = 0; i < length; ++i )
        {
            driver->history[ i ] = inputInit;
            sumOfTaps += taps[ i ];
        }

        driver->output = inputInit * sumOfTaps;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Feeds one sample through the filter and updates its output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample.
 * @note    The history is a circular buffer walked backwards from the newest
 *          sample, by index rather than by a moving pointer. That keeps the
 *          taps in their natural order and costs one compare a tap, against
 *          the alternative of shuffling every sample down the buffer, which
 *          costs a move.
 * @note    The accumulator is a local rather than a member. Unlike maf, which
 *          keeps a running sum and has to periodically rebuild it to drop the
 *          rounding error that accumulates in it, this recomputes the whole
 *          sum every sample, so there is no accumulated error to drop.
 */
void firIteration ( fir_t* driver, float newData )
{
    uint32_t i = 0;
    uint32_t h = 0;
    float accumulator = 0;

    driver->history[ driver->index ] = newData;

    // Walk back from the newest sample, so taps[ 0 ] meets it.
    h = driver->index;

    for ( i = 0; i < driver->length; ++i )
    {
        accumulator += driver->taps[ i ] * driver->history[ h ];

        if ( h == 0 )
        {
            h = driver->length - 1u;
        }
        else
        {
            --h;
        }
    }

    driver->output = accumulator;

    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Gets the current output of the filter.
 * @param[in] driver  Filter state.
 * @return  Most recent filtered sample.
 */
float firGetOutput ( const fir_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Brings a Q32 accumulator back down to plain units, rounding to
 *          nearest.
 * @param[in] value  Sum of Q16 taps against plain samples.
 * @return  The same quantity in plain units.
 * @note    Rounded rather than shifted for biquadIterationi32's reason. There
 *          is only one shift a sample here and no feedback to carry an error
 *          forward, but a plain arithmetic shift still truncates toward minus
 *          infinity, which puts half a count of standing offset on a symmetric
 *          signal. That is the wrong thing to hand a caller measuring a level.
 */
static int64_t firShifti32 ( int64_t value )
{
    int64_t retVal = 0;

    if ( value >= 0 )
    {
        retVal = ( value + FIR_HALF ) >> FIR_Q;
    }
    else
    {
        retVal = - ( ( -value + FIR_HALF ) >> FIR_Q );
    }

    return ( retVal );
}

/**
 * @brief   Initializes the fixed point filter and settles it on a steady input.
 * @param[out] driver     Filter state to initialize.
 * @param[in]  taps       Coefficients in Q16, so 65536 is 1.0. taps[ 0 ]
 *                        multiplies the newest sample.
 * @param[in]  history    Caller owned buffer of length samples, in plain units.
 * @param[in]  length     Number of taps, and the length of history.
 * @param[in]  inputInit  Steady input the filter is being switched onto, in
 *                        plain units.
 * @return  TRUE on success, FALSE when driver, taps or history is NULL, or
 *          when length is zero.
 * @note    Only the taps are scaled. The sample going in and the sample coming
 *          out are plain, so an ADC reading is wired in unscaled, exactly as in
 *          biquadIterationi32.
 * @note    The settled output is computed in int64_t and rounded the same way
 *          an iteration is, so firGetOutputi32 before the first sample agrees
 *          with what the filter converges to after one.
 * @note    Convert a designed float tap once, on a host:
 *          ( int32_t ) ( tap * 65536.0f + ( tap >= 0 ? 0.5f : -0.5f ) ).
 *          There is no designer here for the same reason there is none in the
 *          float variant, and no cosine at boot for biquadIniti32's reason.
 * @note    Quantizing the taps to Q16 costs stopband depth. Each tap is right
 *          to about 1.5e-5, and the stopband of a windowed design cannot be
 *          deeper than the error floor those rounded taps leave, which is
 *          roughly 20 log10 ( 1.5e-5 * sqrt ( length ) ) — about -90 dB for a
 *          32 tap filter. Deeper than that wants the float variant.
 */
uint8_t firIniti32 ( firi32_t* driver, const int32_t* const taps, int32_t* history, uint32_t length, int32_t inputInit )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    int64_t sumOfTaps = 0;

    if ( ( driver != NULL ) && ( taps != NULL ) && ( history != NULL ) &&
            ( length != 0 ) )
    {
        driver->taps = taps;
        driver->history = history;
        driver->length = length;
        driver->index = 0;

        for ( i = 0; i < length; ++i )
        {
            driver->history[ i ] = inputInit;
            sumOfTaps += ( int64_t ) taps[ i ];
        }

        driver->output = ( int32_t ) firShifti32 ( sumOfTaps * ( ( int64_t ) inputInit ) );

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Feeds one sample through the fixed point filter and updates its
 *          output.
 * @param[in,out] driver   Filter state.
 * @param[in]     newData  New sample, in plain units.
 * @note    The accumulator is int64_t and the whole sum is formed in it before
 *          a single shift brings it back down, so the intermediate precision is
 *          not the sixteen bits of one tap but the full width. A Q16 tap
 *          against a plain sample is already Q16, and summing length of those
 *          needs the room.
 * @note    The range limit follows from that: the accumulator holds the sum of
 *          the magnitudes of the taps, in Q16, times the largest sample. A tap
 *          set summing to four in magnitude against a sample of sixteen million
 *          reaches about 2^42, which leaves twenty bits of headroom in an
 *          int64_t. Nothing a converter produces comes near it.
 */
void firIterationi32 ( firi32_t* driver, int32_t newData )
{
    uint32_t i = 0;
    uint32_t h = 0;
    int64_t accumulator = 0;

    driver->history[ driver->index ] = newData;

    // Walk back from the newest sample, so taps[ 0 ] meets it.
    h = driver->index;

    for ( i = 0; i < driver->length; ++i )
    {
        accumulator += ( ( int64_t ) driver->taps[ i ] ) *
                        ( ( int64_t ) driver->history[ h ] );

        if ( h == 0 )
        {
            h = driver->length - 1u;
        }
        else
        {
            --h;
        }
    }

    driver->output = ( int32_t ) firShifti32 ( accumulator );

    ++driver->index;

    if ( driver->index >= driver->length )
    {
        driver->index = 0;
    }
    else
    {
        /* Intentionally blank */
    }
}

/**
 * @brief   Gets the current output of the fixed point filter.
 * @param[in] driver  Filter state.
 * @return  Most recent filtered sample, in plain units.
 */
int32_t firGetOutputi32 ( const firi32_t* const driver )
{
    return ( driver->output );
}
