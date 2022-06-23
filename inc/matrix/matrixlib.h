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

struct mtrx
{
    float *matrix;
};

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

#ifdef __cplusplus
}
#endif

#endif /* MATRIXLIB_H_ */   
