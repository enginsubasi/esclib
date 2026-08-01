#ifndef STATISTIC_H_
#define STATISTIC_H_

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

float statVariance ( float* array, uint32_t length );
int32_t statVariancei32 ( int32_t* array, uint32_t length );
float statStandardDeviation ( float* array, uint32_t length );
int32_t statStandardDeviationi32 ( int32_t* array, uint32_t length );
float statCovariance ( float* array1, float* array2, uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* STATISTIC_H_ */
