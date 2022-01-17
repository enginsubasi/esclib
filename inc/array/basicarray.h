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

void limitUpDw1D ( float* array, float upValue, float dwValue, uint32_t iSize );
void limitUpDw1Du32 ( uint32_t* array, uint32_t upValue, uint32_t dwValue, uint32_t iSize );

#ifdef __cplusplus
}
#endif

#endif /* BASICMATRIX_H_ */
