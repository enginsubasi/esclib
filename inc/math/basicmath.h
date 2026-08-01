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

float mathFindMax ( const float* const array, uint32_t length );
uint32_t mathFindMaxu32 ( const uint32_t* const array, uint32_t length );
int32_t mathFindMaxi32 ( const int32_t* const array, uint32_t length );

float mathFindMin ( const float* const array, uint32_t length );
uint32_t mathFindMinu32 ( const uint32_t* const array, uint32_t length );
int32_t mathFindMini32 ( const int32_t* const array, uint32_t length );

void mathFindMinMax ( const float* const array, uint32_t length, float* min, float* max );
void mathFindMinMaxu32 ( const uint32_t* const array, uint32_t length, uint32_t* min, uint32_t* max );
void mathFindMinMaxi32 ( const int32_t* const array, uint32_t length, int32_t* min, int32_t* max );

float mathCalculateSum ( const float* const array, uint32_t length );
uint32_t mathCalculateSumu32 ( const uint32_t* const array, uint32_t length );
int32_t mathCalculateSumi32 ( const int32_t* const array, uint32_t length );

float mathCalculateMean ( const float* const array, uint32_t length );
uint32_t mathCalculateMeanu32 ( const uint32_t* const array, uint32_t length );
int32_t mathCalculateMeani32 ( const int32_t* const array, uint32_t length );

float mathCalculateMedian ( const float* const array, uint32_t length );
uint32_t mathCalculateMedianu32 ( const uint32_t* const array, uint32_t length );
int32_t mathCalculateMediani32 ( const int32_t* const array, uint32_t length );

float mathCalculateRange ( const float* const array, uint32_t length );
uint32_t mathCalculateRangeu32 ( const uint32_t* const array, uint32_t length );
int32_t mathCalculateRangei32 ( const int32_t* const array, uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATH_H_ */
