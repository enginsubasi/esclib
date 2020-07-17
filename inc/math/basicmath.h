#ifndef BASICMATH_H_
#define BASICMATH_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define TRUE    1
#define FALSE   0

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

double findMax ( double* array, uint32_t length );
double findMin ( double* array, uint32_t length );

double calculateSum ( double* array, uint32_t length );
double calculateMean ( double* array, uint32_t length );

#endif /* BASICMATH_H_ */
