/**
  ******************************************************************************
  *
  * @file:      basicmatrix.c
  * @author:    Engin Subasi
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     04/12/2021
  * @history:   04/12/2021 Created.
  *             11/12/2021 threshold2Du8 is addded.
  *
  *
  * @about:     Basic matrix function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         threshold1D     : Applies thresholding on 1D array.
  *         threshold2D     : Applies thresholding on 2D array.
  *         threshold2Du8   : Applies thresholding on 2D array 8-bits.
  *         threshold2Du32  : Applies thresholding on 2D array 32-bits.
  *
  * @notes:
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
void threshold2D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize, uint32_t ySize )
{
    uint32_t i = 0;
    uint32_t j = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * iSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * iSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * iSize ) + j ] = dwValue;
            }
        }
    }
}

/*
 * @about:
 */
void threshold2Du8 ( uint8_t* matrix, uint8_t thresholdValue, uint8_t upValue, uint8_t dwValue, uint32_t iSize, uint32_t ySize )
{
    uint32_t i = 0;
    uint32_t j = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * iSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * iSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * iSize ) + j ] = dwValue;
            }
        }
    }
}

/*
 * @about:
 */
void threshold2Du32 ( uint32_t* matrix, uint32_t thresholdValue, uint32_t upValue, uint32_t dwValue, uint32_t iSize, uint32_t ySize )
{
    uint32_t i = 0;
    uint32_t j = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * iSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * iSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * iSize ) + j ] = dwValue;
            }
        }
    }
}

/*
 * @about:
 */
void threshold2Di32 ( int32_t* matrix, int32_t thresholdValue, int32_t upValue, int32_t dwValue, uint32_t iSize, uint32_t ySize )
{
    uint32_t i = 0;
    uint32_t j = 0;
    
    for ( i = 0; i < iSize; ++i )
    {
        for ( j = 0; j < jSize; ++j )
        {
            if ( matrix [ ( i * iSize ) + j ] > thresholdValue )
            {
                matrix [ ( i * iSize ) + j ] = upValue;
            }
            else
            {
                matrix [ ( i * iSize ) + j ] = dwValue;
            }
        }
    }
}

