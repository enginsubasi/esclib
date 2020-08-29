#ifndef EMAF_H_
#define EMAF_H_

#include <stdint.h>

/* FUNCTION DEFINITIONS */

/* DEFINITIONS */

#define TRUE    1
#define FALSE   0

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

#endif /* EMAF_H_ */
