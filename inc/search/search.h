#ifndef SEARCH_H_
#define SEARCH_H_

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

uint8_t linearSearch_i32 ( int32_t* array, uint32_t length, int32_t item, uint32_t* foundIndex );

#endif /* SEARCH_H_ */