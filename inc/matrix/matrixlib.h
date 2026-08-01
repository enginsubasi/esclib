/**
 * @file
 * @brief   Placeholder. The matrixlib module is not implemented.
 *
 * @warning matrixlib.c holds only a file banner. This header declares types and nothing
 *          else. Including it compiles, so nothing warns you here, but
 *          calling anything from this module fails at link time. Treat
 *          it as a reserved name, not as a module you can use.
 */

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
    float *matrix;
} mtrx_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

#ifdef __cplusplus
}
#endif

#endif /* MATRIXLIB_H_ */   
