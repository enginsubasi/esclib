/**
  ******************************************************************************
  *
  * @file      basicmatrix.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.1.0
  * @date      04/12/2021
  *
  * @brief     Basic matrix function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 04/12/2021 Created. @n
  * 11/12/2021 matrixThreshold2Du8 is addded. @n
  * 29/07/2026 Bug fix. The undeclared jSize identifier is fixed by @n
  *            renaming the ySize parameter to jSize. @n
  * 29/07/2026 Bug fix. The row stride of the 2D index calculation @n
  *            was iSize instead of jSize. Non square matrices were @n
  *            processed incorrectly. @n
  * 29/07/2026 Bug fix. matrixLimitUpDw2D had an unused thresholdValue @n
  *            parameter which did not match the prototype, and it @n
  *            compared the elements against its own address instead @n
  *            of upValue. @n
  * 01/08/2026 Every function in this module carries the matrix prefix @n
  *            now. The old names sat in the global namespace @n
  *            with no library marker, which invited a clash in @n
  *            any project that links other libraries. @n
  * 01/08/2026 threshold1D gains its u8, u32 and i32 variants and @n
  *            limitUpDw2D its u32 and i32 ones. Both were float only @n
  *            while threshold2D already had the full set. @n
  *
  * @note      The 2D functions expect a row major matrix. iSize is the row
  *            count and jSize is the column count, so the row stride is
  *            jSize.
  *
  ******************************************************************************
  */

#include "basicmatrix.h"

/**
 * @brief   Replaces every element of a flat array with upValue or dwValue.
 * @param[in,out] matrix          Flat array, modified in place.
 * @param[in]     thresholdValue  Elements above this become upValue.
 * @param[in]     upValue         Value written above the threshold.
 * @param[in]     dwValue         Value written at or below the threshold.
 * @param[in]     iSize           Number of elements in the array.
 */
void matrixThreshold1D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize )
{
    uint32_t i = 0;

    for ( i = 0; i < iSize; ++i )
    {
        if ( matrix [ i ] > thresholdValue )
        {
            matrix [ i ] = upValue;
        }
        else
        {
            matrix [ i ] = dwValue;
        }
    }
}

/**
 * @brief   Replaces every element of a 2D matrix with upValue or dwValue.
 * @param[in,out] matrix          Row major matrix, modified in place.
 * @param[in]     thresholdValue  Elements above this become upValue.
 * @param[in]     upValue         Value written above the threshold.
 * @param[in]     dwValue         Value written at or below the threshold.
 * @param[in]     iSize           Row count.
 * @param[in]     jSize           Column count, which is also the row stride.
 */
void matrixThreshold2D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
        }
    }
}

/**
 * @brief   Replaces every element of an unsigned 8-bit 2D matrix with upValue or dwValue.
 * @param[in,out] matrix          Row major matrix, modified in place.
 * @param[in]     thresholdValue  Elements above this become upValue.
 * @param[in]     upValue         Value written above the threshold.
 * @param[in]     dwValue         Value written at or below the threshold.
 * @param[in]     iSize           Row count.
 * @param[in]     jSize           Column count, which is also the row stride.
 */
void matrixThreshold2Du8 ( uint8_t* matrix, uint8_t thresholdValue, uint8_t upValue, uint8_t dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
        }
    }
}

/**
 * @brief   Replaces every element of an unsigned 32-bit 2D matrix with upValue or dwValue.
 * @param[in,out] matrix          Row major matrix, modified in place.
 * @param[in]     thresholdValue  Elements above this become upValue.
 * @param[in]     upValue         Value written above the threshold.
 * @param[in]     dwValue         Value written at or below the threshold.
 * @param[in]     iSize           Row count.
 * @param[in]     jSize           Column count, which is also the row stride.
 */
void matrixThreshold2Du32 ( uint32_t* matrix, uint32_t thresholdValue, uint32_t upValue, uint32_t dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
        }
    }
}

/**
 * @brief   Replaces every element of a signed 32-bit 2D matrix with upValue or dwValue.
 * @param[in,out] matrix          Row major matrix, modified in place.
 * @param[in]     thresholdValue  Elements above this become upValue.
 * @param[in]     upValue         Value written above the threshold.
 * @param[in]     dwValue         Value written at or below the threshold.
 * @param[in]     iSize           Row count.
 * @param[in]     jSize           Column count, which is also the row stride.
 */
void matrixThreshold2Di32 ( int32_t* matrix, int32_t thresholdValue, int32_t upValue, int32_t dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
        }
    }
}

/**
 * @brief   Clamps each element of a row major 2D matrix in place to the range [dwValue, upValue].
 * @param[in,out] matrix   Row major matrix, indexed as ( i * jSize ) + j, modified in place.
 * @param[in]     upValue  Upper clamp value.
 * @param[in]     dwValue  Lower clamp value.
 * @param[in]     iSize    Row count.
 * @param[in]     jSize    Column count, which is also the row stride.
 */
void matrixLimitUpDw2D ( float* matrix, float upValue, float dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > upValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else if ( matrix [ ( i * jSize ) + j ] < dwValue )
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
    }
}

/**
 * @brief   Replaces every element of the unsigned 8-bit 1D matrix with upValue or
 *          dwValue, depending on the threshold.
 * @param[in,out] matrix          Matrix to threshold in place.
 * @param[in]     thresholdValue  Value each element is compared against.
 * @param[in]     upValue         Written when the element is above the threshold.
 * @param[in]     dwValue         Written when the element is at or below the threshold.
 * @param[in]     iSize           Number of elements in the matrix.
 */
void matrixThreshold1Du8 ( uint8_t* matrix, uint8_t thresholdValue, uint8_t upValue, uint8_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;

    for ( i = 0; i < iSize; ++i )
    {
        if ( matrix [ i ] > thresholdValue )
        {
            matrix [ i ] = upValue;
        }
        else
        {
            matrix [ i ] = dwValue;
        }
    }
}

/**
 * @brief   Replaces every element of the unsigned 32-bit 1D matrix with upValue or
 *          dwValue, depending on the threshold.
 * @param[in,out] matrix          Matrix to threshold in place.
 * @param[in]     thresholdValue  Value each element is compared against.
 * @param[in]     upValue         Written when the element is above the threshold.
 * @param[in]     dwValue         Written when the element is at or below the threshold.
 * @param[in]     iSize           Number of elements in the matrix.
 */
void matrixThreshold1Du32 ( uint32_t* matrix, uint32_t thresholdValue, uint32_t upValue, uint32_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;

    for ( i = 0; i < iSize; ++i )
    {
        if ( matrix [ i ] > thresholdValue )
        {
            matrix [ i ] = upValue;
        }
        else
        {
            matrix [ i ] = dwValue;
        }
    }
}

/**
 * @brief   Replaces every element of the signed 32-bit 1D matrix with upValue or
 *          dwValue, depending on the threshold.
 * @param[in,out] matrix          Matrix to threshold in place.
 * @param[in]     thresholdValue  Value each element is compared against.
 * @param[in]     upValue         Written when the element is above the threshold.
 * @param[in]     dwValue         Written when the element is at or below the threshold.
 * @param[in]     iSize           Number of elements in the matrix.
 */
void matrixThreshold1Di32 ( int32_t* matrix, int32_t thresholdValue, int32_t upValue, int32_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;

    for ( i = 0; i < iSize; ++i )
    {
        if ( matrix [ i ] > thresholdValue )
        {
            matrix [ i ] = upValue;
        }
        else
        {
            matrix [ i ] = dwValue;
        }
    }
}

/**
 * @brief   Clamps every element of the unsigned 32-bit 2D matrix into the range between
 *          dwValue and upValue.
 * @param[in,out] matrix   Matrix to clamp in place, stored row by row.
 * @param[in]     upValue  Upper bound written to any element above it.
 * @param[in]     dwValue  Lower bound written to any element below it.
 * @param[in]     iSize    Number of rows.
 * @param[in]     jSize    Number of columns, which is also the row stride.
 */
void matrixLimitUpDw2Du32 ( uint32_t* matrix, uint32_t upValue, uint32_t dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > upValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else if ( matrix [ ( i * jSize ) + j ] < dwValue )
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
    }
}

/**
 * @brief   Clamps every element of the signed 32-bit 2D matrix into the range between
 *          dwValue and upValue.
 * @param[in,out] matrix   Matrix to clamp in place, stored row by row.
 * @param[in]     upValue  Upper bound written to any element above it.
 * @param[in]     dwValue  Lower bound written to any element below it.
 * @param[in]     iSize    Number of rows.
 * @param[in]     jSize    Number of columns, which is also the row stride.
 */
void matrixLimitUpDw2Di32 ( int32_t* matrix, int32_t upValue, int32_t dwValue, uint32_t iSize, uint32_t jSize )
{
    uint32_t i = 0;
    uint32_t j = 0;

    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * jSize ) + j ] > upValue )
            {
                matrix [ ( i * jSize ) + j ] = upValue;
            }
            else if ( matrix [ ( i * jSize ) + j ] < dwValue )
            {
                matrix [ ( i * jSize ) + j ] = dwValue;
            }
            else
            {
                /* Intentionally blank. */
            }
        }
    }
}
