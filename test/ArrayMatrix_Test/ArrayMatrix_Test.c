/*
 * Covers basicarray and basicmatrix.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * Both modules write through a caller owned buffer and return nothing, so what
 * matters is what they leave behind and, just as much, what they leave alone.
 * Every case here puts a guard element on each side of the region the function
 * is allowed to touch.
 */

#include <stdio.h>

#include "basicarray.h"
#include "basicmatrix.h"

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

/* ------------------------------------------------------------ basicarray */

static void arrayCase ( void )
{
    float work[ 6 ] = { -50.0f, -1.0f, 0.0f, 1.0f, 50.0f, 999.0f };
    uint32_t workU[ 6 ] = { 0u, 5u, 10u, 20u, 4000000000u, 7u };
    int32_t workI[ 6 ] = { -2147483647 - 1, -20, 0, 20, 2147483647, 3 };

    printf ( "arrayLimitUpDw1D\n" );

    /* Only the first five elements are in scope; work[5] is the guard. */
    arrayLimitUpDw1D ( work, 10.0f, -10.0f, 5u );

    check ( "a value below the lower limit is pulled up", nearly ( work[ 0 ], -10.0f ) );
    check ( "a value inside the band is left alone", nearly ( work[ 1 ], -1.0f ) );
    check ( "zero is left alone", nearly ( work[ 2 ], 0.0f ) );
    check ( "another value inside the band", nearly ( work[ 3 ], 1.0f ) );
    check ( "a value above the upper limit is clamped", nearly ( work[ 4 ], 10.0f ) );
    check ( "the element past the length is untouched", nearly ( work[ 5 ], 999.0f ) );

    arrayLimitUpDw1Du32 ( workU, 100u, 10u, 5u );
    check ( "u32 below the lower limit", ( uint8_t ) ( workU[ 0 ] == 10u ) );
    check ( "u32 still below the lower limit", ( uint8_t ) ( workU[ 1 ] == 10u ) );
    check ( "u32 exactly at the lower limit stays", ( uint8_t ) ( workU[ 2 ] == 10u ) );
    check ( "u32 inside the band", ( uint8_t ) ( workU[ 3 ] == 20u ) );
    check ( "u32 above the upper limit, past INT32_MAX",
            ( uint8_t ) ( workU[ 4 ] == 100u ) );
    check ( "u32 guard untouched", ( uint8_t ) ( workU[ 5 ] == 7u ) );

    arrayLimitUpDw1Di32 ( workI, 10, -10, 5u );
    check ( "i32 clamps INT32_MIN up", ( uint8_t ) ( workI[ 0 ] == -10 ) );
    check ( "i32 clamps a negative up", ( uint8_t ) ( workI[ 1 ] == -10 ) );
    check ( "i32 leaves zero alone", ( uint8_t ) ( workI[ 2 ] == 0 ) );
    check ( "i32 clamps a positive down", ( uint8_t ) ( workI[ 3 ] == 10 ) );
    check ( "i32 clamps INT32_MAX down", ( uint8_t ) ( workI[ 4 ] == 10 ) );
    check ( "i32 guard untouched", ( uint8_t ) ( workI[ 5 ] == 3 ) );

    /* A zero size must not write anything at all. */
    arrayLimitUpDw1D ( work, 0.0f, 0.0f, 0u );
    check ( "a zero size writes nothing", nearly ( work[ 0 ], -10.0f ) );
}

/* ------------------------------------------------- basicmatrix, 1D form */

static void threshold1DCase ( void )
{
    float work[ 5 ] = { 1.0f, 5.0f, 5.5f, 9.0f, 111.0f };
    uint8_t work8[ 5 ] = { 0u, 5u, 6u, 255u, 42u };

    printf ( "matrixThreshold1D\n" );

    matrixThreshold1D ( work, 5.0f, 100.0f, -100.0f, 4u );

    check ( "below the threshold becomes dwValue", nearly ( work[ 0 ], -100.0f ) );
    check ( "exactly at the threshold also becomes dwValue, the test is strict",
            nearly ( work[ 1 ], -100.0f ) );
    check ( "just above becomes upValue", nearly ( work[ 2 ], 100.0f ) );
    check ( "well above becomes upValue", nearly ( work[ 3 ], 100.0f ) );
    check ( "the guard past the size is untouched", nearly ( work[ 4 ], 111.0f ) );

    matrixThreshold1Du8 ( work8, 5u, 1u, 0u, 4u );
    check ( "u8 below", ( uint8_t ) ( work8[ 0 ] == 0u ) );
    check ( "u8 at the threshold", ( uint8_t ) ( work8[ 1 ] == 0u ) );
    check ( "u8 above", ( uint8_t ) ( work8[ 2 ] == 1u ) );
    check ( "u8 at the top of the range", ( uint8_t ) ( work8[ 3 ] == 1u ) );
    check ( "u8 guard untouched", ( uint8_t ) ( work8[ 4 ] == 42u ) );
}

static void threshold1DIntegerCase ( void )
{
    uint32_t workU[ 5 ] = { 0u, 100u, 101u, 4000000000u, 42u };
    int32_t workI[ 5 ] = { -2147483647 - 1, 0, 1, 2147483647, 42 };

    printf ( "matrixThreshold1D, integer variants\n" );

    matrixThreshold1Du32 ( workU, 100u, 1u, 0u, 4u );
    check ( "u32 well below", ( uint8_t ) ( workU[ 0 ] == 0u ) );
    check ( "u32 exactly at the threshold is not above it",
            ( uint8_t ) ( workU[ 1 ] == 0u ) );
    check ( "u32 one past the threshold", ( uint8_t ) ( workU[ 2 ] == 1u ) );
    check ( "u32 past INT32_MAX still compares as larger",
            ( uint8_t ) ( workU[ 3 ] == 1u ) );
    check ( "u32 guard untouched", ( uint8_t ) ( workU[ 4 ] == 42u ) );

    matrixThreshold1Di32 ( workI, 0, 1, -1, 4u );
    check ( "i32 INT32_MIN is below zero", ( uint8_t ) ( workI[ 0 ] == -1 ) );
    check ( "i32 zero is not above zero", ( uint8_t ) ( workI[ 1 ] == -1 ) );
    check ( "i32 one is above zero", ( uint8_t ) ( workI[ 2 ] == 1 ) );
    check ( "i32 INT32_MAX is above zero", ( uint8_t ) ( workI[ 3 ] == 1 ) );
    check ( "i32 guard untouched", ( uint8_t ) ( workI[ 4 ] == 42 ) );
}

static void threshold2DFloatCase ( void )
{
    float work[ 7 ] = { 1.0f, 9.0f,
                        2.0f, 8.0f,
                        3.0f, 7.0f,
                        555.0f };
    uint8_t work8[ 7 ] = { 1u, 9u,
                           2u, 8u,
                           3u, 7u,
                           99u };

    printf ( "matrixThreshold2D, float and u8\n" );

    matrixThreshold2D ( work, 5.0f, 1.0f, -1.0f, 3u, 2u );
    check ( "float row 0 low", nearly ( work[ 0 ], -1.0f ) );
    check ( "float row 0 high", nearly ( work[ 1 ], 1.0f ) );
    check ( "float row 1 low", nearly ( work[ 2 ], -1.0f ) );
    check ( "float row 1 high", nearly ( work[ 3 ], 1.0f ) );
    check ( "float row 2 low", nearly ( work[ 4 ], -1.0f ) );
    check ( "float row 2 high", nearly ( work[ 5 ], 1.0f ) );
    check ( "float guard untouched", nearly ( work[ 6 ], 555.0f ) );

    matrixThreshold2Du8 ( work8, 5u, 255u, 0u, 3u, 2u );
    check ( "u8 row 0 low", ( uint8_t ) ( work8[ 0 ] == 0u ) );
    check ( "u8 row 0 high", ( uint8_t ) ( work8[ 1 ] == 255u ) );
    check ( "u8 row 1 low", ( uint8_t ) ( work8[ 2 ] == 0u ) );
    check ( "u8 row 1 high", ( uint8_t ) ( work8[ 3 ] == 255u ) );
    check ( "u8 row 2 low", ( uint8_t ) ( work8[ 4 ] == 0u ) );
    check ( "u8 row 2 high", ( uint8_t ) ( work8[ 5 ] == 255u ) );
    check ( "u8 guard untouched", ( uint8_t ) ( work8[ 6 ] == 99u ) );
}

/* ------------------------------------------------- basicmatrix, 2D form */

static void threshold2DCase ( void )
{
    /*
     * Three rows of two columns held in a flat buffer, plus a guard. The row
     * stride is jSize, so a function that walked the buffer with the wrong
     * stride would leave the guard or the last cell wrong.
     */
    uint32_t work[ 7 ] = { 1u, 9u,
                           2u, 8u,
                           3u, 7u,
                           123456u };
    int32_t workI[ 7 ] = { -5, 5,
                           -1, 1,
                            0, 100,
                            777 };

    printf ( "matrixThreshold2D\n" );

    matrixThreshold2Du32 ( work, 5u, 1000u, 0u, 3u, 2u );

    check ( "row 0 column 0 is below", ( uint8_t ) ( work[ 0 ] == 0u ) );
    check ( "row 0 column 1 is above", ( uint8_t ) ( work[ 1 ] == 1000u ) );
    check ( "row 1 column 0 is below", ( uint8_t ) ( work[ 2 ] == 0u ) );
    check ( "row 1 column 1 is above", ( uint8_t ) ( work[ 3 ] == 1000u ) );
    check ( "row 2 column 0 is below", ( uint8_t ) ( work[ 4 ] == 0u ) );
    check ( "row 2 column 1 is above", ( uint8_t ) ( work[ 5 ] == 1000u ) );
    check ( "the cell past the last row is untouched",
            ( uint8_t ) ( work[ 6 ] == 123456u ) );

    matrixThreshold2Di32 ( workI, 0, 1, -1, 3u, 2u );
    check ( "i32 negative is at or below zero", ( uint8_t ) ( workI[ 0 ] == -1 ) );
    check ( "i32 positive is above zero", ( uint8_t ) ( workI[ 1 ] == 1 ) );
    check ( "i32 minus one", ( uint8_t ) ( workI[ 2 ] == -1 ) );
    check ( "i32 plus one", ( uint8_t ) ( workI[ 3 ] == 1 ) );
    check ( "i32 zero itself is not above zero", ( uint8_t ) ( workI[ 4 ] == -1 ) );
    check ( "i32 large positive", ( uint8_t ) ( workI[ 5 ] == 1 ) );
    check ( "i32 guard untouched", ( uint8_t ) ( workI[ 6 ] == 777 ) );
}

/* ------------------------------------------- basicmatrix, the 2D clamps */

static void limit2DCase ( void )
{
    float work[ 7 ] = { -99.0f, 0.0f,
                          2.0f, 99.0f,
                         -3.0f, 3.0f,
                        555.0f };
    uint32_t workU[ 5 ] = { 0u, 50u,
                            500u, 5000u,
                            31337u };
    int32_t workI[ 5 ] = { -2147483647 - 1, -5,
                            5, 2147483647,
                            31337 };

    printf ( "matrixLimitUpDw2D\n" );

    matrixLimitUpDw2D ( work, 5.0f, -5.0f, 3u, 2u );

    check ( "clamped up to the lower limit", nearly ( work[ 0 ], -5.0f ) );
    check ( "inside the band", nearly ( work[ 1 ], 0.0f ) );
    check ( "still inside", nearly ( work[ 2 ], 2.0f ) );
    check ( "clamped down to the upper limit", nearly ( work[ 3 ], 5.0f ) );
    check ( "inside on the low side", nearly ( work[ 4 ], -3.0f ) );
    check ( "inside on the high side", nearly ( work[ 5 ], 3.0f ) );
    check ( "the guard is untouched", nearly ( work[ 6 ], 555.0f ) );

    matrixLimitUpDw2Du32 ( workU, 1000u, 100u, 2u, 2u );
    check ( "u32 clamped up", ( uint8_t ) ( workU[ 0 ] == 100u ) );
    check ( "u32 clamped up again", ( uint8_t ) ( workU[ 1 ] == 100u ) );
    check ( "u32 left alone", ( uint8_t ) ( workU[ 2 ] == 500u ) );
    check ( "u32 clamped down", ( uint8_t ) ( workU[ 3 ] == 1000u ) );
    check ( "u32 guard untouched", ( uint8_t ) ( workU[ 4 ] == 31337u ) );

    matrixLimitUpDw2Di32 ( workI, 10, -10, 2u, 2u );
    check ( "i32 INT32_MIN clamped up", ( uint8_t ) ( workI[ 0 ] == -10 ) );
    check ( "i32 inside on the low side", ( uint8_t ) ( workI[ 1 ] == -5 ) );
    check ( "i32 inside on the high side", ( uint8_t ) ( workI[ 2 ] == 5 ) );
    check ( "i32 INT32_MAX clamped down", ( uint8_t ) ( workI[ 3 ] == 10 ) );
    check ( "i32 guard untouched", ( uint8_t ) ( workI[ 4 ] == 31337 ) );

    /* A zero row or column count must write nothing. */
    matrixLimitUpDw2D ( work, 0.0f, 0.0f, 0u, 2u );
    check ( "zero rows writes nothing", nearly ( work[ 0 ], -5.0f ) );

    matrixLimitUpDw2D ( work, 0.0f, 0.0f, 3u, 0u );
    check ( "zero columns writes nothing", nearly ( work[ 0 ], -5.0f ) );
}

int main ( void )
{
    arrayCase ( );
    printf ( "\n" );
    threshold1DCase ( );
    printf ( "\n" );
    threshold1DIntegerCase ( );
    printf ( "\n" );
    threshold2DFloatCase ( );
    printf ( "\n" );
    threshold2DCase ( );
    printf ( "\n" );
    limit2DCase ( );

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
