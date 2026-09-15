/**
  ******************************************************************************
  *
  * @file      cordic.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      15/09/2026
  *
  * @brief     Sine, cosine, arc tangent and magnitude in fixed point, by
  *            CORDIC, with no float anywhere in the file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 15/09/2026 Created. @n
  *
  * @note      This module is here because two absences in this library were
  *            written down rather than filled, and both of them were the same
  *            absence. complexi32 has no polar pair, because a fixed point
  *            magnitude needs a square root and a fixed point angle needs an
  *            atan2. And biquad has no i32 designer, because turning a corner
  *            in hertz into coefficients takes a cosine, and calling cosf at
  *            boot on a part with no FPU links the whole software float
  *            library — which is the cost the fixed point variant exists to
  *            avoid. Both wanted the same three functions, and both said so in
  *            their own files. These are those functions.
  *
  * @note      The angle is a binary angle. The whole turn is 2^32 counts in a
  *            uint32_t, so a quarter turn is 0x40000000 and a half turn is
  *            0x80000000. Three things follow, and together they are the
  *            reason this and not Q16 radians. An angle wraps by itself, so
  *            there is no range reduction to get wrong and no pi to represent
  *            — and pi is not representable in Q16 anyway. The type is
  *            unsigned, so that wrap is defined rather than the undefined
  *            behaviour a signed overflow would be. And the conversion a
  *            caller actually needs becomes trivial: a frequency as a fraction
  *            of the sample rate is that fraction of the turn, so a notch at
  *            50 Hz in a 1 kHz loop is ( 50 * 4294967296 ) / 1000, a compile
  *            time constant with no pi in it.
  *
  * @note      No float appears in this file, deliberately, for q16.c's reason:
  *            the whole point is a part with no FPU, and a designed constant
  *            is a compile time expression at the call site. The atan table
  *            below is const and lives in flash.
  *
  * @note      Twenty iterations, and the rotation carries its intermediates in
  *            Q24 rather than in the Q16 it reports. Both numbers were
  *            measured against an exact model rather than chosen: at Q16
  *            internally the sine is out by up to 11 counts of 65536 whatever
  *            the iteration count, because the shifted terms fall below the
  *            last bit before the algorithm has finished with them, and at Q24
  *            it is out by 1. Past twenty iterations nothing improves, because
  *            the table entries are then smaller than the angle's own last
  *            bit. Measured over two hundred thousand angles and as many
  *            vectors: the sine and the cosine are within 1 count of 65536,
  *            the angle from cordicAtan2 within 1307 counts of a turn, which
  *            is a ten thousandth of a degree, the length from
  *            cordicMagnitude within four parts in a hundred million, and a
  *            component of a rotated vector within two parts in a million of
  *            the vector's length. The rotation is the loosest of them
  *            because its error is the angular resolution of twenty
  *            iterations rather than any constant in the file.
  *
  * @note      CORDIC does not rotate a vector, it rotates and stretches one,
  *            by a factor that depends only on the iteration count. So the
  *            rotation starts from 1/gain rather than from one, and the
  *            vectoring modes divide it out at the end.
  *
  * @note      The vectoring modes normalize their arguments first, shifting
  *            both components together until the larger one sits near 2^29.
  *            This is not an optimization: the shifted terms of the iteration
  *            are integers, so on an argument of 3 the algorithm has three
  *            bits to work with and answers nonsense. Shifting is safe because
  *            an angle does not depend on the scale of the vector it is taken
  *            from, and the magnitude is shifted back at the end — saturating
  *            rather than wrapping, since a length reaches past int32_t before
  *            its components do.
  *
  * @note      A right shift of a negative value is implementation defined in
  *            C, and this file takes one in every iteration. That is the same
  *            shift emafi32, biquad and fir take, and it is accepted here for
  *            their reason: every compiler this library targets shifts in the
  *            sign bit. A left shift of a negative value is a different thing
  *            — undefined outright — so where this file has to scale a signed
  *            value up it multiplies by two instead.
  *
  * @note      Both arguments zero is the one case with no answer: every angle
  *            is as true as every other. It reports a magnitude of zero and an
  *            angle of zero rather than whatever the iteration would leave
  *            behind, which is neither.
  *
  * @note      The pointers are not checked, which is this library's rule for a
  *            module with no Init to check them in. pack and complex take the
  *            same pointers on the same terms.
  *
  ******************************************************************************
  */

#include "cordic.h"

/* Iterations. Twenty, for the reason the banner gives. */
#define CORDIC_ITERATIONS       20u

/* The scale the rotation carries internally, and the shift back to Q16. */
#define CORDIC_INTERNAL_SHIFT   8
#define CORDIC_INTERNAL_HALF    128

/*
 * 1/gain for twenty iterations, in Q24. The gain is the product of
 * sqrt ( 1 + 4^-i ) over the iterations, which is 1.6467602581 here, and it is
 * a property of the iteration count alone.
 *
 * Q24 rather than the Q16 everything else here reports in, because the
 * constant's own rounding is the error floor of every length this module
 * returns: at Q16 it is 1.8 parts in a million and the magnitudes came back
 * biased high by exactly that, which a check on a round number catches
 * immediately. At Q24 it is 34 parts in a thousand million, below the last bit
 * of the answer.
 */
#define CORDIC_INVGAIN_Q24      10188014
#define CORDIC_INVGAIN_SHIFT    24
#define CORDIC_INVGAIN_HALF     8388608

/*
 * Where the normalization puts the larger component before the iteration
 * starts, and the point past which one more doubling would overshoot it. The
 * bound is what keeps the iteration inside int32_t: it stretches a vector by
 * the gain, and a component may reach the length, so 2^29 times 1.65 times
 * the root of two is the worst intermediate and it fits with room to spare.
 */
#define CORDIC_NORMAL_TARGET    0x20000000
#define CORDIC_NORMAL_HALF      0x10000000

/* The residual angle inside one quadrant. */
#define CORDIC_QUADRANT_MASK    0x3FFFFFFFu

/*
 * atan ( 2^-i ) expressed in binary angle counts, which is
 * atan ( 2^-i ) / ( 2 * pi ) * 2^32 rounded. The first entry is a full
 * eighth of a turn because atan ( 1 ) is 45 degrees.
 */
static const int32_t cordicAtanTable [ CORDIC_ITERATIONS ] =
{
    536870912, 316933406, 167458907,  85004756,
     42667331,  21354465,  10679838,   5340245,
      2670163,   1335087,    667544,    333772,
       166886,     83443,     41722,     20861,
        10430,      5215,      2608,      1304
};

/**
 * @brief   Narrows a wide intermediate into an int32_t, saturating rather than
 *          wrapping.
 *
 * @param[in]   value   the wide value.
 *
 * @return  The value, held at the ends of int32_t.
 *
 * @note    A wrapped length changes sign, and a magnitude that reports
 *          negative is worse than one that reports too small.
 */
static int32_t cordicSaturate ( int64_t value )
{
    int32_t retVal = 0;

    if ( value > ( ( int64_t ) 2147483647 ) )
    {
        retVal = 2147483647;
    }
    else if ( value < ( ( int64_t ) ( -2147483647 - 1 ) ) )
    {
        retVal = -2147483647 - 1;
    }
    else
    {
        retVal = ( int32_t ) value;
    }

    return ( retVal );
}

/**
 * @brief   Rotates a vector by an angle inside one quadrant.
 *
 * @param[in,out]   x       the first component.
 * @param[in,out]   y       the second component.
 * @param[in]       angle   the angle, in binary angle counts, no larger than a
 *                          quarter turn.
 *
 * @note    The result is the rotation stretched by the CORDIC gain. The caller
 *          divides it out, because the two modes do so at different scales.
 */
static void cordicRotation ( int32_t* x, int32_t* y, int32_t angle )
{
    uint32_t i = 0u;
    int32_t xi = *x;
    int32_t yi = *y;
    int32_t zi = angle;
    int32_t xn = 0;

    for ( i = 0u; i < CORDIC_ITERATIONS; ++i )
    {
        xn = xi;

        if ( zi >= 0 )
        {
            xi = xi - ( yi >> i );
            yi = yi + ( xn >> i );
            zi = zi - cordicAtanTable[ i ];
        }
        else
        {
            xi = xi + ( yi >> i );
            yi = yi - ( xn >> i );
            zi = zi + cordicAtanTable[ i ];
        }
    }

    *x = xi;
    *y = yi;
}

/**
 * @brief   Drives the second component to zero and accumulates the angle it
 *          took to do it.
 *
 * @param[in,out]   x       the first component, which ends as the gain times
 *                          the length of the vector.
 * @param[in]       y       the second component.
 * @param[out]      angle   the angle of the original vector, in binary angle
 *                          counts, within a quarter turn of zero.
 *
 * @note    Only defined for a first component that is not negative, which is
 *          why the callers fold the left half plane over first.
 */
static void cordicVectoring ( int32_t* x, int32_t y, int32_t* angle )
{
    uint32_t i = 0u;
    int32_t xi = *x;
    int32_t yi = y;
    int32_t zi = 0;
    int32_t xn = 0;

    for ( i = 0u; i < CORDIC_ITERATIONS; ++i )
    {
        xn = xi;

        if ( yi < 0 )
        {
            xi = xi - ( yi >> i );
            yi = yi + ( xn >> i );
            zi = zi - cordicAtanTable[ i ];
        }
        else
        {
            xi = xi + ( yi >> i );
            yi = yi - ( xn >> i );
            zi = zi + cordicAtanTable[ i ];
        }
    }

    *x = xi;
    *angle = zi;
}

/**
 * @brief   Brings the larger component of a vector near the working range.
 *
 * @param[in,out]   x       the first component.
 * @param[in,out]   y       the second component.
 * @param[out]      left    how many doublings were applied.
 * @param[out]      right   how many halvings were applied.
 *
 * @note    Scaling up is a multiply rather than a left shift, because the
 *          components are signed and may be negative. Scaling down is a right
 *          shift, which is implementation defined for a negative value and
 *          accepted here as it is everywhere else in this tree.
 */
static void cordicNormalize ( int32_t* x, int32_t* y, uint32_t* left,
                              uint32_t* right )
{
    int32_t xi = *x;
    int32_t yi = *y;
    uint32_t up = 0u;
    uint32_t down = 0u;

    while ( ( xi > CORDIC_NORMAL_TARGET ) || ( xi < -CORDIC_NORMAL_TARGET ) ||
            ( yi > CORDIC_NORMAL_TARGET ) || ( yi < -CORDIC_NORMAL_TARGET ) )
    {
        xi = xi >> 1;
        yi = yi >> 1;
        ++down;
    }

    while ( ( xi <= CORDIC_NORMAL_HALF ) && ( xi >= -CORDIC_NORMAL_HALF ) &&
            ( yi <= CORDIC_NORMAL_HALF ) && ( yi >= -CORDIC_NORMAL_HALF ) &&
            ( ( xi != 0 ) || ( yi != 0 ) ) )
    {
        xi = xi * 2;
        yi = yi * 2;
        ++up;
    }

    *x = xi;
    *y = yi;
    *left = up;
    *right = down;
}

/**
 * @brief   Restores a value the normalization had scaled.
 *
 * @param[in]   value   the value at the normalized scale.
 * @param[in]   left    how many doublings the normalization applied.
 * @param[in]   right   how many halvings it applied.
 *
 * @return  The value at the caller's scale, saturated.
 *
 * @note    The halvings are undone by repeated doubling rather than by one
 *          shift, so that the saturation catches a length that has grown past
 *          int32_t on the way back.
 */
static int32_t cordicDenormalize ( int64_t value, uint32_t left,
                                   uint32_t right )
{
    int64_t scaled = value;
    uint32_t i = 0u;

    if ( left > 0u )
    {
        scaled = scaled + ( ( int64_t ) ( ( ( int32_t ) 1 ) << ( left - 1u ) ) );
        scaled = scaled >> left;
    }
    else
    {
        /* Intentionally blank */
    }

    for ( i = 0u; i < right; ++i )
    {
        scaled = scaled * 2;
    }

    return ( cordicSaturate ( scaled ) );
}

/**
 * @brief   Calculates the sine and the cosine of an angle together.
 *
 * @param[in]   angle   the angle, in binary angle counts, where the whole turn
 *                      is 2^32 and any value is in range.
 * @param[out]  sine    the sine, in Q16, from -CORDIC_ONE to CORDIC_ONE.
 * @param[out]  cosine  the cosine, in the same scale.
 *
 * @note    Both come out of one run of the algorithm, which is why this is the
 *          function and cordicSin and cordicCos are the convenience. A caller
 *          that needs the pair should take it here rather than pay for the
 *          iteration twice.
 *
 * @note    The quadrant is taken off the top two bits and put back onto the
 *          results as a swap and a sign, so the iteration only ever sees an
 *          angle inside the first quadrant, where it converges.
 */
void cordicSinCos ( uint32_t angle, int32_t* sine, int32_t* cosine )
{
    uint32_t quadrant = angle >> 30;
    int32_t residual = ( int32_t ) ( angle & CORDIC_QUADRANT_MASK );
    int32_t xi = CORDIC_INVGAIN_Q24;
    int32_t yi = 0;
    int32_t c = 0;
    int32_t s = 0;

    cordicRotation ( &xi, &yi, residual );

    c = ( xi + CORDIC_INTERNAL_HALF ) >> CORDIC_INTERNAL_SHIFT;
    s = ( yi + CORDIC_INTERNAL_HALF ) >> CORDIC_INTERNAL_SHIFT;

    if ( quadrant == 0u )
    {
        *sine = s;
        *cosine = c;
    }
    else if ( quadrant == 1u )
    {
        *sine = c;
        *cosine = -s;
    }
    else if ( quadrant == 2u )
    {
        *sine = -s;
        *cosine = -c;
    }
    else
    {
        *sine = -c;
        *cosine = s;
    }
}

/**
 * @brief   Calculates the sine of an angle.
 *
 * @param[in]   angle   the angle, in binary angle counts.
 *
 * @return  The sine, in Q16.
 *
 * @note    Runs the same iteration as cordicSinCos and throws the cosine away.
 */
int32_t cordicSin ( uint32_t angle )
{
    int32_t retVal = 0;
    int32_t cosine = 0;

    cordicSinCos ( angle, &retVal, &cosine );

    return ( retVal );
}

/**
 * @brief   Calculates the cosine of an angle.
 *
 * @param[in]   angle   the angle, in binary angle counts.
 *
 * @return  The cosine, in Q16.
 *
 * @note    Runs the same iteration as cordicSinCos and throws the sine away.
 */
int32_t cordicCos ( uint32_t angle )
{
    int32_t retVal = 0;
    int32_t sine = 0;

    cordicSinCos ( angle, &sine, &retVal );

    return ( retVal );
}

/**
 * @brief   Converts a vector into a length and an angle.
 *
 * @param[in]   x           the first component.
 * @param[in]   y           the second component.
 * @param[out]  magnitude   the length of the vector, in the units of the
 *                          components, saturated at int32_t.
 * @param[out]  angle       the angle of the vector, in binary angle counts,
 *                          measured from the first axis towards the second.
 *
 * @note    Both come out of one run for cordicSinCos's reason, and cordicAtan2
 *          and cordicMagnitude are the convenience over this.
 *
 * @note    A first component below zero is folded over before the iteration
 *          and paid for with a half turn on the angle afterwards, because the
 *          algorithm only converges within a quarter turn of the first axis.
 *
 * @note    Both components zero gives a magnitude of zero and an angle of
 *          zero. No angle is more true than another there.
 */
void cordicPolar ( int32_t x, int32_t y, int32_t* magnitude, uint32_t* angle )
{
    int32_t xi = x;
    int32_t yi = y;
    int32_t residual = 0;
    int64_t length = 0;
    uint32_t left = 0u;
    uint32_t right = 0u;
    uint32_t offset = 0u;

    if ( ( x == 0 ) && ( y == 0 ) )
    {
        *magnitude = 0;
        *angle = 0u;
    }
    else
    {
        cordicNormalize ( &xi, &yi, &left, &right );

        if ( xi < 0 )
        {
            xi = -xi;
            yi = -yi;
            offset = CORDIC_HALF;
        }
        else
        {
            /* Intentionally blank */
        }

        cordicVectoring ( &xi, yi, &residual );

        length = ( ( ( int64_t ) xi ) * CORDIC_INVGAIN_Q24 ) +
                 CORDIC_INVGAIN_HALF;
        length = length >> CORDIC_INVGAIN_SHIFT;

        *magnitude = cordicDenormalize ( length, left, right );
        *angle = ( ( uint32_t ) residual ) + offset;
    }
}

/**
 * @brief   Calculates the angle of a vector.
 *
 * @param[in]   y   the second component.
 * @param[in]   x   the first component.
 *
 * @return  The angle, in binary angle counts, measured from the first axis
 *          towards the second.
 *
 * @note    The arguments are in the order C's atan2 takes them, which is the
 *          opposite of the order everything else in this module takes them,
 *          because that order is what a caller reaching for this expects.
 */
uint32_t cordicAtan2 ( int32_t y, int32_t x )
{
    uint32_t retVal = 0u;
    int32_t magnitude = 0;

    cordicPolar ( x, y, &magnitude, &retVal );

    return ( retVal );
}

/**
 * @brief   Calculates the length of a vector.
 *
 * @param[in]   x   the first component.
 * @param[in]   y   the second component.
 *
 * @return  The length, in the units of the components, saturated at int32_t.
 *
 * @note    This is a hypotenuse rather than a square root of a sum: nothing
 *          here squares a component, so a pair whose squares would overflow
 *          still gives an answer.
 */
int32_t cordicMagnitude ( int32_t x, int32_t y )
{
    int32_t retVal = 0;
    uint32_t angle = 0u;

    cordicPolar ( x, y, &retVal, &angle );

    return ( retVal );
}

/**
 * @brief   Rotates a vector by an angle.
 *
 * @param[in,out]   x       the first component.
 * @param[in,out]   y       the second component.
 * @param[in]       angle   the angle to rotate by, in binary angle counts.
 *
 * @note    This is the operation complexi32 cannot offer, and it is one run of
 *          the algorithm rather than the four multiplies a caller would write
 *          from a sine and a cosine.
 *
 * @note    The whole quadrant of the rotation is applied to the vector as a
 *          swap and a sign before the iteration, which leaves the iteration
 *          the residual it converges over.
 */
void cordicRotate ( int32_t* x, int32_t* y, uint32_t angle )
{
    uint32_t quadrant = angle >> 30;
    int32_t residual = ( int32_t ) ( angle & CORDIC_QUADRANT_MASK );
    int32_t xi = *x;
    int32_t yi = *y;
    int32_t swap = 0;
    uint32_t left = 0u;
    uint32_t right = 0u;

    if ( ( xi == 0 ) && ( yi == 0 ) )
    {
        /* Intentionally blank */
    }
    else
    {
        cordicNormalize ( &xi, &yi, &left, &right );

        if ( quadrant == 1u )
        {
            swap = xi;
            xi = -yi;
            yi = swap;
        }
        else if ( quadrant == 2u )
        {
            xi = -xi;
            yi = -yi;
        }
        else if ( quadrant == 3u )
        {
            swap = xi;
            xi = yi;
            yi = -swap;
        }
        else
        {
            /* Intentionally blank */
        }

        cordicRotation ( &xi, &yi, residual );

        *x = cordicDenormalize (
                 ( ( ( ( int64_t ) xi ) * CORDIC_INVGAIN_Q24 ) +
                   CORDIC_INVGAIN_HALF ) >> CORDIC_INVGAIN_SHIFT,
                 left, right );
        *y = cordicDenormalize (
                 ( ( ( ( int64_t ) yi ) * CORDIC_INVGAIN_Q24 ) +
                   CORDIC_INVGAIN_HALF ) >> CORDIC_INVGAIN_SHIFT,
                 left, right );
    }
}
