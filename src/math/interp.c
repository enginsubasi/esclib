/**
  ******************************************************************************
  *
  * @file      interp.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
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
