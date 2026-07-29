/**
  ******************************************************************************
  *
  * @file:      basicmatrix.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.2
  * @cdate:     04/12/2021
  * @history:   04/12/2021 Created.
  *             11/12/2021 threshold2Du8 is addded.
  *             29/07/2026 Bug fix. The undeclared jSize identifier is fixed by
  *                        renaming the ySize parameter to jSize.
  *             29/07/2026 Bug fix. The row stride of the 2D index calculation
  *                        was iSize instead of jSize. Non square matrices were
  *                        processed incorrectly.
  *             29/07/2026 Bug fix. limitUpDw2D had an unused thresholdValue
  *                        parameter which did not match the prototype, and it
  *                        compared the elements against its own address instead
  *                        of upValue.
  *
  *
  * @about:     Basic matrix function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         threshold1D     : Applies thresholding on 1D array.
  *         threshold2D     : Applies thresholding on 2D array.
  *         threshold2Du8   : Applies thresholding on 2D array for u8.
  *         threshold2Du32  : Applies thresholding on 2D array for u32.
  *         threshold2Di32  : Applies thresholding on 2D array for i32.
  *         limitUpDw2D     : Applies limitting up and down on 1D array.
  *
  * @notes:
  *     The 2D functions expect a row major matrix. iSize is the row count and
  *     jSize is the column count, so the row stride is jSize.
  *
  ******************************************************************************
  */

#include "basicmatrix.h"

/*
 * @about:
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

/*
 * @about:
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

/*
 * @about:
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

/*
 * @about:
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

/*
 * @about:
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

/*
 * @about:
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
