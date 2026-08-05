#ifndef INTERP_H_
#define INTERP_H_

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
    const float* xTable;
    const float* yTable;
    uint32_t length;
} interp_t;

typedef struct
{
    const int32_t* xTable;
    const int32_t* yTable;
    uint32_t length;
} interpi32_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

uint8_t interpInit ( interp_t* driver, const float* const xTable, const float* const yTable, uint32_t length );
float interpCalculate ( const interp_t* const driver, float x );
uint8_t interpInRange ( const interp_t* const driver, float x );

uint8_t interpIniti32 ( interpi32_t* driver, const int32_t* const xTable, const int32_t* const yTable, uint32_t length );
int32_t interpCalculatei32 ( const interpi32_t* const driver, int32_t x );
uint8_t interpInRangei32 ( const interpi32_t* const driver, int32_t x );

#ifdef __cplusplus
}
#endif

#endif /* INTERP_H_ */
