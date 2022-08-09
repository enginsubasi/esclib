#ifndef GENERIC_H_
#define GENERIC_H_

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

typedef struct
{
    float re;
    float im;
} complex_t;

/* STRUCTURES */

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

void complexInit ( complex_t* cprm1, float re, float im );
void complexSum ( complex_t* cprm1, complex_t* cprm2, complex_t* result );
void complexSub ( complex_t* cprm1, complex_t* cprm2, complex_t* result );
void complexMul ( complex_t* cprm1, complex_t* cprm2, complex_t* result );
void complexDiv ( complex_t* cprm1, complex_t* cprm2, complex_t* result );

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_H_ */   
