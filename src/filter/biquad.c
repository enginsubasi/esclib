/**
  ******************************************************************************
  *
  * @file      biquad.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.2.1
  * @date      02/08/2026
  *
  * @brief     Second order IIR filter, transposed direct form II.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
  * 15/09/2026 The Q16 fixed point variant is added, for parts with @n
  *            no FPU. It takes coefficients rather than a design @n
  *            in hertz: computing one cosine at boot would pull @n
  *            the whole software float library in, which is the @n
  *            cost the variant exists to avoid. @n
  * 15/09/2026 The Q16 scaling is a multiply rather than a left @n
  *            shift. Shifting a negative signed value left is @n
  *            undefined in C, and every one of these took one @n
  *            while giving the right answer. UBSan found them. @n
  *
  * @note      The exponential moving average is a single pole low pass whose
  *            corner is buried in its alpha and which rolls off at 6 dB per
  *            octave. This shapes a response properly: low pass, high pass,
  *            band pass or notch, at a corner given in hertz, at 12 dB per
  *            octave, and with a chosen Q.
  *
  * @note      The use that has no substitute elsewhere in this library is the
  *            notch. A load cell, a thermocouple, a biopotential front end, any
  *            high impedance analog input picks up mains hum, and no amount of
  *            averaging removes a 50 or 60 Hz tone without also destroying the
  *            signal band.
  *
  * @note      Transposed direct form II is used rather than the textbook direct
  *            form I. It carries two state variables instead of four and is the
  *            better behaved arrangement in floating point, because the state
  *            holds partial sums rather than raw history.
  *
  * @note      Cascade instances for a steeper response. Two of these in series
  *            give a fourth order filter; feed the output of one into the input
  *            of the next.
  *
  ******************************************************************************
  */

#include <math.h>
#include <stddef.h>

#include "biquad.h"

/*
 * Kept local and single precision on purpose. Pulling in the double M_PI would
 * promote the design arithmetic to double and drag the software double
 * routines in with it on a single precision part.
 */
#define BIQUAD_PI   3.14159265358979323846f

/**
 * @brief   Computes the two terms every design in this module is built from.
 * @param[in]  sampleRate  Rate the filter will be iterated at.
 * @param[in]  frequency   Corner or centre frequency.
 * @param[in]  q           Quality factor.
 * @param[out] cosw0       Cosine of the normalized frequency.
 * @param[out] alpha       Bandwidth term, sin( w0 ) / ( 2 * q ).
 * @return  TRUE when the arguments describe a realisable filter, FALSE
 *          otherwise.
 * @note    frequency must stay below half the sample rate. At or above Nyquist
 *          the bilinear transform folds the response back on itself and the
 *          coefficients stop meaning what the caller asked for, so it is
 *          rejected rather than silently aliased.
 */
static uint8_t biquadCommon ( float sampleRate, float frequency, float q, float* cosw0, float* alpha )
{
    uint8_t retVal = FALSE;
    float w0 = 0;

    if ( ( sampleRate > 0 ) && ( frequency > 0 ) &&
            ( frequency < ( sampleRate / 2.0f ) ) && ( q > 0 ) )
    {
        w0 = ( 2.0f * BIQUAD_PI * frequency ) / sampleRate;

        *cosw0 = cosf ( w0 );
        *alpha = sinf ( w0 ) / ( 2.0f * q );

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes the filter from coefficients the caller already has.
 * @param[out] driver  Filter state to initialize.
 * @param[in]  b0      Feed forward coefficient for the current sample.
 * @param[in]  b1      Feed forward coefficient for the previous sample.
 * @param[in]  b2      Feed forward coefficient for the one before that.
 * @param[in]  a1      Feedback coefficient for the previous output.
 * @param[in]  a2      Feedback coefficient for the one before that.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    All five must already be normalized so that a0 is 1. The designer
 *          functions in this module do that division themselves; a set worked
 *          out elsewhere usually needs every coefficient divided by a0 first.
 * @note    Nothing here checks that the coefficients describe a stable filter.
 *          That is a property of where the poles sit, which cannot be read off
 *          the arguments cheaply, and a caller supplying raw coefficients is
 *          assumed to have designed them. The designer functions can only
 *          produce stable sets.
 * @note    The state starts at zero, so the output settles from zero rather
 *          than from the first sample. Feed the filter its own steady input for
 *          a few samples before trusting the output, or call biquadReset.
 */
uint8_t biquadInit ( biquad_t* driver, float b0, float b1, float b2, float a1, float a2 )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        driver->b0 = b0;
        driver->b1 = b1;
        driver->b2 = b2;
        driver->a1 = a1;
        driver->a2 = a2;

        driver->s1 = 0;
        driver->s2 = 0;
        driver->output = 0;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes the filter as a second order low pass.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  sampleRate  Rate the filter will be iterated at, in hertz.
 * @param[in]  cutoff      Corner frequency, in hertz.
 * @param[in]  q           Quality factor. 0.707 gives the flattest passband.
 * @return  TRUE on success, FALSE when driver is NULL or the frequencies and q
 *          do not describe a realisable filter.
 * @note    Above a q of about 0.707 the response peaks before it falls. That is
 *          useful when a resonance is wanted and a mistake when it is not.
 */
uint8_t biquadInitLowPass ( biquad_t* driver, float sampleRate, float cutoff, float q )
{
    uint8_t retVal = FALSE;
    float cosw0 = 0;
    float alpha = 0;
    float a0 = 0;

    if ( ( driver != NULL ) &&
            ( biquadCommon ( sampleRate, cutoff, q, &cosw0, &alpha ) == TRUE ) )
    {
        a0 = 1.0f + alpha;

        retVal = biquadInit ( driver,
                                ( ( 1.0f - cosw0 ) / 2.0f ) / a0,
                                ( 1.0f - cosw0 ) / a0,
                                ( ( 1.0f - cosw0 ) / 2.0f ) / a0,
                                ( -2.0f * cosw0 ) / a0,
                                ( 1.0f - alpha ) / a0 );
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes the filter as a second order high pass.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  sampleRate  Rate the filter will be iterated at, in hertz.
 * @param[in]  cutoff      Corner frequency, in hertz.
 * @param[in]  q           Quality factor. 0.707 gives the flattest passband.
 * @return  TRUE on success, FALSE when driver is NULL or the frequencies and q
 *          do not describe a realisable filter.
 * @note    Strips a slow drift or a dc offset without the settling time a
 *          subtracted running average would add.
 */
uint8_t biquadInitHighPass ( biquad_t* driver, float sampleRate, float cutoff, float q )
{
    uint8_t retVal = FALSE;
    float cosw0 = 0;
    float alpha = 0;
    float a0 = 0;

    if ( ( driver != NULL ) &&
            ( biquadCommon ( sampleRate, cutoff, q, &cosw0, &alpha ) == TRUE ) )
    {
        a0 = 1.0f + alpha;

        retVal = biquadInit ( driver,
                                ( ( 1.0f + cosw0 ) / 2.0f ) / a0,
                                ( - ( 1.0f + cosw0 ) ) / a0,
                                ( ( 1.0f + cosw0 ) / 2.0f ) / a0,
                                ( -2.0f * cosw0 ) / a0,
                                ( 1.0f - alpha ) / a0 );
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes the filter as a band pass with unity gain at the centre.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  sampleRate  Rate the filter will be iterated at, in hertz.
 * @param[in]  centre      Centre frequency, in hertz.
 * @param[in]  q           Quality factor. Higher values narrow the band.
 * @return  TRUE on success, FALSE when driver is NULL or the frequencies and q
 *          do not describe a realisable filter.
 * @note    The bandwidth is the centre frequency divided by q, so a q of 10 at
 *          1 kHz passes roughly 100 Hz.
 */
uint8_t biquadInitBandPass ( biquad_t* driver, float sampleRate, float centre, float q )
{
    uint8_t retVal = FALSE;
    float cosw0 = 0;
    float alpha = 0;
    float a0 = 0;

    if ( ( driver != NULL ) &&
            ( biquadCommon ( sampleRate, centre, q, &cosw0, &alpha ) == TRUE ) )
    {
        a0 = 1.0f + alpha;

        retVal = biquadInit ( driver,
                                alpha / a0,
                                0.0f,
                                ( -alpha ) / a0,
                                ( -2.0f * cosw0 ) / a0,
                                ( 1.0f - alpha ) / a0 );
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes the filter as a notch, rejecting one narrow band.
 * @param[out] driver      Filter state to initialize.
 * @param[in]  sampleRate  Rate the filter will be iterated at, in hertz.
 * @param[in]  centre      Frequency to reject, in hertz.
 * @param[in]  q           Quality factor. Higher values narrow the notch.
 * @return  TRUE on success, FALSE when driver is NULL or the frequencies and q
 *          do not describe a realisable filter.
 * @note    This is the mains hum remover. For 50 Hz sampled at 1 kHz a q around
 *          30 cuts the tone hard while leaving everything a few hertz away
 *          almost untouched.
 * @note    A narrow notch takes correspondingly longer to settle, because a
 *          high q puts the poles close to the unit circle.
 */
uint8_t biquadInitNotch ( biquad_t* driver, float sampleRate, float centre, float q )
{
    uint8_t retVal = FALSE;
    float cosw0 = 0;
    float alpha = 0;
    float a0 = 0;

    if ( ( driver != NULL ) &&
            ( biquadCommon ( sampleRate, centre, q, &cosw0, &alpha ) == TRUE ) )
    {
        a0 = 1.0f + alpha;

        retVal = biquadInit ( driver,
                                1.0f / a0,
                                ( -2.0f * cosw0 ) / a0,
                                1.0f / a0,
                                ( -2.0f * cosw0 ) / a0,
                                ( 1.0f - alpha ) / a0 );
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
 * @note    Call at the sample rate the coefficients were designed for. This
 *          filter has no clock; its whole response is defined relative to the
 *          rate at which this function is called, so an irregular period moves
 *          the corner frequency around.
 */
void biquadIteration ( biquad_t* driver, float newData )
{
    float result = 0;

    result = ( driver->b0 * newData ) + driver->s1;

    driver->s1 = ( driver->b1 * newData ) - ( driver->a1 * result ) + driver->s2;
    driver->s2 = ( driver->b2 * newData ) - ( driver->a2 * result );

    driver->output = result;
}

/**
 * @brief   Gets the current output of the filter.
 * @param[in] driver  Filter state.
 * @return  Most recent filtered sample.
 */
float biquadGetOutput ( const biquad_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Settles the filter state as though the given input had been present
 *          forever, removing the startup transient.
 * @param[in,out] driver     Filter state.
 * @param[in]     inputInit  Steady input the filter is being switched onto.
 * @note    After a plain Init the state is zero, so a filter attached to a
 *          signal already sitting at 1000 spends its whole settling time
 *          climbing from nothing. This puts the state where it would have been
 *          had the input always been inputInit.
 * @note    inputInit is an input, not an output. The settled output is
 *          inputInit multiplied by the gain at dc, so a low pass or a notch
 *          settles on inputInit itself, while a high pass or a band pass has no
 *          gain at dc and correctly settles on zero. Read the result back with
 *          biquadGetOutput rather than assuming it.
 * @note    The gain at dc is undefined when the poles sit on the unit circle at
 *          dc, which makes 1 + a1 + a2 zero. Nothing the designer functions
 *          produce lands there, but a caller supplied coefficient set can, so
 *          the state is simply cleared in that case.
 */
void biquadReset ( biquad_t* driver, float inputInit )
{
    float denominator = 0;
    float settled = 0;

    // Gain at dc, evaluated at z = 1.
    denominator = 1.0f + driver->a1 + driver->a2;

    if ( denominator != 0 )
    {
        settled = inputInit *
                    ( ( driver->b0 + driver->b1 + driver->b2 ) / denominator );
    }
    else
    {
        settled = 0;
    }

    /*
     * The two partial sums that reproduce this output when the input holds
     * still, read straight off the transposed form's own update.
     */
    driver->s1 = settled - ( driver->b0 * inputInit );
    driver->s2 = ( driver->b2 * inputInit ) - ( driver->a2 * settled );

    driver->output = settled;
}

/*
 * Q16 fixed point. A coefficient of 1.0 is 65536, and the scale sits on the
 * coefficients alone: the sample going in and the sample coming out are in
 * plain units, so an ADC reading is wired in unscaled.
 *
 * The state is Q16 in int64_t, as alphabetai32's is, so the fixed point scale
 * does not cost sixteen bits of usable signal range.
 *
 * Designing in hertz stays a float activity and there is no i32 designer. A
 * part with no FPU would pull the whole software float library in to compute
 * one cosine at boot, which is the cost this variant exists to avoid, and the
 * design is a compile time constant in every real use. Run the float designer
 * on a host, or work the coefficients out by hand, and convert each one once:
 *
 *     b0q = ( int32_t ) ( b0 * 65536.0f + ( b0 >= 0 ? 0.5f : -0.5f ) )
 *
 * Q16 is the same scale pid, ramp and alphabeta use, and it holds a1 and a2
 * comfortably: both stay inside plus or minus two for any stable filter, and
 * that bound is also what keeps the feedback product inside int64_t.
 *
 * What Q16 does not hold is a very narrow filter. The feed forward
 * coefficients of a low pass fall as the square of the corner ratio, so at a
 * cutoff of one thousandth of the sample rate b0 is 0.0000102 and quantizes
 * to a single LSB, which lands the dc gain 50 percent high. Measured against
 * an exact model, the dc gain of a quantized low pass is correct to better
 * than 0.1 percent down to a cutoff ratio of about one in five hundred, and
 * unusable by one in a thousand. Below that ratio the float variant is the
 * right tool. A notch is not affected the same way: its feed forward
 * coefficients stay near unity whatever its q, and its dc gain holds to
 * within 0.02 percent at every q tested.
 */
#define BIQUAD_Q        16
#define BIQUAD_ONE      65536
#define BIQUAD_HALF     32768

/**
 * @brief   Brings a Q32 product back down to Q16, rounding to nearest.
 * @param[in] value  Product of a Q16 coefficient and a Q16 term.
 * @return  The same quantity in Q16.
 * @note    A plain arithmetic shift would be cheaper, and emafIterationi32 and
 *          alphabetaIterationi32 both use one. It is not enough here. That
 *          shift truncates toward minus infinity, so it loses half an LSB on
 *          average, and in this filter the loss lands on the output every
 *          sample: a symmetric ac signal through a quantized low pass comes
 *          out with a standing offset of half a count, measured, where the
 *          rounded form has none. Half a count is nothing on a filtered
 *          waveform and is exactly the wrong thing to hand a caller who is
 *          notching mains hum out of a dc measurement.
 * @note    The negative branch negates rather than shifting, because rounding
 *          a negative value by adding half before an arithmetic shift rounds
 *          it the wrong way. Taking the magnitude first makes the rounding
 *          symmetric about zero, which is what mathMapi32 and
 *          interpCalculatei32 do with their divisions.
 */
static int64_t biquadShifti32 ( int64_t value )
{
    int64_t retVal = 0;

    if ( value >= 0 )
    {
        retVal = ( value + BIQUAD_HALF ) >> BIQUAD_Q;
    }
    else
    {
        retVal = - ( ( -value + BIQUAD_HALF ) >> BIQUAD_Q );
    }

    return ( retVal );
}

/**
 * @brief   Initializes the fixed point filter from Q16 coefficients.
 * @param[out] driver  Filter state to initialize.
 * @param[in]  b0      Feed forward coefficient for the current sample, in Q16.
 * @param[in]  b1      Feed forward coefficient for the previous sample, in Q16.
 * @param[in]  b2      Feed forward coefficient for the one before that, in Q16.
 * @param[in]  a1      Feedback coefficient for the previous output, in Q16.
 * @param[in]  a2      Feedback coefficient for the one before that, in Q16.
 * @return  TRUE on success, FALSE when driver is NULL.
 * @note    All five must already be normalized so that a0 is 1, exactly as in
 *          the float variant, and then multiplied by 65536. There is no
 *          designer for this width; the comment above this function gives the
 *          conversion.
 * @note    Only the pointer is checked, for the reason pidIniti32 gives. The
 *          float variant does not test stability either, because that is a
 *          property of where the poles sit rather than of the arguments, and a
 *          guard on one width that the other does not apply would make the two
 *          disagree about what a valid filter is.
 * @note    a1 and a2 stay inside plus or minus two for any stable filter, and
 *          that is what bounds the feedback product in biquadIterationi32.
 *          Coefficients outside it describe an unstable filter, which will run
 *          away in this width as it does in the float one, only sooner.
 */
uint8_t biquadIniti32 ( biquadi32_t* driver, int32_t b0, int32_t b1, int32_t b2, int32_t a1, int32_t a2 )
{
    uint8_t retVal = FALSE;

    if ( driver != NULL )
    {
        driver->b0 = b0;
        driver->b1 = b1;
        driver->b2 = b2;
        driver->a1 = a1;
        driver->a2 = a2;

        driver->s1 = 0;
        driver->s2 = 0;
        driver->output = 0;

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
 * @note    Call at the sample rate the coefficients were designed for, for the
 *          reason biquadIteration gives. This filter has no clock either.
 * @note    Every product is formed in int64_t. The binding one is the feedback
 *          term: a1 reaches two in Q16, so the product overflows once the
 *          filter output passes about 2^30, a little over one thousand
 *          million. Nothing a sensor produces comes close, and the caller who
 *          is feeding something that does has already overflowed the int32_t
 *          the output is reported in.
 * @note    The transposed form keeps two state words rather than four, which
 *          matters more here than in the float variant: each one is an int64_t.
 */
void biquadIterationi32 ( biquadi32_t* driver, int32_t newData )
{
    int64_t result = 0;
    int64_t sample = 0;

    sample = ( int64_t ) newData;

    // Q16 coefficient times a plain sample is already Q16.
    result = ( ( ( int64_t ) driver->b0 ) * sample ) + driver->s1;

    driver->s1 = ( ( ( int64_t ) driver->b1 ) * sample ) -
                    biquadShifti32 ( ( ( int64_t ) driver->a1 ) * result ) +
                    driver->s2;

    driver->s2 = ( ( ( int64_t ) driver->b2 ) * sample ) -
                    biquadShifti32 ( ( ( int64_t ) driver->a2 ) * result );

    driver->output = ( int32_t ) biquadShifti32 ( result );
}

/**
 * @brief   Gets the current output of the fixed point filter.
 * @param[in] driver  Filter state.
 * @return  Most recent filtered sample, in plain units.
 */
int32_t biquadGetOutputi32 ( const biquadi32_t* const driver )
{
    return ( driver->output );
}

/**
 * @brief   Settles the fixed point filter state as though the given input had
 *          been present forever, removing the startup transient.
 * @param[in,out] driver     Filter state.
 * @param[in]     inputInit  Steady input the filter is being switched onto, in
 *                           plain units.
 * @note    Everything biquadReset says applies here. The settled output is
 *          inputInit multiplied by the gain at dc, so a low pass or a notch
 *          settles on inputInit itself while a high pass or a band pass
 *          settles on zero, and the result is read back with
 *          biquadGetOutputi32 rather than assumed.
 * @note    The gain at dc is evaluated as a Q16 ratio, which is where this
 *          width shows its coefficient quantization most plainly: a filter too
 *          narrow for Q16 settles on a visibly wrong value rather than failing
 *          later and subtly. That is the useful direction for the error to
 *          point.
 * @note    inputInit is bounded by the same int64_t arithmetic as everything
 *          else. The numerator carries it shifted into Q16 and multiplied by
 *          the sum of the feed forward coefficients, so an inputInit beyond
 *          about 2^24 can overflow. That is sixteen million counts of a steady
 *          signal, well past any converter this library is copied into.
 * @note    A dc gain denominator of zero clears the state, as the float
 *          variant does, because the poles sit on the unit circle at dc and
 *          there is no settled value to compute.
 */
void biquadReseti32 ( biquadi32_t* driver, int32_t inputInit )
{
    int64_t denominator = 0;
    int64_t numerator = 0;
    int64_t settled = 0;
    int64_t half = 0;

    // Gain at dc, evaluated at z = 1, in Q16.
    denominator = ( int64_t ) BIQUAD_ONE + driver->a1 + driver->a2;

    if ( denominator != 0 )
    {
        /* Multiplied rather than shifted: a negative inputInit shifted
           left is undefined in C. */
        numerator = ( ( ( int64_t ) inputInit ) * BIQUAD_ONE ) *
                        ( ( ( int64_t ) driver->b0 ) + driver->b1 + driver->b2 );

        half = denominator / 2;

        if ( half < 0 )
        {
            half = -half;
        }
        else
        {
            /* Intentionally blank */
        }

        if ( numerator >= 0 )
        {
            numerator += half;
        }
        else
        {
            numerator -= half;
        }

        settled = numerator / denominator;
    }
    else
    {
        settled = 0;
    }

    /*
     * The two partial sums that reproduce this output when the input holds
     * still, read straight off the transposed form's own update.
     */
    driver->s1 = settled - ( ( ( int64_t ) driver->b0 ) * inputInit );
    driver->s2 = ( ( ( int64_t ) driver->b2 ) * inputInit ) -
                    biquadShifti32 ( ( ( int64_t ) driver->a2 ) * settled );

    driver->output = ( int32_t ) biquadShifti32 ( settled );
}
