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

float statVariance ( const float* const array, uint32_t length );
int32_t statVariancei32 ( const int32_t* const array, uint32_t length );
uint32_t statVarianceu32 ( const uint32_t* const array, uint32_t length );
float statStandardDeviation ( const float* const array, uint32_t length );
int32_t statStandardDeviationi32 ( const int32_t* const array, uint32_t length );
uint32_t statStandardDeviationu32 ( const uint32_t* const array, uint32_t length );
float statCovariance ( const float* const array1, const float* const array2, uint32_t length );
int32_t statCovariancei32 ( const int32_t* const array1, const int32_t* const array2, uint32_t length );

#ifdef __cplusplus
}
#endif

#endif /* STATISTIC_H_ */
