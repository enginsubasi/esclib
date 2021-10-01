#ifndef EMAF_H_
#define EMAF_H_

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
    float alpha;
    float alphan;
    float output;
} emaf_t;

/* ENUMS */

/* EXTERNS */

/* FUNCTION PROTOTYPES */

int8_t emafInit ( emaf_t* driver, float alpha, float outputInit );
void emafIteration ( emaf_t* driver, float newData );
float emafGetOutput ( emaf_t* driver );

#ifdef __cplusplus
}
#endif

#endif /* EMAF_H_ */
