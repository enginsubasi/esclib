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
  *
  *
  * @about:     Basic matrix function library file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         threshold2D     : Applies thresholding on 2D array.
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "basicmath.h"

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
            if ( matrix[ ( i * iSize ) + j ] > thresholdValue )
            {
                matrix[ ( i * iSize ) + j ] = upValue;
            }
            else
            {
                matrix[ ( i * iSize ) + j ] = dwValue;
            }
        }
    }
}



