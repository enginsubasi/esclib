#ifndef BASICMATRIX_H_
#define BASICMATRIX_H_

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

void threshold1D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize );
void threshold2D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize, uint32_t ySize );
void threshold2Du8 ( uint8_t* matrix, uint8_t thresholdValue, uint8_t upValue, uint8_t dwValue, uint32_t iSize, uint32_t ySize );
void threshold2Du32 ( uint32_t* matrix, uint32_t thresholdValue, uint32_t upValue, uint32_t dwValue, uint32_t iSize, uint32_t ySize );
void threshold2Di32 ( int32_t* matrix, int32_t thresholdValue, int32_t upValue, int32_t dwValue, uint32_t iSize, uint32_t ySize );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATRIX_H_ */
