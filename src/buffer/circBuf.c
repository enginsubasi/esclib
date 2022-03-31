/**
  ******************************************************************************
  *
  * @file:      circBuf.c
  * @author:    Engin Subaşı
  * @email:     enginsubasi@gmail.com
  * @address:   github.com/enginsubasi
  *
  * @version:   v 0.0.1
  * @cdate:     29/03/2022
  * @history:   29/03/2022 Created.
  *
  * @about:     Circular buffer implementation.
  * @device:    Generic
  *
  * @content:
  *     FUNCTIONS:
  *         foo1                : Initialize comstxetx structure.
  *         foo2                : 
  *
  * @notes:
  *
  ******************************************************************************
  */

#include "circBuf.h"

/*
 * @about:
 */
void circBufInitu32 ( circBufu32_t* driver, uint32_t* buffer, uint32_t capacity )
{
    driver->buffer = buffer;
    driver->capacity = capacity;
    driver->rp = 0;
    driver->wp = 0;
}

/*
 * @about:
 */
uint32_t circBufGetsizeu32 ( circBufu32_t* driver )
{

}

