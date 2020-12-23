#ifndef BASICMATH_H_
#define BASICMATH_H_

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

float findMax ( float* array, uint32_t length );
float findMin ( float* array, uint32_t length );
void findMinMax ( float* array, uint32_t length, float* min, float* max );

float calculateSum ( float* array, uint32_t length );
float calculateMean ( float* array, uint32_t length );
float calculateMedian ( float* array, uint32_t length );
float calculateRange ( float* array, uint32_t length );

#endif /* BASICMATH_H_ */
