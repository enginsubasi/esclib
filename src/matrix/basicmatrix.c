/**
  ******************************************************************************
  *
  * @file      basicmatrix.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.2
  * @date      04/12/2021
  *
  * @brief     Basic matrix function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 04/12/2021 Created. @n
  * 11/12/2021 threshold2Du8 is addded. @n
  * 29/07/2026 Bug fix. The undeclared jSize identifier is fixed by @n
  *            renaming the ySize parameter to jSize. @n
  * 29/07/2026 Bug fix. The row stride of the 2D index calculation @n
  *            was iSize instead of jSize. Non square matrices were @n
  *            processed incorrectly. @n
  * 29/07/2026 Bug fix. limitUpDw2D had an unused thresholdValue @n
  *            parameter which did not match the prototype, and it @n
  *            compared the elements against its own address instead @n
  *            of upValue. @n
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
void threshold1D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize )
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
void threshold2D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize, uint32_t jSize )
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
void threshold2Du8 ( uint8_t* matrix, uint8_t thresholdValue, uint8_t upValue, uint8_t dwValue, uint32_t iSize, uint32_t jSize )
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
void threshold2Du32 ( uint32_t* matrix, uint32_t thresholdValue, uint32_t upValue, uint32_t dwValue, uint32_t iSize, uint32_t jSize )
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
void threshold2Di32 ( int32_t* matrix, int32_t thresholdValue, int32_t upValue, int32_t dwValue, uint32_t iSize, uint32_t jSize )
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
void limitUpDw2D ( float* matrix, float upValue, float dwValue, uint32_t iSize, uint32_t jSize )
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
