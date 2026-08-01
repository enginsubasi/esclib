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

float mathAbsolute ( float inp );
int32_t mathAbsolutei32 ( int32_t inp );

float mathFindMax ( float* array, uint32_t length );
uint32_t mathFindMaxu32 ( uint32_t* array, uint32_t length );
int32_t mathFindMaxi32 ( int32_t* array, uint32_t length );

float mathFindMin ( float* array, uint32_t length );
uint32_t mathFindMinu32 ( uint32_t* array, uint32_t length );
int32_t mathFindMini32 ( int32_t* array, uint32_t length );

void mathFindMinMax ( float* array, uint32_t length, float* min, float* max );
void mathFindMinMaxu32 ( uint32_t* array, uint32_t length, uint32_t* min, uint32_t* max );

float mathCalculateSum ( float* array, uint32_t length );
uint32_t mathCalculateSumu32 ( uint32_t* array, uint32_t length );

float mathCalculateMean ( float* array, uint32_t length );
uint32_t mathCalculateMeanu32 ( uint32_t* array, uint32_t length );

float mathCalculateMedian ( float* array, uint32_t length );
uint32_t mathCalculateMedianu32 ( uint32_t* array, uint32_t length );

float mathCalculateRange ( float* array, uint32_t length );
uint32_t mathCalculateRangeu32 ( uint32_t* array, uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATH_H_ */
