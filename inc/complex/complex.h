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

typedef struct
{
    int32_t re;
    int32_t im;
} complexi32_t;

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

void complexIniti32 ( complexi32_t* cprm1, int32_t re, int32_t im );
void complexSumi32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result );
void complexSubi32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result );
void complexMuli32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result );
void complexDivi32 ( const complexi32_t* const cprm1, const complexi32_t* const cprm2, complexi32_t* result );

#ifdef __cplusplus
}
#endif

#endif /* COMPLEX_H_ */   
