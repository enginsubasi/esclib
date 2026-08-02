/*
 * Covers basicmath and statistic.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure. Neither module had a test before
 * this file, and basicmath was the file the July 2026 audit changed most.
 *
 * Every float expectation was worked out from an IEEE binary32 model before the
 * C was written. The float checks use a tolerance rather than an exact
 * comparison: this library is built for several targets and the order a
 * compiler evaluates an expression in is its own business.
 */

#include <stdio.h>

#include "basicmath.h"
#include "statistic.h"

static uint32_t failures = 0;

static void check ( const char* what, uint8_t condition )
{
    if ( condition == TRUE )
    {
        printf ( "  PASS  %s\n", what );
    }
    else
    {
        printf ( "  FAIL  %s\n", what );
        ++failures;
    }
}

static uint8_t nearly ( float got, float wanted )
{
    uint8_t retVal = FALSE;
    float diff = 0;

    diff = got - wanted;

    if ( diff < 0 )
    {
        diff = -diff;
    }

    if ( diff < 0.0001f )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* ------------------------------------------------------------- fixtures */

static const float    fa[ 5 ] = { 3.5f, -1.25f, 9.0f, 0.0f, -4.5f };
static const uint32_t ua[ 5 ] = { 30u, 5u, 900u, 0u, 45u };
static const int32_t  ia[ 5 ] = { 5, -3, 9, 0, -7 };

/* Median takes an array that is already sorted. */
static const float    fOdd[ 3 ]  = { 1.0f, 3.0f, 7.0f };
static const float    fEven[ 4 ] = { 1.0f, 3.0f, 7.0f, 9.0f };
static const uint32_t uOdd[ 3 ]  = { 1u, 3u, 7u };
static const uint32_t uEven[ 2 ] = { 10u, 15u };
static const int32_t  iOdd[ 3 ]  = { -5, 0, 5 };
static const int32_t  iEven[ 2 ] = { -10, -5 };
static const int32_t  iEven2[ 2 ] = { -9, -4 };

/* ------------------------------------------------------------- absolute */

static void absoluteCase ( void )
{
    printf ( "mathAbsolute\n" );

    check ( "a negative float", nearly ( mathAbsolute ( -2.5f ), 2.5f ) );
    check ( "a positive float", nearly ( mathAbsolute ( 2.5f ), 2.5f ) );
    check ( "zero", nearly ( mathAbsolute ( 0.0f ), 0.0f ) );

    check ( "a negative integer", ( uint8_t ) ( mathAbsolutei32 ( -7 ) == 7 ) );
    check ( "a positive integer", ( uint8_t ) ( mathAbsolutei32 ( 7 ) == 7 ) );
    check ( "integer zero", ( uint8_t ) ( mathAbsolutei32 ( 0 ) == 0 ) );

    /*
     * mathAbsolutei32 ( INT32_MIN ) is deliberately not checked. Negating
     * INT32_MIN overflows int32_t, which is undefined, so there is no correct
     * answer to assert against.
     */
}

/* ---------------------------------------------------------- min and max */

static void minMaxCase ( void )
{
    float fMin = 0;
    float fMax = 0;
    uint32_t uMin = 0;
    uint32_t uMax = 0;
    int32_t iMin = 0;
    int32_t iMax = 0;

    printf ( "mathFindMax, mathFindMin and mathFindMinMax\n" );

    check ( "float max", nearly ( mathFindMax ( fa, 5u ), 9.0f ) );
    check ( "float min", nearly ( mathFindMin ( fa, 5u ), -4.5f ) );
    check ( "u32 max", ( uint8_t ) ( mathFindMaxu32 ( ua, 5u ) == 900u ) );
    check ( "u32 min", ( uint8_t ) ( mathFindMinu32 ( ua, 5u ) == 0u ) );
    check ( "i32 max", ( uint8_t ) ( mathFindMaxi32 ( ia, 5u ) == 9 ) );

    /*
     * mathFindMini32 used to compare with < instead of >, so it returned the
     * largest element. An array whose minimum and maximum differ, with the
     * minimum not at index 0, is what catches that.
     */
    check ( "i32 min, the function that once returned the maximum",
            ( uint8_t ) ( mathFindMini32 ( ia, 5u ) == -7 ) );

    mathFindMinMax ( fa, 5u, &fMin, &fMax );
    check ( "float minmax agrees with the separate calls",
            ( uint8_t ) ( nearly ( fMin, -4.5f ) && nearly ( fMax, 9.0f ) ) );

    mathFindMinMaxu32 ( ua, 5u, &uMin, &uMax );
    check ( "u32 minmax", ( uint8_t ) ( ( uMin == 0u ) && ( uMax == 900u ) ) );

    mathFindMinMaxi32 ( ia, 5u, &iMin, &iMax );
    check ( "i32 minmax", ( uint8_t ) ( ( iMin == -7 ) && ( iMax == 9 ) ) );

    check ( "a single element is both", nearly ( mathFindMax ( fa, 1u ), 3.5f ) );
    check ( "and so is its minimum", nearly ( mathFindMin ( fa, 1u ), 3.5f ) );
}

/* ------------------------------------------------- sum, mean and range */

static void sumMeanRangeCase ( void )
{
    printf ( "mathCalculateSum, Mean and Range\n" );

    check ( "float sum", nearly ( mathCalculateSum ( fa, 5u ), 6.75f ) );
    check ( "float mean", nearly ( mathCalculateMean ( fa, 5u ), 1.35f ) );
    check ( "float range", nearly ( mathCalculateRange ( fa, 5u ), 13.5f ) );

    check ( "u32 sum", ( uint8_t ) ( mathCalculateSumu32 ( ua, 5u ) == 980u ) );
    check ( "u32 mean", ( uint8_t ) ( mathCalculateMeanu32 ( ua, 5u ) == 196u ) );
    check ( "u32 range", ( uint8_t ) ( mathCalculateRangeu32 ( ua, 5u ) == 900u ) );

    check ( "i32 sum", ( uint8_t ) ( mathCalculateSumi32 ( ia, 5u ) == 4 ) );

    /* 4 / 5 truncates towards zero, which is what the documentation promises. */
    check ( "i32 mean truncates towards zero",
            ( uint8_t ) ( mathCalculateMeani32 ( ia, 5u ) == 0 ) );
    check ( "i32 range spans both signs",
            ( uint8_t ) ( mathCalculateRangei32 ( ia, 5u ) == 16 ) );
}

/* --------------------------------------------------------------- median */

static void medianCase ( void )
{
    printf ( "mathCalculateMedian\n" );

    check ( "float odd length is the middle element",
            nearly ( mathCalculateMedian ( fOdd, 3u ), 3.0f ) );

    /*
     * The even length case used to return the upper of the two middle elements
     * instead of their mean, so this would have been 7.0.
     */
    check ( "float even length averages the two middle elements",
            nearly ( mathCalculateMedian ( fEven, 4u ), 5.0f ) );

    check ( "u32 odd length", ( uint8_t ) ( mathCalculateMedianu32 ( uOdd, 3u ) == 3u ) );
    check ( "u32 even length, computed as low + (high-low)/2",
            ( uint8_t ) ( mathCalculateMedianu32 ( uEven, 2u ) == 12u ) );

    check ( "i32 odd length", ( uint8_t ) ( mathCalculateMediani32 ( iOdd, 3u ) == 0 ) );

    /*
     * The even length integer midpoint rounds towards the lower element, so
     * these are -8 and -7 rather than -7 and -6.
     */
    check ( "i32 even length on negatives",
            ( uint8_t ) ( mathCalculateMediani32 ( iEven, 2u ) == -8 ) );
    check ( "i32 even length, odd gap",
            ( uint8_t ) ( mathCalculateMediani32 ( iEven2, 2u ) == -7 ) );
}

/* ------------------------------------------------------- zero length */

static void zeroLengthCase ( void )
{
    float fMin = 5.0f;
    float fMax = 5.0f;
    int32_t iMin = 5;
    int32_t iMax = 5;

    printf ( "a zero length array returns zero everywhere\n" );

    /*
     * These functions used to read array[0] or divide by length before checking
     * it. A real buffer is passed so a read past the guard would still be
     * reading something, and the value it would find is not zero.
     */
    check ( "findMax", nearly ( mathFindMax ( fa, 0u ), 0.0f ) );
    check ( "findMin", nearly ( mathFindMin ( fa, 0u ), 0.0f ) );
    check ( "findMaxu32", ( uint8_t ) ( mathFindMaxu32 ( ua, 0u ) == 0u ) );
    check ( "findMinu32", ( uint8_t ) ( mathFindMinu32 ( ua, 0u ) == 0u ) );
    check ( "findMaxi32", ( uint8_t ) ( mathFindMaxi32 ( ia, 0u ) == 0 ) );
    check ( "findMini32", ( uint8_t ) ( mathFindMini32 ( ia, 0u ) == 0 ) );

    check ( "calculateSum", nearly ( mathCalculateSum ( fa, 0u ), 0.0f ) );
    check ( "calculateMean", nearly ( mathCalculateMean ( fa, 0u ), 0.0f ) );
    check ( "calculateMedian", nearly ( mathCalculateMedian ( fa, 0u ), 0.0f ) );
    check ( "calculateRange", nearly ( mathCalculateRange ( fa, 0u ), 0.0f ) );

    check ( "calculateSumu32", ( uint8_t ) ( mathCalculateSumu32 ( ua, 0u ) == 0u ) );
    check ( "calculateMeanu32", ( uint8_t ) ( mathCalculateMeanu32 ( ua, 0u ) == 0u ) );
    check ( "calculateMedianu32", ( uint8_t ) ( mathCalculateMedianu32 ( ua, 0u ) == 0u ) );
    check ( "calculateRangeu32", ( uint8_t ) ( mathCalculateRangeu32 ( ua, 0u ) == 0u ) );

    check ( "calculateSumi32", ( uint8_t ) ( mathCalculateSumi32 ( ia, 0u ) == 0 ) );
    check ( "calculateMeani32", ( uint8_t ) ( mathCalculateMeani32 ( ia, 0u ) == 0 ) );
    check ( "calculateMediani32", ( uint8_t ) ( mathCalculateMediani32 ( ia, 0u ) == 0 ) );
    check ( "calculateRangei32", ( uint8_t ) ( mathCalculateRangei32 ( ia, 0u ) == 0 ) );

    mathFindMinMax ( fa, 0u, &fMin, &fMax );
    check ( "findMinMax writes zero to both outputs",
            ( uint8_t ) ( nearly ( fMin, 0.0f ) && nearly ( fMax, 0.0f ) ) );

    mathFindMinMaxi32 ( ia, 0u, &iMin, &iMax );
    check ( "findMinMaxi32 writes zero to both outputs",
            ( uint8_t ) ( ( iMin == 0 ) && ( iMax == 0 ) ) );
}

/* ------------------------------------------------------------ statistic */

static void statisticCase ( void )
{
    /* The textbook set: mean 5, variance 4, standard deviation 2, all exact. */
    static const float    tf[ 8 ] = { 2.0f, 4.0f, 4.0f, 4.0f, 5.0f, 5.0f, 7.0f, 9.0f };
    static const int32_t  ti[ 8 ] = { 2, 4, 4, 4, 5, 5, 7, 9 };
    static const uint32_t tu[ 8 ] = { 2u, 4u, 4u, 4u, 5u, 5u, 7u, 9u };

    static const float    sf[ 4 ] = { 1.0f, 2.0f, 3.0f, 4.0f };
    static const int32_t  si[ 4 ] = { 1, 2, 3, 4 };
    static const uint32_t su[ 4 ] = { 1u, 2u, 3u, 4u };

    static const float    ff[ 5 ] = { 7.0f, 7.0f, 7.0f, 7.0f, 7.0f };
    static const int32_t  ni[ 5 ] = { -8, -2, 0, 2, 8 };

    static const float c1[ 5 ] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    static const float c2[ 5 ] = { 2.0f, 4.0f, 6.0f, 8.0f, 10.0f };
    static const float c3[ 5 ] = { 10.0f, 8.0f, 6.0f, 4.0f, 2.0f };
    static const int32_t d1[ 5 ] = { 1, 2, 3, 4, 5 };
    static const int32_t d2[ 5 ] = { 2, 4, 6, 8, 10 };
    static const int32_t d3[ 5 ] = { 10, 8, 6, 4, 2 };

    printf ( "statistic\n" );

    check ( "float variance", nearly ( statVariance ( tf, 8u ), 4.0f ) );
    check ( "float standard deviation", nearly ( statStandardDeviation ( tf, 8u ), 2.0f ) );
    check ( "i32 variance", ( uint8_t ) ( statVariancei32 ( ti, 8u ) == 4 ) );
    check ( "i32 standard deviation", ( uint8_t ) ( statStandardDeviationi32 ( ti, 8u ) == 2 ) );
    check ( "u32 variance", ( uint8_t ) ( statVarianceu32 ( tu, 8u ) == 4u ) );
    check ( "u32 standard deviation", ( uint8_t ) ( statStandardDeviationu32 ( tu, 8u ) == 2u ) );

    /*
     * The integer variants do the mean in integer arithmetic too, so they lose
     * precision the float one keeps. This is documented behaviour, and pinning
     * it here means a change to it has to be deliberate.
     */
    check ( "float variance of {1,2,3,4} is 1.25", nearly ( statVariance ( sf, 4u ), 1.25f ) );
    check ( "the i32 variance of the same data truncates to 1",
            ( uint8_t ) ( statVariancei32 ( si, 4u ) == 1 ) );
    check ( "and so does the u32 one",
            ( uint8_t ) ( statVarianceu32 ( su, 4u ) == 1u ) );

    check ( "a constant array has zero variance", nearly ( statVariance ( ff, 5u ), 0.0f ) );

    /*
     * statVarianceu32 takes each difference in whichever direction is non
     * negative, because an unsigned subtraction wraps rather than going below
     * zero. Data on both sides of the mean is what exercises that.
     */
    check ( "u32 variance with elements below the mean",
            ( uint8_t ) ( statVarianceu32 ( tu, 8u ) == 4u ) );

    check ( "i32 variance on data spanning zero",
            ( uint8_t ) ( statVariancei32 ( ni, 5u ) == 27 ) );

    check ( "float covariance of two rising arrays",
            nearly ( statCovariance ( c1, c2, 5u ), 4.0f ) );
    check ( "and it goes negative when one array falls",
            nearly ( statCovariance ( c1, c3, 5u ), -4.0f ) );
    check ( "i32 covariance", ( uint8_t ) ( statCovariancei32 ( d1, d2, 5u ) == 4 ) );
    check ( "i32 covariance carries the sign",
            ( uint8_t ) ( statCovariancei32 ( d1, d3, 5u ) == -4 ) );

    check ( "zero length variance", nearly ( statVariance ( tf, 0u ), 0.0f ) );
    check ( "zero length standard deviation",
            nearly ( statStandardDeviation ( tf, 0u ), 0.0f ) );
    check ( "zero length variancei32", ( uint8_t ) ( statVariancei32 ( ti, 0u ) == 0 ) );
    check ( "zero length varianceu32", ( uint8_t ) ( statVarianceu32 ( tu, 0u ) == 0u ) );
    check ( "zero length covariance", nearly ( statCovariance ( c1, c2, 0u ), 0.0f ) );
    check ( "zero length covariancei32",
            ( uint8_t ) ( statCovariancei32 ( d1, d2, 0u ) == 0 ) );
}

int main ( void )
{
    absoluteCase ( );
    printf ( "\n" );
    minMaxCase ( );
    printf ( "\n" );
    sumMeanRangeCase ( );
    printf ( "\n" );
    medianCase ( );
    printf ( "\n" );
    zeroLengthCase ( );
    printf ( "\n" );
    statisticCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%u check(s) failed\n", ( unsigned ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
