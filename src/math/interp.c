/**
  ******************************************************************************
  *
  * @file      interp.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
  * @date      05/08/2026
  *
  * @brief     Linear interpolation over an ascending table.
  *
  * @par Device
  * Generic
  *
  * @note      The bracketing binary search is this module's own code and
  *            duplicates the shape of searchUpperBound on purpose. Module
  *            independence forbids including search.h, and that independence
  *            is what lets a single module be copied out on its own. Do not
  *            "fix" this into a cross module include.
  *
  * @par History
  * 05/08/2026 Created @n
  * 05/08/2026 Float compute path: interpCalculate and interpInRange. @n
  *
  ******************************************************************************
  */

#include <stddef.h>

#include "interp.h"

// Two points are the fewest that span an interval to interpolate across.
#define INTERP_MIN_LENGTH   2

/**
 * @brief   Initializes an interpolator over a caller owned table.
 * @param[out] driver   Interpolator state to initialize.
 * @param[in]  xTable   Caller owned, strictly ascending x values.
 * @param[in]  yTable   Caller owned y values, one per x. Free to ascend,
 *                      descend or do neither.
 * @param[in]  length   Number of entries in each table.
 * @return  TRUE on success, FALSE when a pointer is NULL, length is below
 *          two, or xTable does not ascend strictly.
 * @note    Neither table is copied. Both must outlive the driver. They are
 *          held as pointers to const, so a table in flash needs no cast.
 * @note    The ascending check is why this module is a driver rather than a
 *          set of stateless functions. It is O(N) and is paid once here, so
 *          interpCalculate can divide without testing the divisor. A
 *          repeated x would make that divisor zero.
 */
uint8_t interpInit ( interp_t* driver, const float* const xTable, const float* const yTable, uint32_t length )
{
    uint8_t retVal = FALSE;
    uint8_t ascending = TRUE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( xTable != NULL ) && ( yTable != NULL ) &&
         ( length >= INTERP_MIN_LENGTH ) )
    {
        for ( i = 1u; i < length; ++i )
        {
            if ( xTable[ i ] <= xTable[ i - 1u ] )
            {
                ascending = FALSE;
            }
            else
            {
                /* Intentionally blank */
            }
        }

        if ( ascending == TRUE )
        {
            driver->xTable = xTable;
            driver->yTable = yTable;
            driver->length = length;

            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Initializes an integer interpolator over a caller owned table.
 * @param[out] driver   Interpolator state to initialize.
 * @param[in]  xTable   Caller owned, strictly ascending x values.
 * @param[in]  yTable   Caller owned y values, one per x.
 * @param[in]  length   Number of entries in each table.
 * @return  TRUE on success, FALSE when a pointer is NULL, length is below
 *          two, or xTable does not ascend strictly.
 * @note    Neither table is copied. Both must outlive the driver.
 */
uint8_t interpIniti32 ( interpi32_t* driver, const int32_t* const xTable, const int32_t* const yTable, uint32_t length )
{
    uint8_t retVal = FALSE;
    uint8_t ascending = TRUE;
    uint32_t i = 0;

    if ( ( driver != NULL ) && ( xTable != NULL ) && ( yTable != NULL ) &&
         ( length >= INTERP_MIN_LENGTH ) )
    {
        for ( i = 1u; i < length; ++i )
        {
            if ( xTable[ i ] <= xTable[ i - 1u ] )
            {
                ascending = FALSE;
            }
            else
            {
                /* Intentionally blank */
            }
        }

        if ( ascending == TRUE )
        {
            driver->xTable = xTable;
            driver->yTable = yTable;
            driver->length = length;

            retVal = TRUE;
        }
        else
        {
            retVal = FALSE;
        }
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}

/**
 * @brief   Finds the interval that holds x, as the index of its lower end.
 * @param[in]  xTable   Strictly ascending x values.
 * @param[in]  length   Number of entries.
 * @param[in]  x        Value to bracket. Must lie strictly inside the table.
 * @return  Index i such that xTable[ i ] <= x < xTable[ i + 1 ].
 * @note    This is the upper bound search: the first index whose x is
 *          strictly greater than the input, minus one. The caller has
 *          already handled both clamps, so x is strictly inside the table,
 *          which puts the upper bound in [ 1, length - 1 ] and the returned
 *          index in [ 0, length - 2 ]. Neither the subtraction below nor the
 *          i + 1 read at the call site can leave the table.
 */
static uint32_t interpBracket ( const float* const xTable, uint32_t length, float x )
{
    uint32_t retVal = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;

    high = length;

    while ( low < high )
    {
        mid = low + ( ( high - low ) >> 1 ); // divide by 2

        if ( xTable[ mid ] <= x )
        {
            low = mid + 1u;
        }
        else
        {
            high = mid;
        }
    }

    retVal = low - 1u;

    return ( retVal );
}

/**
 * @brief   Returns the table value at x, interpolated linearly between the
 *          two neighbouring entries and held flat past either end.
 * @param[in]  driver   Initialized interpolator.
 * @param[in]  x        Value to look up.
 * @return  The interpolated y. Past the first x it is the first y, past the
 *          last x it is the last y.
 * @note    A pure read, which is what lets one driver be shared between the
 *          main loop and an interrupt. It caches nothing between calls on
 *          purpose: caching the last interval would cost the const and buy
 *          nothing on the small tables this library is used with.
 * @note    The divisor cannot be zero and the index cannot leave the table.
 *          interpInit proved the strict ascent, and the two clamps run
 *          first, so the interpolating branch is only reached when x lies
 *          strictly inside the table.
 * @note    Clamping is an answer rather than an error, so the value comes
 *          back directly. interpInRange answers separately whether x fell
 *          inside the table, for the caller who needs to tell a saturated
 *          reading from a broken sensor.
 */
float interpCalculate ( const interp_t* const driver, float x )
{
    float retVal = 0;
    uint32_t i = 0;

    if ( x <= driver->xTable[ 0 ] )
    {
        retVal = driver->yTable[ 0 ];
    }
    else if ( x >= driver->xTable[ driver->length - 1u ] )
    {
        retVal = driver->yTable[ driver->length - 1u ];
    }
    else
    {
        i = interpBracket ( driver->xTable, driver->length, x );

        retVal = driver->yTable[ i ] +
                 ( ( ( x - driver->xTable[ i ] ) *
                     ( driver->yTable[ i + 1u ] - driver->yTable[ i ] ) ) /
                   ( driver->xTable[ i + 1u ] - driver->xTable[ i ] ) );
    }

    return ( retVal );
}

/**
 * @brief   Reports whether x falls inside the table rather than past an end.
 * @param[in]  driver   Initialized interpolator.
 * @param[in]  x        Value to test.
 * @return  TRUE when x lies between the first and last table x, endpoints
 *          included. FALSE when interpCalculate would clamp.
 */
uint8_t interpInRange ( const interp_t* const driver, float x )
{
    uint8_t retVal = FALSE;

    if ( ( x >= driver->xTable[ 0 ] ) &&
         ( x <= driver->xTable[ driver->length - 1u ] ) )
    {
        retVal = TRUE;
    }
    else
    {
        retVal = FALSE;
    }

    return ( retVal );
}
