/**
  ******************************************************************************
  *
  * @file      matrixlib.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      23/06/2022
  *
  * @brief     Linear algebra on a matrix that knows its own shape.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 23/06/2022 Created. @n
  * 15/09/2026 Implemented. The file had held a banner and nothing @n
  *            else since 2022, and mtrx_t carried a float pointer @n
  *            with no dimensions, which no operation could use. @n
  *
  * @note      basicmatrix works on a bare array the caller describes at every
  *            call, and does thresholding and limiting — element by element
  *            work where the shape barely matters. This is the other half: a
  *            matrix that carries its rows and columns, and the operations
  *            where the shape is the whole point. Multiply, transpose and
  *            invert are what a coordinate transform or a Kalman update is
  *            made of, and they are the three an embedded project gets wrong
  *            when it writes them inline against a flat array.
  *
  * @note      Row major, so element ( r, c ) lives at index r * cols + c. The
  *            caller owns the storage and passes it to matrixInit; nothing
  *            here allocates, and the struct is a description of somebody
  *            else's array rather than a container.
  *
  * @note      **Every operation returns a status**, which is unusual in this
  *            library and is not a departure from the Init contract. Those
  *            functions take *other matrices* as new arguments, and whether
  *            their shapes agree cannot be known when any one of them was
  *            initialized. pidChangeCoefficients returns a status for the same
  *            reason. A shape mismatch leaves the result untouched.
  *
  * @note      Aliasing is allowed where it is safe and refused where it is
  *            not. matrixAdd, matrixSub and matrixScale walk their operands
  *            once in step, so a result that is also an input is fine and
  *            useful. matrixMul and matrixTranspose read elements after the
  *            position they are writing, so a result aliasing an input would
  *            quietly compute against values it had already overwritten; both
  *            compare the data pointers and refuse. That check is cheap and
  *            the failure it prevents is silent.
  *
  * @note      matrixInverse takes a scratch matrix rather than allocating one,
  *            which is the same bargain every other module here makes. It runs
  *            Gauss-Jordan with partial pivoting and refuses a matrix whose
  *            pivot is exactly zero after the best row has been chosen. It
  *            does not invent an epsilon for "nearly singular": how close to
  *            singular is too close depends on what the numbers mean, which is
  *            the caller's to know and not this module's to guess.
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "matrixlib.h"

/**
 * @brief   Initializes a matrix over caller owned storage.
 * @param[out] driver  Matrix to initialize.
 * @param[in]  data    Row major storage of at least rows times cols floats.
 * @param[in]  rows    Number of rows.
 * @param[in]  cols    Number of columns.
 * @return  TRUE on success, FALSE when driver or data is NULL, or when either
 *          dimension is zero.
 * @note    The storage is not cleared. matrixZero does that, and a caller
 *          filling every element itself should not pay for a pass that writes
 *          values it is about to replace.
 */
uint8_t matrixInit ( mtrx_t* driver, float* data, uint32_t rows, uint32_t cols )
{
    uint8_t retVal = FALSE;

    if ( ( driver != NULL ) && ( data != NULL ) && ( rows != 0 ) &&
            ( cols != 0 ) )
    {
        driver->data = data;
        driver->rows = rows;
        driver->cols = cols;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Sets every element to zero.
 * @param[in,out] driver  Initialized matrix.
 */
void matrixZero ( mtrx_t* driver )
{
    uint32_t i = 0;
    uint32_t total = 0;

    total = driver->rows * driver->cols;

    for ( i = 0; i < total; ++i )
    {
        driver->data[ i ] = 0.0f;
    }
}

/**
 * @brief   Makes a square matrix the identity.
 * @param[in,out] driver  Initialized matrix.
 * @return  TRUE on success, FALSE when the matrix is not square.
 * @note    A non square identity does not exist, so this refuses rather than
 *          writing ones down a diagonal that runs off the edge.
 */
uint8_t matrixIdentity ( mtrx_t* driver )
{
    uint8_t retVal = FALSE;
    uint32_t r = 0;
    uint32_t c = 0;

    if ( driver->rows == driver->cols )
    {
        for ( r = 0; r < driver->rows; ++r )
        {
            for ( c = 0; c < driver->cols; ++c )
            {
                if ( r == c )
                {
                    driver->data[ ( r * driver->cols ) + c ] = 1.0f;
                }
                else
                {
                    driver->data[ ( r * driver->cols ) + c ] = 0.0f;
                }
            }
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Reads one element.
 * @param[in] driver  Initialized matrix.
 * @param[in] row     Row index, from zero.
 * @param[in] col     Column index, from zero.
 * @return  The element, or zero when the indices are outside the matrix.
 * @note    Out of range reads zero rather than reading past the array. There
 *          is no status to return from a function whose whole job is to hand
 *          back a value, and zero is the answer that keeps a sum or a product
 *          from turning into a nan. matrixSet, which can report, does.
 */
float matrixGet ( const mtrx_t* const driver, uint32_t row, uint32_t col )
{
    float retVal = 0.0f;

    if ( ( row < driver->rows ) && ( col < driver->cols ) )
    {
        retVal = driver->data[ ( row * driver->cols ) + col ];
    }
    else
    {
        retVal = 0.0f;
    }

    return ( retVal );
}

/**
 * @brief   Writes one element.
 * @param[in,out] driver  Initialized matrix.
 * @param[in]     row     Row index, from zero.
 * @param[in]     col     Column index, from zero.
 * @param[in]     value   Value to store.
 * @return  TRUE on success, FALSE when the indices are outside the matrix.
 */
uint8_t matrixSet ( mtrx_t* driver, uint32_t row, uint32_t col, float value )
{
    uint8_t retVal = FALSE;

    if ( ( row < driver->rows ) && ( col < driver->cols ) )
    {
        driver->data[ ( row * driver->cols ) + col ] = value;

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Adds two matrices of the same shape.
 * @param[in]  a       First operand.
 * @param[in]  b       Second operand.
 * @param[out] result  Destination, which may be either operand.
 * @return  TRUE on success, FALSE when the three shapes do not agree.
 * @note    In place is allowed. Each element is read and written once at the
 *          same position, so a result that is also an operand reads what it
 *          needs before it writes over it.
 */
uint8_t matrixAdd ( const mtrx_t* const a, const mtrx_t* const b, mtrx_t* result )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t total = 0;

    if ( ( a->rows == b->rows ) && ( a->cols == b->cols ) &&
            ( a->rows == result->rows ) && ( a->cols == result->cols ) )
    {
        total = a->rows * a->cols;

        for ( i = 0; i < total; ++i )
        {
            result->data[ i ] = a->data[ i ] + b->data[ i ];
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Subtracts the second matrix from the first.
 * @param[in]  a       Matrix to subtract from.
 * @param[in]  b       Matrix to subtract.
 * @param[out] result  Destination, which may be either operand.
 * @return  TRUE on success, FALSE when the three shapes do not agree.
 */
uint8_t matrixSub ( const mtrx_t* const a, const mtrx_t* const b, mtrx_t* result )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t total = 0;

    if ( ( a->rows == b->rows ) && ( a->cols == b->cols ) &&
            ( a->rows == result->rows ) && ( a->cols == result->cols ) )
    {
        total = a->rows * a->cols;

        for ( i = 0; i < total; ++i )
        {
            result->data[ i ] = a->data[ i ] - b->data[ i ];
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Multiplies every element by a scalar.
 * @param[in]  a       Matrix to scale.
 * @param[in]  scalar  Factor.
 * @param[out] result  Destination, which may be the operand.
 * @return  TRUE on success, FALSE when the shapes do not agree.
 */
uint8_t matrixScale ( const mtrx_t* const a, float scalar, mtrx_t* result )
{
    uint8_t retVal = FALSE;
    uint32_t i = 0;
    uint32_t total = 0;

    if ( ( a->rows == result->rows ) && ( a->cols == result->cols ) )
    {
        total = a->rows * a->cols;

        for ( i = 0; i < total; ++i )
        {
            result->data[ i ] = a->data[ i ] * scalar;
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Multiplies two matrices.
 * @param[in]  a       Left operand, rows by inner.
 * @param[in]  b       Right operand, inner by cols.
 * @param[out] result  Destination, rows by cols. Must not be either operand.
 * @return  TRUE on success, FALSE when the inner dimensions do not match, when
 *          the result is the wrong shape, or when the result aliases an
 *          operand.
 * @note    The alias check is not fussiness. Every element of the result is a
 *          sum over a whole row and column, so writing one destroys values the
 *          next element still needs; a result that shared storage with an
 *          operand would compute against numbers it had already overwritten
 *          and give an answer that is wrong without being obviously wrong.
 */
uint8_t matrixMul ( const mtrx_t* const a, const mtrx_t* const b, mtrx_t* result )
{
    uint8_t retVal = FALSE;
    uint32_t r = 0;
    uint32_t c = 0;
    uint32_t k = 0;
    float sum = 0.0f;

    if ( ( a->cols == b->rows ) && ( result->rows == a->rows ) &&
            ( result->cols == b->cols ) && ( result->data != a->data ) &&
            ( result->data != b->data ) )
    {
        for ( r = 0; r < a->rows; ++r )
        {
            for ( c = 0; c < b->cols; ++c )
            {
                sum = 0.0f;

                for ( k = 0; k < a->cols; ++k )
                {
                    sum += a->data[ ( r * a->cols ) + k ] *
                            b->data[ ( k * b->cols ) + c ];
                }

                result->data[ ( r * result->cols ) + c ] = sum;
            }
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Transposes a matrix.
 * @param[in]  a       Matrix to transpose, rows by cols.
 * @param[out] result  Destination, cols by rows. Must not be the operand.
 * @return  TRUE on success, FALSE when the result is the wrong shape or
 *          aliases the operand.
 * @note    Refused in place even for a square matrix. An in place transpose is
 *          a different algorithm — a walk of swap cycles rather than a copy —
 *          and quietly accepting the pointer here would run the copy version
 *          over itself.
 */
uint8_t matrixTranspose ( const mtrx_t* const a, mtrx_t* result )
{
    uint8_t retVal = FALSE;
    uint32_t r = 0;
    uint32_t c = 0;

    if ( ( result->rows == a->cols ) && ( result->cols == a->rows ) &&
            ( result->data != a->data ) )
    {
        for ( r = 0; r < a->rows; ++r )
        {
            for ( c = 0; c < a->cols; ++c )
            {
                result->data[ ( c * result->cols ) + r ] =
                        a->data[ ( r * a->cols ) + c ];
            }
        }

        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Inverts a square matrix.
 * @param[in]     a        Matrix to invert.
 * @param[out]    result   Destination, the same shape as a.
 * @param[in,out] scratch  Working matrix of the same shape, contents
 *                         destroyed.
 * @return  TRUE on success, FALSE when the matrix is not square, when the
 *          shapes do not agree, when any two of the three share storage, or
 *          when the matrix is singular.
 * @note    Gauss-Jordan with partial pivoting, run on the scratch copy while
 *          the same row operations are applied to the result, which starts as
 *          the identity. The scratch is a parameter rather than something this
 *          function allocates, which is the bargain every module in this
 *          library makes.
 * @note    Singular means a pivot of exactly zero once the largest available
 *          row has been swapped in. There is no epsilon for "nearly
 *          singular", on purpose: how close to singular is too close depends
 *          on the units and the conditioning of the problem, which the caller
 *          knows and this module does not. A matrix that is nearly singular
 *          returns TRUE with a large result, which is the honest answer.
 * @note    On FALSE the result and the scratch may both have been written.
 *          That is the one place in this library where a failed call leaves
 *          something changed, and it is unavoidable: singularity is not
 *          visible until the elimination reaches the row that shows it.
 */
uint8_t matrixInverse ( const mtrx_t* const a, mtrx_t* result, mtrx_t* scratch )
{
    uint8_t retVal = FALSE;
    uint32_t n = 0;
    uint32_t i = 0;
    uint32_t r = 0;
    uint32_t c = 0;
    uint32_t pivotRow = 0;
    float pivot = 0.0f;
    float magnitude = 0.0f;
    float best = 0.0f;
    float factor = 0.0f;
    float swap = 0.0f;

    if ( ( a->rows == a->cols ) && ( result->rows == a->rows ) &&
            ( result->cols == a->cols ) && ( scratch->rows == a->rows ) &&
            ( scratch->cols == a->cols ) && ( result->data != a->data ) &&
            ( scratch->data != a->data ) && ( result->data != scratch->data ) )
    {
        n = a->rows;

        for ( i = 0; i < ( n * n ); ++i )
        {
            scratch->data[ i ] = a->data[ i ];
        }

        ( void ) matrixIdentity ( result );

        retVal = TRUE;

        for ( c = 0; ( c < n ) && ( retVal == TRUE ); ++c )
        {
            /* Partial pivoting: the largest magnitude in this column, at or
               below the diagonal, becomes the pivot row. */
            pivotRow = c;
            best = 0.0f;

            for ( r = c; r < n; ++r )
            {
                magnitude = scratch->data[ ( r * n ) + c ];

                if ( magnitude < 0.0f )
                {
                    magnitude = -magnitude;
                }
                else
                {
                    /* Intentionally blank */
                }

                if ( magnitude > best )
                {
                    best = magnitude;
                    pivotRow = r;
                }
                else
                {
                    /* Intentionally blank */
                }
            }

            if ( best == 0.0f )
            {
                retVal = FALSE;
            }
            else
            {
                if ( pivotRow != c )
                {
                    for ( i = 0; i < n; ++i )
                    {
                        swap = scratch->data[ ( c * n ) + i ];
                        scratch->data[ ( c * n ) + i ] = scratch->data[ ( pivotRow * n ) + i ];
                        scratch->data[ ( pivotRow * n ) + i ] = swap;

                        swap = result->data[ ( c * n ) + i ];
                        result->data[ ( c * n ) + i ] = result->data[ ( pivotRow * n ) + i ];
                        result->data[ ( pivotRow * n ) + i ] = swap;
                    }
                }
                else
                {
                    /* Intentionally blank */
                }

                pivot = scratch->data[ ( c * n ) + c ];

                for ( i = 0; i < n; ++i )
                {
                    scratch->data[ ( c * n ) + i ] /= pivot;
                    result->data[ ( c * n ) + i ] /= pivot;
                }

                for ( r = 0; r < n; ++r )
                {
                    if ( r != c )
                    {
                        factor = scratch->data[ ( r * n ) + c ];

                        for ( i = 0; i < n; ++i )
                        {
                            scratch->data[ ( r * n ) + i ] -=
                                    factor * scratch->data[ ( c * n ) + i ];
                            result->data[ ( r * n ) + i ] -=
                                    factor * result->data[ ( c * n ) + i ];
                        }
                    }
                    else
                    {
                        /* Intentionally blank */
                    }
                }
            }
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
