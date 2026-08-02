/**
  ******************************************************************************
  *
  * @file      biquad.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      02/08/2026
  *
  * @brief     Second order IIR filter, transposed direct form II.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 02/08/2026 Created. @n
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
