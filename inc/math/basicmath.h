#ifndef BASICMATH_H_
#define BASICMATH_H_

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

float absolute ( float inp );

float findMax ( float* array, uint32_t length );
uint32_t findMaxu32 ( uint32_t* array, uint32_t length );
int32_t findMaxi32 ( int32_t* array, uint32_t length );
float findMin ( float* array, uint32_t length );
uint32_t findMinu32 ( uint32_t* array, uint32_t length );
void findMinMax ( float* array, uint32_t length, float* min, float* max );
void findMinMaxu32 ( uint32_t* array, uint32_t length, uint32_t* min, uint32_t* max );

float calculateSum ( float* array, uint32_t length );
uint32_t calculateSumu32 ( uint32_t* array, uint32_t length );
float calculateMean ( float* array, uint32_t length );
uint32_t calculateMeanu32 ( uint32_t* array, uint32_t length );
float calculateMedian ( float* array, uint32_t length );
uint32_t calculateMedianu32 ( uint32_t* array, uint32_t length );
float calculateRange ( float* array, uint32_t length );
uint32_t calculateRangeu32 ( uint32_t* array, uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATH_H_ */
