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

void threshold2D ( float* matrix, float thresholdValue, float upValue, float dwValue, uint32_t iSize, uint32_t ySize );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATRIX_H_ */
