#ifndef SEARCH_H_
#define SEARCH_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* TYPEDEFS */

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t linearSearch ( const float* const array, uint32_t length, float item, uint32_t* const foundIndex, float epsilon );
uint8_t linearSearchu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex );
uint8_t linearSearchi32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex );
uint8_t binarySearchu32 ( const uint32_t* const array, uint32_t length, uint32_t item, uint32_t* const foundIndex );
uint8_t binarySearchi32 ( const int32_t* const array, uint32_t length, int32_t item, uint32_t* const foundIndex );

#ifdef __cplusplus
}
#endif

#endif /* SEARCH_H_ */