/*
 * Covers matrixlib.
 *
 * Asserts rather than printing values for a human to compare, so it needs no
 * output.txt and returns non zero on failure.
 *
 * The inverse is checked the way ComplexMath_Test checks division: against the
 * answer where the answer is short enough to write down, and by multiplying
 * the result back into the original and requiring the identity where it is
 * not. The second form is the stronger of the two, because it cannot agree
 * with a wrong implementation by construction.
 */

#include <stddef.h>
#include <stdio.h>

#include "matrixlib.h"

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

static uint8_t near ( float got, float want )
{
    uint8_t retVal = FALSE;
    float diff = 0;

    diff = got - want;

    if ( diff < 0 )
    {
        diff = -diff;
    }
    else
    {
        /* Intentionally blank. */
    }

    if ( diff < 0.0005f )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/* Every element of a square matrix against the identity. */
static uint8_t isIdentity ( const mtrx_t* const m )
{
    uint8_t retVal = TRUE;
    uint32_t r = 0;
    uint32_t c = 0;
    float want = 0.0f;

    for ( r = 0; r < m->rows; ++r )
    {
        for ( c = 0; c < m->cols; ++c )
        {
            if ( r == c )
            {
                want = 1.0f;
            }
            else
            {
                want = 0.0f;
            }

            if ( near ( matrixGet ( m, r, c ), want ) == FALSE )
            {
                retVal = FALSE;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
    }

    return ( retVal );
}

/* ---------------------------------------------------------------- init */

static void initCase ( void )
{
    mtrx_t m;
    float data[ 6 ];

    printf ( "matrixInit\n" );

    check ( "a NULL driver is rejected",
            ( uint8_t ) ( matrixInit ( NULL, data, 2u, 3u ) == FALSE ) );
    check ( "a NULL data pointer is rejected",
            ( uint8_t ) ( matrixInit ( &m, NULL, 2u, 3u ) == FALSE ) );
    check ( "zero rows are rejected",
            ( uint8_t ) ( matrixInit ( &m, data, 0u, 3u ) == FALSE ) );
    check ( "zero columns are rejected",
            ( uint8_t ) ( matrixInit ( &m, data, 2u, 0u ) == FALSE ) );
    check ( "a full init succeeds", matrixInit ( &m, data, 2u, 3u ) );

    matrixZero ( &m );
    check ( "zero clears every element",
            ( uint8_t ) ( near ( matrixGet ( &m, 0u, 0u ), 0.0f ) &&
                          near ( matrixGet ( &m, 1u, 2u ), 0.0f ) ) );

    /* Row major: element ( 1, 2 ) is index 1 * 3 + 2, which is five. */
    check ( "set", matrixSet ( &m, 1u, 2u, 42.0f ) );
    check ( "and get reads it back",
            near ( matrixGet ( &m, 1u, 2u ), 42.0f ) );
    check ( "at the row major position the caller can predict",
            near ( data[ 5 ], 42.0f ) );

    check ( "a row past the end is refused",
            ( uint8_t ) ( matrixSet ( &m, 2u, 0u, 1.0f ) == FALSE ) );
    check ( "a column past the end is refused",
            ( uint8_t ) ( matrixSet ( &m, 0u, 3u, 1.0f ) == FALSE ) );

    /*
     * Get has nothing to report with, so it answers zero rather than reading
     * past the array. A sum or a product built on it stays a number.
     */
    check ( "an out of range read is zero rather than whatever is there",
            near ( matrixGet ( &m, 9u, 9u ), 0.0f ) );

    /* A non square matrix has no identity, so this refuses instead of trying. */
    check ( "a non square identity is refused",
            ( uint8_t ) ( matrixIdentity ( &m ) == FALSE ) );

    check ( "a square init", matrixInit ( &m, data, 2u, 2u ) );
    check ( "and its identity", matrixIdentity ( &m ) );
    check ( "is the identity", isIdentity ( &m ) );
}

/* ------------------------------------------------------- element wise */

static void elementCase ( void )
{
    mtrx_t a;
    mtrx_t b;
    mtrx_t r;
    mtrx_t wrong;
    float dataA[ 4 ] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float dataB[ 4 ] = { 10.0f, 20.0f, 30.0f, 40.0f };
    float dataR[ 4 ];
    float dataW[ 6 ];

    printf ( "add, sub and scale\n" );

    check ( "init a", matrixInit ( &a, dataA, 2u, 2u ) );
    check ( "init b", matrixInit ( &b, dataB, 2u, 2u ) );
    check ( "init r", matrixInit ( &r, dataR, 2u, 2u ) );
    check ( "init a mismatched one", matrixInit ( &wrong, dataW, 2u, 3u ) );

    check ( "add", matrixAdd ( &a, &b, &r ) );
    check ( "sums element by element",
            ( uint8_t ) ( near ( matrixGet ( &r, 0u, 0u ), 11.0f ) &&
                          near ( matrixGet ( &r, 1u, 1u ), 44.0f ) ) );

    check ( "sub", matrixSub ( &b, &a, &r ) );
    check ( "subtracts element by element",
            ( uint8_t ) ( near ( matrixGet ( &r, 0u, 0u ), 9.0f ) &&
                          near ( matrixGet ( &r, 1u, 1u ), 36.0f ) ) );

    check ( "scale", matrixScale ( &a, 2.5f, &r ) );
    check ( "multiplies every element",
            ( uint8_t ) ( near ( matrixGet ( &r, 0u, 0u ), 2.5f ) &&
                          near ( matrixGet ( &r, 1u, 1u ), 10.0f ) ) );

    /*
     * In place is allowed for these three: every element is read and written
     * at the same position, so a result that is also an operand reads what it
     * needs before overwriting it.
     */
    check ( "add in place", matrixAdd ( &a, &b, &a ) );
    check ( "and the operand now holds the sum",
            ( uint8_t ) ( near ( matrixGet ( &a, 0u, 0u ), 11.0f ) &&
                          near ( matrixGet ( &a, 1u, 1u ), 44.0f ) ) );

    check ( "sub in place", matrixSub ( &a, &b, &a ) );
    check ( "and it is back where it started",
            ( uint8_t ) ( near ( matrixGet ( &a, 0u, 0u ), 1.0f ) &&
                          near ( matrixGet ( &a, 1u, 1u ), 4.0f ) ) );

    /* Shapes that do not agree are refused, and the result is left alone. */
    check ( "adding mismatched shapes is refused",
            ( uint8_t ) ( matrixAdd ( &a, &wrong, &r ) == FALSE ) );
    check ( "a mismatched destination too",
            ( uint8_t ) ( matrixAdd ( &a, &b, &wrong ) == FALSE ) );
    check ( "and subtracting",
            ( uint8_t ) ( matrixSub ( &a, &wrong, &r ) == FALSE ) );
    check ( "and scaling into the wrong shape",
            ( uint8_t ) ( matrixScale ( &a, 2.0f, &wrong ) == FALSE ) );
    check ( "the refused result still holds what scale last wrote",
            near ( matrixGet ( &r, 0u, 0u ), 2.5f ) );
}

/* ------------------------------------------------------------ multiply */

static void multiplyCase ( void )
{
    mtrx_t a;
    mtrx_t b;
    mtrx_t r;
    mtrx_t aliased;
    float dataA[ 6 ] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    float dataB[ 6 ] = { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f };
    float dataR[ 4 ];

    printf ( "matrixMul and matrixTranspose\n" );

    /* A is 2 by 3, B is 3 by 2, so the product is 2 by 2. */
    check ( "init a", matrixInit ( &a, dataA, 2u, 3u ) );
    check ( "init b", matrixInit ( &b, dataB, 3u, 2u ) );
    check ( "init r", matrixInit ( &r, dataR, 2u, 2u ) );

    check ( "multiply", matrixMul ( &a, &b, &r ) );
    check ( "the top left is the first row against the first column",
            near ( matrixGet ( &r, 0u, 0u ), 58.0f ) );
    check ( "the top right", near ( matrixGet ( &r, 0u, 1u ), 64.0f ) );
    check ( "the bottom left", near ( matrixGet ( &r, 1u, 0u ), 139.0f ) );
    check ( "the bottom right", near ( matrixGet ( &r, 1u, 1u ), 154.0f ) );

    /* The inner dimensions have to match, and the result has to be the shape
       they produce. */
    check ( "multiplying with the inner dimensions swapped is refused",
            ( uint8_t ) ( matrixMul ( &b, &a, &r ) == FALSE ) );

    /*
     * A result that aliases an operand is refused rather than quietly
     * computing against values it has already overwritten. Each element of the
     * product is a sum over a whole row and column, so there is no order that
     * makes it safe.
     */
    check ( "a view aliasing a", matrixInit ( &aliased, dataA, 2u, 2u ) );
    check ( "a result that aliases the left operand is refused",
            ( uint8_t ) ( matrixMul ( &a, &b, &aliased ) == FALSE ) );

    check ( "a view aliasing b", matrixInit ( &aliased, dataB, 2u, 2u ) );
    check ( "and one that aliases the right operand",
            ( uint8_t ) ( matrixMul ( &a, &b, &aliased ) == FALSE ) );

    /* Transpose: 2 by 3 becomes 3 by 2. */
    check ( "init a transpose destination", matrixInit ( &r, dataR, 2u, 2u ) );
    check ( "transposing into the wrong shape is refused",
            ( uint8_t ) ( matrixTranspose ( &a, &r ) == FALSE ) );

    {
        mtrx_t t;
        float dataT[ 6 ];

        check ( "init a 3 by 2 destination", matrixInit ( &t, dataT, 3u, 2u ) );
        check ( "transpose", matrixTranspose ( &a, &t ) );
        check ( "rows become columns",
                ( uint8_t ) ( near ( matrixGet ( &t, 0u, 0u ), 1.0f ) &&
                              near ( matrixGet ( &t, 0u, 1u ), 4.0f ) &&
                              near ( matrixGet ( &t, 2u, 0u ), 3.0f ) &&
                              near ( matrixGet ( &t, 2u, 1u ), 6.0f ) ) );

        /*
         * Refused in place even for a square matrix: an in place transpose is
         * a walk of swap cycles, not the copy this function runs.
         */
        check ( "a square view of a", matrixInit ( &aliased, dataA, 2u, 2u ) );
        check ( "transposing onto itself is refused",
                ( uint8_t ) ( matrixTranspose ( &aliased, &aliased ) == FALSE ) );
    }
}

/* ------------------------------------------------------------- inverse */

static void inverseCase ( void )
{
    mtrx_t a;
    mtrx_t inv;
    mtrx_t scratch;
    mtrx_t product;
    float dataA[ 9 ];
    float dataI[ 9 ];
    float dataS[ 9 ];
    float dataP[ 9 ];

    printf ( "matrixInverse\n" );

    check ( "init a", matrixInit ( &a, dataA, 2u, 2u ) );
    check ( "init inv", matrixInit ( &inv, dataI, 2u, 2u ) );
    check ( "init scratch", matrixInit ( &scratch, dataS, 2u, 2u ) );
    check ( "init product", matrixInit ( &product, dataP, 2u, 2u ) );

    /* [ 1 2 ; 3 4 ] has determinant minus two and a short enough inverse to
       write down: [ -2 1 ; 1.5 -0.5 ]. */
    ( void ) matrixSet ( &a, 0u, 0u, 1.0f );
    ( void ) matrixSet ( &a, 0u, 1u, 2.0f );
    ( void ) matrixSet ( &a, 1u, 0u, 3.0f );
    ( void ) matrixSet ( &a, 1u, 1u, 4.0f );

    check ( "invert", matrixInverse ( &a, &inv, &scratch ) );
    check ( "the top left", near ( matrixGet ( &inv, 0u, 0u ), -2.0f ) );
    check ( "the top right", near ( matrixGet ( &inv, 0u, 1u ), 1.0f ) );
    check ( "the bottom left", near ( matrixGet ( &inv, 1u, 0u ), 1.5f ) );
    check ( "the bottom right", near ( matrixGet ( &inv, 1u, 1u ), -0.5f ) );

    check ( "and the product with the original", matrixMul ( &a, &inv, &product ) );
    check ( "is the identity", isIdentity ( &product ) );

    /*
     * A three by three with a zero in the top left. Without partial pivoting
     * the first division is by zero and everything after it is a nan; with it
     * the row below is swapped in first. The answer is checked by multiplying
     * back rather than by writing nine numbers down.
     */
    check ( "init a 3 by 3", matrixInit ( &a, dataA, 3u, 3u ) );
    check ( "init a 3 by 3 inv", matrixInit ( &inv, dataI, 3u, 3u ) );
    check ( "init a 3 by 3 scratch", matrixInit ( &scratch, dataS, 3u, 3u ) );
    check ( "init a 3 by 3 product", matrixInit ( &product, dataP, 3u, 3u ) );

    ( void ) matrixSet ( &a, 0u, 0u, 0.0f );
    ( void ) matrixSet ( &a, 0u, 1u, 1.0f );
    ( void ) matrixSet ( &a, 0u, 2u, 2.0f );
    ( void ) matrixSet ( &a, 1u, 0u, 1.0f );
    ( void ) matrixSet ( &a, 1u, 1u, 0.0f );
    ( void ) matrixSet ( &a, 1u, 2u, 3.0f );
    ( void ) matrixSet ( &a, 2u, 0u, 4.0f );
    ( void ) matrixSet ( &a, 2u, 1u, -3.0f );
    ( void ) matrixSet ( &a, 2u, 2u, 8.0f );

    check ( "inverting a matrix with a zero pivot on the diagonal",
            matrixInverse ( &a, &inv, &scratch ) );
    check ( "multiplying it back", matrixMul ( &a, &inv, &product ) );
    check ( "gives the identity, so the row swap happened",
            isIdentity ( &product ) );

    check ( "and the other way round too",
            ( uint8_t ) ( matrixMul ( &inv, &a, &product ) &&
                          isIdentity ( &product ) ) );

    /* A singular matrix is refused rather than answered with infinities. */
    ( void ) matrixSet ( &a, 0u, 0u, 1.0f );
    ( void ) matrixSet ( &a, 0u, 1u, 2.0f );
    ( void ) matrixSet ( &a, 0u, 2u, 3.0f );
    ( void ) matrixSet ( &a, 1u, 0u, 2.0f );
    ( void ) matrixSet ( &a, 1u, 1u, 4.0f );
    ( void ) matrixSet ( &a, 1u, 2u, 6.0f );
    ( void ) matrixSet ( &a, 2u, 0u, 1.0f );
    ( void ) matrixSet ( &a, 2u, 1u, 1.0f );
    ( void ) matrixSet ( &a, 2u, 2u, 1.0f );

    check ( "a matrix whose second row is twice its first is singular",
            ( uint8_t ) ( matrixInverse ( &a, &inv, &scratch ) == FALSE ) );

    /* Shape and aliasing are refused the same way as everywhere else. */
    {
        mtrx_t oblong;
        float dataO[ 6 ];

        check ( "init a non square", matrixInit ( &oblong, dataO, 2u, 3u ) );
        check ( "inverting a non square matrix is refused",
                ( uint8_t ) ( matrixInverse ( &oblong, &inv, &scratch ) == FALSE ) );
    }

    check ( "a result that aliases the input is refused",
            ( uint8_t ) ( matrixInverse ( &a, &a, &scratch ) == FALSE ) );
    check ( "a scratch that aliases the input is refused",
            ( uint8_t ) ( matrixInverse ( &a, &inv, &a ) == FALSE ) );
    check ( "and a scratch that aliases the result",
            ( uint8_t ) ( matrixInverse ( &a, &inv, &inv ) == FALSE ) );
}

int main ( void )
{
    initCase ( );
    printf ( "\n" );
    elementCase ( );
    printf ( "\n" );
    multiplyCase ( );
    printf ( "\n" );
    inverseCase ( );

    printf ( "\n" );

    if ( failures == 0 )
    {
        printf ( "all checks passed\n" );
    }
    else
    {
        printf ( "%lu check(s) failed\n", ( unsigned long ) failures );
    }

    return ( ( failures == 0 ) ? 0 : 1 );
}
