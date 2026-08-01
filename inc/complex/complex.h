#ifndef COMPLEX_H_
#define COMPLEX_H_

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
void complexSum ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result );
void complexSub ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result );
void complexMul ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result );
void complexDiv ( const complex_t* const cprm1, const complex_t* const cprm2, complex_t* result );

void complexToPolar ( const complex_t* const prm1, float* r, float* a );
void complexFromPolar ( complex_t* prm1, float r, float a );

#ifdef __cplusplus
}
#endif

#endif /* COMPLEX_H_ */   
