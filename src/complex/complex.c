/**
  ******************************************************************************
  *
  * @file:      generic.c
  * @author:    Name Surname
  * @email:     mail@mail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     20/02/2020
  * @history:   20/02/2020 Created
  *
  * @about:     Generic template file.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         foo1            : Brief
  *         foo2            : Brief
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "generic.h"

/**
 * @brief Circular buffer initialization
 * @param driver
 * @param buffer
 * @param capacity
 * @return Success
 */
void complexInit ( complex_t* driver, float re, float im )
{
    driver->re = re;
    driver->im = im;
}


