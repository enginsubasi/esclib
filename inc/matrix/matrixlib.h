#ifndef MATRIXLIB_H_
#define MATRIXLIB_H_

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

typedef struct
{
    float* data;
    uint32_t rows;
    uint32_t cols;
} mtrx_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t matrixInit ( mtrx_t* driver, float* data, uint32_t rows, uint32_t cols );
void matrixZero ( mtrx_t* driver );
uint8_t matrixIdentity ( mtrx_t* driver );
float matrixGet ( const mtrx_t* const driver, uint32_t row, uint32_t col );
uint8_t matrixSet ( mtrx_t* driver, uint32_t row, uint32_t col, float value );
uint8_t matrixAdd ( const mtrx_t* const a, const mtrx_t* const b, mtrx_t* result );
uint8_t matrixSub ( const mtrx_t* const a, const mtrx_t* const b, mtrx_t* result );
uint8_t matrixScale ( const mtrx_t* const a, float scalar, mtrx_t* result );
uint8_t matrixMul ( const mtrx_t* const a, const mtrx_t* const b, mtrx_t* result );
uint8_t matrixTranspose ( const mtrx_t* const a, mtrx_t* result );
uint8_t matrixInverse ( const mtrx_t* const a, mtrx_t* result, mtrx_t* scratch );

#ifdef __cplusplus
}
#endif

#endif /* MATRIXLIB_H_ */
