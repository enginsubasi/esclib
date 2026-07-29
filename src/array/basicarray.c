/**
  ******************************************************************************
  *
  * @file      basicarray.c
  * @author    Engin Subasi <enginsubasi@gmail.com>, github.com/enginsubasi
  * @version   0.0.1
  * @date      03/01/2022
  *
  * @brief     Basic array function library file.
  *
  * @par Device
  * Generic
  *
  * @par History
  * 03/01/2022 Created. @n
  *
  ******************************************************************************
  */

#include "basicarray.h"

/**
 * @brief   Clamps each element of the array in place to the range [dwValue, upValue].
 * @param[in,out] array    Array to clamp, modified in place.
 * @param[in]     upValue  Upper clamp value.
 * @param[in]     dwValue  Lower clamp value.
 * @param[in]     iSize    Number of elements in the array.
 */
void limitUpDw1D ( float* array, float upValue, float dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            array[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            array[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}

/**
 * @brief   Clamps each element of the unsigned 32-bit array in place to the range [dwValue, upValue].
 * @param[in,out] array    Array to clamp, modified in place.
 * @param[in]     upValue  Upper clamp value.
 * @param[in]     dwValue  Lower clamp value.
 * @param[in]     iSize    Number of elements in the array.
 */
void limitUpDw1Du32 ( uint32_t* array, uint32_t upValue, uint32_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            array[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            array[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}

/**
 * @brief   Clamps each element of the signed 32-bit array in place to the range [dwValue, upValue].
 * @param[in,out] array    Array to clamp, modified in place.
 * @param[in]     upValue  Upper clamp value.
 * @param[in]     dwValue  Lower clamp value.
 * @param[in]     iSize    Number of elements in the array.
 */
void limitUpDw1Di32 ( int32_t* array, int32_t upValue, int32_t dwValue, uint32_t iSize )
{
    uint32_t i = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        if ( array[ i ] > upValue )
        {
            array[ i ] = upValue;
        }
        else if ( array[ i ] < dwValue )
        {
            array[ i ] = dwValue;
        }
        else
        {
            /* Intentionally blank. */
        }
    }
}
